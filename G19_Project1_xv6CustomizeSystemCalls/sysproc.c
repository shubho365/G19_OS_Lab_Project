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

// ============================================================
//   New syscalls — covers Process Creation, Signals, IPC
// ============================================================

// 6. waitpid(pid): wait for a *specific* child process.
int
sys_waitpid(void)
{
  int pid;
  if(argint(0, &pid) < 0) return -1;

  struct proc *p;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    int found = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc || p->pid != pid) continue;
      found = 1;
      if(p->state == ZOMBIE){
        int cpid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state  = UNUSED;
        p->pid    = 0;
        p->parent = 0;
        p->name[0]= 0;
        p->killed = 0;
        release(&ptable.lock);
        return cpid;
      }
    }
    if(!found || curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    sleep(curproc, &ptable.lock);
  }
}

// 7. sigkill(pid): send a kill signal to any process by PID.
int
sys_sigkill(void)
{
  int pid;
  if(argint(0, &pid) < 0) return -1;
  return kill(pid);
}

// 8. shmem(key): shared-memory IPC (simple copy-based, keys 0-3).
#define SHM_NKEYS 4
static char *shm_pages[SHM_NKEYS];

int
sys_shmem(void)
{
  int key;
  if(argint(0, &key) < 0 || key < 0 || key >= SHM_NKEYS) return -1;

  if(shm_pages[key] == 0){
    shm_pages[key] = kalloc();
    if(shm_pages[key] == 0) return -1;
    memset(shm_pages[key], 0, PGSIZE);
  }

  struct proc *curproc = myproc();
  uint va = curproc->sz;

  if(growproc(PGSIZE) < 0) return -1;

  if(copyout(curproc->pgdir, va, shm_pages[key], PGSIZE) < 0)
    return -1;

  return (int)va;
}

// ============================================================
//   fork2 + true shared memory (shmget / shmem_count)
// ============================================================

// 9. fork2(priority): fork with caller-specified child priority.
int
sys_fork2(void)
{
  int priority;
  if(argint(0, &priority) < 0)
    return -1;
  return fork2(priority);
}

// 10. shmget(id): map shared physical page `id` (0-7) into process.
int
sys_shmget(void)
{
  int id;
  if(argint(0, &id) < 0)
    return -1;
  return shmget(id);
}

// 11. shmem_count(id): return reference count for shared page `id`.
int
sys_shmem_count(void)
{
  int id;
  if(argint(0, &id) < 0)
    return -1;
  return shmem_count(id);
}

// ============================================================
//   Mutex Lock System Calls
// ============================================================

#define MAX_MUTEXES 10

static struct spinlock mutex_table_lock;
static int mutex_table_inited = 0;

static struct {
  int allocated;
  int locked;
  int pid;
} user_mutexes[MAX_MUTEXES];

static void
mutex_table_init(void)
{
  if(!mutex_table_inited){
    initlock(&mutex_table_lock, "mutextbl");
    mutex_table_inited = 1;
  }
}

// 12. mutex_create(): returns an ID of a newly allocated mutex
int
sys_mutex_create(void)
{
  int i;
  mutex_table_init();
  acquire(&mutex_table_lock);
  for(i = 0; i < MAX_MUTEXES; i++){
    if(!user_mutexes[i].allocated){
      user_mutexes[i].allocated = 1;
      user_mutexes[i].locked = 0;
      user_mutexes[i].pid = 0;
      release(&mutex_table_lock);
      return i;
    }
  }
  release(&mutex_table_lock);
  return -1; // No free mutexes
}

// 13. mutex_lock(id): blocks until the mutex is free, then acquires it
int
sys_mutex_lock(void)
{
  int id;
  if(argint(0, &id) < 0 || id < 0 || id >= MAX_MUTEXES)
    return -1;

  mutex_table_init();
  acquire(&mutex_table_lock);
  if(!user_mutexes[id].allocated){
    release(&mutex_table_lock);
    return -1;
  }
  while(user_mutexes[id].locked){
    sleep(&user_mutexes[id], &mutex_table_lock);
  }
  user_mutexes[id].locked = 1;
  user_mutexes[id].pid = myproc()->pid;
  release(&mutex_table_lock);
  return 0;
}

// 14. mutex_unlock(id): releases the mutex and wakes up waiters
int
sys_mutex_unlock(void)
{
  int id;
  if(argint(0, &id) < 0 || id < 0 || id >= MAX_MUTEXES)
    return -1;

  mutex_table_init();
  acquire(&mutex_table_lock);
  if(!user_mutexes[id].allocated || !user_mutexes[id].locked || user_mutexes[id].pid != myproc()->pid){
    release(&mutex_table_lock);
    return -1;
  }
  user_mutexes[id].locked = 0;
  user_mutexes[id].pid = 0;
  release(&mutex_table_lock);
  // wakeup acquires ptable.lock internally, so call it AFTER releasing our lock
  wakeup(&user_mutexes[id]);
  return 0;
}

// ============================================================
//   Message Queue System Calls (Linked Lists)
// ============================================================

#include "msgqueue.h"

struct msg_node {
  struct msg_node *next;
  long mtype;
  size_t length;
  char data[4000]; // Fits in a 4096 page with padding
};

struct msg_q {
  struct msg_q *next;   
  struct spinlock lock; 
  int id;               
  key_t key;            
  struct msg_node *head;
  struct msg_node *tail;
  uint qnum;            
};

static struct msg_q *queue_list_head = 0;
static struct spinlock queue_list_lock;
static int queue_list_inited = 0;
static int next_q_id = 1;

static void queue_table_init(void) {
  if(!queue_list_inited) {
    initlock(&queue_list_lock, "msgqueue");
    queue_list_inited = 1;
  }
}

int sys_msgget(void) {
  int key, msgflg;
  if(argint(0, &key) < 0 || argint(1, &msgflg) < 0) return -1;
  
  queue_table_init();
  acquire(&queue_list_lock);
  
  struct msg_q *q = queue_list_head;
  while(q) {
    if(q->key == key) {
      release(&queue_list_lock);
      return q->id;
    }
    q = q->next;
  }
  
  if(!(msgflg & IPC_CREAT)) {
    release(&queue_list_lock);
    return -1;
  }
  
  q = (struct msg_q*)kalloc();
  if(!q) {
    release(&queue_list_lock);
    return -1;
  }
  
  q->key = key;
  q->id = next_q_id++;
  q->head = 0;
  q->tail = 0;
  q->qnum = 0;
  initlock(&q->lock, "msg_q_lock");
  
  q->next = queue_list_head;
  queue_list_head = q;
  
  int ret_id = q->id;
  release(&queue_list_lock);
  return ret_id;
}

int sys_msgsnd(void) {
  int msqid, msgflg;
  int msgsz;
  char *msgp;
  
  if(argint(0, &msqid) < 0 || argint(2, &msgsz) < 0 || argint(3, &msgflg) < 0) return -1;
  if(msgsz < 0 || msgsz > 4000) return -1;
  
  if(argptr(1, &msgp, msgsz + sizeof(long)) < 0) return -1;
  
  queue_table_init();
  acquire(&queue_list_lock);
  struct msg_q *q = queue_list_head;
  while(q) {
    if(q->id == msqid) break;
    q = q->next;
  }
  release(&queue_list_lock);
  
  if(!q) return -1;
  
  struct msg_node *node = (struct msg_node*)kalloc();
  if(!node) return -1;
  
  long *user_mtype = (long*)msgp;
  node->mtype = *user_mtype;
  node->length = msgsz;
  node->next = 0;
  if(msgsz > 0)
      memmove(node->data, msgp + sizeof(long), msgsz);
  
  acquire(&q->lock);
  if(!q->head) {
    q->head = node;
    q->tail = node;
  } else {
    q->tail->next = node;
    q->tail = node;
  }
  q->qnum++;
  wakeup(q);
  release(&q->lock);
  
  return 0;
}

int sys_msgrcv(void) {
  int msqid, msgtyp, msgflg;
  int msgsz;
  char *msgp;
  
  if(argint(0, &msqid) < 0 || argint(2, &msgsz) < 0 || argint(3, &msgtyp) < 0 || argint(4, &msgflg) < 0) return -1;
  if(argptr(1, &msgp, sizeof(long)) < 0) return -1; // At least long is required
  
  queue_table_init();
  acquire(&queue_list_lock);
  struct msg_q *q = queue_list_head;
  while(q) {
    if(q->id == msqid) break;
    q = q->next;
  }
  release(&queue_list_lock);
  
  if(!q) return -1;
  
  acquire(&q->lock);
  for(;;) {
    struct msg_node *prev = 0;
    struct msg_node *curr = q->head;
    
    while(curr) {
      if(msgtyp == 0 || curr->mtype == msgtyp) {
        if(prev) prev->next = curr->next;
        else q->head = curr->next;
        
        if(curr == q->tail) q->tail = prev;
        
        q->qnum--;
        release(&q->lock);
        
        int copysz = (curr->length < (uint)msgsz) ? curr->length : msgsz;
        if(argptr(1, &msgp, sizeof(long) + copysz) < 0) {
            kfree((char*)curr);
            return -1;
        }

        long *user_mtype = (long*)msgp;
        *user_mtype = curr->mtype;
        if(copysz > 0)
            memmove(msgp + sizeof(long), curr->data, copysz);
        
        kfree((char*)curr);
        return copysz;
      }
      prev = curr;
      curr = curr->next;
    }
    
    if(msgflg & IPC_NOWAIT) {
      release(&q->lock);
      return -1;
    }
    
    // Check if process killed
    if(myproc()->killed){
        release(&q->lock);
        return -1;
    }
    sleep(q, &q->lock);
  }
}

int sys_msgctl(void) {
  int msqid, cmd;
  char *buf;
  if(argint(0, &msqid) < 0 || argint(1, &cmd) < 0 || argptr(2, &buf, 0) < 0) return -1;
  
  if(cmd != IPC_RMID) return -1; 
  
  queue_table_init();
  acquire(&queue_list_lock);
  
  struct msg_q *prev = 0;
  struct msg_q *curr = queue_list_head;
  
  while(curr) {
    if(curr->id == msqid) {
       if(prev) prev->next = curr->next;
       else queue_list_head = curr->next;
       release(&queue_list_lock);
       
       acquire(&curr->lock);
       struct msg_node *n = curr->head;
       while(n) {
         struct msg_node *nxt = n->next;
         kfree((char*)n);
         n = nxt;
       }
       curr->head = curr->tail = 0;
       curr->qnum = 0;
       wakeup(curr);
       release(&curr->lock);
       
       kfree((char*)curr);
       return 0;
    }
    prev = curr;
    curr = curr->next;
  }
  
  release(&queue_list_lock);
  return -1;
}
