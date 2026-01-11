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
#include <semaphore.h>



// #include <asm-generic/socket.h> used for SO_REUSEPORT

// Define constants for the shared memory
const char* SHM_NAME = "/my_ipc_shm";
//const size_t SHM_SIZE = 1024;
#define SHM_SIZE sizeof(SharedGameState)
sem_t turn_finish;

typedef struct { //typedef creates a shortcut alias so we can refer to that structure using just a single name.struct defines a new data structure that requires the struct keyword for every declaration.
int current_player;
int client_sockets[2]; // num of players ig
char buffer[1024];
int counter;
int turn_done;
} SharedGameState;
////////////////

// Global Mutex Locks (Resources)
pthread_mutex_t turn_mutex; // Assume Order = 1

void* turn(void *arg)
{
    SharedGameState *gameState_ptr = ( SharedGameState *)arg;
    int i = 0 ;
 while (i<3){
    if(gameState_ptr->counter < 0){
        pthread_mutex_lock(&turn_mutex);
        printf("Thread 1: Acquired first_mutex.\n");
        // CRITICAL SECTION
        printf("Thread 1: Acquired lock and is doing work.\n");
        i = i % 3;
        gameState_ptr->current_player = i; // Write data into the shared memory
        i++;
        pthread_mutex_unlock(&turn_mutex);
        printf("Thread 1: Finished and released both locks.\n");
    }
        sem_wait(&turn_finish);

    }
        pthread_exit(0);

}

int main()
{

    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    //char buffer[1024] = {0};
    char hello[] = "Hello from server";
    int player_no = 1;
    sem_init(&turn_finish, 0, 1);

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
//pthread_t scheduler ;
    // create the threads
   // pthread_create(&scheduler, NULL, turn, shm_ptr);
// Initialize the shared data structure
// shm_ptr->current_player= 0;

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
                if(shm_ptr->counter<2){
                shm_ptr->client_sockets[player_no-1] = new_socket;
                shm_ptr->counter++;
                }
                close(server_fd); //no need for child to listen for connections
                printf("Player % d joined.\n" , player_no);
                //while(1){
                // if(shm_ptr->current_player == player_no-1){
                new_socket = shm_ptr->client_sockets[shm_ptr->current_player];
                send(new_socket, hello, strlen(hello), 0);
                valread = read(new_socket, shm_ptr->buffer,1024 - 1);
                printf("%s\n", shm_ptr->buffer);
                sem_post(&turn_finish);
                close(new_socket);
                _exit(32); // child exits}
                //break;
                   // }
                   //}

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

    // struct SharedGameState* gameState_ptr;
    // declare thread
     pthread_t scheduler ;
    // // create the threads
     pthread_create(&scheduler, NULL, turn, shm_ptr);
    // wait for the threads to complete
   //pthread_join(scheduler, NULL);

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