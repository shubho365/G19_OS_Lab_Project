/*
 * custom_grep.c
 * --------------
 * Prints lines from a file that contain a given substring.
 *
 * Usage: custom_grep <pattern> <file>
 *
 * Notes:
 *   - We implement plain substring matching (no regex) so the code stays
 *     small and easy to defend in the viva. The C library function
 *     strstr() does Boyer-Moore-style matching internally.
 *   - We use the FILE* (stdio) wrapper here because line-oriented input
 *     is much easier with fgets() than with raw read().
 *   - We print the matching lines with a 1-based line number, mimicking
 *     `grep -n` so the output looks familiar.
 *
 * Build: gcc -Wall -o custom_grep custom_grep.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE 4096

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: custom_grep <pattern> <file>\n");
        return 1;
    }

    const char *pattern = argv[1];
    const char *path    = argv[2];

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "custom_grep: %s: %s\n", path, strerror(errno));
        return 1;
    }

    char  line[MAX_LINE];
    int   lineno = 0;
    int   matches = 0;

    while (fgets(line, sizeof line, fp) != NULL) {
        lineno++;
        if (strstr(line, pattern) != NULL) {
            /* fgets keeps the trailing '\n', so no extra newline needed. */
            printf("%d: %s", lineno, line);
            matches++;
        }
    }

    fclose(fp);
    /* Exit code mirrors grep: 0 if found, 1 if not. */
    return matches > 0 ? 0 : 1;
}
