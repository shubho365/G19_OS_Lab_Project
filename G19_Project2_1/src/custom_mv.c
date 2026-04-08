/*
 * custom_mv.c
 * ------------
 * Moves or renames a file.
 *
 * Usage: custom_mv <source> <destination>
 *
 * Strategy:
 *   1. First try rename(): on the same filesystem this is atomic and
 *      essentially free because it just relinks the inode under a new
 *      directory entry.
 *   2. If rename() fails with EXDEV ("cross-device link"), the source and
 *      destination live on different filesystems and the kernel cannot
 *      simply move the inode. We then fall back to copy + unlink.
 *
 * Build: gcc -Wall -o custom_mv custom_mv.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define BUF_SIZE 4096

static int copy_then_unlink(const char *src_path, const char *dst_path)
{
    int src = open(src_path, O_RDONLY);
    if (src < 0) { perror(src_path); return 1; }

    struct stat st;
    mode_t mode = 0644;
    if (fstat(src, &st) == 0) mode = st.st_mode & 0777;

    int dst = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (dst < 0) { perror(dst_path); close(src); return 1; }

    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(src, buf, BUF_SIZE)) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(dst, buf + off, n - off);
            if (w < 0) { perror("write"); close(src); close(dst); return 1; }
            off += w;
        }
    }
    close(src); close(dst);

    if (unlink(src_path) < 0) { perror("unlink"); return 1; }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: custom_mv <source> <destination>\n");
        return 1;
    }

    if (rename(argv[1], argv[2]) == 0) {
        printf("Moved '%s' -> '%s'\n", argv[1], argv[2]);
        return 0;
    }

    if (errno == EXDEV) {
        /* Different filesystems → fall back to copy + delete. */
        if (copy_then_unlink(argv[1], argv[2]) == 0) {
            printf("Moved (cross-device) '%s' -> '%s'\n", argv[1], argv[2]);
            return 0;
        }
        return 1;
    }

    fprintf(stderr, "custom_mv: %s\n", strerror(errno));
    return 1;
}
