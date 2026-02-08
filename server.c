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
#include <stdarg.h>

#define PORT 5555
#define MIN_PLAYERS 3
#define MAX_PLAYERS 5
#define STR_LEN 255
#define CHAR_SET_SIZE 26
#define MAX_NAME_LEN 32
#define SCORE_FILE "scores.txt"
#define LOG_FILE "game.log"
#define IDIOM_FILE "idioms.txt"
#define MAX_SCORES 100
#define LOG_QUEUE_SIZE 50

const char CHAR_MAP[26] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
};

//data struct
typedef struct {
    char name[MAX_NAME_LEN];
    int wins;
    int games_played;
} PlayerScore;

typedef struct {
    char messages[LOG_QUEUE_SIZE][STR_LEN];
    int head;
    int tail;
    sem_t count_sem;
    sem_t space_sem;
    pthread_mutex_t mutex;
} LogQueue;

typedef struct {
    //game control
    int current_turn;
    int total_players;      
    int players_ready;      
    int game_over;          
    int game_active;        
    int round_number;       
    
    //voting logic
    int votes_yes;          
    int votes_no;           
    int voting_pool_size;   
    
    //turn logic
    int turn_done;
    int last_guess_correct;
    
    //player status
    int player_active[MAX_PLAYERS]; 
    char player_names[MAX_PLAYERS][MAX_NAME_LEN];
    int client_socks[MAX_PLAYERS];
    
    //game data
    char idiom[STR_LEN];
    char formatted_idiom[STR_LEN];
    int char_selection_list[CHAR_SET_SIZE];
    int char_selected_list[CHAR_SET_SIZE];
    int idiom_char_list[CHAR_SET_SIZE];
    
    //scoring data
    PlayerScore scores[MAX_SCORES];
    int score_count;
    
    //synchros
    pthread_mutex_t state_mutex;
    pthread_mutex_t score_mutex;
    sem_t turn_sem[MAX_PLAYERS];
    
    //log data
    LogQueue log_queue; 
} shared_state_t;

shared_state_t *state;
FILE *log_fp;

//helper funcs
void log_event(const char *format, ...) {
    va_list args;
    char buf[STR_LEN];
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    sem_wait(&state->log_queue.space_sem);
    pthread_mutex_lock(&state->log_queue.mutex);
    strncpy(state->log_queue.messages[state->log_queue.head], buf, STR_LEN);
    state->log_queue.head = (state->log_queue.head + 1) % LOG_QUEUE_SIZE;
    pthread_mutex_unlock(&state->log_queue.mutex);
    sem_post(&state->log_queue.count_sem);
}

void flush_socket(int sock) {
    char buf[1024];
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    while(read(sock, buf, sizeof(buf)) > 0); 
    fcntl(sock, F_SETFL, flags);
}

void load_random_idiom(char *dest) {
    FILE *fp = fopen(IDIOM_FILE, "r");
    if (!fp) {
        log_event("Error: Could not open %s. Using fallback.", IDIOM_FILE);
        strcpy(dest, "DEFAULT IDIOM");
        return;
    }
    char line[STR_LEN];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) < 2) continue;
        count++;
        if (rand() % count == 0) {
            strncpy(dest, line, STR_LEN - 1);
            dest[STR_LEN - 1] = '\0';
        }
    }
    fclose(fp);
    if (count == 0) strcpy(dest, "EMPTY FILE IDIOM");
    for(int i = 0; dest[i]; i++) dest[i] = toupper(dest[i]);
    log_event("Loaded idiom: %s", dest);
}

void load_scores() {
    FILE *fp = fopen(SCORE_FILE, "r");
    state->score_count = 0;
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp) && state->score_count < MAX_SCORES) {
            char name[MAX_NAME_LEN];
            int wins, total;
            if (sscanf(line, "%s %d %d", name, &wins, &total) == 3) {
                strncpy(state->scores[state->score_count].name, name, MAX_NAME_LEN);
                state->scores[state->score_count].wins = wins;
                state->scores[state->score_count].games_played = total;
                state->score_count++;
            }
        }
        fclose(fp);
    }
}

void save_scores() {
    FILE *fp = fopen(SCORE_FILE, "w");
    if (!fp) return;
    pthread_mutex_lock(&state->score_mutex);
    for (int i = 0; i < state->score_count; i++) {
        fprintf(fp, "%s %d %d\n", state->scores[i].name, state->scores[i].wins, state->scores[i].games_played);
    }
    pthread_mutex_unlock(&state->score_mutex);
    fclose(fp);
}

void update_player_score(const char *name, int is_winner) {
    pthread_mutex_lock(&state->score_mutex);
    int found = -1;
    for (int i = 0; i < state->score_count; i++) {
        if (strcmp(state->scores[i].name, name) == 0) {
            found = i; break;
        }
    }
    if (found != -1) {
        state->scores[found].games_played++;
        if (is_winner) state->scores[found].wins++;
    } else if (state->score_count < MAX_SCORES) {
        strncpy(state->scores[state->score_count].name, name, MAX_NAME_LEN);
        state->scores[state->score_count].games_played = 1;
        state->scores[state->score_count].wins = (is_winner ? 1 : 0);
        state->score_count++;
    }
    pthread_mutex_unlock(&state->score_mutex);
}

void get_score_board_string(char *buffer) {
    pthread_mutex_lock(&state->score_mutex);
    sprintf(buffer, "\n--- LEADERBOARD ---\nName\t\tWins\tGames\tWin %%\n");
    char line[128];
    for(int i=0; i<state->score_count; i++) {
        float pct = (state->scores[i].games_played > 0) ? ((float)state->scores[i].wins / state->scores[i].games_played) * 100.0 : 0.0;
        sprintf(line, "%-10s\t%d\t%d\t%.1f%%\n", state->scores[i].name, state->scores[i].wins, state->scores[i].games_played, pct);
        strcat(buffer, line);
    }
    strcat(buffer, "-------------------\n");
    pthread_mutex_unlock(&state->score_mutex);
}

void idiom_char_list_init(int* idiom_char_list, const char* idiom) {
    int size = strlen(idiom);
    int count = 0;
    for(int i=0; i<CHAR_SET_SIZE; i++) idiom_char_list[i] = 0;
    for (int i = 0; i < size; i++) {
        if (!isalpha(idiom[i])) continue;
        int num = toupper(idiom[i]) - 'A' + 1;
        int found = 0;
        for (int j = 0; j < count; j++) { if (idiom_char_list[j] == num) { found = 1; break; } }
        if (!found) idiom_char_list[count++] = num;
    }
}

void idiom_char_list_clear(int* list, int index) {
    int i = index;
    while (list[i] != 0 && i < CHAR_SET_SIZE - 1) { list[i] = list[i+1]; i++; }
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
        if (toupper(idiom[i]) == toupper(c)) formatted[i] = idiom[i]; 
    }
}

int from_to_list(int* from, int* to, char c) {
    if (!isalpha(c)) return 0;
    int idx = toupper(c) - 'A';
    if (from[idx] <= 0) return 0; 
    from[idx] = -1; 
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        if (to[i] == 0) { to[i] = idx + 1; return 1; }
    }
    return 0;
}

int is_char_available(int* from, char c) {
    if (!isalpha(c)) return 0;
    int idx = toupper(c) - 'A';
    return (from[idx] > 0);
}

void construct_state_string(char *buf) {
    sprintf(buf, "\033[H\033[J\n\n--- Round %d ---\n", state->round_number);
    char tmp[512] = {0};
    
    strcat(buf, "SELECTION: [ ");
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        if (state->char_selection_list[i] == 0) break;
        if (state->char_selection_list[i] == -1) strcat(buf, "_ ");
        else {
            sprintf(tmp, "%c ", CHAR_MAP[state->char_selection_list[i]-1]);
            strcat(buf, tmp);
        }
    }
    strcat(buf, "]\nSELECTED:  [ ");
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        if (state->char_selected_list[i] == 0) break;
        sprintf(tmp, "%c ", CHAR_MAP[state->char_selected_list[i]-1]);
        strcat(buf, tmp);
    }
    sprintf(tmp, "]\n\nIdiom: %s\n", state->formatted_idiom);
    strcat(buf, tmp);
}

void send_state(int sock) {
    char buf[2048];
    construct_state_string(buf);
    if (send(sock, buf, strlen(buf), 0) < 0) {
        //handle error
    }
}

void reveal_starting_letters() {
    int pool[26];
    for(int i=0; i<26; i++) pool[i] = i+1; 
    for(int i=0; i<26; i++) { int r = rand() % 26; int temp = pool[i]; pool[i] = pool[r]; pool[r] = temp; }
    for(int i=0; i<4; i++) {
        char c = 'A' + pool[i] - 1;
        from_to_list(state->char_selection_list, state->char_selected_list, c);
        int idx = get_list_index_by_char(state->idiom_char_list, c);
        if(idx >= 0) {
            to_format_idiom(state->idiom, state->formatted_idiom, c);
            idiom_char_list_clear(state->idiom_char_list, idx);
        }
    }
}

void reset_game() {
    state->round_number++;
    log_event("Resetting game for Round %d...", state->round_number);
    
    // Cclear voting
    state->votes_yes = 0;
    state->votes_no = 0;
    state->voting_pool_size = 0;

    for(int i=0; i<26; i++) state->char_selection_list[i] = i+1;
    memset(state->char_selected_list, 0, sizeof(state->char_selected_list));
    load_random_idiom(state->idiom);
    for(int i=0; state->idiom[i]; i++) {
        state->formatted_idiom[i] = isalpha(state->idiom[i]) ? '_' : state->idiom[i];
    }
    state->formatted_idiom[strlen(state->idiom)] = '\0';
    idiom_char_list_init(state->idiom_char_list, state->idiom);
    reveal_starting_letters();
    state->game_over = 0;
    state->turn_done = 1;
    log_event("Game reset complete.");
}

//threads
void *logger_thread_func(void *arg) {
    log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) pthread_exit(NULL);
    while (1) {
        sem_wait(&state->log_queue.count_sem);
        pthread_mutex_lock(&state->log_queue.mutex);
        char *msg = state->log_queue.messages[state->log_queue.tail];
        time_t now = time(NULL);
        fprintf(log_fp, "[%lu] %s\n", (unsigned long)now, msg);
        fflush(log_fp);
        printf("[%lu] %s\n", (unsigned long)now, msg);
        fflush(stdout); 
        state->log_queue.tail = (state->log_queue.tail + 1) % LOG_QUEUE_SIZE;
        pthread_mutex_unlock(&state->log_queue.mutex);
        sem_post(&state->log_queue.space_sem);
    }
    fclose(log_fp);
    return NULL;
}

void *scheduler_thread(void *arg) {
    sleep(1);
    while (1) {
        pthread_mutex_lock(&state->state_mutex);
        
        //game over and voting
        if (state->game_over) {
            
            if ((state->votes_yes + state->votes_no) >= state->voting_pool_size && state->voting_pool_size > 0) {
                
                //majority check
                if (state->votes_no > (state->voting_pool_size / 2)) {
                    log_event("MAJORITY VOTED NO (%d/%d). Server Shutting Down.", state->votes_no, state->voting_pool_size);
                    sleep(1); //wait for log to print
                    kill(0, SIGTERM); 
                    exit(0);
                }

                //min player check
                if (state->players_ready < 2) {
                    log_event("Too few players to continue (%d). Server shutting down.", state->players_ready);
                    sleep(1); //wait for log to print
                    kill(0, SIGTERM);
                    exit(0);
                } else {
                    reset_game();
                }
            } else if (state->players_ready == 0) {
                log_event("All players disconnected. Server shutting down.");
                kill(0, SIGTERM);
                exit(0);
            }
            
            pthread_mutex_unlock(&state->state_mutex);
            sleep(1); 
            continue;
        }

        //turn logic
        if (state->turn_done) {
            if (!state->last_guess_correct) {
                int attempts = 0;
                do {
                    state->current_turn = (state->current_turn + 1) % state->total_players;
                    attempts++;
                } while (!state->player_active[state->current_turn] && attempts <= state->total_players);
                
                if (attempts <= state->total_players) {
                    log_event("Scheduler: Turn passed to Player %d", state->current_turn + 1);
                }
            }
            state->turn_done = 0;
            state->last_guess_correct = 0;
            int turn = state->current_turn;
            pthread_mutex_unlock(&state->state_mutex);
            
            sem_post(&state->turn_sem[turn]);
        } 
        else {
            pthread_mutex_unlock(&state->state_mutex);
        }
        usleep(100000); 
    }
    return NULL;
}

//child proccess logic

int check_connection(int sock) {
    char peek;
    int ret = recv(sock, &peek, 1, MSG_PEEK | MSG_DONTWAIT);
    if (ret == 0) return 0; 
    if (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) return 0;
    return 1; 
}

void game_start(int id) {
    int sock = state->client_socks[id];
    char buf[STR_LEN];
    int local_round = 1;

    if (send(sock, "Enter your name: ", 17, 0) < 0) exit(0);
    memset(buf, 0, STR_LEN);
    int n = read(sock, buf, STR_LEN-1);
    if (n <= 0) {
        state->player_active[id] = 0;
        return;
    }
    buf[strcspn(buf, "\r\n")] = 0;
    strncpy(state->player_names[id], buf, MAX_NAME_LEN-1);

    pthread_mutex_lock(&state->state_mutex);
    state->players_ready++;
    state->player_active[id] = 1;
    log_event("Player %d (%s) connected. (%d/%d)", id+1, state->player_names[id], state->players_ready, state->total_players);
    if (state->players_ready == state->total_players) {
        state->game_active = 1;
        log_event("All players ready. Game active. Idiom: %s", state->idiom);
    }
    pthread_mutex_unlock(&state->state_mutex);

    while (!state->game_active) usleep(100000);

    while (1) {
        while (state->game_over || state->round_number < local_round) {
            usleep(200000);
            if (state->round_number > local_round) {
                local_round = state->round_number;
            }
            if (!check_connection(sock)) {
                pthread_mutex_lock(&state->state_mutex);
                log_event("Player %d disconnected while waiting.", id+1);
                state->player_active[id] = 0;
                state->players_ready--;
                pthread_mutex_unlock(&state->state_mutex);
                exit(0);
            }
        }

        while (!state->game_over) {
            char wait_msg[64];
            sprintf(wait_msg, "\n[Round %d] Waiting for other players...\n", local_round);
            send(sock, wait_msg, strlen(wait_msg), 0); 

            int my_turn = 0;
            while (!my_turn && !state->game_over) {
                if (sem_trywait(&state->turn_sem[id]) == 0) {
                    my_turn = 1;
                } else {
                    if (!check_connection(sock)) {
                        pthread_mutex_lock(&state->state_mutex);
                        log_event("Player %d disconnected out of turn.", id+1);
                        state->player_active[id] = 0;
                        state->players_ready--;
                        pthread_mutex_unlock(&state->state_mutex);
                        exit(0);
                    }
                    usleep(100000); 
                }
            }
            
            if (state->game_over) break;

            flush_socket(sock);
            
            pthread_mutex_lock(&state->state_mutex);
            send_state(sock);
            pthread_mutex_unlock(&state->state_mutex);

            char l1 = 0, l2 = 0;
            int input_valid = 0;
            while (!input_valid) {
                char turn_msg[128];
                sprintf(turn_msg, "\n*** YOUR TURN (%s) ***\nChoose 2 letters (e.g., 'AB'): ", state->player_names[id]);
                if (send(sock, turn_msg, strlen(turn_msg), 0) < 0) {
                    pthread_mutex_lock(&state->state_mutex);
                    state->player_active[id] = 0; state->players_ready--; state->turn_done = 1; 
                    pthread_mutex_unlock(&state->state_mutex);
                    exit(0);
                }
                
                memset(buf, 0, STR_LEN);
                n = read(sock, buf, STR_LEN-1);
                if (n <= 0) { 
                    pthread_mutex_lock(&state->state_mutex);
                    log_event("Player %d disconnected during turn.", id+1);
                    state->player_active[id] = 0; state->players_ready--; state->turn_done = 1; 
                    pthread_mutex_unlock(&state->state_mutex);
                    exit(0); 
                } 
                buf[strcspn(buf, "\r\n")] = 0;
                int count = 0; char temp[3] = {0};
                for(int i=0; buf[i] && count < 2; i++) if(isalpha(buf[i])) temp[count++] = toupper(buf[i]);

                if (count < 2) { send(sock, "ERROR: Enter 2 letters.\n", 24, 0); continue; }
                
                pthread_mutex_lock(&state->state_mutex);
                int avail1 = is_char_available(state->char_selection_list, temp[0]);
                int avail2 = is_char_available(state->char_selection_list, temp[1]);
                if (temp[0] == temp[1]) avail2 = 0; 
                if (!avail1 || !avail2) { send(sock, "ERROR: Invalid letters.\n", 24, 0); pthread_mutex_unlock(&state->state_mutex); continue; }
                
                l1 = temp[0]; l2 = temp[1]; input_valid = 1;
                from_to_list(state->char_selection_list, state->char_selected_list, l1);
                from_to_list(state->char_selection_list, state->char_selected_list, l2);
                int idx = get_list_index_by_char(state->idiom_char_list, l1);
                if(idx >= 0) { to_format_idiom(state->idiom, state->formatted_idiom, l1); idiom_char_list_clear(state->idiom_char_list, idx); }
                idx = get_list_index_by_char(state->idiom_char_list, l2);
                if(idx >= 0) { to_format_idiom(state->idiom, state->formatted_idiom, l2); idiom_char_list_clear(state->idiom_char_list, idx); }
                log_event("Player %d chose %c, %c.", id+1, l1, l2);
                pthread_mutex_unlock(&state->state_mutex);
            }

            pthread_mutex_lock(&state->state_mutex);
            send_state(sock);
            
            if (state->idiom_char_list[0] == 0) {
                state->game_over = 1; 
                state->voting_pool_size = state->players_ready;
                state->votes_yes = 0;
                state->votes_no = 0;

                update_player_score(state->player_names[id], 1); 
                for(int k=0; k<state->total_players; k++) if(k != id) update_player_score(state->player_names[k], 0);
                save_scores();
                
                char win_msg[2048], score_board[1024];
                get_score_board_string(score_board);
                sprintf(win_msg, "\nGAME OVER! Idiom: %s\nWINNER: %s\n%s\n", state->idiom, state->player_names[id], score_board);
                for(int i=0; i<state->total_players; i++) send(state->client_socks[i], win_msg, strlen(win_msg), 0);
                log_event("Game Won by Player %d (Letters).", id+1);
                state->turn_done = 1;
                pthread_mutex_unlock(&state->state_mutex);
                break; 
            }

            send(sock, "Guess idiom (Enter to skip): ", 29, 0);
            pthread_mutex_unlock(&state->state_mutex);
            memset(buf, 0, STR_LEN);
            read(sock, buf, STR_LEN-1);
            buf[strcspn(buf, "\r\n")] = 0;

            pthread_mutex_lock(&state->state_mutex);
            if (strlen(buf) > 0) {
                log_event("Player %d guessed: %s", id+1, buf);
                if (strcasecmp(buf, state->idiom) == 0) {
                    state->game_over = 1;
                    state->voting_pool_size = state->players_ready;
                    state->votes_yes = 0;
                    state->votes_no = 0;

                    update_player_score(state->player_names[id], 1);
                    for(int k=0; k<state->total_players; k++) if(k != id) update_player_score(state->player_names[k], 0);
                    save_scores();

                    char win_msg[2048], score_board[1024];
                    get_score_board_string(score_board);
                    sprintf(win_msg, "\nWINNER! %s guessed it: %s\n%s\n", state->player_names[id], state->idiom, score_board);
                    for(int i=0; i<state->total_players; i++) send(state->client_socks[i], win_msg, strlen(win_msg), 0);
                    log_event("Game Won by Player %d (Guess).", id+1);
                    state->turn_done = 1;
                    pthread_mutex_unlock(&state->state_mutex);
                    break;
                } else {
                    send(sock, "Wrong guess!\n", 13, 0);
                }
            } else {
                 log_event("Player %d skipped guessing.", id+1);
            }
            state->turn_done = 1;
            pthread_mutex_unlock(&state->state_mutex);
        }

        usleep(250000); 
        flush_socket(sock); 

        char choice = 'n';
        char msg[] = "\n--- Round Over ---\nPlay again? (y/n): ";
        send(sock, msg, strlen(msg), 0); 
        
        memset(buf, 0, STR_LEN);
        n = read(sock, buf, STR_LEN-1);
        
        if (n > 0) choice = tolower(buf[0]);
        else choice = 'n';
        
        pthread_mutex_lock(&state->state_mutex);
        if (choice == 'y') {
            state->votes_yes++;
            log_event("Player %d voted YES.", id+1);
            send(sock, "Waiting for others...\n", 22, 0);
        } else {
            state->votes_no++;
            log_event("Player %d voted NO/Left.", id+1);
            state->player_active[id] = 0;
            state->players_ready--;
            send(sock, "Goodbye!\n", 9, 0);
            pthread_mutex_unlock(&state->state_mutex);
            exit(0); 
        }
        pthread_mutex_unlock(&state->state_mutex);
    }
    close(sock);
}

void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_sigint(int sig) {
    if (state) {
        save_scores();
        log_event("Server shutting down via signal.");
        munmap(state, sizeof(shared_state_t));
    }
    kill(0, SIGTERM); 
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGINT, handle_sigint); 
    signal(SIGCHLD, handle_sigchld);
    signal(SIGPIPE, SIG_IGN); 

    int num = 0;
    
    //check cmd line arg first
    if (argc > 1) {
        num = atoi(argv[1]);
    }

    //how many players prompt
    while (num < MIN_PLAYERS || num > MAX_PLAYERS) {
        printf("Enter number of players (%d-%d): ", MIN_PLAYERS, MAX_PLAYERS);
        if (scanf("%d", &num) != 1) {
            //clear buffer for invalid inputs
            while (getchar() != '\n');
            num = 0; //ensure loop continues
        }
    }
//one
    state = mmap(NULL, sizeof(shared_state_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&state->state_mutex, &attr);
    pthread_mutex_init(&state->score_mutex, &attr);
    pthread_mutex_init(&state->log_queue.mutex, &attr);
    
    state->total_players = num;
    state->turn_done = 1;
    state->round_number = 1;
    for(int i=0; i<MAX_PLAYERS; i++) sem_init(&state->turn_sem[i], 1, 0);
    
    sem_init(&state->log_queue.count_sem, 1, 0);            
    sem_init(&state->log_queue.space_sem, 1, LOG_QUEUE_SIZE); 

    srand(time(NULL));

    pthread_t sched_tid, logger_tid;
    pthread_create(&logger_tid, NULL, logger_thread_func, NULL);
    
    //bind logic for error check
    // two
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {AF_INET, htons(PORT), INADDR_ANY};
    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    listen(sfd, num);

    log_event("--- SERVER STARTED ---");
    log_event("Waiting for %d players...", num);
    
    load_random_idiom(state->idiom);
    
    
    for(int i=0; i<26; i++) state->char_selection_list[i] = i+1;
    for(int i=0; state->idiom[i]; i++) {
        state->formatted_idiom[i] = isalpha(state->idiom[i]) ? '_' : state->idiom[i];
    }
    state->formatted_idiom[strlen(state->idiom)] = '\0';
    idiom_char_list_init(state->idiom_char_list, state->idiom);
    
    load_scores();
    reveal_starting_letters();

    //three
    for (int i = 0; i < num; i++) {
        state->client_socks[i] = accept(sfd, NULL, NULL);
        log_event("Connection received for Player %d", i+1);
    }

    pthread_create(&sched_tid, NULL, scheduler_thread, NULL);

    //four
    for (int i = 0; i < num; i++) {
        if (fork() == 0) {
            game_start(i);
            exit(0);
        }
    }

    while (1) {
        sleep(10);
    }
    
    return 0;
}