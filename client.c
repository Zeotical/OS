
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5555
#define BUF_SIZE 128

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server.\n");

    while (1) {
        memset(buffer, 0, BUF_SIZE);

        int n = read(sock, buffer, BUF_SIZE - 1);
        if (n <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        printf("%s", buffer);

        if (strstr(buffer, "Guess")) {
            fgets(buffer, BUF_SIZE, stdin);
            write(sock, buffer, strlen(buffer));
        }
    }

    close(sock);
    return 0;
}
