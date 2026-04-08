/*
 * custom_shell.c
 * ---------------
 * A minimal UNIX-like shell that runs the custom_* utilities
 * (custom_ls, custom_cat, custom_grep, custom_wc, custom_cp,
 *  custom_mv, custom_rm) as well as any program on PATH.
 *
 * Key OS concepts demonstrated:
 *   - fork() to create a child process
 *   - execvp() to replace the child's image with the requested program
 *   - waitpid() so the parent waits for the child to finish
 *   - signal() to ignore SIGINT in the parent so Ctrl-C kills only the child
 *   - Built-in commands (cd, exit, help) that MUST run in the shell process
 *     itself (because cd inside a child would not change the parent's CWD)
 *
 * Build: gcc -Wall -o custom_shell custom_shell.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_LINE   1024     /* maximum characters in one input line   */
#define MAX_TOKENS  64      /* maximum number of whitespace tokens    */
#define PROMPT     "myshell> "

/* ------------------------------------------------------------------ */
/*  Tokenizer                                                          */
/* ------------------------------------------------------------------ */
/* Splits `line` in-place using whitespace as the delimiter and stores
 * pointers to each token in `tokens[]`. Returns the number of tokens. */
static int tokenize(char *line, char *tokens[])
{
    int count = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok != NULL && count < MAX_TOKENS - 1) {
        tokens[count++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    tokens[count] = NULL;        /* execvp() needs a NULL-terminated list */
    return count;
}

/* ------------------------------------------------------------------ */
/*  Built-in commands                                                  */
/* ------------------------------------------------------------------ */
/* Returns 1 if the command was a built-in (and was handled),
 * 0 otherwise so that the caller knows it must fork+exec. */
static int run_builtin(char *tokens[], int n)
{
    if (n == 0) return 1;                       /* empty line */

    if (strcmp(tokens[0], "exit") == 0) {
        printf("Goodbye!\n");
        exit(EXIT_SUCCESS);
    }

    if (strcmp(tokens[0], "cd") == 0) {
        const char *path = (n >= 2) ? tokens[1] : getenv("HOME");
        if (chdir(path) != 0)
            perror("cd");
        return 1;
    }

    if (strcmp(tokens[0], "help") == 0) {
        printf("Built-ins: cd <dir>, exit, help\n");
        printf("Custom utilities you can run:\n");
        printf("  custom_ls [dir]\n");
        printf("  custom_cat <file> [file ...]\n");
        printf("  custom_grep <pattern> <file>\n");
        printf("  custom_wc <file>\n");
        printf("  custom_cp <src> <dst>\n");
        printf("  custom_mv <src> <dst>\n");
        printf("  custom_rm [-r] <path>\n");
        return 1;
    }
    return 0;     /* not a built-in */
}

/* ------------------------------------------------------------------ */
/*  External-command launcher                                          */
/* ------------------------------------------------------------------ */
static void launch_external(char *tokens[])
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        /* --- Child process --- */
        /* Restore default Ctrl-C handling so the child can be killed. */
        signal(SIGINT, SIG_DFL);

        /* execvp searches PATH. We assume the user has added the
         * project directory to PATH (or is running from it with `./`). */
        execvp(tokens[0], tokens);
        /* If exec returns, it failed. */
        fprintf(stderr, "myshell: %s: %s\n", tokens[0], strerror(errno));
        exit(127);
    } else {
        /* --- Parent process --- */
        int status;
        if (waitpid(pid, &status, 0) < 0)
            perror("waitpid");
    }
}

/* ------------------------------------------------------------------ */
/*  Main read-eval-print loop                                          */
/* ------------------------------------------------------------------ */
int main(void)
{
    char  line[MAX_LINE];
    char *tokens[MAX_TOKENS];

    /* Ignore Ctrl-C in the shell so a stray Ctrl-C doesn't kill the shell;
     * the child process restores the default handler so it can be killed. */
    signal(SIGINT, SIG_IGN);

    printf("==== Custom UNIX-like Shell ====\n");
    printf("Type 'help' to see available commands, 'exit' to quit.\n\n");

    while (1) {
        printf(PROMPT);
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL) {
            printf("\n");
            break;                           /* EOF (Ctrl-D) */
        }

        int n = tokenize(line, tokens);
        if (n == 0) continue;

        if (run_builtin(tokens, n)) continue;

        launch_external(tokens);
    }
    return 0;
}
