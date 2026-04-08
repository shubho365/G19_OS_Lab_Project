# Project 1 — Adding 5 Custom System Calls to xv6

This document walks through every kernel file you need to edit and shows
the **exact code** you must paste in. Read the "Why" boxes carefully —
your viva will mostly come from those.

---

## Background: how a system call flows through xv6

When user code calls `getppid()`:

```
user prog ──► usys.S stub  ──► int $T_SYSCALL (0x40 trap)
                                       │
                                       ▼
                              trap.c: trap()
                                       │
                                       ▼
                              syscall.c: syscall()
                                       │
                              looks up syscalls[num]
                                       │
                                       ▼
                              sysproc.c: sys_getppid()
                                       │
                                       ▼
                              proc.c:  myproc()->parent->pid
```

Adding a syscall therefore means touching **6 places** in the source:

1. **`syscall.h`** — give the call a number (`SYS_getppid 22`).
2. **`syscall.c`** — declare the kernel function and add it to the
   dispatch table.
3. **`sysproc.c`** — write the actual kernel implementation.
4. **`usys.S`** — assembly stub that loads the syscall number into
   `%eax` and traps into the kernel.
5. **`user.h`** — user-space prototype so test programs can call it.
6. **`Makefile`** — add the new user test program to `UPROGS`.

That's the whole pattern. Every one of our five syscalls follows it.

---

## Step 1 — `syscall.h`: assign syscall numbers

Open `syscall.h`. The bottom looks like:

```c
#define SYS_close  21
```

Add **after** it:

```c
#define SYS_getppid       22
#define SYS_getprocs      23
#define SYS_set_priority  24
#define SYS_cps           25
#define SYS_waitx         26
```

> **Why:** The syscall number is the index into the kernel's dispatch
> table. The user stub will load this number into `%eax` before issuing
> the trap, and the kernel uses it to look up which handler to call.

---

## Step 2 — `syscall.c`: register the handlers

Find the block of `extern int sys_*` declarations and **add five lines**
at the bottom:

```c
extern int sys_getppid(void);
extern int sys_getprocs(void);
extern int sys_set_priority(void);
extern int sys_cps(void);
extern int sys_waitx(void);
```

Then find the `static int (*syscalls[])(void)` table and add the
matching entries:

```c
[SYS_getppid]      sys_getppid,
[SYS_getprocs]     sys_getprocs,
[SYS_set_priority] sys_set_priority,
[SYS_cps]          sys_cps,
[SYS_waitx]        sys_waitx,
```

> **Why:** This is the dispatch table. When `syscall()` runs it does
> `num = curproc->tf->eax;` then calls `syscalls[num]()`. Without an
> entry here, `eax` would index out of bounds and the kernel would
> kill the process with "unknown sys call".

---

## Step 3 — `proc.h`: add accounting fields

We need extra fields on `struct proc` so `set_priority`, `cps`, and
`waitx` have something to read/write. Open `proc.h` and find
`struct proc { ... };`. Add these lines just before the closing brace:

```c
  int priority;        // process priority (lower number = higher priority)
  int ctime;           // creation time (in ticks)
  int etime;           // exit time
  int rtime;           // total time the process has been RUNNING
```

> **Why:** xv6's `struct proc` is the PCB (process control block).
> Anything the kernel must remember about a process across context
> switches lives here. We use ticks (the value of the global `ticks`
> variable, incremented by the timer interrupt every 10 ms) as our
> clock.

---

## Step 4 — `proc.c`: initialize and update the fields

### 4a. In `allocproc()`

After the line `p->state = EMBRYO;` add:

```c
  p->priority = 60;     // default priority — middle of the range
  p->ctime    = ticks;
  p->rtime    = 0;
  p->etime    = 0;
```

You'll also need to add `#include "proc.h"` and have access to the
global `ticks` — it's already declared in `defs.h`, but you may need
`extern uint ticks;` at the top of `proc.c` if it isn't already there.

### 4b. In `exit()`

Find the line `curproc->state = ZOMBIE;` and **just before** it add:

```c
  curproc->etime = ticks;
```

### 4c. In `trap.c` — accumulate `rtime` on every tick

Open `trap.c`. Find the timer-interrupt branch:

```c
case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
```

Just **after** the `release(&tickslock);` line, add a small block that
walks the process table and bumps `rtime` for whoever is currently
running:

```c
    /* ---- accounting for waitx() ---- */
    {
      struct proc *p;
      acquire(&ptable.lock);
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == RUNNING)
          p->rtime++;
      }
      release(&ptable.lock);
    }
    /* -------------------------------- */
```

You'll need to declare `ptable` extern at the top of `trap.c`. The
cleanest way is to add a tiny helper in `proc.c`:

```c
// Place this at the bottom of proc.c
void
update_rtime(void)
{
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNING)
      p->rtime++;
  }
  release(&ptable.lock);
}
```

Add the prototype to `defs.h`:

```c
void update_rtime(void);
```

And in `trap.c`, after `release(&tickslock);`, just call:

```c
update_rtime();
```

> **Why the helper?** `ptable` is `static` inside `proc.c`, which means
> code in `trap.c` can't see it. The helper keeps the encapsulation
> intact and gives us a single place where the accounting happens.

> **Why the lock?** Two CPUs could be reading/writing the table at the
> same time. The same `ptable.lock` that the scheduler uses guarantees
> the table is in a consistent state. This is your **locks**
> demonstration for the rubric.

---

## Step 5 — `sysproc.c`: implement the five system calls

Open `sysproc.c` and add this block at the end of the file:

```c
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
```

> **Why `argint`?** User pointers and ints arrive on the user stack
> when the syscall traps. `argint(n, &dst)` is the safe accessor that
> reads the n-th argument and validates it before the kernel touches
> it. Always use it — never read user memory directly.

> **Note about `*(int*)wtime_addr = wtime;`** — for a project this is
> acceptable, but the *strictly correct* xv6 way is to use `argptr()`,
> which validates the pointer lies in the user's address space. If
> your instructor is strict, replace the two `argint` calls with
> `argptr(0, (char**)&wtime, sizeof(int))` and
> `argptr(1, (char**)&rtime, sizeof(int))` and write through the
> resulting kernel pointers. The provided form is the simpler one most
> tutorials show.

---

## Step 6 — `usys.S`: user-space stubs

xv6 has a tiny macro `SYSCALL(name)` that emits the assembly stub.
Open `usys.S` and add to the bottom:

```asm
SYSCALL(getppid)
SYSCALL(getprocs)
SYSCALL(set_priority)
SYSCALL(cps)
SYSCALL(waitx)
```

> **Why:** Each `SYSCALL(x)` expands into roughly:
> ```
>     movl  $SYS_x, %eax
>     int   $T_SYSCALL
>     ret
> ```
> This is the only place in user space where the actual `int 0x40`
> trap instruction lives.

---

## Step 7 — `user.h`: user-facing prototypes

Open `user.h` and add the prototypes near the other syscall declarations
(below `int uptime(void);`):

```c
int getppid(void);
int getprocs(void);
int set_priority(int pid, int prio);
int cps(void);
int waitx(int *wtime, int *rtime);
```

---

## Step 8 — `Makefile`: register the test program

Open the xv6 `Makefile` and find the `UPROGS=\` block. Add a new line:

```
	_syscalltest\
```

(The leading underscore is the xv6 convention for compiled user
programs.)

Below the block, find the `EXTRA=\` list and **also add** `syscalltest.c`
so `make tar` will pick it up:

```
	syscalltest.c\
```

Then copy `syscalltest.c` (provided in `user_program/`) into the xv6
source directory.

---

## Step 9 — Build and test

```bash
cd ~/xv6-public
make clean
make qemu-nox
```

In the xv6 shell:

```
$ syscalltest
```

You should see the output of all five syscalls. Take a screenshot
(`Ctrl-Shift-PrtSc` on most Linux setups) and save it to
`docs/screenshots/`.

---

## Common pitfalls

1. **Forgetting to include `proc.h`** in `sysproc.c` — `myproc()` and
   `struct proc` won't be visible.
2. **Forgetting to declare `ptable` extern** in `sysproc.c` — you'll
   get a linker error.
3. **Not holding `ptable.lock`** when reading the table — works most of
   the time, then deadlocks or corrupts state once in 100 runs.
4. **Wrong order of argint calls** — `argint(0, …)` is the *first*
   user argument, `argint(1, …)` is the second.
5. **Not adding the test program to both UPROGS and EXTRA** — `make`
   will succeed but `syscalltest` won't be inside the xv6 filesystem.

---

## What you'll be asked in the viva

Be ready to answer these in your own words:

1. **What is a trap, and how does `int $T_SYSCALL` work?**  
   It's a software interrupt. The CPU jumps through the IDT entry for
   vector 64 into the kernel's `vector64` stub, saves the user
   registers in a `trapframe`, and ends up in `trap()`.

2. **Why is the syscall number passed in `%eax`?**  
   It's the calling convention chosen by the xv6 designers — kernel
   code reads `tf->eax` in `syscall()` to dispatch.

3. **What does `myproc()` return and why is it safe?**  
   The `proc` structure of the currently running process on this CPU.
   It's safe because it disables interrupts while accessing
   `mycpu()->proc`.

4. **Why use `ptable.lock` in `cps`?**  
   Another CPU could be modifying the process table at the same time.
   The lock guarantees the snapshot we print is consistent.

5. **What's the difference between `wait` and `waitx`?**  
   `waitx` returns two extra values — the wait time (time spent
   `RUNNABLE` or `SLEEPING`) and the run time (time spent `RUNNING`).
   We compute `wtime = etime - ctime - rtime`.
