/*
 * custom_tail.c
 * -------------
 * Prints the last N (default 10) lines of a file.
 *
 * Usage: custom_tail [-n N] <file>
 *
 * OS concepts:
 *   - Uses low-level POSIX I/O calls open() / read() / write() / close().
 *   - Implements a circular buffer dynamically allocating line strings
 *     to prevent unbounded memory usage if standard files are read.
 *
 * Build: gcc -Wall -o custom_tail custom_tail.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 4096
#define MAX_LINE_LEN 4096

static int dump_tail(const char *path, int keep_lines)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "custom_tail: %s: %s\n", path, strerror(errno));
        return 1;
    }

    char **circ = calloc(keep_lines, sizeof(char*));
    if (!circ) {
        perror("custom_tail: calloc");
        close(fd);
        return 1;
    }

    char temp_line[MAX_LINE_LEN];
    int t_len = 0;
    int count = 0;

    char buf[BUF_SIZE];
    ssize_t n;

    while ((n = read(fd, buf, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            temp_line[t_len++] = buf[i];
            
            /* End of line or reaching max line length */
            if (buf[i] == '\n' || t_len == (MAX_LINE_LEN - 1)) {
                temp_line[t_len] = '\0';
                if (circ[count % keep_lines]) {
                    free(circ[count % keep_lines]);
                }
                circ[count % keep_lines] = strdup(temp_line);
                count++;
                t_len = 0;
            }
        }
    }

    if (n < 0) {
        perror("custom_tail: read");
    }

    /* Process any trailing characters not ending in a newline */
    if (t_len > 0) {
        temp_line[t_len] = '\0';
        if (circ[count % keep_lines]) {
            free(circ[count % keep_lines]);
        }
        circ[count % keep_lines] = strdup(temp_line);
        count++;
    }

    /* Print out the saved lines sequentially */
    int start = (count > keep_lines) ? (count % keep_lines) : 0;
    int limit = (count > keep_lines) ? keep_lines : count;

    for (int i = 0; i < limit; i++) {
        const char *s = circ[(start + i) % keep_lines];
        if (s) {
            int w_len = strlen(s);
            int off = 0;
            while (off < w_len) {
                ssize_t w = write(STDOUT_FILENO, s + off, w_len - off);
                if (w < 0) break;
                off += w;
            }
            free(circ[(start + i) % keep_lines]);
        }
    }

    free(circ);
    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: custom_tail [-n N] <file>\n");
        return 1;
    }

    int lines = 10;
    int file_idx = 1;

    if (strcmp(argv[1], "-n") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: custom_tail [-n N] <file>\n");
            return 1;
        }
        lines = atoi(argv[2]);
        if (lines <= 0) lines = 10;
        file_idx = 3;
    }

    return dump_tail(argv[file_idx], lines);
}
