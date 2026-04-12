/*
 * custom_chmod.c — Change file permissions (simplified UNIX chmod)
 * ================================================================
 *
 * Usage:
 *   custom_chmod <mode> <file> [file ...]
 *
 * <mode> is an octal permission string (e.g. 755, 644, 400).
 *
 * Examples:
 *   custom_chmod 755 script.sh
 *   custom_chmod 644 readme.txt notes.txt
 *   custom_chmod 600 secret.key
 *
 * OS concepts demonstrated:
 *   - chmod() system call for modifying inode permission bits
 *   - Octal string parsing (strtol with base 8)
 *   - Error handling with errno / perror
 *
 * Build:
 *   gcc -Wall -Wextra -O2 -o custom_chmod custom_chmod.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* ------------------------------------------------------------------ */
/*  Print usage information                                            */
/* ------------------------------------------------------------------ */
static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s <mode> <file> [file ...]\n", progname);
    fprintf(stderr, "  <mode>  Octal permission (e.g. 755, 644, 400)\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  %s 755 script.sh\n", progname);
    fprintf(stderr, "  %s 644 readme.txt notes.txt\n", progname);
}

/* ------------------------------------------------------------------ */
/*  Parse an octal mode string, return -1 on error                     */
/* ------------------------------------------------------------------ */
static int parse_mode(const char *str, mode_t *mode)
{
    char *end;
    long val;

    /* Reject empty strings */
    if (str == NULL || *str == '\0')
        return -1;

    errno = 0;
    val = strtol(str, &end, 8);   /* base-8 (octal) */

    /* Check for parse errors */
    if (errno != 0 || *end != '\0')
        return -1;

    /* Valid UNIX permission range: 0000 – 7777 */
    if (val < 0 || val > 07777)
        return -1;

    *mode = (mode_t)val;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    mode_t mode;
    int i, errors = 0;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    /* Parse the octal mode argument */
    if (parse_mode(argv[1], &mode) < 0) {
        fprintf(stderr, "custom_chmod: invalid mode '%s'\n", argv[1]);
        return 1;
    }

    /* Apply chmod() to each file argument */
    for (i = 2; i < argc; i++) {
        if (chmod(argv[i], mode) < 0) {
            fprintf(stderr, "custom_chmod: cannot change permissions of '%s': %s\n",
                    argv[i], strerror(errno));
            errors++;
        } else {
            printf("'%s' -> %04o\n", argv[i], (unsigned)mode);
        }
    }

    return (errors > 0) ? 1 : 0;
}
