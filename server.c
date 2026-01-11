#include <stdio.h>
#include <fcntl.h> //used to manipulate file descriptors
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#define PORT 8080 // Mr sharaf
#include <netinet/in.h>
#include <sys/socket.h>

#include <pthread.h>
#include <sys/mman.h>


// #include <asm-generic/socket.h> used for SO_REUSEPORT

// Define constants for the shared memory
const char* SHM_NAME = "/my_ipc_shm";
const size_t SHM_SIZE = 1024;
const char* MESSAGE = "Hello from the Writer! Data cycle:\n";

typedef struct {
int current_player;
int client_sockets[3]; // num of players ig
char buffer[256];
int counter;
} SharedGameState;
////////////////
int x = 10;

// Global Mutex Locks (Resources)
pthread_mutex_t turn_mutex; // Assume Order = 1

void* turn(SharedGameState *gameState_ptr)
{
//  usleep(50000); // sleep for 50000 micro seconds
//  printf("Thread 1: %d\n", getpid());
//  x++;
//  printf("Thread 1: x = %d\n", x);
//decide whose turn it is
// //while smth either all connected player didn't exit 
// //client_socket[current_player_turn]
// //child with that socket gets siignaled
// //wakes up and talks with client
// //send this to other player's waiting for other's turn OR jus with name/number
    int i = 0 ;
 while (gameState_ptr->current_player<3){
    // gameState_ptr->current_player = i;
    if(gameState_ptr->client_sockets[gameState_ptr->current_player]){
        pthread_mutex_lock(&turn_mutex);
        printf("Thread 1: Acquired first_mutex. Now acquiring second_mutex...\n");
 
        // CRITICAL SECTION
        printf("Thread 1: Acquired lock and is doing work.\n");
// Write data into the shared memory
// shm_ptr->counter = i;
    gameState_ptr->current_player = i;

    // char str[20]; // Buffer to hold the string why we usin 20 here chat?
    // sprintf(str, "%d", i); // Converts int to string
    // const char* full_msg = MESSAGE;
    // strncpy(gameState_ptr->buffer, full_msg, sizeof(gameState_ptr->buffer) - 1);
    // shm_ptr->buffer[sizeof(shm_ptr->buffer) - 1] = '\0';
    // strncpy(shm_ptr->buffer + strlen(shm_ptr->buffer), str, strlen(str)+1);

    printf("[WRITER] Wrote message %d. Pausing...\n",i);
    sleep(2); // Pause to let the reader read
    }
// Set a stop signal for the reader
gameState_ptr->counter = -1;
printf("[WRITER] Sent termination signal (-1). Waiting for reader...\n");
        //thread updating memory
        // Release locks in reverse order of acquisition
        pthread_mutex_unlock(&turn_mutex);
        printf("Thread 1: Finished and released both locks.\n");
        pthread_exit(0);
        i++;
    }
}


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
    int client_sockets[3];

int shm_fd;
SharedGameState* shm_ptr = NULL;
// 1. Create and open the shared memory object
shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
if (shm_fd == -1) {
perror("shm_open failed");
return 1;
}
// 2. Set the size of the shared memory object
if (ftruncate(shm_fd, SHM_SIZE) == -1) {
perror("ftruncate failed");
shm_unlink(SHM_NAME);
return 1;
}
// 3. Map the shared memory object into the process's address space
shm_ptr = (SharedGameState*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,shm_fd, 0);
if (shm_ptr == MAP_FAILED) {
perror("mmap failed");
shm_unlink(SHM_NAME);
return 1;
}
// Initialize the shared data structure
shm_ptr->current_player= 0;
shm_ptr->counter = 0;

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
                //add connected client to array
                client_sockets[player_no-1] = new_socket;
                close(server_fd); //no need for child to listen for connections
                printf("Player % d joined.\n" , player_no);
                send(new_socket, hello, strlen(hello), 0);
                valread = read(new_socket, buffer,1024 - 1);
                printf("%s\n", buffer);

                while (shm_ptr->counter != -1) {
                if (shm_ptr->counter > 0) {
                printf("[READER] Received Counter: %d  | Message: %s \n", shm_ptr->counter,shm_ptr->buffer );
                }
                usleep(500000); // Wait 0.5 seconds before checking again
                }
                //sleep(2);
                close(new_socket);
                _exit(32); // child exits
                //exit(EXIT_SUCCESS);

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

    struct SharedGameState* gameState;
    // declare thread
    pthread_t scheduler ;
    // create the threads
    pthread_create(&scheduler, NULL, turn, gameState);
    // wait for the threads to complete
    pthread_join(scheduler, NULL);

    //run_writer(shm_ptr, pid);
    // Wait for the child to finish
    wait(NULL);
    
    //TODO thread for client turn ++ signchld for non blocking reapin
    // Reap all child processes
    while(wait(NULL)>0){ //wait(NULL) returns a positive value(PID) of child if it exits or no error happens
    wait(NULL);
    }

    close(server_fd); // Close listening socket

// Cleanup: Unmap the memory and remove the shared object
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);
    printf("[WRITER] Parent finished and cleaned up shared memory.\n");

    printf("\n[SERVER] Parent finished and reaped all child processes.");
    return 0;
}