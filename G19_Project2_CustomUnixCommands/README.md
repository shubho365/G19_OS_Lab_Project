# Project 2 — Custom UNIX-like Utilities

**Course:** NSCS210 / CSC211 — Operating Systems Lab  
**Instructor:** Dr. Jaishree Mayank  
**Department of CSE, IIT (ISM) Dhanbad**

This project implements a small collection of UNIX-style command-line
utilities **from scratch in C**, plus a minimal shell that runs them.
Every utility is prefixed `custom_` so it cannot be confused with the
real GNU coreutils.

## Folder layout

```
Group_Project2_1/
├── Makefile
├── README.md
├── docs/
│   └── screenshots/      <-- put your execution screenshots here
└── src/
    ├── custom_shell.c
    ├── custom_ls.c
    ├── custom_cat.c
    ├── custom_grep.c
    ├── custom_wc.c
    ├── custom_cp.c
    ├── custom_mv.c
    └── custom_rm.c
```

## Building

```bash
make             # builds everything into ./bin/
make test        # runs an automatic smoke test of every utility
make run         # launches the custom shell
make clean       # deletes ./bin and any temp files
```

The `bin/` directory is added to `PATH` automatically by `make run`,
so inside the shell you can simply type `custom_ls`, `custom_cat …`,
etc.

## Utilities at a glance

| Utility       | What it does                          | Key syscalls used                       |
|---------------|----------------------------------------|------------------------------------------|
| `custom_ls`   | List directory contents (`-l` long)    | `opendir`, `readdir`, `stat`, `closedir` |
| `custom_cat`  | Print files to stdout                  | `open`, `read`, `write`, `close`         |
| `custom_grep` | Print lines containing a substring     | `fopen`, `fgets` (line-buffered I/O)     |
| `custom_wc`   | Count lines, words, and characters     | `fopen`, `fgetc`                         |
| `custom_cp`   | Copy a file                            | `open`, `read`, `write`, `fstat`         |
| `custom_mv`   | Rename / move a file                   | `rename` + `EXDEV` fallback              |
| `custom_rm`   | Delete file or directory tree (`-r`)   | `unlink`, `rmdir`, `lstat`, `opendir`    |

## The shell — `custom_shell`

The shell is a classic **read-eval-print loop**:

1. Print the prompt and `fgets()` one line of input.
2. Tokenize on whitespace with `strtok()`.
3. If the first token is a built-in (`cd`, `exit`, `help`), run it
   *inside* the shell process — `cd` cannot be done in a child because
   each process has its own current working directory.
4. Otherwise call `fork()`. The child uses `execvp()` to load the
   requested program; the parent calls `waitpid()` so it waits for the
   child to finish before printing the next prompt.
5. The shell **ignores `SIGINT`** so a stray Ctrl-C only kills the
   running child, never the shell itself. The child restores the
   default `SIGINT` handler before `execvp()` so it *can* be killed.

This is exactly the model used by `bash`, `dash`, and every other
POSIX shell — only stripped down to the bare essentials.

## Why we use raw `open/read/write` instead of `fopen/fread`

`fopen` and friends are part of the C standard library; internally they
buffer data in user space and eventually call the underlying POSIX
syscalls anyway. For an Operating Systems course we deliberately use
the lower-level API in `custom_cat`, `custom_cp`, and `custom_mv` so
the system-call boundary is visible. `custom_grep` and `custom_wc`
need *line-oriented* input, where stdio is far more convenient, so
they use `fopen` / `fgetc`.

## Reproducing the screenshots in `docs/`

```bash
make
PATH="$PWD/bin:$PATH" ./bin/custom_shell
myshell> help
myshell> custom_ls -l
myshell> custom_cat src/custom_cat.c
myshell> custom_grep open src/custom_cat.c
myshell> custom_wc src/custom_shell.c
myshell> custom_cp src/custom_ls.c /tmp/ls_backup.c
myshell> custom_mv /tmp/ls_backup.c /tmp/ls_renamed.c
myshell> custom_rm /tmp/ls_renamed.c
myshell> exit
```

Take screenshots of each command and save them under
`docs/screenshots/`.

## Group members & contributions

| Name | Roll No | Contribution |
|------|---------|--------------|
| _to fill_ | _to fill_ | Shell, custom_ls, custom_cat |
| _to fill_ | _to fill_ | custom_grep, custom_wc |
| _to fill_ | _to fill_ | custom_cp, custom_mv, custom_rm |
| _to fill_ | _to fill_ | Makefile, README, screenshots |
