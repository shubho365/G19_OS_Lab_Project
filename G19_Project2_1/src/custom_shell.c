/*
 * custom_shell.c  —  Enhanced UNIX-like Shell
 * =============================================
 * A minimal but capable UNIX shell for the OS Lab project.
 *
 * OS concepts demonstrated:
 *   fork()      — create child processes for every external command
 *   execvp()    — replace child image with the requested program
 *   waitpid()   — parent waits for child(ren) to finish
 *   pipe()      — create a unidirectional kernel buffer between two processes
 *   dup2()      — redirect stdin / stdout by duplicating file descriptors
 *   open()      — open/create files for I/O redirection
 *   signal()    — ignore SIGINT in the shell so Ctrl-C kills only the child
 *   getcwd()    — query the kernel for the current working directory (pwd)
 *
 * Features:
 *   - Built-ins : cd, exit, help, pwd, echo
 *   - Pipe      : cmd1 | cmd2
 *   - Redirect  : cmd > file   cmd >> file   cmd < file
 *   - External  : any program on PATH via execvp()
 *
 * Build : gcc -Wall -Wextra -O2 -o custom_shell custom_shell.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_LINE    1024
#define MAX_TOKENS   64
#define PROMPT      "myshell> "

/* ------------------------------------------------------------------ */
/*  Tokenizer                                                          */
/* ------------------------------------------------------------------ */
static int tokenize(char *line, char *tokens[])
{
    int count = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok && count < MAX_TOKENS - 1) {
        tokens[count++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    tokens[count] = NULL;
    return count;
}

/* ------------------------------------------------------------------ */
/*  I/O Redirection helper                                             */
/* ------------------------------------------------------------------ */
/*
 * Scans tokens[] for >, >>, < operators. When found, opens the file
 * and redirects the appropriate file descriptor using dup2(). The
 * operator and filename tokens are removed from the array (shifted
 * left) so execvp() never sees them.
 *
 * Called INSIDE the child process after fork(), before execvp().
 */
static void apply_redirections(char *tokens[], int *n)
{
    int i = 0;
    while (i < *n) {
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) {
            if (i + 1 >= *n) {
                fprintf(stderr, "myshell: syntax error near '%s'\n", tokens[i]);
                exit(1);
            }
            int flags = O_WRONLY | O_CREAT |
                        (strcmp(tokens[i], ">>") == 0 ? O_APPEND : O_TRUNC);
            int fd = open(tokens[i + 1], flags, 0644);
            if (fd < 0) { perror(tokens[i + 1]); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            /* Remove operator + filename from tokens */
            int rm = i;
            for (; rm + 2 < *n; rm++) tokens[rm] = tokens[rm + 2];
            *n -= 2;
            tokens[*n] = NULL;
        } else if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 >= *n) {
                fprintf(stderr, "myshell: syntax error near '<'\n");
                exit(1);
            }
            int fd = open(tokens[i + 1], O_RDONLY);
            if (fd < 0) { perror(tokens[i + 1]); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
            int rm = i;
            for (; rm + 2 < *n; rm++) tokens[rm] = tokens[rm + 2];
            *n -= 2;
            tokens[*n] = NULL;
        } else {
            i++;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Find pipe position                                                 */
/* ------------------------------------------------------------------ */
/* Returns index of '|' in tokens[], or -1 if not found. */
static int find_pipe(char *tokens[], int n)
{
    for (int i = 0; i < n; i++)
        if (strcmp(tokens[i], "|") == 0)
            return i;
    return -1;
}

/* ------------------------------------------------------------------ */
/*  Built-in commands                                                  */
/* ------------------------------------------------------------------ */
static int run_builtin(char *tokens[], int n)
{
    if (n == 0) return 1;

    /* exit */
    if (strcmp(tokens[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(EXIT_SUCCESS);
    }

    /* cd [dir] */
    if (strcmp(tokens[0], "cd") == 0) {
        const char *path = (n >= 2) ? tokens[1] : getenv("HOME");
        if (!path) path = "/";
        if (chdir(path) != 0) perror("cd");
        return 1;
    }

    /* pwd */
    if (strcmp(tokens[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof cwd))
            printf("%s\n", cwd);
        else
            perror("pwd");
        return 1;
    }

    /* echo [args...] */
    if (strcmp(tokens[0], "echo") == 0) {
        for (int i = 1; i < n; i++) {
            printf("%s", tokens[i]);
            if (i < n - 1) printf(" ");
        }
        printf("\n");
        return 1;
    }

    /* help */
    if (strcmp(tokens[0], "help") == 0) {
        printf("\n=== Custom Shell — Available Commands ===\n\n");
        printf("Built-ins (run in the shell process itself):\n");
        printf("  cd <dir>       Change working directory\n");
        printf("  pwd            Print working directory\n");
        printf("  echo <msg>     Print a message\n");
        printf("  exit           Quit the shell\n");
        printf("  help           Show this help\n\n");
        printf("Custom utilities:\n");
        printf("  custom_ls   [-l] [dir]           List directory contents\n");
        printf("  custom_cat  <file> [file ...]     Print files to stdout\n");
        printf("  custom_grep <pattern> <file>      Search for pattern in file\n");
        printf("  custom_wc   <file>                Count lines/words/chars\n");
        printf("  custom_cp   <src> <dst>           Copy a file\n");
        printf("  custom_mv   <src> <dst>           Move/rename a file\n");
        printf("  custom_rm   [-r] <path>           Remove file or directory\n");
        printf("  custom_head [-n N] <file>         Print first N lines (default 10)\n\n");
        printf("Shell features:\n");
        printf("  cmd1 | cmd2                       Pipe output of cmd1 into cmd2\n");
        printf("  cmd > file                        Redirect stdout to file\n");
        printf("  cmd >> file                       Append stdout to file\n");
        printf("  cmd < file                        Redirect stdin from file\n\n");
        return 1;
    }

    return 0;   /* not a built-in */
}

/* ------------------------------------------------------------------ */
/*  Single command (no pipe)                                           */
/* ------------------------------------------------------------------ */
static void launch_single(char *tokens[], int n)
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        /* Child: restore Ctrl-C so it can be interrupted. */
        signal(SIGINT, SIG_DFL);
        /* Apply any I/O redirections before exec. */
        apply_redirections(tokens, &n);
        execvp(tokens[0], tokens);
        fprintf(stderr, "myshell: %s: %s\n", tokens[0], strerror(errno));
        exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

/* ------------------------------------------------------------------ */
/*  Two-command pipeline: left_tokens | right_tokens                  */
/* ------------------------------------------------------------------ */
/*
 * How it works:
 *   1. pipe(pfd) creates a kernel FIFO: pfd[1] is the write-end,
 *      pfd[0] is the read-end.
 *   2. Left child:  close read-end, dup2 write-end onto stdout, exec.
 *   3. Right child: close write-end, dup2 read-end onto stdin,  exec.
 *   4. Parent closes both ends (otherwise right child never sees EOF)
 *      and waits for both children.
 */
static void launch_pipe(char *left[], int ln, char *right[], int rn)
{
    int pfd[2];
    if (pipe(pfd) < 0) { perror("pipe"); return; }

    /* --- Left child (writer) --- */
    pid_t pid_left = fork();
    if (pid_left < 0) { perror("fork"); return; }
    if (pid_left == 0) {
        signal(SIGINT, SIG_DFL);
        close(pfd[0]);                      /* don't need read end */
        dup2(pfd[1], STDOUT_FILENO);        /* stdout → pipe write-end */
        close(pfd[1]);
        apply_redirections(left, &ln);
        execvp(left[0], left);
        fprintf(stderr, "myshell: %s: %s\n", left[0], strerror(errno));
        exit(127);
    }

    /* --- Right child (reader) --- */
    pid_t pid_right = fork();
    if (pid_right < 0) { perror("fork"); return; }
    if (pid_right == 0) {
        signal(SIGINT, SIG_DFL);
        close(pfd[1]);                      /* don't need write end */
        dup2(pfd[0], STDIN_FILENO);         /* stdin  ← pipe read-end */
        close(pfd[0]);
        apply_redirections(right, &rn);
        execvp(right[0], right);
        fprintf(stderr, "myshell: %s: %s\n", right[0], strerror(errno));
        exit(127);
    }

    /* Parent: close both ends so right child sees EOF when left exits */
    close(pfd[0]);
    close(pfd[1]);

    /* Wait for both children */
    waitpid(pid_left,  NULL, 0);
    waitpid(pid_right, NULL, 0);
}

/* ------------------------------------------------------------------ */
/*  Main read-eval-print loop                                          */
/* ------------------------------------------------------------------ */
int main(void)
{
    char  line[MAX_LINE];
    char *tokens[MAX_TOKENS];

    /* Ignore Ctrl-C in the shell; children restore the default handler. */
    signal(SIGINT, SIG_IGN);

    printf("==== Custom UNIX-like Shell (G19) ====\n");
    printf("Type 'help' to see available commands, 'exit' to quit.\n\n");

    while (1) {
        printf(PROMPT);
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL) {
            printf("\n");
            break;
        }

        int n = tokenize(line, tokens);
        if (n == 0) continue;

        /* Built-ins run inside the shell process. */
        if (run_builtin(tokens, n)) continue;

        /* Check for a pipe. */
        int pipe_pos = find_pipe(tokens, n);
        if (pipe_pos >= 0) {
            /* Split into left and right command arrays. */
            char *left[MAX_TOKENS], *right[MAX_TOKENS];
            int ln = 0, rn = 0;
            for (int i = 0; i < pipe_pos; i++) left[ln++]  = tokens[i];
            for (int i = pipe_pos + 1; i < n; i++) right[rn++] = tokens[i];
            left[ln]  = NULL;
            right[rn] = NULL;
            if (ln == 0 || rn == 0) {
                fprintf(stderr, "myshell: syntax error near '|'\n");
                continue;
            }
            launch_pipe(left, ln, right, rn);
        } else {
            launch_single(tokens, n);
        }
    }
    return 0;
}
