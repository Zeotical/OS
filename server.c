#include <stdio.h>
#include <fcntl.h> //used to manipulate file descriptors
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#define PORT 8080 // Mr sharaf
#include <netinet/in.h>
#include <sys/socket.h>

// #include <asm-generic/socket.h> used for SO_REUSEPORT

// typedef struct {
// int client_sockets[2]; // num of players ig
// } GameState;
////////////////

int main()
{

    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};
    char hello[] = "Hello from server";
    int player_no = 1;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    // Might consider adding SO_REUSEPORT later on currently unsure
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Binding socket to port 8080\n");
    }
    // make this a loop wait for clients to join
    if (listen(server_fd, 3) < 0)
    {
        perror("listen\n");
        exit(EXIT_FAILURE);
    }
    else
    {
     printf("Server listening on port 8080\n");
    }
    // make this a loop wait for clients to join
    while (player_no < 3) // while loop for connecting clients
   {
        printf("Waiting for player %d to join\n", player_no);
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else
        {
            // --- Fork the Child (CLient) Process ---
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork failed");
                close(new_socket); 
                return 1;
            }
            else if (pid == 0)
            {
                close(server_fd); //no need for child to listen for connections
                printf("Player % d joined.\n" , player_no);
                // // Child Process: Execute the Receiver Program
                send(new_socket, hello, strlen(hello), 0);
                valread = read(new_socket, buffer,1024 - 1);
                printf("%s\n", buffer);
                
                close(new_socket);
                _exit(32); // child exits

            }

            else
            {
                // Parent Process: Run the Server logic
                    wait(NULL); // keepin this for printf player no for now.
                close(new_socket); // close connected client socket, let child deal with it
                player_no++;
    
            }
        }
    } // while loop

    //TODO thread for client turn ++ signchld for non blocking reapin
    // Reap all child processes
    while(wait(NULL)>0){ //wait(NULL) returns a positive value(PID) of child if it exits or no error happens
    wait(NULL);
    }

    close(server_fd); // Close listening socket

    printf("\n[SERVER] Parent finished and reaped all child processes.");
    return 0;
}