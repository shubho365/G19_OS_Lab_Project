/*
 * custom_sort.c
 * ---------------
 * Sort lines of a text file and print to stdout (like the standard `sort`).
 *
 * Usage:
 *   custom_sort <file>         -> sort lines alphabetically (ascending)
 *   custom_sort -r <file>      -> sort lines in reverse order
 *   custom_sort -n <file>      -> sort lines numerically
 *   custom_sort -n -r <file>   -> sort lines numerically in reverse order
 *
 * OS concepts:
 *   - open() / read() / close() : raw POSIX I/O to read the file
 *   - Dynamic memory (malloc / realloc / free) : demonstrates how a
 *     user-space process requests heap memory from the kernel via brk/mmap
 *     behind the scenes — the allocator carves that memory into the array
 *     of line pointers we use here
 *   - qsort() : the C library quicksort; we supply custom comparators
 *     to show function pointers and the compare-based sorting API
 *
 * Build: gcc -Wall -Wextra -O2 -o custom_sort custom_sort.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 4096
#define INITIAL_LINES 256

/* Flags set by command-line options. */
static int opt_reverse  = 0;
static int opt_numeric  = 0;

/* Comparator for alphabetical sort. */
static int cmp_alpha(const void *a, const void *b)
{
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    int result = strcmp(sa, sb);
    return opt_reverse ? -result : result;
}

/* Comparator for numeric sort (compares leading integer values). */
static int cmp_numeric(const void *a, const void *b)
{
    long la = strtol(*(const char **)a, NULL, 10);
    long lb = strtol(*(const char **)b, NULL, 10);
    int result = (la > lb) - (la < lb);
    return opt_reverse ? -result : result;
}

/*
 * Read the entire file into a dynamically-allocated buffer.
 * Returns the buffer (caller must free) and sets *out_len.
 */
static char *read_file(const char *path, size_t *out_len)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "custom_sort: %s: %s\n", path, strerror(errno));
        return NULL;
    }

    size_t cap = BUF_SIZE;
    size_t len = 0;
    char *data = malloc(cap);
    if (!data) {
        perror("custom_sort: malloc");
        close(fd);
        return NULL;
    }

    ssize_t n;
    while ((n = read(fd, data + len, cap - len)) > 0) {
        len += (size_t)n;
        if (len == cap) {
            cap *= 2;
            char *tmp = realloc(data, cap);
            if (!tmp) {
                perror("custom_sort: realloc");
                free(data);
                close(fd);
                return NULL;
            }
            data = tmp;
        }
    }
    if (n < 0) {
        perror("custom_sort: read");
        free(data);
        close(fd);
        return NULL;
    }

    close(fd);
    *out_len = len;
    return data;
}

int main(int argc, char *argv[])
{
    const char *path = NULL;

    /* Argument parsing. */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            opt_reverse = 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            opt_numeric = 1;
        } else {
            path = argv[i];
        }
    }

    if (!path) {
        fprintf(stderr, "Usage: custom_sort [-r] [-n] <file>\n");
        return 1;
    }

    /* Read the whole file into memory. */
    size_t data_len = 0;
    char *data = read_file(path, &data_len);
    if (!data) return 1;

    if (data_len == 0) {
        free(data);
        return 0;  /* empty file — nothing to sort */
    }

    /*
     * Split the buffer into an array of line pointers.
     * We replace each '\n' with '\0' so each line is a proper C string.
     */
    size_t lines_cap = INITIAL_LINES;
    size_t lines_count = 0;
    char **lines = malloc(lines_cap * sizeof(char *));
    if (!lines) {
        perror("custom_sort: malloc");
        free(data);
        return 1;
    }

    lines[lines_count++] = data;
    for (size_t i = 0; i < data_len; i++) {
        if (data[i] == '\n') {
            data[i] = '\0';
            /* Start a new line if there's more data after the newline. */
            if (i + 1 < data_len) {
                if (lines_count == lines_cap) {
                    lines_cap *= 2;
                    char **tmp = realloc(lines, lines_cap * sizeof(char *));
                    if (!tmp) {
                        perror("custom_sort: realloc");
                        free(lines);
                        free(data);
                        return 1;
                    }
                    lines = tmp;
                }
                lines[lines_count++] = data + i + 1;
            }
        }
    }

    /* Sort using the chosen comparator. */
    int (*cmp)(const void *, const void *) = opt_numeric ? cmp_numeric : cmp_alpha;
    qsort(lines, lines_count, sizeof(char *), cmp);

    /* Print sorted lines. */
    for (size_t i = 0; i < lines_count; i++) {
        printf("%s\n", lines[i]);
    }

    free(lines);
    free(data);
    return 0;
}
