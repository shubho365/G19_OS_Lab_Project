/*
 * custom_head.c
 * --------------
 * Print the first N lines of a file (like the standard `head` command).
 *
 * Usage:
 *   custom_head <file>          -> prints first 10 lines (default)
 *   custom_head -n <N> <file>   -> prints first N lines
 *
 * OS concepts:
 *   - open()  : ask the kernel to open a file and return a file descriptor
 *   - read()  : copy bytes from the file into a user-space buffer
 *   - write() : copy bytes from user-space buffer to stdout (fd 1)
 *   - close() : release the file descriptor back to the kernel
 *
 *   We deliberately use the raw POSIX syscalls (open/read/write) instead
 *   of fopen/fgets so the kernel boundary is visible — the same reason
 *   custom_cat and custom_cp use them.
 *
 * Build: gcc -Wall -Wextra -O2 -o custom_head custom_head.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 4096
#define DEFAULT_LINES 10

int main(int argc, char *argv[])
{
    int   nlines = DEFAULT_LINES;
    const char *path = NULL;

    /* Argument parsing: custom_head [-n N] <file> */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "custom_head: option -n requires an argument\n");
                return 1;
            }
            nlines = atoi(argv[++i]);
            if (nlines <= 0) {
                fprintf(stderr, "custom_head: invalid line count '%s'\n", argv[i]);
                return 1;
            }
        } else {
            path = argv[i];
        }
    }

    if (!path) {
        fprintf(stderr, "Usage: custom_head [-n N] <file>\n");
        return 1;
    }

    /* Open the file using the raw POSIX syscall. */
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "custom_head: %s: %s\n", path, strerror(errno));
        return 1;
    }

    /*
     * Read the file one byte at a time seeking '\n' so we can count
     * lines precisely. When we reach the Nth newline we stop.
     * A buffer-based approach (read in 4096-byte chunks) would be faster
     * but harder to follow; this version is easier to understand and defend.
     */
    char buf[BUF_SIZE];
    ssize_t bytes_read;
    int lines_printed = 0;
    int print_buf_len = 0;
    char print_buf[BUF_SIZE];

    while (lines_printed < nlines) {
        bytes_read = read(fd, buf, sizeof buf);
        if (bytes_read < 0) {
            perror("custom_head: read");
            close(fd);
            return 1;
        }
        if (bytes_read == 0) break;   /* EOF */

        for (ssize_t i = 0; i < bytes_read && lines_printed < nlines; i++) {
            print_buf[print_buf_len++] = buf[i];
            if (buf[i] == '\n') {
                /* Flush the accumulated line. */
                write(STDOUT_FILENO, print_buf, print_buf_len);
                print_buf_len = 0;
                lines_printed++;
            }
            /* Flush buffer if it's getting full (no newline yet). */
            if (print_buf_len == BUF_SIZE - 1) {
                write(STDOUT_FILENO, print_buf, print_buf_len);
                print_buf_len = 0;
            }
        }
    }

    /* Print any remaining partial line (file with no trailing newline). */
    if (print_buf_len > 0)
        write(STDOUT_FILENO, print_buf, print_buf_len);

    close(fd);
    return 0;
}
