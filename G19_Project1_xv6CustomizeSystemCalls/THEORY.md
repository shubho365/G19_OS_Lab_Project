# Viva-prep Theory Cheat Sheet

Both projects boil down to the same handful of OS concepts. Master
these answers and you can defend either project comfortably.

---

## Section A — System call mechanics (Project 1)

### Q1. What exactly happens when a user program calls `getppid()`?

1. The C library / `usys.S` stub places the syscall number `22`
   (`SYS_getppid`) into register `%eax`.
2. It executes `int $T_SYSCALL` (`int 0x40`). This is a software
   interrupt that traps into the kernel.
3. The CPU consults the **Interrupt Descriptor Table (IDT)** entry for
   vector 64. xv6 sets that entry up in `tvinit()` (`trap.c`) so it
   points at `vector64`.
4. `vector64` (in `vectors.S`, generated automatically) pushes the
   trap number and jumps to `alltraps`. `alltraps` saves all user
   registers into a `struct trapframe` on the kernel stack and calls
   `trap()`.
5. `trap()` sees `tf->trapno == T_SYSCALL` and calls `syscall()`.
6. `syscall()` reads `num = curproc->tf->eax`, looks up
   `syscalls[num]`, calls it, and stores the return value back into
   `tf->eax`.
7. When `trap()` returns, `trapret` restores user registers from the
   trapframe and `iret`s back to user space. The user sees the return
   value in `%eax`.

### Q2. Why is the syscall table `static int (*syscalls[])(void)`?

`static` keeps it private to `syscall.c` (encapsulation). The array is
indexed by syscall number, so the lookup is O(1). Each entry is a
pointer to a function returning `int` and taking no arguments — the
arguments come *implicitly* via the trapframe and are extracted with
`argint`/`argptr`/`argstr`.

### Q3. Why don't kernel functions take arguments directly?

The arguments live on the **user stack**, not the kernel stack. The
helpers `argint(n, &x)` and `argptr(n, &p, sz)` walk into the user's
address space safely (they validate the address belongs to the
process) and copy the value into a kernel variable. Reading user
memory directly without these helpers is a classic kernel bug.

### Q4. What is `myproc()` and why is it not just a global?

It returns the `struct proc *` of the process running on **this CPU**.
On a multi-core system there can be `NCPU` different "current
processes". `myproc()` disables interrupts briefly, looks at
`mycpu()->proc`, then re-enables them — preventing a context switch
mid-read.

### Q5. Why do we need `ptable.lock`?

`ptable` is a single global array of `NPROC` process slots. Multiple
CPUs read and write it simultaneously (the scheduler picks runnable
processes from it; `wait` reaps zombies; `fork` allocates new slots).
Without a lock you'd see torn reads, double-allocated PIDs, or two
CPUs running the same process. xv6 uses a spinlock because the
critical sections are very short.

### Q6. What's the difference between a spinlock and a sleeplock?

- **Spinlock**: busy-waits in a tight loop. Used in interrupt
  handlers and short critical sections (like `ptable.lock`). The
  holder must not sleep.
- **Sleeplock**: if the lock is held, the caller `sleep()`s and is
  woken up when it's released. Used for long operations like disk
  I/O. Can only be acquired from a context where sleeping is
  allowed.

### Q7. How does `waitx` compute wait time?

Total lifetime = `etime - ctime` (exit minus creation tick).  
Of that, `rtime` ticks were spent **running** (we incremented it from
the timer ISR). Everything else was spent either `RUNNABLE`
(waiting for the CPU) or `SLEEPING` (waiting for I/O):

```
wtime = etime - ctime - rtime
```

### Q8. Why update `rtime` from the timer interrupt and not from the scheduler?

The timer interrupt fires every 10 ms regardless of what the CPU is
doing. By updating `rtime` there, we get a steady, fair tick-based
accounting. Updating in the scheduler would only count switches,
which is much coarser.

### Q9. What is `EMBRYO`, `RUNNABLE`, `RUNNING`, `SLEEPING`, `ZOMBIE`?

The five non-trivial states a process can be in:

- **EMBRYO** — `allocproc()` has reserved the slot but the new
  process isn't ready yet.
- **RUNNABLE** — ready to run, waiting for the scheduler to pick it.
- **RUNNING** — currently executing on some CPU.
- **SLEEPING** — blocked on a channel (e.g. `sleep(chan)`); only
  `wakeup(chan)` will move it back to RUNNABLE.
- **ZOMBIE** — has called `exit()` but its parent hasn't `wait()`-ed
  for it yet, so its slot is still kept around for the return code.

---

## Section B — Process & file syscalls (Project 2)

### Q1. What does `fork()` do?

It creates an exact copy of the calling process. The kernel:

1. Allocates a new `struct proc` slot.
2. Copies the parent's page table (`copyuvm`) — modern kernels use
   copy-on-write but xv6 just duplicates the pages.
3. Copies the parent's open file table.
4. Returns **0 in the child** and **the child's PID in the parent**.

### Q2. What does `exec()` do? Why is `execvp` different?

`exec()` *replaces* the current process image with a new program.
`execvp` is a libc wrapper that searches `PATH` and takes a
NULL-terminated `argv` array. After a successful `execvp` you never
return from it — the new program takes over the process.

### Q3. Why does our shell `fork` then `exec`?

We want the child to run the new program but the shell to keep
running. `fork` clones; the child immediately `exec`s into the
target program; the parent `waitpid`s. This is *the* fundamental
UNIX idiom.

### Q4. Why do `cd` and `exit` have to be built-ins?

Each process has its own current working directory and its own
address space. If `cd` were an external program:

1. The shell would `fork` a child.
2. The child would call `chdir()` — but only the *child's* CWD
   would change.
3. The child would exit and the parent shell's CWD would be
   unchanged.

Same logic for `exit` — exiting a child wouldn't exit the shell.

### Q5. Why does the shell ignore `SIGINT`?

When you press Ctrl-C the kernel sends `SIGINT` to every process in
the foreground process group. We want the *running command* to die,
not the shell. `signal(SIGINT, SIG_IGN)` in the parent + restoring
`SIG_DFL` in the child gives that behavior.

### Q6. What's the difference between a system call and a library call?

A library call (like `fopen`) is plain C code in user space. It
eventually invokes a system call (like `open`) when it needs the
kernel to do something. The system call is the boundary between
user mode and kernel mode and costs ~100x more than a normal
function call (mode switch, register save, TLB pressure).

### Q7. Why does `read()` sometimes return fewer bytes than asked?

- It reached end-of-file.
- It was interrupted by a signal (`EINTR`).
- The underlying device only had a partial chunk available
  (sockets, pipes, terminals).

That's why every robust copy loop wraps `read`/`write` in a loop
that re-issues the call until the buffer is fully drained.

### Q8. What does `unlink` actually do?

It removes a directory entry. The inode's link count drops by 1.
The kernel only frees the inode (and its data blocks) when **both**:

1. The link count is 0 — no directory entry references it anymore.
2. No process still has the file open.

That's why you can `rm` a log file while a daemon is writing to it
and the daemon can keep writing — it's holding the inode open.

### Q9. Why does `rename()` fail with `EXDEV` across mount points?

`rename()` is just an inode-level relink: same filesystem, same
inode, new directory entry. Across filesystems the inode lives on a
different device, so the kernel can't relink — you have to copy the
data over and unlink the original. That's exactly what our
`custom_mv` falls back to.

### Q10. How does `opendir`/`readdir` work under the hood?

A directory is just a special file whose contents are a sequence of
`(inode_number, name)` records. `opendir` `open()`s the directory;
each `readdir` reads the next record and returns a `struct dirent`.
`closedir` closes the underlying file descriptor.

---

## Section C — Common follow-ups

### Q1. What is a process? What is a thread?

A **process** is the kernel's unit of resource ownership: address
space, open files, PID, signal handlers. A **thread** is the unit
of scheduling: a stack and a set of registers. Multiple threads
can share one process's address space, which is why they can
communicate cheaply but also why they need locks.

### Q2. What is the difference between user mode and kernel mode?

Kernel mode (ring 0 on x86) can execute privileged instructions
(I/O ports, page-table writes, `cli`/`sti`, etc.). User mode
(ring 3) cannot — attempting them faults. The CPU enforces this
with a 2-bit field in the code segment selector. System calls are
the controlled entry from ring 3 to ring 0.

### Q3. What is a context switch?

Saving the current process's CPU state (registers, EIP, ESP) to
its `struct context`, loading another process's state, and
returning. In xv6 this is `swtch.S`. It's expensive because of
TLB flushes and cache pollution.

### Q4. What is a race condition? How do we prevent it?

When the result of two concurrent operations depends on their
ordering. We prevent races by **mutual exclusion** — only one
thread/CPU may be in the critical section at a time. xv6 uses
spinlocks (`acquire`/`release`); POSIX threads use
`pthread_mutex_lock`.

### Q5. What is a deadlock? Give an example.

Four conditions: mutual exclusion, hold-and-wait, no preemption,
circular wait. Example: thread A holds lock X and wants Y; thread
B holds Y and wants X. Both wait forever. Avoidance: always
acquire locks in the same global order.
