#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}



// ============================================================
//   Custom system calls — Project 1
// ============================================================

#include "spinlock.h"   // already included via proc.c, but harmless

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

// 1. getppid: return parent's PID
int
sys_getppid(void)
{
  struct proc *cur = myproc();
  if(cur->parent == 0) return -1;
  return cur->parent->pid;
}

// 2. getprocs: count active (non-UNUSED) processes
int
sys_getprocs(void)
{
  struct proc *p;
  int count = 0;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state != UNUSED)
      count++;
  release(&ptable.lock);
  return count;
}

// 3. set_priority(pid, prio): set priority of a given pid
int
sys_set_priority(void)
{
  int pid, prio;
  if(argint(0, &pid)  < 0) return -1;
  if(argint(1, &prio) < 0) return -1;
  if(prio < 0 || prio > 100) return -1;

  struct proc *p;
  int old = -1;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      old = p->priority;
      p->priority = prio;
      break;
    }
  }
  release(&ptable.lock);
  return old;     // returns previous priority, or -1 if not found
}

// 4. cps: print every process to the console (like ps)
int
sys_cps(void)
{
  static char *states[] = {
    [UNUSED]    "unused",
    [EMBRYO]    "embryo",
    [SLEEPING]  "sleep ",
    [RUNNABLE]  "runble",
    [RUNNING]   "run   ",
    [ZOMBIE]    "zombie"
  };

  struct proc *p;
  cprintf("\nPID\tState\tPrio\tName\n");
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED) continue;
    cprintf("%d\t%s\t%d\t%s\n",
            p->pid,
            states[p->state] ? states[p->state] : "?",
            p->priority,
            p->name);
  }
  release(&ptable.lock);
  return 0;
}

// 5. waitx(*wtime, *rtime): like wait() but also fills in
//    the wait time and run time of the reaped child.
int
sys_waitx(void)
{
  int wtime_addr, rtime_addr;
  if(argint(0, &wtime_addr) < 0) return -1;
  if(argint(1, &rtime_addr) < 0) return -1;

  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc) continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Compute timing info before tearing the process down.
        int wtime = p->etime - p->ctime - p->rtime;
        int rtime = p->rtime;

        // Copy into the user-supplied pointers.
        *(int*)wtime_addr = wtime;
        *(int*)rtime_addr = rtime;

        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid    = 0;
        p->parent = 0;
        p->name[0]= 0;
        p->killed = 0;
        p->state  = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    sleep(curproc, &ptable.lock);
  }
}