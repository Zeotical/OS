#include <stdio.h>
#include <mqueue.h> //
#include <fcntl.h>  //
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

// Define constants for the message queue
const char *MQ_NAME = "/my_ipc_mq";
// const long MAX_MSG_SIZE = 256;
// const long MAX_MESSAGES = 10;
// const char* MESSAGE_PREFIX = "Data cycle: ";
// Define the data structure for the message (must be identical in sender and receiver)
struct Message
{
    int counter;
    char buffer[252]; // Max size is 256, leaving 4 bytes for the int
};
// void run_sender(mqd_t mqd, pid_t receiver_pid) {
// printf("\n[SENDER] Parent Process (PID: %d) started.",  getpid());
// printf("\n[SENDER] Sending 3 messages to Child (PID: %d) via Queue...", receiver_pid);
// struct Message msg;
// for (int i = 1; i <= 3; ++i) {
// // 1. Prepare the message
// msg.counter = i;
// //TODO
// char str[20]; // Buffer to hold the string why we usin 20 here chat?
// sprintf(str, "%d", i); // Converts int to string
// const char* full_msg_str = MESSAGE_PREFIX ;
// strncpy(msg.buffer, full_msg_str, sizeof(msg.buffer) - 1);
// strncpy(msg.buffer+ strlen(msg.buffer), str, strlen(str)+1);

// msg.buffer[sizeof(msg.buffer) - 1] = '\0';
// // 2. Send the message
// if (mq_send(mqd, (const char*)&msg, sizeof(msg), 0) == -1) {
// perror("n[SENDER] mq_send failed");
// } else {
// printf ("\n[SENDER] Sent message %d. Pausing...", i);
// }
// sleep(2);
// }
// // 3. Send a termination signal message
// msg.counter = -1;
// if (mq_send(mqd, (const char*)&msg, sizeof(msg), 0) == -1) {
// perror("\n[SENDER] mq_send failed for termination signal");
// } else {
// printf("\n[SENDER] Sent termination signal (-1). Waiting for receiver...");
// //TO DO fix sync problem here
// }
//}
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
                // mq_close(mqd);
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
    // send(new_socket, hello, strlen(hello), 0);

    // valread = read(new_socket, buffer,1024 - 1);
    // printf("%s\n", buffer);
    //  send(new_socket, hello, strlen(hello), 0);

    // mqd_t mqd; // Message Queue Descriptor
    //  Attributes structure for the message queue
    //  struct mq_attr attr;
    //  attr.mq_flags = 0;
    //  attr.mq_maxmsg = MAX_MESSAGES;
    //  attr.mq_msgsize = MAX_MSG_SIZE;
    //  attr.mq_curmsgs = 0; // Current number of messages
    //  // 1. Create and open the message queue
    //  mqd = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);

    // if (mqd == (mqd_t)-1) {
    // perror("mq_open failed");
    // return 1;
    // }
    // --- Fork the Child (Receiver) Process ---
    // pid_t pid = fork();
    // if (pid < 0) {
    // perror("fork failed");
    // // mq_close(mqd);
    // // mq_unlink(MQ_NAME);
    // close(server_fd);
    // return 1;
    // } else if (pid == 0) {
    // // Child Process: Execute the Receiver Program
    // execl("./client", "client", MQ_NAME, NULL);
    // // execl only returns if an error occurred
    // perror("execl failed");
    // mq_close(mqd);
    // mq_unlink(MQ_NAME);
    //     close(new_socket);

    // //close(server_fd);

    // exit(EXIT_FAILURE);
    // } else {
    // // Parent Process: Run the Sender logic

    // run_sender(mqd, pid);
    //      send(new_socket, hello, strlen(hello), 0);

    // // Wait for the child to finish
    // wait(NULL);
    // // Cleanup: Close and unlink the message queue
    // // mq_close(mqd);
    // // mq_unlink(MQ_NAME);
    //     close(new_socket);

    // close(server_fd);

    // printf("\n[SENDER] Parent finished and cleaned up message queue.");
    // }
    //send(new_socket, hello, strlen(hello), 0);

    // Wait for the child to finish
    wait(NULL);
    // Cleanup: Close and unlink the message queue
    // mq_close(mqd);
    // mq_unlink(MQ_NAME);
    close(new_socket);

    close(server_fd);

    printf("\n[SENDER] Parent finished and cleaned up message queue.");

    return 0;
}