#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define SERVER_FIFO "/tmp/server_fifo"
#define BUFFER_SIZE 512

void handle_client_request(const char *request) {
    char filename[100], mode[10], data[300], client_fifo[100];
    memset(filename, 0, sizeof(filename));
    memset(mode, 0, sizeof(mode));
    memset(data, 0, sizeof(data));
    memset(client_fifo, 0, sizeof(client_fifo));

    // 수동으로 파싱: filename|mode|data|client_fifo
    char *ptr = (char*)request;
    
    // filename 파싱
    char *end = strchr(ptr, '|');
    if (end) {
        strncpy(filename, ptr, end - ptr);
        filename[end - ptr] = '\0';
        ptr = end + 1;
    }
    
    // mode 파싱
    end = strchr(ptr, '|');
    if (end) {
        strncpy(mode, ptr, end - ptr);
        mode[end - ptr] = '\0';
        ptr = end + 1;
    }
    
    // data 파싱
    end = strchr(ptr, '|');
    if (end) {
        strncpy(data, ptr, end - ptr);
        data[end - ptr] = '\0';
        ptr = end + 1;
    }
    
    // client_fifo 파싱 (마지막 필드)
    if (ptr && strlen(ptr) > 0) {
        strcpy(client_fifo, ptr);
    }
    
    printf("[SERVER] Parsed fields: filename='%s', mode='%s', data='%s', client_fifo='%s'\n", 
           filename, mode, data, client_fifo);

    // 클라이언트 FIFO 열기 (잠시 대기 후 시도)
    int client_fd = -1;
    int retry_count = 0;
    while (client_fd == -1 && retry_count < 5) {
        client_fd = open(client_fifo, O_WRONLY);
        if (client_fd == -1) {
            usleep(10000); // 10ms 대기
            retry_count++;
        }
    }
    
    if (client_fd == -1) {
        printf("[SERVER] Failed to open client fifo: %s\n", client_fifo);
        return;
    }

    if (strcmp(mode, "r") == 0) {
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            char err[] = "Error: cannot open file for reading\n";
            write(client_fd, err, strlen(err));
        } else {
            char content[256];
            memset(content, 0, sizeof(content)); // 버퍼 초기화
            
            // 파일 크기 확인
            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            
            // 파일 크기가 버퍼보다 작으면 그만큼만 읽기
            size_t read_size = (file_size < sizeof(content)-1) ? file_size : sizeof(content)-1;
            size_t bytes_read = fread(content, sizeof(char), read_size, fp);
            content[bytes_read] = '\0'; // null terminator 확실히 설정
            
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

    // 서버 FIFO를 한 번만 열고 유지
    int server_fd = open(SERVER_FIFO, O_RDONLY);
    if (server_fd == -1) {
        perror("open server fifo");
        exit(1);
    }

    while (1) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes = read(server_fd, buffer, sizeof(buffer));
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("[SERVER] Request received: %s\n", buffer);

            pid_t pid = fork();
            if (pid == 0) {
                // Child process: handle the request
                close(server_fd); // 자식 프로세스에서는 서버 FIFO 닫기
                handle_client_request(buffer);
                exit(0);
            } else if (pid > 0) {
                // 부모 프로세스: 자식이 완료될 때까지 대기
                wait(NULL);
            }
        }
    }

    close(server_fd);
    unlink(SERVER_FIFO);
    return 0;
}
