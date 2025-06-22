#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_FILES 100
#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void prepare_directories(const char* src, const char* dest) {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    if (!opendir(src) && ENOENT == errno) {
        printf("Error: Source directory '%s' does not exist.\n", src);
        exit(1);
    }

    if (!opendir(dest) && ENOENT == errno) {
        char temp_path[PATH_MAX];
        strncpy(temp_path, dest, PATH_MAX);
        temp_path[PATH_MAX - 1] = '\0';
        
        for (char *p = temp_path + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                mkdir(temp_path, 0777);
                *p = '/';
            }
        }
        mkdir(temp_path, 0777);
        printf("Created destination directory '%s'.\n", dest);
    }
    
    printf("Synchronizing from %s/%s to %s/%s\n", cwd, src, cwd, dest);
}

void sync_files(const char* src, const char* dest) {
    DIR *source_dir = opendir(src);
    DIR *dest_dir = opendir(dest);
    struct dirent *entry;
    char filenames[MAX_FILES][MAX_FILENAME_LEN];
    int count = 0;

    // Collect filenames
    while ((entry = readdir(source_dir)) != NULL) {
        if (entry->d_type == DT_REG && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // Only process regular files
            strcpy(filenames[count], entry->d_name);
            filenames[count][MAX_FILENAME_LEN - 1] = '\0'; // Ensure null termination
            count++;
        }
    }
    closedir(source_dir);
    
    if (count == 0) {
        printf("Synchronization complete.\n");
        closedir(dest_dir);
        return;
    }

    // Create an array of pointers for sorting
    char *filename_ptrs[MAX_FILES];
    for (int i = 0; i < count; i++) {
        filename_ptrs[i] = filenames[i];
    }

    // Sort filenames alphabetically
    qsort(filename_ptrs, count, sizeof(char *), compare_strings);

    char src_path[MAX_PATH_LEN];
    char dest_path[MAX_PATH_LEN];
    char cwd[MAX_PATH_LEN];
    getcwd(cwd, sizeof(cwd));

    // Process each file
    for (int i = 0; i < count; i++) {
        // concats the direcotry with the filname for the path
        snprintf(src_path, MAX_PATH_LEN, "%s/%s", src, filename_ptrs[i]);
        snprintf(dest_path, MAX_PATH_LEN, "%s/%s", dest, filename_ptrs[i]);
        
        struct stat src_stat, dest_stat;
        
        // Get source file stats
        if (stat(src_path, &src_stat) != 0) {
            perror("Failed to get source file stats");
            continue;
        }
        
        // Check if destination file exists
        if (stat(dest_path, &dest_stat) != 0) {
            // File doesn't exist in destination so it copys it using 'cp'
            printf("New file found: %s\n", filename_ptrs[i]);
            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) {
                // Child process to copy file
                execl("/bin/cp", "cp", src_path, dest_path, NULL);
                perror("execl failed");
                exit(1);
            } else {
                // Parent process
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("Copied: %s/%s -> %s/%s\n", cwd, src_path, cwd, dest_path);
                } else {
                    printf("Failed to copy %s\n", filename_ptrs[i]);
                }
            }
        } else {
            // File exists in both directories, compare them
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) {
                // Child process to run diff
                int null_fd = open("/dev/null", O_WRONLY);
                dup2(null_fd, STDOUT_FILENO);
                close(null_fd);
                execl("/usr/bin/diff", "diff", "-q", src_path, dest_path, NULL);
                perror("execl failed");
                exit(1);
            } else {
                // Parent process
                int status;
                waitpid(pid, &status, 0);
                
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    // Files are identical
                    printf("File %s is identical. Skipping...\n", filename_ptrs[i]);
                } else {
                    // Files differ, check which is newer
                    if (src_stat.st_mtime > dest_stat.st_mtime) {
                        printf("File %s is newer in source. Updating...\n", filename_ptrs[i]);
                        
                        pid_t cp_pid = fork();
                        if (cp_pid < 0) {
                            perror("fork failed");
                            exit(1);
                        } else if (cp_pid == 0) {
                            // Child process to copy file
                            execl("/bin/cp", "cp", src_path, dest_path, NULL);
                            perror("execl failed");
                            exit(1);
                        } else {
                            // Parent process
                            int cp_status;
                            waitpid(cp_pid, &cp_status, 0);
                            if (WIFEXITED(cp_status) && WEXITSTATUS(cp_status) == 0) {
                                printf("Copied: %s/%s -> %s/%s\n", cwd, src_path, cwd, dest_path);
                            } else {
                                printf("Failed to copy %s\n", filename_ptrs[i]);
                            }
                        }
                    } else {
                        printf("File %s is newer in destination. Skipping...\n", filename_ptrs[i]);
                    }
                }
            }
        }
    }
    
    closedir(dest_dir);
    printf("Synchronization complete.\n");
}

int main(int argc, char* argv[]) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    }
    if (argc != 3) {
        printf("Usage: file_sync <source_directory> <destination_directory>\n");
        exit(1);
    }

    prepare_directories(argv[1], argv[2]);
    sync_files(argv[1], argv[2]);

    return 0;
}