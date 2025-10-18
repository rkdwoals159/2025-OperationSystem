#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#define SERVER_FIFO "/tmp/server_fifo"
#define BUFFER_SIZE 512

int main() {
    char filename[100], mode[10], data[300] = "";
    char client_fifo[100];
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sprintf(client_fifo, "/tmp/client_%d_fifo", getpid());
    mkfifo(client_fifo, 0666);

    printf("=== CLIENT STARTED (PID %d) ===\n", getpid());
    printf("Type 'exit' as filename to quit.\n\n");

    while (1) {
        printf("Enter filename: ");
        scanf("%s", filename);

        // 종료 조건
        if (strcmp(filename, "exit") == 0) {
            break;
        }

        printf("Enter mode (r/w): ");
        scanf("%s", mode);

        memset(data, 0, sizeof(data));

        if (strcmp(mode, "w") == 0) {
            printf("Enter data to write: ");
            getchar(); // flush
            fgets(data, sizeof(data), stdin);
            data[strcspn(data, "\n")] = '\0';
        }

        // 서버로 요청 전송
        sprintf(request, "%s|%s|%s|%s", filename, mode, data, client_fifo);
        printf("[CLIENT] Sending request: %s\n", request);
        int server_fd = open(SERVER_FIFO, O_WRONLY);
        if (server_fd == -1) {
            perror("open server fifo");
            break;
        }
        write(server_fd, request, strlen(request));
        close(server_fd);

        // 서버가 응답을 보낼 시간을 주기 위해 잠시 대기
        usleep(50000); // 50ms 대기

        // 서버 응답 수신
        int client_fd = open(client_fifo, O_RDONLY);
        if (client_fd == -1) {
            perror("open client fifo for reading");
            continue;
        }
        
        ssize_t bytes = read(client_fd, response, sizeof(response) - 1);
        if (bytes > 0) {
            response[bytes] = '\0';
            printf("[CLIENT] Response:\n%s\n", response);
        } else {
            printf("[CLIENT] No response received\n");
        }
        close(client_fd);
        
        // FIFO 정리 (읽기 후)
        unlink(client_fifo);
        // 다음 요청을 위해 FIFO 재생성
        mkfifo(client_fifo, 0666);

        printf("-----------------------------\n");
    }

    unlink(client_fifo);
    printf("=== CLIENT TERMINATED ===\n");
    return 0;
}
