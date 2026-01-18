#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <time.h> //rand() amd seed()

const int MIN_PLAYERS = 3;
const int MAX_PLAYERS = 5;
// const int STR_LEN = 255;
#define STR_LEN 255
// const int CHAR_SET_SIZE = 26;
#define CHAR_SET_SIZE 26
const int INIT_RANDOM_SIZE = 4;
// const int USER_INPUT_SIZE = 2;
#define USER_INPUT_SIZE 2
const char CHAR_MAP[26] =   {
                                'A','B','C','D','E','F','G','H','I','J','K','L','M',
                                'N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
                            };

typedef struct {
    char name[STR_LEN];
} Player;


// prototype (typing script)
void printOpening();
void printTitle();
void printBorder(int upper);
// prototypes (player system)
int get_player_count(int min, int max);
void get_player_name(Player players[], int player_count);
void clear_input_buffer();
//prototypes (random idiom system)
int count_lines(FILE* file);
void get_random_idiom(const char* filename, char* idiom, int buffer_size);
//prototypes (game system)
void char_selection_list_init(int* list);
void idiom_char_list_init(int* idiom_char_list,const char* idiom);
void idiom_char_list_clear(int* idiom_char_list, int index);
int get_list_index_by_char(const int* idiom_char_list, char character);
void print_list_with_number(const int* list); // use to testing
void print_list_with_alph(const int* list);
void start_alph_selection(int* from_list,int* to_list, int random_size);
int get_random_num();
int get_last_index(const int* list);
void formatted_idiom_init(const char* idiom, char* to_format_idiom);
void to_format_idiom(const char* idiom, char* formatted_idiom, char character);
int from_to_list(int* from_list,int* to_list, char character);
void get_user_char_input(char* user_in);
int check_guessing(const char* idiom, const char* user_guess);


int main(){

    srand(time(NULL)); // seed the random generator (do this ONCE)
    printOpening();

    // /*
    // (START) PLAYER SYSTEM
    // */

    // printBorder();
    // printf("\n");

    // //insert how many players (3-5)
    // int player_count;
    // player_count = get_player_count(MIN_PLAYERS, MAX_PLAYERS);

    // //get player names
    // Player players[player_count];
    // get_player_name(players, player_count);


    // printf("\n");
    // printBorder();

    // // (END) PLAYER SYSTEM


    /*
    (START) RANDOM IDIOM SYSTEM
    */

    char idiom[STR_LEN];
    char infile_name[] = "idioms.txt";
    get_random_idiom(infile_name, idiom, sizeof(idiom));

    // (END) RANDOM IDIOM SYSTEM


    /*
    (START) GAME SYSTEM
    */

    printBorder(1);
    //// (TEMP) testing Idiom = "Happy New Year!"
    // char idiom[STR_LEN] = "Happy New Year!"; 
    char formatted_idiom[STR_LEN] = ""; // Idiom = "_____ ___ ____!"
    int char_selection_list[CHAR_SET_SIZE] = {0}; // [ A B C D _ F G H I J _ L M N O P Q R _ T U V W _ Y Z ]
    int char_selected_list[CHAR_SET_SIZE] = {0}; // [ E S X K .... '\0' ]
    int idiom_char_list[CHAR_SET_SIZE] = {0}; // [ H A P Y N E W R '\0' ]

    char_selection_list_init(char_selection_list); // [ A B C D E F ... ]
    formatted_idiom_init(idiom, formatted_idiom); // Idiom = "_____ ___ ____!"
    idiom_char_list_init(idiom_char_list,idiom); // "Happy New Year!" = [ H A P Y N E W R ];

    // select 4 characters randomly
    int count = 0;
    while(count < INIT_RANDOM_SIZE)
    {
        // randomly select a character
        char rand_char = CHAR_MAP[get_random_num()];
        // check if successfully added to the char_selected_list
        int added = from_to_list(char_selection_list,char_selected_list,rand_char);

        if(!added){  //failed to add because of duplicates
            continue;
        }

        // settle formatted_idiom and idiom_char_list
        int idiom_char_index = get_list_index_by_char(idiom_char_list,rand_char); 
        if(idiom_char_index >= 0){ // -1 = cant find, >= 0 is list's index
            to_format_idiom(idiom,formatted_idiom, rand_char);
            idiom_char_list_clear(idiom_char_list, idiom_char_index);
        }

        count++;
    }
    
    // print initialization info
    printf("*** Initiation ***\n\n");
    // printf("Hidden Idiom: %s\n",idiom); // (temp)
    printf("SELECTION: ");
    print_list_with_alph(char_selection_list);
    printf("SELECT(ED): ");
    print_list_with_alph(char_selected_list);
    printf("Idiom: %s\n",formatted_idiom);
    // printf("Idiom_char_list: ");
    // print_list_with_alph(idiom_char_list);
    printBorder(0);
    
    // WINNING CONDITION 1 = user guess the idiom
    // winning condition 2 = all idiom's characters been selected.
    // User starts here
    while(1){

        printBorder(1);
        
        char user_char_input[USER_INPUT_SIZE] = {0};
        char user_guess_input[STR_LEN];
        
        // get user char input
        get_user_char_input(user_char_input);

        // list processing && format idiom
        for(int i=0; i< USER_INPUT_SIZE;i++){

            if(user_char_input[i] == 0){
                break;
            }
            from_to_list(char_selection_list,char_selected_list,user_char_input[i]);


            // settle formatted_idiom and idiom_char_list
            int idiom_char_index = get_list_index_by_char(idiom_char_list,user_char_input[i]); 
            if(idiom_char_index >= 0){ // -1 = cant find, >= 0 is list's index
                to_format_idiom(idiom,formatted_idiom, user_char_input[i]);
                idiom_char_list_clear(idiom_char_list, idiom_char_index);
            }
        }

        // print initialization info
        printf("*** In While Loop ***\n\n");
        // printf("Hidden Idiom: %s\n",idiom); // (temp)
        printf("SELECTION: ");
        print_list_with_alph(char_selection_list);
        printf("SELECT(ED): ");
        print_list_with_alph(char_selected_list);
        printf("Idiom: %s\n",formatted_idiom);
        // printf("Idiom_char_list: ");
        // print_list_with_alph(idiom_char_list);

        if(idiom_char_list[0] == 0){
            break;
        }

        //prompt of get user guess
        printf("Make the guess: ");
        fgets(user_guess_input, sizeof(user_guess_input),stdin);
        // remove newline
        user_guess_input[strcspn(user_guess_input, "\n")] = '\0';
        

        int check_guess = check_guessing(idiom, user_guess_input);
        if(check_guess == 1){
            break;
        }

        user_char_input[0] = 0;
        user_char_input[1] = 0;
        user_guess_input[0] = '\0';

        printBorder(0);
    }

    // print WINNING info
    printBorder(1);
    printTitle();
    printf("\n\n");
    printf("*** Game Finish ***\n\n");
    printf("Hidden Idiom: %s\n",idiom); // (temp)
    printBorder(0);

    // (END) GAME SYSTEM

    return 0;
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

void printOpening(){
    printBorder(1);
    printf("\n");
    printf("                               ▐▌    ▐▌ █▐▌▌ █    █▐▌▌   █▐    ▐▌ ▐▌▌  █▐▌▌  ▐▌    \n");
    printf("                               ▐▌ █  ▐▌ █▗▖  █    █    █    █ ▐▌ █  ▐▌ █▗▖   ▐▌    \n");
    printf("                               ▐▌ █  ▐▌ █▝▘  █    █    █    █ ▐▌ █  ▐▌ █▝▘   ▝▘    \n");
    printf("                                ▐▌ ▐▌▌  █▐▌▌ █▐▌▌ █▐▌▌   █▐   ▐▌    ▐▌ █▐▌▌  ▗▖    \n");
    printTitle();
    printf("\n");
    printBorder(0);
}

void printTitle(){
    printf("  ▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂\n");
    printf(" ▞ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ▚\n");
    printf("▞ *                                                                                                         * ▚\n");
    printf("▌*   ▐▌    ▐▌ █  █ █▐▌▌ █▐▌▌ █         ▐▐███▌▌ █  █ █▐▌▌      █▐▌▌   █▐   █▐▌▌  ▐▐███▌▌ █  ▐▌▌ ▐▌  ▐▌ █▐▌▌   * ▌\n");
    printf("▌*   ▐▌ █  ▐▌ █▗▖█ █▗▖  █▗▖  █            ▄    █▗▖█ █▗▖       █▗▖  █    █ █   █    ▄    █  ▐▌▌ ▐▌█ ▐▌ █▗▖    * ▌\n");
    printf("▌*   ▐▌ █  ▐▌ █▝▘█ █▝▘  █▝▘  █            █    █▝▘█ █▝▘       █▝▘  █    █ █▐▌▌     █    █  ▐▌▌ ▐▌ █▐▌ █▝▘    * ▌\n");
    printf("▌*    ▐▌ ▐▌▌  █  █ █▐▌▌ █▐▌▌ █▐▌▌         █    █  █ █▐▌▌      █      █▐   █   █    █     ███   ▐▌  ▐▌ █▐▌▌   * ▌\n");
    printf("▚ *                                                                                                         * ▞                                        \n");
    printf(" ▚ * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ▞\n");
    printf("  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\n");
}

int check_guessing(const char* idiom, const char* user_guess){
    for (int i = 0; i < strlen(idiom); i++)
    {
        if(toupper(idiom[i]) != toupper(user_guess[i])){
            return 0;
        }
    }

    printf("You win!\n");
    printf("idiom= %s\n", idiom);
    return 1;
    
}

void get_user_char_input(char* user_in) {

    printf("Choose 2 char in SELECTION: ");

    int c;
    int count = 0;

    while ((c = getchar()) != '\n' && isalpha(c)) {
        if (count < 2) {
            user_in[count] = toupper((unsigned char)c);
            count++;
        } else {
            break;
        }
    }
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

int get_last_index(const int* list){
    for(int i=0;i < CHAR_SET_SIZE;i++){
        if(list[i] == 0){
            return i;
        }
    }
    return -1;
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

void print_list_with_alph(const int* list){
    printf("[ ");
    for(int i =0; i < CHAR_SET_SIZE; i++){

        // if list[i] is 0, end of list
        if(list[i] == 0){
            break;
        }

        if(list[i] == -1){ // if list is -1, no character
            printf("_ ");
        } else { // else print character
           printf("%c ",CHAR_MAP[list[i]-1]); 
        }

    }
    printf("]\n");
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

// get players' name
void get_player_name(Player players[], int player_count){
    for(int i=0; i<player_count; i++){
        printf("Key in player %d's name: ", i+1);
        fgets(players[i].name,sizeof(players[i].name),stdin);
        players[i].name[strcspn(players[i].name, "\n")] = '\0';
    }
}

// get player counts
int get_player_count(int min, int max){
    int count;

    while (1) {
        printf("How many players? (%d-%d): ", min, max);

        if (scanf("%d", &count) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }

        if (count >= min && count <= max) {
            break;
        }

        printf("Number out of range. Try again.\n");
        clear_input_buffer();
    }

    clear_input_buffer();
    return count;

}