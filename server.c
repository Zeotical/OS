#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define PORT 5555
#define MAX_PLAYERS 3
#define WORD "apple"

typedef struct {
    int current_turn;
    int total_players;
    int game_over;
    char revealed[16];
    int wrong;
    int turn_done;
    int last_guess_correct;
    int client_socks[MAX_PLAYERS]; 
    pthread_mutex_t mutex;
    sem_t turn_sem[MAX_PLAYERS];
} shared_state_t;

shared_state_t *state;

//send word to everyone
void broadcast_game_state() {
    char msg[128];
    sprintf(msg, "\n--- Update ---\nWord: %s\nWrong: %d\n--------------\n", state->revealed, state->wrong);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (state->client_socks[i] > 0) {
            send(state->client_socks[i], msg, strlen(msg), 0);
        }
    }
}

void *scheduler_thread(void *arg) {
    //wait for players to be ready
    sleep(1); 
    while (!state->game_over) {
        pthread_mutex_lock(&state->mutex);
        if (!state->turn_done) {
            pthread_mutex_unlock(&state->mutex);
            usleep(100000);
            continue;
        }

        if (!state->last_guess_correct) {
            state->current_turn = (state->current_turn + 1) % state->total_players;
        }

        int next = state->current_turn;
        state->turn_done = 0;
        pthread_mutex_unlock(&state->mutex);

        //signal for acitve player turn
        sem_post(&state->turn_sem[next]);
    }
    //try semophores dk if works or not
    for(int i=0; i<MAX_PLAYERS; i++) sem_post(&state->turn_sem[i]);
    return NULL;
}

void handle_client(int player_id) {
    int sock = state->client_socks[player_id];
    char buf[64];

    while (!state->game_over) {
        sem_wait(&state->turn_sem[player_id]);
        if (state->game_over) break;

        //active player prompt
        char *prompt = "\n*** YOUR TURN ***\nGuess a letter: ";
        send(sock, prompt, strlen(prompt), 0);

        memset(buf, 0, sizeof(buf));
        int n = read(sock, buf, sizeof(buf));
        if (n <= 0) break;

        char guess = buf[0];
        pthread_mutex_lock(&state->mutex);
        
        int found = 0;
        for (int i = 0; WORD[i]; i++) {
            if (WORD[i] == guess) {
                state->revealed[i] = guess;
                found = 1;
            }
        }

        if (!found) {
            state->wrong++;
            state->last_guess_correct = 0;
        } else {
            state->last_guess_correct = 1;
        }

        if (strcmp(state->revealed, WORD) == 0) state->game_over = 1;

        // Update all terminals
        broadcast_game_state();

        state->turn_done = 1;
        pthread_mutex_unlock(&state->mutex);
    }

    send(sock, "\nGAME OVER! The word was apple.\n", 32, 0);
    close(sock);
}

int main() {
    //shared mem
    state = mmap(NULL, sizeof(shared_state_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&state->mutex, &mattr);

    state->total_players = MAX_PLAYERS;
    state->current_turn = 0;
    state->game_over = 0;
    state->turn_done = 1;
    state->last_guess_correct = 0;
    state->wrong = 0;
    strcpy(state->revealed, "_____");

    for (int i = 0; i < MAX_PLAYERS; i++) {
        sem_init(&state->turn_sem[i], 1, 0);
    }

    //network stuff
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, MAX_PLAYERS);

    printf("SERVER READY. Connect %d clients to start.\n", MAX_PLAYERS);

    //accept before forking
    for (int i = 0; i < MAX_PLAYERS; i++) {
        state->client_socks[i] = accept(server_fd, NULL, NULL);
        printf("Player %d joined (Socket: %d)\n", i, state->client_socks[i]);
    }

    //start
    pthread_t sched;
    pthread_create(&sched, NULL, scheduler_thread, NULL);

    //fork
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (fork() == 0) {
            handle_client(i);
            exit(0);
        }
    }

    //wait for players
    while(wait(NULL) > 0);
    printf("Game finished. Server shutting down.\n");
    return 0;
}