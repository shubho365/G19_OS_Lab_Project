/*
 * custom_rm.c
 * ------------
 * Removes a file, or recursively removes a directory tree with -r.
 *
 * Usage:
 *   custom_rm <file>
 *   custom_rm -r <directory>
 *
 * OS concepts:
 *   - unlink() removes a directory entry. The kernel only frees the inode
 *     when the link count drops to zero AND no process still has it open.
 *   - rmdir() removes an *empty* directory. To delete a non-empty tree we
 *     have to walk it ourselves with opendir/readdir, recursing into
 *     subdirectories and unlinking the files we find.
 *
 * Build: gcc -Wall -o custom_rm custom_rm.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

static int remove_recursive(const char *path)
{
    struct stat st;
    if (lstat(path, &st) < 0) {
        fprintf(stderr, "custom_rm: %s: %s\n", path, strerror(errno));
        return 1;
    }

    if (!S_ISDIR(st.st_mode)) {
        if (unlink(path) < 0) {
            fprintf(stderr, "custom_rm: %s: %s\n", path, strerror(errno));
            return 1;
        }
        return 0;
    }

    /* It is a directory: walk it. */
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "custom_rm: %s: %s\n", path, strerror(errno));
        return 1;
    }

    struct dirent *entry;
    int rc = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".")  == 0 ||
            strcmp(entry->d_name, "..") == 0) continue;

        char child[2048];
        snprintf(child, sizeof child, "%s/%s", path, entry->d_name);
        rc |= remove_recursive(child);
    }
    closedir(dir);

    if (rmdir(path) < 0) {
        fprintf(stderr, "custom_rm: rmdir %s: %s\n", path, strerror(errno));
        return 1;
    }
    return rc;
}

int main(int argc, char *argv[])
{
    int recursive = 0;
    const char *target = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) recursive = 1;
        else                            target    = argv[i];
    }

    if (!target) {
        fprintf(stderr, "Usage: custom_rm [-r] <path>\n");
        return 1;
    }

    if (recursive) return remove_recursive(target);

    if (unlink(target) < 0) {
        fprintf(stderr, "custom_rm: %s: %s\n", target, strerror(errno));
        return 1;
    }
    return 0;
}
