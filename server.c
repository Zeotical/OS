#include <stdio.h>
#include <fcntl.h>  //used to manipulate file descriptors
#include <unistd.h>
#include <sys/wait.h>

#include <string.h>
#include <stdlib.h>

// TODO
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
    int counter = 0;

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

        printf("\nBinding socket to port 8080");
    }
    // make this a loop wait for clients to join
    while (counter < 2)
    {
        if (listen(server_fd, 3) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }
        else
        {

            printf("\nServer listening on port 8080");
            printf("\nWaiting for player to join");
        }
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else
        {
            counter++;
            printf("\nPlayer joined.");
            // --- Fork the Child (Receiver) Process ---
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork failed");
                // mq_unlink(MQ_NAME);
                close(server_fd);
                return 1;
            }
            else if (pid == 0)
            {
                // // Child Process: Execute the Receiver Program
                send(new_socket, hello, strlen(hello), 0);
                close(new_socket);

                close(server_fd);

                //execl("./client", "client", NULL);

                // execl only returns if an error occurred
                // perror("execl failed");
                // close(new_socket);

                // close(server_fd);

                // exit(EXIT_FAILURE);
            }
            // else
            // {
            //     // // Parent Process: Run the Sender logic

            //     // send(new_socket, hello, strlen(hello), 0);

            //     // // Wait for the child to finish
            //     wait(NULL);
            //     // // Cleanup: Close and unlink the message queue
            //     close(new_socket);

            //     close(server_fd);

            //     printf("\n[SENDER] Parent finished and cleaned up message queue.");
            // }
        }
    }

    // Wait for the child to finish
    wait(NULL);
    // Cleanup: Close and unlink the message queue
   
    close(new_socket);

    close(server_fd);

    printf("\n[SENDER] Parent finished and cleaned up message queue.");

    return 0;
}