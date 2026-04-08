/*
 * custom_ls.c
 * ------------
 * Lists the contents of a directory.
 *
 * Usage:
 *   custom_ls           -> lists current directory
 *   custom_ls <path>    -> lists the given directory
 *   custom_ls -l <path> -> long format (size + type + name)
 *
 * OS concepts:
 *   - opendir() / readdir() / closedir() are POSIX directory-stream APIs
 *     that talk to the filesystem via the kernel and return one
 *     `struct dirent` at a time.
 *   - stat() asks the kernel for the inode metadata (size, mode, etc.).
 *
 * Build: gcc -Wall -o custom_ls custom_ls.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

static void print_long(const char *dirpath, const char *name)
{
    char full[1024];
    snprintf(full, sizeof full, "%s/%s", dirpath, name);

    struct stat st;
    if (stat(full, &st) < 0) {
        perror(full);
        return;
    }

    char type = '-';
    if      (S_ISDIR(st.st_mode)) type = 'd';
    else if (S_ISLNK(st.st_mode)) type = 'l';
    else if (S_ISCHR(st.st_mode)) type = 'c';
    else if (S_ISBLK(st.st_mode)) type = 'b';

    printf("%c  %8ld  %s\n", type, (long)st.st_size, name);
}

int main(int argc, char *argv[])
{
    int long_format = 0;
    const char *path = ".";

    /* Trivial flag parsing — supports `custom_ls -l [dir]` */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) long_format = 1;
        else                            path = argv[i];
    }

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "custom_ls: cannot open '%s': %s\n",
                path, strerror(errno));
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip the "." and ".." pseudo-entries to keep output clean. */
        if (entry->d_name[0] == '.') continue;

        if (long_format) print_long(path, entry->d_name);
        else             printf("%s  ", entry->d_name);
    }
    if (!long_format) printf("\n");

    closedir(dir);
    return 0;
}
