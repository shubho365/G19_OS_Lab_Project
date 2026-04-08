/*
 * custom_cat.c
 * -------------
 * Concatenates one or more files to standard output.
 *
 * Usage: custom_cat <file1> [file2 ...]
 *
 * OS concepts:
 *   - We deliberately use the *low-level* POSIX I/O calls
 *     open() / read() / write() / close() instead of fopen()/fread().
 *     These are the actual system calls; the FILE* family is built on top.
 *   - read() returns the number of bytes actually read (which may be less
 *     than requested), 0 on EOF, and -1 on error.
 *   - We use a 4 KB buffer because that matches a typical filesystem page,
 *     keeping I/O efficient.
 *
 * Build: gcc -Wall -o custom_cat custom_cat.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 4096

static int dump(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "custom_cat: %s: %s\n", path, strerror(errno));
        return 1;
    }

    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(fd, buf, BUF_SIZE)) > 0) {
        ssize_t off = 0;
        /* write() may write fewer bytes than requested → loop until done */
        while (off < n) {
            ssize_t w = write(STDOUT_FILENO, buf + off, n - off);
            if (w < 0) {
                perror("custom_cat: write");
                close(fd);
                return 1;
            }
            off += w;
        }
    }
    if (n < 0) perror("custom_cat: read");

    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: custom_cat <file> [file ...]\n");
        return 1;
    }
    int rc = 0;
    for (int i = 1; i < argc; i++)
        rc |= dump(argv[i]);
    return rc;
}
