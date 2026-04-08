# Project 1 — xv6 Custom System Calls

**Course:** NSCS210 / CSC211 — Operating Systems Lab  
**Instructor:** Dr. Jaishree Mayank  
**Department of CSE, IIT (ISM) Dhanbad**

This project modifies the **xv6-public (x86)** teaching operating system
to add **five new system calls** spanning process management, locking,
and process accounting.

## What's in this folder

```
Group_Project1_xv6CustomizeSystemCalls/
├── README.md           <-- this file
├── SETUP.md            <-- install guide for Ubuntu / WSL (Linux)
├── SETUP-MAC.md        <-- install guide for macOS (Apple Silicon M1/M2/M3)
├── INSTRUCTIONS.md     <-- step-by-step kernel modifications
├── THEORY.md           <-- viva-prep cheat sheet
├── user_program/
│   └── syscalltest.c   <-- xv6 user program that calls every new syscall
├── patches/            <-- you may keep diffed copies of modified xv6 files here
└── docs/
    └── screenshots/    <-- put your QEMU screenshots here
```

## The five system calls

| # | Name | Signature | Purpose |
|---|---|---|---|
| 1 | `getppid` | `int getppid(void)` | Returns parent's PID |
| 2 | `getprocs` | `int getprocs(void)` | Counts active processes |
| 3 | `set_priority` | `int set_priority(int pid, int prio)` | Sets priority of a process |
| 4 | `cps` | `int cps(void)` | Prints all processes (ps-like) |
| 5 | `waitx` | `int waitx(int *wtime, int *rtime)` | wait() + run/wait timing |

These touch the rubric's required areas:

- **Process management:** `getppid`, `getprocs`, `cps`
- **Locks (kernel synchronization):** `set_priority`, `cps`, `waitx`
  all hold `ptable.lock` while reading or modifying the process table
- **Scheduling / accounting:** `waitx` works only because we extended
  `struct proc` and updated `rtime` from the timer interrupt

## How to reproduce

1. Install xv6 and tool-chain — **SETUP.md** for Ubuntu/WSL, **SETUP-MAC.md** for Apple Silicon Macs.
2. Apply each modification — follow **INSTRUCTIONS.md** in order.
3. Copy `user_program/syscalltest.c` into the xv6 source directory.
4. Build and run:

   ```bash
   make clean
   make qemu-nox
   ```

5. Inside the xv6 shell:

   ```
   $ syscalltest
   ```

6. Take a screenshot of the output and save it under
   `docs/screenshots/syscalltest.png`.

## Files modified inside xv6-public

| File | What changed |
|------|--------------|
| `syscall.h` | Added 5 `SYS_*` numbers |
| `syscall.c` | 5 `extern` decls + 5 dispatch table entries |
| `sysproc.c` | 5 `sys_*` implementations |
| `usys.S` | 5 `SYSCALL(name)` stubs |
| `user.h` | 5 user-space prototypes |
| `proc.h` | Added `priority`, `ctime`, `etime`, `rtime` fields |
| `proc.c` | Init fields in `allocproc`, set `etime` in `exit`, helper `update_rtime` |
| `trap.c` | Call `update_rtime()` from timer ISR |
| `defs.h` | Prototype for `update_rtime` |
| `Makefile` | `_syscalltest` in `UPROGS` and `syscalltest.c` in `EXTRA` |

## Group members & contributions

| Name | Roll No | Contribution |
|------|---------|--------------|
| _to fill_ | _to fill_ | `getppid`, `getprocs`, syscall plumbing |
| _to fill_ | _to fill_ | `set_priority`, `cps`, locking |
| _to fill_ | _to fill_ | `waitx`, `proc.h` & `trap.c` accounting |
| _to fill_ | _to fill_ | `syscalltest.c`, documentation, screenshots |
