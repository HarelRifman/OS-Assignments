#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

void process_read(int data_fd, int results_fd, off_t start, off_t end) {
    struct stat file_stat;
    if (fstat(data_fd, &file_stat) == -1) {
        perror("fstat");
        return;
    }

    if (start < 0 || end < 0 || start > end || start >= file_stat.st_size) {
        return;
    }

    if (end >= file_stat.st_size) {
        end = file_stat.st_size - 1;
    }

    off_t length = end - start + 1;
    char *buffer = malloc(length);
    if (!buffer) {
        perror("malloc");
        return;
    }

    if (lseek(data_fd, start, SEEK_SET) == -1) {
        perror("lseek");
        free(buffer);
        return;
    }

    ssize_t bytes_read = read(data_fd, buffer, length);
    if (bytes_read <= 0) {
        free(buffer);
        return;
    }

    if (write(results_fd, buffer, bytes_read) == -1 || write(results_fd, "\n", 1) == -1) {
        perror("write");
    }

    free(buffer);
}

void process_write(int data_fd, off_t offset, const char *text) {
    struct stat file_stat;
    if (fstat(data_fd, &file_stat) == -1) {
        perror("fstat");
        return;
    }

    if (offset < 0 || offset > file_stat.st_size) {
        return;
    }

    size_t text_length = strlen(text);
    size_t trailing_size = file_stat.st_size - offset;
    char *trailing_data = malloc(trailing_size);
    if (!trailing_data && trailing_size > 0) {
        perror("malloc");
        return;
    }

    if (trailing_size > 0) {
        if (lseek(data_fd, offset, SEEK_SET) == -1 ||
            read(data_fd, trailing_data, trailing_size) != (ssize_t)trailing_size) {
            perror("read trailing");
            free(trailing_data);
            return;
        }
    }

    if (lseek(data_fd, offset, SEEK_SET) == -1 ||
        write(data_fd, text, text_length) != (ssize_t)text_length) {
        perror("write");
        free(trailing_data);
        return;
    }

    if (trailing_size > 0 &&
        write(data_fd, trailing_data, trailing_size) != (ssize_t)trailing_size) {
        perror("write trailing");
    }

    free(trailing_data);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <data_file> <requests_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int data_fd = open(argv[1], O_RDWR);
    if (data_fd == -1) {
        perror("data.txt");
        return EXIT_FAILURE;
    }

    int requests_fd = open(argv[2], O_RDONLY);
    if (requests_fd == -1) {
        perror("requests.txt");
        close(data_fd);
        return EXIT_FAILURE;
    }

    int results_fd = open("read_results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (results_fd == -1) {
        perror("read_results.txt");
        close(data_fd);
        close(requests_fd);
        return EXIT_FAILURE;
    }

    FILE *requests_file = fdopen(requests_fd, "r");
    if (!requests_file) {
        perror("fdopen");
        close(data_fd);
        close(results_fd);
        return EXIT_FAILURE;
    }

    char line[1024];
    while (fgets(line, sizeof(line), requests_file)) {
        if (line[0] == 'Q') {
            break;
        }

        if (line[0] == 'R') {
            off_t start, end;
            if (sscanf(line, "R %ld %ld", &start, &end) == 2) {
                process_read(data_fd, results_fd, start, end);
            }
        } else if (line[0] == 'W') {
            off_t offset;
            char *text = strchr(line, ' ');
            if (text) {
                text++;
                offset = strtol(text, &text, 10);
                while (*text == ' ') {
                    text++;
                }
                if (*text) {
                    text[strcspn(text, "\n")] = '\0';
                    process_write(data_fd, offset, text);
                }
            }
        }
    }

    close(data_fd);
    fclose(requests_file);
    close(results_fd);

    return EXIT_SUCCESS;
}
