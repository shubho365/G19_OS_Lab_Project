# Project 1: XV6 System Calls Customization

## Overview
This project adds extensive new functionalities to the `xv6` kernel, introducing several system calls focusing on Process Creation, Priority, IPC (Inter-process Communication), Signals, and Locks (Mutex).

## System Calls Added

### 1. Process Attributes and Scheduling
- `SYS_getppid (22)`: Returns the Process ID of the parent process.
- `SYS_getprocs (23)`: Counts the total number of active/runnable processes in the system.
- `SYS_set_priority (24)`: Changes process precedence for scheduler manipulation.
- `SYS_cps (25)`: Custom ps-like functionality that lists all processes, states, priorities, and names.
- `SYS_waitx (26)`: Like `wait()`, but returns wait times and run times back into user space.
- `SYS_waitpid (27)`: Will strictly block until a *specific* PID completes instead of any child.
- `SYS_fork2 (30)`: Allows priority assignment directly during the fork sequence.

### 2. Signals
- `SYS_sigkill (28)`: Directly send a kill signal to an arbitrary PID without using CLI utilities.

### 3. Inter-process Communication (IPC / Shared Memory)
- `SYS_shmem (29)`
- `SYS_shmget (31)`
- `SYS_shmem_count (32)`
Permits multiple independent processes to request pages mapped to the exact same physical frame, allowing zero-copy sharing of variables.

### 4. Locks (Mutexes)
- `SYS_mutex_create (33)`: Instantiates a unique kernel-level Mutex in an array.
- `SYS_mutex_lock (34)`: Reassigns CPU (`sleep()`) if a Mutex is held, preventing busy waiting spin locking.
- `SYS_mutex_unlock (35)`: Re-awakes (`wakeup()`) suspended processes eagerly waiting on the Mutex ID queue.

## Group Contribution

Each member participated equally in drafting and executing the codebase modifications, testing, and verifying the xv6 kernel stability.
