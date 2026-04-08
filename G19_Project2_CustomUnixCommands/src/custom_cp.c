/*
 * custom_cp.c
 * ------------
 * Copies one file to another using raw POSIX I/O.
 *
 * Usage: custom_cp <source> <destination>
 *
 * OS concepts:
 *   - O_CREAT | O_WRONLY | O_TRUNC means: create the file if it doesn't
 *     exist, open for writing, and truncate it to zero length if it does.
 *   - The 0644 mode means: owner can read+write, group/others read only.
 *     The kernel ANDs this with the umask to get the actual permissions.
 *   - We loop on read()/write() because both calls can return short counts
 *     if interrupted by a signal or constrained by buffer sizes.
 *
 * Build: gcc -Wall -o custom_cp custom_cp.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: custom_cp <source> <destination>\n");
        return 1;
    }

    int src = open(argv[1], O_RDONLY);
    if (src < 0) {
        fprintf(stderr, "custom_cp: %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    /* Preserve the source's permission bits if we can stat() it. */
    struct stat st;
    mode_t mode = 0644;
    if (fstat(src, &st) == 0) mode = st.st_mode & 0777;

    int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (dst < 0) {
        fprintf(stderr, "custom_cp: %s: %s\n", argv[2], strerror(errno));
        close(src);
        return 1;
    }

    char    buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(src, buf, BUF_SIZE)) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(dst, buf + off, n - off);
            if (w < 0) {
                perror("custom_cp: write");
                close(src); close(dst);
                return 1;
            }
            off += w;
        }
    }
    if (n < 0) perror("custom_cp: read");

    close(src);
    close(dst);
    printf("Copied '%s' -> '%s'\n", argv[1], argv[2]);
    return 0;
}
