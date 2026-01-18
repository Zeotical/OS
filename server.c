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

#include <ctype.h> //toupper
#include <time.h> //rand() amd seed()

#define PORT 5555
//#define MAX_PLAYERS 2
#define WORD "apple"

//Game global constraints
//const int MIN_PLAYERS = 3;
#define MAX_PLAYERS 2
#define STR_LEN 255
#define CHAR_SET_SIZE 26
const int INIT_RANDOM_SIZE = 4;
#define USER_INPUT_SIZE 2
const char CHAR_MAP[26] =   {
                                'A','B','C','D','E','F','G','H','I','J','K','L','M',
                                'N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
                            };
//SHARED GAME STATE
typedef struct {
    int current_turn;
    int total_players;
    int game_over;
    char revealed[16];
    int wrong;
    int turn_done;
    int last_guess_correct;
    char idiom[STR_LEN];
    char formatted_idiom[STR_LEN];
    int char_selection_list[CHAR_SET_SIZE];
    int char_selected_list[CHAR_SET_SIZE]; // [ E S X K .... '\0' ]
    int idiom_char_list[CHAR_SET_SIZE]; // [ H A P Y N E W R '\0' ]
    int client_socks[MAX_PLAYERS]; 
    pthread_mutex_t mutex;
    sem_t turn_sem[MAX_PLAYERS];
    sem_t logging_sem[MAX_PLAYERS];
} shared_state_t;

shared_state_t *state;

//GAME FUNCTIONS

int get_last_index(const int* list){
    for(int i=0;i < CHAR_SET_SIZE;i++){
        if(list[i] == 0){
            return i;
        }
    }
    return -1;
}

void idiom_char_list_init(int* idiom_char_list,const char* idiom){
    int size = strlen(idiom);
    for (int i = 0; i < size; i++) //loop through idiom
    {
        if(!isalpha(idiom[i])){
            continue;
        }

        int num = toupper(idiom[i]) - 'A' + 1;
        for (int j = 0; j < size; j++)
        {
            if(idiom_char_list[j] == 0){
                idiom_char_list[j] = num;
                break;
            } else if(idiom_char_list[j] == num){
                break;
            }
        }   
    }
}

void idiom_char_list_clear(int* idiom_char_list, int index){
    int size = get_last_index(idiom_char_list);

    
    for(int i = index; i < size; i++){
        int next = idiom_char_list[i+1];
        idiom_char_list[i] = next;
    }
}

int get_list_index_by_char(const int* list, char character){

    int size = get_last_index(list);
    int char_num = toupper(character) - 'A' + 1;

    for(int i = 0; i < size; i++){
        if(list[i] == char_num){
            return i;
        }
    }

    return -1;
}

void printBorder(int upper){
    if(upper){
        printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n\n");
    } else {
        printf("\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    }
}

//printOpening()

//printTitle()

int check_guessing(const char* idiom, const char* user_guess){
    for (int i = 0; i < strlen(idiom); i++)
    {
        if(toupper((unsigned char)idiom[i]) != toupper((unsigned char)user_guess[i])){
            return 0;
        }
    }

    printf("You win!\n");
    printf("idiom= %s\n", idiom);
    return 1;
    
}

void get_user_char_input(char* user_in) {

    //printf("Choose 2 char in SELECTION: ");
    int count = 0;

        for(int i = 0; i < 2; i++) {

            if (user_in[i] == '\0') {
            break; }

            if ((unsigned char)user_in[count]) {
            user_in[count] = toupper((unsigned char)user_in[count]);
            count++;
        }
        }
        // user_in[count] = '\0'; 
}


int from_to_list(int* from_list,int* to_list, char character){

    int index = toupper(character) - 'A';
    int char_num = index + 1;

    if(from_list[index] == -1){
        return 0;
    } else {
        from_list[index] = -1;
        to_list[get_last_index(to_list)] = char_num;
        return 1;
    }
}

void to_format_idiom(const char* idiom, char* idiom2, char character){

    int size = strlen(idiom2);

    for (int i = 0; i < size; i++)
    {
        if(idiom2[i] == '_' && toupper(idiom[i]) == character){
            idiom2[i] = character;
        }
    }

}

void formatted_idiom_init(const char* idiom, char* to_format_idiom){
    int len = strlen(idiom);

    for (int i = 0; i < len; i++)
    {

        if(isalpha(idiom[i])){
            to_format_idiom[i] = '_';
        } else {
            to_format_idiom[i] = idiom[i];
        }
    }

    to_format_idiom[len] = '\0';
}

int get_random_num(){
    int r = rand() % CHAR_SET_SIZE;     // random number
    return r;
}

void start_alph_selection(int* from_list,int* to_list, int random_size){
    int count = random_size;

    // loop till all selection finished
    while(count > 0 ){
        int i = get_random_num();

        if(from_list[i] == 0){
            continue;
        }

        from_list[i] = -1;
        to_list[get_last_index(to_list)] = i+1;

        count--;
    }
}

void print_list_with_alph(const int* list, char* alphList ){
    //printf("[ ");
    strcat(alphList, "[");
    for(int i =0; i < CHAR_SET_SIZE; i++){

        // if list[i] is 0, end of list
        if(list[i] == 0){
            break;
        }

        if(list[i] == -1){ // if list is -1, no character
           // printf("_ ");
            strcat(alphList, "_ ");

        } else { // else print character
          // printf("%c ",CHAR_MAP[list[i]-1]); 
           sprintf(alphList + strlen(alphList), "%c ", CHAR_MAP[list[i]-1]);

        }

    }
    //printf("]\n");
    strcat(alphList, "]");
}

void print_list_with_number(const int* list){
    printf("[ ");
    for(int i =0; i < CHAR_SET_SIZE; i++){
        printf("%d",list[i]);
        if(i != CHAR_SET_SIZE-1){
            printf(" ");
        }
    }
    printf(" ]\n");
}

void char_selection_list_init(int* list){
    for(int i =0; i< CHAR_SET_SIZE; i ++){
        list[i] = i+1;
    }
}

// count how many lines in the file
int count_lines(FILE *file) {
    int count = 0;
    int c;

    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') count++;
    }
    rewind(file); // go back to beginning
    return count;
}

// pick a random idiom from file and store in idiom
void get_random_idiom(const char* filename, char* idiom, int buffer_size) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Could not open file: %s\n", filename);
        idiom[0] = '\0';
        return;
    }

    int total_lines = count_lines(file);

    int random_line = rand() % total_lines;

    char line[STR_LEN];
    int current_line = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (current_line == random_line) {
            // remove trailing newline
            line[strcspn(line, "\n")] = '\0';
            strncpy(idiom, line, buffer_size - 1);
            idiom[buffer_size - 1] = '\0'; // ensure null-terminated
            break;
        }
        current_line++;
    }

    fclose(file);
}

void clear_input_buffer(){
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

//Broadcast game,handle clients, scheduler and logging thread
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

void *logging_thread(void *arg) {
    //wait for players to be ready
    // sleep(1); 
    FILE *fptr;
    char msg[128];
    char player_no[128];
    //Create the logging file
    fptr = fopen("game.log", "w");
    while (!state->game_over) {
        sem_wait(state->logging_sem); // wait until player finishes entering their choice
        sprintf(player_no, "\n---- Player no: %d's Move ----\n",state->current_turn+1);
        fprintf(fptr, player_no);
        sprintf(msg, "\n--- Update ---\nWord: %s\nWrong: %d\n--------------\n", state->revealed, state->wrong);
        fprintf(fptr, msg);
    }
    return NULL;
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
        sem_post(state->logging_sem); // signal to logger that player finished
        pthread_mutex_unlock(&state->mutex);
    }

    send(sock, "\nGAME OVER! The word was apple.\n", 32, 0);
    close(sock);
}

void game_init(){
    srand(time(NULL)); // seed the random generator (do this ONCE)
    char infile_name[] = "idioms.txt";
    //Get random idiom before game starts
    get_random_idiom(infile_name, state->idiom, sizeof(state->idiom));
//TO DO print upper border after idiom

// declarations
    // state->formatted_idiom[STR_LEN] = ""; // Idiom = "_____ ___ ____!"
    // state->char_selection_list[CHAR_SET_SIZE] = {0}; // [ A B C D _ F G H I J _ L M N O P Q R _ T U V W _ Y Z ]
    // state->char_selected_list[CHAR_SET_SIZE] = {0}; // [ E S X K .... '\0' ]
    // state->idiom_char_list[CHAR_SET_SIZE] = {0}; // [ H A P Y N E W R '\0' ]

    char_selection_list_init(state->char_selection_list); // [ A B C D E F ... ]
    formatted_idiom_init(state->idiom, state->formatted_idiom); // Idiom = "_____ ___ ____!"
    idiom_char_list_init(state->idiom_char_list,state->idiom); // "Happy New Year!" = [ H A P Y N E W R ];

    int count = 0;
    // char buf[64];
    // char alph_list[128];
    // char format_idiom[128];
    // alph_list[0] = '\0';
    while(count < INIT_RANDOM_SIZE)
    {
        // randomly select a character
        char rand_char = CHAR_MAP[get_random_num()];
        // check if successfully added to the char_selected_list
        int added = from_to_list(state->char_selection_list,state->char_selected_list,rand_char);

        if(!added){  //failed to add because of duplicates
            continue;
        }

        // settle formatted_idiom and idiom_char_list
        int idiom_char_index = get_list_index_by_char(state->idiom_char_list,rand_char); 
        if(idiom_char_index >= 0){ // -1 = cant find, >= 0 is list's index
            to_format_idiom(state->idiom,state->formatted_idiom, rand_char);
            idiom_char_list_clear(state->idiom_char_list, idiom_char_index);
        }

        count++;
    }
}


void game_start(int player_id){

    int count = 0;
    int sock = state->client_socks[player_id];
    char buf[64];
    char alph_list[128];
    char format_idiom[128];
    char end_game_msg[128];
    alph_list[0] = '\0';

    // print initialization info
    //TODO print for all
    //Print Initiation
    char *init = "*** Initiation ***\n\n";
    send(sock, init, strlen(init), 0);
    //memset(alph_list, 0, sizeof());

    //PRINT SELECTION
    char *selection = "SELECTION: ";
    send(sock, selection, strlen(selection), 0);
    memset(alph_list, 0, sizeof(alph_list));
    print_list_with_alph(state->char_selection_list, alph_list);
    send(sock, alph_list, strlen(alph_list), 0);

    //Print SELECTED
    char *selected = "\nSELECT(ED): ";
    send(sock, selected, strlen(selected), 0);
    memset(alph_list, 0, sizeof(alph_list));
    print_list_with_alph(state->char_selected_list, alph_list);
    send(sock, alph_list, strlen(alph_list), 0);

    //PRINT IDIOM
    sprintf(format_idiom, "\nIdiom: %s\n",state->formatted_idiom);
    send(sock, format_idiom, strlen(format_idiom), 0);
    
    //TO DO print lower border after idiom
    
    // WINNING CONDITION 1 = user guess the idiom
    // winning condition 2 = all idiom's characters been selected.
    // User starts here

    while(!state->game_over){

        //TO DO print upper border after idiom

        char user_char_input[USER_INPUT_SIZE] = {0};
        char user_guess_input[STR_LEN] = {0};
        
        // get user char input
        sem_wait(&state->turn_sem[player_id]);
            if (state->game_over) break;

            //active player prompt
            char *prompt = "\n*** YOUR TURN ***\nChoose 2 characters from SELECTION: ";
            send(sock, prompt, strlen(prompt), 0);
            memset(buf, 0, sizeof(buf));
            int n = read(sock, buf, sizeof(buf));
            if (n <= 0) break;
            
            memcpy(user_char_input, buf, n);  //user_char_input = what the user inputted in buffer (expected 2 chars)
            user_char_input[n] = '\0';  //memcpy does not null terminate
            get_user_char_input(user_char_input);
            pthread_mutex_lock(&state->mutex);

        // list processing && format idiom
        for(int i=0; i< USER_INPUT_SIZE;i++){
            if(user_char_input[i] == 0){
                break;
            }
            from_to_list(state->char_selection_list,state->char_selected_list,user_char_input[i]);

            // settle formatted_idiom and idiom_char_list
            int idiom_char_index = get_list_index_by_char(state->idiom_char_list,user_char_input[i]); 
            if(idiom_char_index >= 0){ // -1 = cant find, >= 0 is list's index
                to_format_idiom(state->idiom,state->formatted_idiom, user_char_input[i]);
                idiom_char_list_clear(state->idiom_char_list, idiom_char_index);
            }
        }

        // print initialization info
        char *loop = "*** In While Loop ***\n\n";
        send(sock, loop, strlen(loop), 0);

        //PRINT SELECTION
        char *selection = "SELECTION: ";
        send(sock, selection, strlen(selection), 0);
        memset(alph_list, 0, sizeof(alph_list));
        print_list_with_alph(state->char_selection_list, alph_list);
        send(sock, alph_list, strlen(alph_list), 0);

        //PRINT SELECTED
        char *selected = "\nSELECT(ED): ";
        send(sock, selected, strlen(selected), 0);
        memset(alph_list, 0, sizeof(alph_list));
        print_list_with_alph(state->char_selected_list, alph_list);

        //PRINT IDIOM
        send(sock, alph_list, strlen(alph_list), 0);
        sprintf(format_idiom, "\nIdiom: %s\n",state->formatted_idiom);
        send(sock, format_idiom, strlen(format_idiom), 0);
        

        if(state->idiom_char_list[0] == 0){
            break;
        }

        //prompt of get user guess
        char *guess_prompt = "Make a guess: \n";
        send(sock, guess_prompt, strlen(guess_prompt), 0);
        memset(buf, 0, sizeof(buf));
        int l = read(sock, buf, sizeof(buf));
        if (l <= 0) break;
        memcpy(user_guess_input, buf, l);  //user_char_input = what the user inputted in buffer (expected 2 chars)
        user_guess_input[l] = '\0';  //memcpy does not null terminate
        // remove newline
        user_guess_input[strcspn(user_guess_input, "\n")] = '\0';
        printf("DEBUG: Comparing Guess [%s] against Idiom [%s]\n", user_guess_input, state->idiom);
        int check_guess = check_guessing(state->idiom, user_guess_input);
        if(check_guess == 1){
            state->game_over = 1;
             user_char_input[0] = 0;
        user_char_input[1] = 0;
        user_guess_input[0] = '\0';

        //TO DO lower border

        state->turn_done = 1;          
        sem_post(state->logging_sem); // signal to logger that player finished
        pthread_mutex_unlock(&state->mutex);
            break;
        }
       
        user_char_input[0] = 0;
        user_char_input[1] = 0;
        user_guess_input[0] = '\0';

        //TO DO lower border

        state->turn_done = 1;          
        sem_post(state->logging_sem); // signal to logger that player finished
        pthread_mutex_unlock(&state->mutex);
    }
    //TO DO print WINNING info
    // printBorder(1);
    // printTitle();
    // printf("\n\n");
    // printf("*** Game Finish ***\n\n");
    // printf("Hidden Idiom: %s\n",idiom); // (temp)
    // printBorder(0);
    //printBorder(1);
    sprintf(end_game_msg, "\nGame Over! You Win! The Idiom was: %s\n",state->idiom);
   // printBorder(0);
    send(sock, end_game_msg, strlen(end_game_msg), 0);
    close(sock);
    // (END) GAME SYSTEM
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
        sem_init(state->logging_sem, 1, 0);

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
        printf("Player %d joined (Socket: %d)\n", i+1, state->client_socks[i]);
    }

    //start
    game_init();
    pthread_t sched;
    pthread_t logger;
    pthread_create(&sched, NULL, scheduler_thread, NULL);
    pthread_create(&logger, NULL, logging_thread, NULL);

    //fork
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (fork() == 0) {
           // handle_client(i);
            game_start(i);
            exit(0);
        }
    }

    //wait for players
    while(wait(NULL) > 0);
    printf("Game finished. Server shutting down.\n");
    return 0;
}