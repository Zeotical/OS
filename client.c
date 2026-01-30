#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5555
#define BUF_SIZE 2048

void printBorder(int upper){
    if(upper){
        printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n\n");
    } else {
        printf("\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    }
}

void printTitle(){
    printf("  ▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂\n");
    printf(" ▞ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ▚\n");
    printf("▞ * * ▚\n");
    printf("▌* ▐▌    ▐▌ █  █ █▐▌▌ █▐▌▌ █         ▐▐███▌▌ █  █ █▐▌▌      █▐▌▌   █▐   █▐▌▌  ▐▐███▌▌ █  ▐▌▌ ▐▌  ▐▌ █▐▌▌   * ▌\n");
    printf("▌* ▐▌ █  ▐▌ █▗▖█ █▗▖  █▗▖  █            ▄    █▗▖█ █▗▖       █▗▖  █    █ █   █    ▄    █  ▐▌▌ ▐▌█ ▐▌ █▗▖    * ▌\n");
    printf("▌* ▐▌ █  ▐▌ █▝▘█ █▝▘  █▝▘  █            █    █▝▘█ █▝▘       █▝▘  █    █ █▐▌▌     █    █  ▐▌▌ ▐▌ █▐▌ █▝▘    * ▌\n");
    printf("▌* ▐▌ ▐▌▌  █  █ █▐▌▌ █▐▌▌ █▐▌▌         █    █  █ █▐▌▌      █      █▐   █   █    █     ███   ▐▌  ▐▌ █▐▌▌   * ▌\n");
    printf("▚ * * ▞                                        \n");
    printf(" ▚ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ▞\n");
    printf("  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\n");
}

void printOpening(){
    printBorder(1);
    printf("\n");
    printf("                               ▐▌    ▐▌ █▐▌▌ █    █▐▌▌   █▐    ▐▌ ▐▌▌  █▐▌▌  ▐▌    \n");
    printf("                               ▐▌ █  ▐▌ █▗▖  █    █    █    █ ▐▌ █  ▐▌ █▗▖   ▐▌    \n");
    printf("                               ▐▌ █  ▐▌ █▝▘  █    █    █    █ ▐▌ █  ▐▌ █▝▘   ▝▘    \n");
    printf("                                ▐▌ ▐▌▌  █▐▌▌ █▐▌▌ █▐▌▌   █▐   ▐▌    ▐▌ █▐▌▌  ▗▖    \n");
    printTitle();
    printf("\n");
    printBorder(0);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];
    fd_set read_fds;
    int max_sd;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server.\n");
    printOpening();
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_sd = sock;

        //wait for activity on socket
        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        if ((activity < 0) && (activity != 0)) {
            printf("select error");
        }

        //check data comes from server
        if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, BUF_SIZE);
            int n = read(sock, buffer, BUF_SIZE - 1);
            if (n <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            //print what server sent(clears screen also using ansi)
            printf("%s", buffer);
            fflush(stdout);

            if (strstr(buffer, "Enter your name:") || 
                strstr(buffer, "Choose") || 
                strstr(buffer, "guess") ||
                strstr(buffer, ">")) {
                printf("> ");
                fflush(stdout);
            }
        }

        //check if user typed something
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(input, 0, BUF_SIZE);
            if (fgets(input, BUF_SIZE, stdin) != NULL) {
                write(sock, input, strlen(input));
            }
        }
    }

    close(sock);
    return 0;
}