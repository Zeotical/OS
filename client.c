#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5555
#define BUF_SIZE 2048

void printBorder(int upper){
    const char* border = "============================================================";
    if(upper){
        printf("\n%s\n\n", border);
    } else {
        printf("\n%s\n", border);
    }
}

void printTitle(){
    printf(" __      __  _                  _   \n");
    printf(" \\ \\    / / | |                | |  \n");
    printf("  \\ \\/\\/ /__| |__   ___ ___ ___| |  \n");
    printf("   \\    / _ \\ '_ \\ / _ \\/ _ \\/ _ \\ |  \n");
    printf("    \\  /  __/ | | |  __/  __/  __/ |  \n");
    printf("     \\/ \\___|_| |_|\\___|\\___|\\___|_|  \n");
    printf("                                      \n");
    printf("         O F   F O R T U N E          \n");
}

void printOpening(){
    
    printBorder(1);
    printTitle();
    printf("\n       Welcome to the Multiplayer C Edition!\n");
    printBorder(0);
    
    //force text
    fflush(stdout); 
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
        printf("ERROR: Could not connect. Is the server running?\n");
        exit(1);
    }

    printf("Connected to server.\n");
    printOpening();
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_sd = sock;

        //wait for activity
        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        if ((activity < 0) && (activity != 0)) {
            printf("select error");
        }

        //data from Server
        if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, BUF_SIZE);
            int n = read(sock, buffer, BUF_SIZE - 1);
            if (n <= 0) {
                printf("\nServer disconnected.\n");
                break;
            }
            
            printf("%s", buffer);
            fflush(stdout); //force text to screen

            //check if we need to show a prompt
            if (strstr(buffer, "Enter your name:") || 
                strstr(buffer, "Choose") || 
                strstr(buffer, "guess") ||
                strstr(buffer, ">")) {
                printf("> ");
                fflush(stdout);
            }
        }

        //data from user
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