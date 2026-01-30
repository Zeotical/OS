#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define PORT 5555
#define MIN_PLAYERS 2
#define MAX_PLAYERS 5
#define STR_LEN 255
#define CHAR_SET_SIZE 26
#define MAX_NAME_LEN 32

const char CHAR_MAP[26] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
};

typedef struct {
    int current_turn;
    int total_players;
    int game_over;
    int game_active;
    int players_ready;
    //int wrong
    int turn_done;
    int last_guess_correct;
    char idiom[STR_LEN];
    char formatted_idiom[STR_LEN];
    int char_selection_list[CHAR_SET_SIZE];
    int char_selected_list[CHAR_SET_SIZE];
    int idiom_char_list[CHAR_SET_SIZE];
    int client_socks[MAX_PLAYERS];
    char player_names[MAX_PLAYERS][MAX_NAME_LEN];
    pthread_mutex_t mutex;
    sem_t turn_sem[MAX_PLAYERS];
} shared_state_t;

shared_state_t *state;

//help funcs

void flush_socket(int sock) {
    char buf[1024];
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    while(read(sock, buf, sizeof(buf)) > 0); 
    fcntl(sock, F_SETFL, flags);
}

void idiom_char_list_init(int* idiom_char_list, const char* idiom) {
    int size = strlen(idiom);
    int count = 0;
    for(int i=0; i<CHAR_SET_SIZE; i++) idiom_char_list[i] = 0;
    
    for (int i = 0; i < size; i++) {
        if (!isalpha(idiom[i])) continue;
        int num = toupper(idiom[i]) - 'A' + 1;
        int found = 0;
        for (int j = 0; j < count; j++) {
            if (idiom_char_list[j] == num) { found = 1; break; }
        }
        if (!found) idiom_char_list[count++] = num;
    }
}

void idiom_char_list_clear(int* list, int index) {
    int i = index;
    while (list[i] != 0 && i < CHAR_SET_SIZE - 1) {
        list[i] = list[i+1];
        i++;
    }
    list[i] = 0;
}

int get_list_index_by_char(const int* list, char character) {
    int char_num = toupper(character) - 'A' + 1;
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        if (list[i] == char_num) return i;
        if (list[i] == 0) break;
    }
    return -1;
}

void to_format_idiom(const char* idiom, char* formatted, char c) {
    for (int i = 0; idiom[i]; i++) {
        if (toupper(idiom[i]) == toupper(c)) {
            formatted[i] = idiom[i]; 
        }
    }
}

int from_to_list(int* from, int* to, char c) {
    if (!isalpha(c)) return 0;
    int idx = toupper(c) - 'A';
    
    if (from[idx] <= 0) return 0; 
    
    from[idx] = -1; //mark as taken
    
    //add to selected list
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        if (to[i] == 0) { 
            to[i] = idx + 1; 
            return 1; 
        }
    }
    return 0;
}

int is_char_available(int* from, char c) {
    if (!isalpha(c)) return 0;
    int idx = toupper(c) - 'A';
    return (from[idx] > 0);
}

void print_list_with_alph(const int* list, char* out) {
    strcat(out, "[ ");
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        if (list[i] == 0) break;
        if (list[i] == -1) strcat(out, "_ ");
        else {
            char tmp[4];
            sprintf(tmp, "%c ", CHAR_MAP[list[i]-1]);
            strcat(out, tmp);
        }
    }
    strcat(out, "]");
}

void broadcast(const char* msg) {
    for (int i = 0; i < state->total_players; i++) {
        if (state->client_socks[i] > 0) {
            send(state->client_socks[i], msg, strlen(msg), 0);
        }
    }
}

//construct string
void construct_state_string(char *buf) {
    // \033[H\033[J clears the screen and moves cursor to top
    //this fix the text issue and also helps clear screen which is pretty neat ngl
    sprintf(buf, "\033[H\033[J\n\n--- Current Game State ---\n");
    char tmp[512] = {0};
    
    strcat(buf, "SELECTION: ");
    tmp[0] = '\0';
    print_list_with_alph(state->char_selection_list, tmp);
    strcat(buf, tmp);
    
    strcat(buf, "\nSELECTED:  ");
    tmp[0] = '\0';
    print_list_with_alph(state->char_selected_list, tmp);
    strcat(buf, tmp);
    
    //wrong guesses here
    sprintf(tmp, "\nIdiom: %s\n", state->formatted_idiom);
    strcat(buf, tmp);
}

void send_state(int sock) {
    char buf[2048];
    construct_state_string(buf);
    send(sock, buf, strlen(buf), 0);
}

//reveal random 4 letters
void reveal_starting_letters() {
    int pool[26];
    for(int i=0; i<26; i++) pool[i] = i+1; 

    //shuffle
    for(int i=0; i<26; i++) {
        int r = rand() % 26;
        int temp = pool[i];
        pool[i] = pool[r];
        pool[r] = temp;
    }

    //pick 4 distinct letters
    for(int i=0; i<4; i++) {
        char c = 'A' + pool[i] - 1;
        
        //mark as taken and add to selected list
        from_to_list(state->char_selection_list, state->char_selected_list, c);
        
        //update idiom
        int idx = get_list_index_by_char(state->idiom_char_list, c);
        if(idx >= 0) {
            to_format_idiom(state->idiom, state->formatted_idiom, c);
            idiom_char_list_clear(state->idiom_char_list, idx);
        }
    }
}

//threading (im dying here)
void *scheduler_thread(void *arg) {
    sleep(1);
    while (!state->game_over) {
        pthread_mutex_lock(&state->mutex);
        
        //turn logic handler
        if (state->turn_done) {
            if (!state->last_guess_correct) {
                state->current_turn = (state->current_turn + 1) % state->total_players;
            }
            state->turn_done = 0;
            state->last_guess_correct = 0;
            int turn = state->current_turn;
            pthread_mutex_unlock(&state->mutex);
            
            sem_post(&state->turn_sem[turn]);
        } 
        else {
            pthread_mutex_unlock(&state->mutex);
        }
        usleep(100000); //100ms btw
    }
    return NULL;
}

void game_start(int id) {
    int sock = state->client_socks[id];
    char buf[STR_LEN];
    
    //name
    send(sock, "Enter your name: ", 17, 0);
    memset(buf, 0, STR_LEN);
    int n = read(sock, buf, STR_LEN-1);
    if (n <= 0) return;
    buf[strcspn(buf, "\r\n")] = 0;
    strncpy(state->player_names[id], buf, MAX_NAME_LEN-1);

    pthread_mutex_lock(&state->mutex);
    state->players_ready++;
    printf("[LOG] Player %d (%s) connected. (%d/%d)\n", id+1, state->player_names[id], state->players_ready, state->total_players);
    if (state->players_ready == state->total_players) {
        state->game_active = 1;
        printf("[LOG] All players ready. Game active. Idiom: %s\n", state->idiom);
    }
    pthread_mutex_unlock(&state->mutex);

    while (!state->game_active) usleep(100000);

    //main game loop
    while (!state->game_over) {
        //waiting msg
        char wait_msg[] = "\nWaiting for other players...\n";
        send(sock, wait_msg, strlen(wait_msg), 0);

        //wait turn
        sem_wait(&state->turn_sem[id]);
        if (state->game_over) break;

        flush_socket(sock);

        //print board when turn start
        pthread_mutex_lock(&state->mutex);
        send_state(sock);
        pthread_mutex_unlock(&state->mutex);

        //loop until have valid input
        char l1 = 0, l2 = 0;
        int input_valid = 0;

        while (!input_valid) {
            char turn_msg[128];
            sprintf(turn_msg, "\n*** YOUR TURN (%s) ***\nChoose 2 letters (e.g., 'AB'): ", state->player_names[id]);
            send(sock, turn_msg, strlen(turn_msg), 0);

            memset(buf, 0, STR_LEN);
            n = read(sock, buf, STR_LEN-1);
            if (n <= 0) break;
            buf[strcspn(buf, "\r\n")] = 0;

            int count = 0;
            char temp[3] = {0, 0, 0};
            for(int i=0; buf[i] && count < 2; i++) {
                if(isalpha(buf[i])) temp[count++] = toupper(buf[i]);
            }

            if (count < 2) {
                send(sock, "ERROR: You must enter exactly 2 letters. Try again.\n", 52, 0);
                continue;
            }

            pthread_mutex_lock(&state->mutex);
            int avail1 = is_char_available(state->char_selection_list, temp[0]);
            int avail2 = is_char_available(state->char_selection_list, temp[1]);
            
            if (temp[0] == temp[1]) avail2 = 0; 

            if (!avail1 || !avail2) {
                char err[128];
                sprintf(err, "ERROR: Invalid selection. %c%c contains used or invalid letters.\n", temp[0], temp[1]);
                send(sock, err, strlen(err), 0);
                pthread_mutex_unlock(&state->mutex);
                continue;
            }
            
            l1 = temp[0];
            l2 = temp[1];
            input_valid = 1;
            
            from_to_list(state->char_selection_list, state->char_selected_list, l1);
            from_to_list(state->char_selection_list, state->char_selected_list, l2);
            
            int hits = 0;
            int idx = get_list_index_by_char(state->idiom_char_list, l1);
            if(idx >= 0) { to_format_idiom(state->idiom, state->formatted_idiom, l1); idiom_char_list_clear(state->idiom_char_list, idx); hits++; }
            
            idx = get_list_index_by_char(state->idiom_char_list, l2);
            if(idx >= 0) { to_format_idiom(state->idiom, state->formatted_idiom, l2); idiom_char_list_clear(state->idiom_char_list, idx); hits++; }

            printf("[LOG] Player %d chose %c, %c. Matches: %d. State: %s\n", id+1, l1, l2, hits, state->formatted_idiom);
                   
            pthread_mutex_unlock(&state->mutex);
        }

        pthread_mutex_lock(&state->mutex);
        send_state(sock);
        
        //win checker
        if (state->idiom_char_list[0] == 0) {
            state->game_over = 1;
            char win_msg[256];
            sprintf(win_msg, "\nGAME OVER! Idiom Completed: %s\nWINNER: %s\n", state->idiom, state->player_names[id]);
            broadcast(win_msg);
            
            pthread_mutex_unlock(&state->mutex);
            kill(0, SIGTERM);
            exit(0);
        }

        send(sock, "Guess the full idiom (or press Enter to skip): ", 47, 0);
        pthread_mutex_unlock(&state->mutex);

        memset(buf, 0, STR_LEN);
        read(sock, buf, STR_LEN-1);
        buf[strcspn(buf, "\r\n")] = 0;

        pthread_mutex_lock(&state->mutex);
        if (strlen(buf) > 0) {
            printf("[LOG] Player %d guessed: %s\n", id+1, buf);
            if (strcasecmp(buf, state->idiom) == 0) {
                state->game_over = 1;
                char win[128];
                sprintf(win, "\nWINNER! %s guessed it: %s\n", state->player_names[id], state->idiom);
                broadcast(win);
                
                pthread_mutex_unlock(&state->mutex);
                kill(0, SIGTERM);
                exit(0);
            } else {
                send(sock, "Wrong guess!\n", 13, 0);
                //staet->wrong++;
                printf("[LOG] Player %d guessed wrong.\n", id+1);
            }
        } else {
             printf("[LOG] Player %d skipped guessing.\n", id+1);
        }
        
        state->turn_done = 1;
        pthread_mutex_unlock(&state->mutex);
    }
    close(sock);
}

int main(int argc, char **argv) {
    int num = (argc > 1) ? atoi(argv[1]) : 2;
    if (num < MIN_PLAYERS) num = MIN_PLAYERS;
    if (num > MAX_PLAYERS) num = MAX_PLAYERS;

    state = mmap(NULL, sizeof(shared_state_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&state->mutex, &attr);
    
    state->total_players = num;
    state->turn_done = 1;
    for(int i=0; i<MAX_PLAYERS; i++) sem_init(&state->turn_sem[i], 1, 0);

    srand(time(NULL));
    const char *idioms[] = {"APPLE PIE", "HARD WORK", "GOOD LUCK", "GAME OVER"};
    strcpy(state->idiom, idioms[rand() % 4]);
    
    for(int i=0; i<26; i++) state->char_selection_list[i] = i+1;
    for(int i=0; state->idiom[i]; i++) {
        state->formatted_idiom[i] = isalpha(state->idiom[i]) ? '_' : state->idiom[i];
    }
    state->formatted_idiom[strlen(state->idiom)] = '\0';
    idiom_char_list_init(state->idiom_char_list, state->idiom);

    reveal_starting_letters();

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {AF_INET, htons(PORT), INADDR_ANY};
    bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sfd, num);

    printf("--- SERVER STARTED ---\n");
    printf("[LOG] Waiting for %d players...\n", num);

    for (int i = 0; i < num; i++) {
        state->client_socks[i] = accept(sfd, NULL, NULL);
        printf("[LOG] Connection received for Player %d\n", i+1);
    }

    pthread_t sched;
    pthread_create(&sched, NULL, scheduler_thread, NULL);

    for (int i = 0; i < num; i++) {
        if (fork() == 0) {
            game_start(i);
            exit(0);
        }
    }

    int status;
    while (wait(&status) > 0);
    
    printf("\n--- GAME SESSION ENDED ---\n");
    
    pthread_cancel(sched);
    pthread_join(sched, NULL);
    pthread_mutex_destroy(&state->mutex);
    munmap(state, sizeof(shared_state_t));
    close(sfd);

    return 0;
}