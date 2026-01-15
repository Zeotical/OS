#include <stdio.h>
#include <fcntl.h> //used to manipulate file descriptors
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#define PORT 8080 
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
// #include <asm-generic/socket.h> used for SO_REUSEPORT

// Define constants for the shared memory
const char *SHM_NAME = "/my_ipc_shm";
#define SHM_SIZE sizeof(SharedGameState)

typedef struct
{ // typedef creates a shortcut alias so we can refer to that structure using just a single name.struct defines a new data structure that requires the struct keyword for every declaration.
    int current_player;
    int client_sockets[2]; // num of players ig
    char buffer[1024];
    int game_done;
    pthread_mutex_t turn_mutex;
    sem_t turn;
    sem_t player_finished;
    int children[2];

} SharedGameState;
////////////////
void *turn(void *arg)
{
    SharedGameState *gameState_ptr = (SharedGameState *)arg;
    int i = 0;
    while (gameState_ptr->game_done != 1)
    {
        // Mutex lock
        pthread_mutex_lock(&gameState_ptr->turn_mutex);
        // CRITICAL SECTION
        gameState_ptr->current_player = i % 2;
        printf("Player %d's turn.\n", gameState_ptr->current_player + 1);
        // Mutex Unlock
        pthread_mutex_unlock(&gameState_ptr->turn_mutex);
        //Wake up all child processes
        for (int s = 0; s < 2; s++) {
        sem_post(&gameState_ptr->turn);            // signal player it's time for it's turn
        }
        //Wait until they finish before moving to the next turn
        for (int w = 0; w < 2; w++) {
        sem_wait(&gameState_ptr->player_finished); // wait until player finishes
        }
        i++;
        // sleep(1);
    }
    pthread_exit(0);
}
void game_start(void *arg, int my_player_id, int local_socket)
{
    SharedGameState *shm_ptr = (SharedGameState *)arg;
    char wait_turn[1024];
    char prompt[] = "YOUR TURN";
    shm_ptr->game_done = 0;
    int i = 0;
    while (i != 5)
    {
        sem_wait(&shm_ptr->turn); // wait for scheduler to assign turn   
    
       if (shm_ptr->current_player == my_player_id)
        { // if current_player == the current child

            local_socket = shm_ptr->client_sockets[my_player_id];
            send(local_socket, prompt, strlen(prompt), 0);
            memset(shm_ptr->buffer, 0, 1024);
            ssize_t valread = read(local_socket, shm_ptr->buffer, 1024 - 1);
            if (valread <= 0)
                continue; // if connection closed, skip Mr sharaf
            if (valread > 0)
            {
                printf("Player %d says: %s\n", my_player_id + 1, shm_ptr->buffer);
                printf("%d\n", i);
            }
            sem_post(&shm_ptr->player_finished); // send signal to scheduler to get next turn (putting it here in case it is not the turn of child yet)

        }

        else{

        local_socket = shm_ptr->client_sockets[my_player_id];
        sprintf(wait_turn, "Player %d's turn.Please wait.", shm_ptr->current_player+1);
        send(local_socket, wait_turn, strlen(wait_turn), 0); 
        memset(shm_ptr->buffer, 0, 1024);
        sem_post(&shm_ptr->player_finished); // send signal to scheduler to get next turn (putting it here in case it is not the turn of child yet)
        }
        // pthread_mutex_unlock(&shm_ptr->turn_mutex); // unlock happens whether we enter if or not //let scheduler change shared data
        if (i == 4)
        {
            printf("WE EXIT\n");
            shm_ptr->game_done = 1;
            close(shm_ptr->client_sockets[my_player_id]); // close socket connection of client who leaves
        }
        // sem_post(&shm_ptr->player_finished); // send signal to scheduler to get next turn (putting it here in case it is not the turn of child yet)
        i++;
        // usleep(100000);
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
    int player_no = 0;
    int shm_fd;
    SharedGameState *shm_ptr = NULL;
    // 1. Create and open the shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open failed");
        return 1;
    }

    // 2. Set the size of the shared memory object
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {
        perror("ftruncate failed");
        shm_unlink(SHM_NAME);
        return 1;
    }

    // 3. Map the shared memory object into the process's address space
    shm_ptr = (SharedGameState *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap failed");
        shm_unlink(SHM_NAME);
        return 1;
    }

    // PTHREAD_PROCESS_SHARED
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shm_ptr->turn_mutex, &mattr);

    sem_init(&shm_ptr->turn, 1, 0);
    sem_init(&shm_ptr->player_finished, 1, 0);
    shm_ptr->current_player = 0;

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
    while (player_no < 2) // while loop for connecting clients
    {
        printf("Waiting for player %d to join\n", player_no + 1);
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else
        {
            // --- Fork the Child (CLient) Process ---
            int my_player_id = player_no;
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork failed");
                close(new_socket);
                return 1;
            }
            else if (pid == 0)
            {
                close(server_fd); // no need for child to listen for connections
                printf("Player % d joined.\n", player_no + 1);
                shm_ptr->client_sockets[my_player_id] = new_socket;
                // // Child Process: Execute the Receiver Program
                send(new_socket, hello, strlen(hello), 0);
                // valread = read(new_socket, buffer,1024 - 1);
                // printf("%s\n", buffer);

                // if players joined satrt the game
                // if(player_no == 1) {
                game_start(shm_ptr, my_player_id, new_socket); // pass id

                close(new_socket);
                exit(0); // child exits
                         // }
            }

            else
            {
                // Parent Process: Run the Server logic
                shm_ptr->children[my_player_id] = pid;
                close(new_socket); // close connected client socket, let child deal with it
                player_no++;
            }
        }
    } // while loop

    // TODO thread for client turn ++ signchld for non blocking reapin
    pthread_t scheduler;
    //create the threads
    pthread_create(&scheduler, NULL, turn, shm_ptr);

    pthread_join(scheduler, NULL);
    int s = 0;
    // kill the children
    while (s != 2)
    {
        kill(shm_ptr->children[s], SIGTERM);
        s++;
    }
    // Reap all child processes
    while (wait(NULL) > 0)
    { // wait(NULL) returns a positive value(PID) of child if it exits or no error happens
        wait(NULL);
    }

    close(server_fd); // Close listening socket
    // Cleanup: Unmap the memory and remove the shared object
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);
    printf("[SERVER] Parent finished and cleaned up shared memory.\n");
    printf("\n[SERVER] Parent finished and reaped all child processes.");
    return 0;
}