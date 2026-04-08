/*
 * custom_wc.c
 * ------------
 * Counts lines, words, and characters in a file.
 *
 * Usage: custom_wc <file>
 *
 * Algorithm:
 *   - chars : every byte we read.
 *   - lines : every '\n' we encounter.
 *   - words : we maintain an `in_word` flag and increment whenever we
 *             transition from "whitespace" to "non-whitespace".
 *
 * The same finite-state-machine technique is used by the real wc.
 *
 * Build: gcc -Wall -o custom_wc custom_wc.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: custom_wc <file>\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "custom_wc: %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    long lines = 0, words = 0, chars = 0;
    int  in_word = 0;
    int  c;

    while ((c = fgetc(fp)) != EOF) {
        chars++;
        if (c == '\n') lines++;

        if (isspace(c)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    fclose(fp);

    printf("  %ld %ld %ld %s\n", lines, words, chars, argv[1]);
    return 0;
}
