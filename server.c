#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define SERVER_FIFO "/tmp/server_fifo"
#define BUFFER_SIZE 512

void handle_client_request(const char *request) {
    char filename[100], mode[10], data[300], client_fifo[100];
    memset(filename, 0, sizeof(filename));
    memset(mode, 0, sizeof(mode));
    memset(data, 0, sizeof(data));

    // 요청 파싱: filename|mode|data|client_fifo
    sscanf(request, "%[^|]|%[^|]|%[^|]|%s", filename, mode, data, client_fifo);

    // 클라이언트 FIFO 열기
    int client_fd = open(client_fifo, O_WRONLY);
    if (client_fd == -1) {
        perror("open client fifo");
        return;
    }

    if (strcmp(mode, "r") == 0) {
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            char err[] = "Error: cannot open file for reading\n";
            write(client_fd, err, strlen(err));
        } else {
            char content[256];
            fread(content, sizeof(char), sizeof(content)-1, fp);
            write(client_fd, content, strlen(content));
            fclose(fp);
        }
    } else if (strcmp(mode, "w") == 0) {
        FILE *fp = fopen(filename, "w");
        if (!fp) {
            char err[] = "Error: cannot open file for writing\n";
            write(client_fd, err, strlen(err));
        } else {
            fwrite(data, sizeof(char), strlen(data), fp);
            char msg[] = "Write success\n";
            write(client_fd, msg, strlen(msg));
            fclose(fp);
        }
    } else {
        char msg[] = "Invalid mode\n";
        write(client_fd, msg, strlen(msg));
    }

    close(client_fd);
}

int main() {
    mkfifo(SERVER_FIFO, 0666);
    printf("[SERVER] Waiting for client requests...\n");

    while (1) {
        int fd = open(SERVER_FIFO, O_RDONLY);
        if (fd == -1) {
            perror("open server fifo");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes = read(fd, buffer, sizeof(buffer));
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("[SERVER] Request received: %s\n", buffer);

            pid_t pid = fork();
            if (pid == 0) {
                // Child process: handle the request
                handle_client_request(buffer);
                exit(0);
            }
        }

        close(fd);
    }

    unlink(SERVER_FIFO);
    return 0;
}
