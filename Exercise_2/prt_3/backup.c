#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <fcntl.h>

void create_hard_link(const char *src, const char *dst) {
    if (link(src, dst) == -1) {
        perror("link");
    }
}

void copy_symlink(const char *src, const char *dst) {
    char target[PATH_MAX];
    ssize_t len = readlink(src, target, sizeof(target) - 1);
    if (len == -1) {
        perror("readlink");
        return;
    }
    target[len] = '\0';
    if (symlink(target, dst) == -1) { //Create a new symlink at dst, pointing to the same target as src
        perror("symlink");
    }
}

void copy_directory(const char *src, const char *dst) {
    DIR *dir = opendir(src);
    if (!dir) {
        perror("opendir");
        return;
    }

    if (mkdir(dst, 0777) == -1) {
        perror("mkdir");
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        struct stat st;
        if (lstat(src_path, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            copy_directory(src_path, dst_path);
        } else if (S_ISLNK(st.st_mode)) {
            copy_symlink(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            create_hard_link(src_path, dst_path);
        } else {
            fprintf(stderr, "Skipping unknown file type: %s\n", src_path);
        }

        // Preserve permissions
        if (!S_ISLNK(st.st_mode)) {
            if (chmod(dst_path, st.st_mode) == -1) {
                perror("chmod");
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <backup_directory>\n", argv[0]);
        return 1;
    }

    struct stat st;

    // Check if source exists and is a directory
    if (stat(argv[1], &st) == -1 || !S_ISDIR(st.st_mode)) {
        perror("src dir");
        return 1;
    }

    // Check that destination does not exist
    if (access(argv[2], F_OK) == 0) {
        perror("backup dir");
        return 1;
    }

    copy_directory(argv[1], argv[2]);
    return 0;
}
