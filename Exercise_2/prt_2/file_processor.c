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

    if (start < 0 || end < 0 || start > end || start >= file_stat.st_size)
        return;

    if (end >= file_stat.st_size)
        end = file_stat.st_size - 1;

    off_t len = end - start + 1;
    char *buffer = malloc(len);
    if (!buffer) {
        perror("malloc");
        return;
    }

    if (lseek(data_fd, start, SEEK_SET) == -1) {
        perror("lseek");
        free(buffer);
        return;
    }

    ssize_t bytes_read = read(data_fd, buffer, len);
    if (bytes_read <= 0) {
        free(buffer);
        return;
    }

    write(results_fd, buffer, bytes_read);
    write(results_fd, "\n", 1);
    free(buffer);
}

void process_write(int data_fd, off_t offset, const char *text) {
    struct stat st;
    if (fstat(data_fd, &st) == -1) {
        perror("fstat");
        return;
    }

    if (offset < 0 || offset > st.st_size)
        return;

    size_t text_len = strlen(text);
    size_t trailing_size = st.st_size - offset;
    char *trailing_data = malloc(trailing_size);

    if (trailing_size > 0 && !trailing_data) {
        perror("malloc");
        return;
    }

    if (trailing_size > 0) {
        if (lseek(data_fd, offset, SEEK_SET) == -1 ||
            read(data_fd, trailing_data, trailing_size) != trailing_size) {
            perror("read trailing");
            free(trailing_data);
            return;
        }
    }

    // Write new text
    if (lseek(data_fd, offset, SEEK_SET) == -1 ||
        write(data_fd, text, text_len) != text_len) {
        perror("write");
        free(trailing_data);
        return;
    }

    // Append old data after inserted text
    if (trailing_size > 0 &&
        write(data_fd, trailing_data, trailing_size) != trailing_size) {
        perror("write trailing");
    }

    free(trailing_data);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <data_file> <requests_file>\n", argv[0]);
        return 1;
    }

    int data_fd = open(argv[1], O_RDWR);
    if (data_fd == -1) {
        perror("data.txt");
        return 1;
    }

    int requests_fd = open(argv[2], O_RDONLY);
    if (requests_fd == -1) {
        perror("requests.txt");
        close(data_fd);
        return 1;
    }

    int results_fd = open("read_results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (results_fd == -1) {
        perror("read_results.txt");
        close(data_fd);
        close(requests_fd);
        return 1;
    }

    FILE *requests_file = fdopen(requests_fd, "r");
    if (!requests_file) {
        perror("fdopen");
        close(data_fd);
        close(results_fd);
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), requests_file)) {
        if (line[0] == 'Q') break;

        if (line[0] == 'R') {
            off_t start, end;
            if (sscanf(line, "R %ld %ld", &start, &end) == 2) {
                process_read(data_fd, results_fd, start, end);
            }
        } else if (line[0] == 'W') {
            off_t offset;
            char *text = strchr(line, ' ');
            if (text) {
                text++; // move past 'W '
                offset = strtol(text, &text, 10);
                while (*text == ' ') text++;
                if (*text) {
                    text[strcspn(text, "\n")] = 0; // trim newline
                    process_write(data_fd, offset, text);
                }
            }
        }
    }

    close(data_fd);
    fclose(requests_file);
    close(results_fd);
    return 0;
}
