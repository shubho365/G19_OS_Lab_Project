# Project 2 — Custom UNIX-like Utilities

**Course:** NSCS210 / CSC211 — Operating Systems Lab  
**Instructor:** Dr. Jaishree Mayank  
**Department of CSE, IIT (ISM) Dhanbad**

This project implements a collection of UNIX-style command-line utilities **from scratch in C**, plus a capable shell that runs them. Every utility is prefixed `custom_` to avoid confusion with standard UNIX commands.

## Folder layout

```
G19_Project2_1/
├── Makefile
├── README.md
├── docs/
│   └── screenshots/      <- execution screenshots
└── src/
    ├── custom_shell.c
    ├── custom_ls.c
    ├── custom_cat.c
    ├── custom_grep.c
    ├── custom_wc.c
    ├── custom_cp.c
    ├── custom_mv.c
    ├── custom_rm.c
    ├── custom_head.c
    └── custom_tail.c     <- NEW
```

## Building

```bash
make             # builds everything into ./bin/
make test        # runs an automatic smoke test of every utility
make run         # launches the custom shell
make clean       # deletes ./bin and any temp files
```

## Utilities at a glance

| Utility        | What it does                           | Key syscalls used                        |
|----------------|----------------------------------------|------------------------------------------|
| `custom_ls`    | List directory contents (`-l` long)    | `opendir`, `readdir`, `stat`, `closedir` |
| `custom_cat`   | Print files to stdout                  | `open`, `read`, `write`, `close`         |
| `custom_grep`  | Print lines containing a pattern       | `fopen`, `fgets`, `strstr`               |
| `custom_wc`    | Count lines, words, and characters     | `fopen`, `fgetc`                         |
| `custom_cp`    | Copy a file                            | `open`, `read`, `write`, `fstat`         |
| `custom_mv`    | Rename / move a file                   | `rename` + `EXDEV` fallback              |
| `custom_rm`    | Delete file or directory tree (`-r`)   | `unlink`, `rmdir`, `lstat`, `opendir`    |
| `custom_head`  | Print first N lines of a file          | `open`, `read`, `write`, `close`         |
| `custom_tail`  | Print last N lines of a file           | `open`, `read`, `write`, `close`         |

## The shell — `custom_shell`

The shell implements a classic **read-eval-print loop** with several OS-level features:

### Built-in commands
| Command        | Description |
|----------------|-------------|
| `cd <dir>`     | Change directory (`chdir` syscall; must run in shell process) |
| `pwd`          | Print working directory (`getcwd`) |
| `echo <msg>`   | Print a message |
| `exit`         | Quit the shell |
| `help`         | Show all commands |

### Pipe support — `cmd1 | cmd2`
1. `pipe(pfd)` creates a kernel FIFO (two file descriptors).
2. **Left child**: `dup2(pfd[1], STDOUT_FILENO)` → writes to pipe; `exec` left command.
3. **Right child**: `dup2(pfd[0], STDIN_FILENO)` → reads from pipe; `exec` right command.
4. Parent closes both ends so the right child sees EOF when the left exits, then `waitpid` for both.

### I/O Redirection
| Operator   | Effect                                   | Syscalls |
|------------|------------------------------------------|----------|
| `cmd > f`  | Overwrite file `f` with stdout           | `open(O_WRONLY\|O_CREAT\|O_TRUNC)`, `dup2` |
| `cmd >> f` | Append stdout to file `f`               | `open(O_WRONLY\|O_CREAT\|O_APPEND)`, `dup2` |
| `cmd < f`  | Use file `f` as stdin                    | `open(O_RDONLY)`, `dup2` |

### SIGINT handling
The shell ignores Ctrl-C (`signal(SIGINT, SIG_IGN)`) so only the running child — which restores the default handler before `execvp()` — gets killed.

## Example session

```bash
make run
myshell> help
myshell> pwd
myshell> echo Hello from G19!
myshell> custom_ls -l
myshell> custom_cat src/custom_cat.c | custom_grep open
myshell> custom_head -n 5 src/custom_shell.c
myshell> custom_ls > listing.txt
myshell> custom_cat listing.txt
myshell> custom_wc < listing.txt
myshell> exit
```

## Why raw `open/read/write` instead of `fopen/fread`

`fopen` and friends buffer data in user space and internally call POSIX syscalls. For an OS course we use the lower-level API in `custom_cat`, `custom_cp`, `custom_head`, and `custom_mv` so the system-call boundary is visible.

## Group members & contributions

| Name | Roll No | Contribution |
|------|---------|--------------|
| _to fill_ | _to fill_ | Shell (pipe, redirection), custom_head |
| _to fill_ | _to fill_ | custom_ls, custom_cat |
| _to fill_ | _to fill_ | custom_grep, custom_wc |
| _to fill_ | _to fill_ | custom_cp, custom_mv, custom_rm, Makefile |
