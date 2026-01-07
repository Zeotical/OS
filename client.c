
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>

#include<stdbool.h> // header file to use bool variables in C. else would use 1 or ;;.

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#define PORT 8080

//const long MAX_MSG_SIZE = 256; // Must match sender's size
// Define the data structure for the message (must be identical)
struct Message {
int counter;
char buffer[252];
};
int main(int argc, char* argv[]) {

// Socket Code provided by Mr Sharaf
// TODO is server listening to connections? fork child for each connection
int sock = 0;
struct sockaddr_in serv_addr;
//ClientState state;
//char buffer [256] = {0};
    int status, valread, client;
    
char buffer[1024] = { 0 };

if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
printf("Socket creation error\n");
return -1; 
}
serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons (PORT);
if (inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
printf("Invalid address\n");
return -1; 
}
if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
printf("Connection failed\n");
return -1;
}
  printf("Connection to server established\n");  
  

// if (argc != 2) {
// fprintf(stderr ,"Usage: %s <MQ_NAME>" , argv[0]);
// return 1;
// }

valread = read(sock, buffer,1024 - 1); 
printf("%s\n", buffer);

// const char* MQ_NAME = argv[1];
// mqd_t mqd;
// struct Message msg;
// 1. Open the existing message queue (read-only)
// Note: We don't need the attributes struct here, as we are not creating it.
// mqd = mq_open(MQ_NAME, O_RDONLY);
// if (mqd == (mqd_t)-1) {
// perror("\n[RECEIVER] mq_open failed");
// return 1;
// }
// printf ("\n[RECEIVER] Child Process (PID: %d) connected to message queue." , getpid()) ;

// // 2. Continuously receive messages until termination signal (-1)
// while (true) {
// // Receive the message. It blocks until a message is available.
// ssize_t bytes_read = mq_receive(mqd, (char*)&msg, MAX_MSG_SIZE, NULL);
// if (bytes_read == -1) {
// perror("\n[RECEIVER] mq_receive failed");
// break;
// }

// if (msg.counter == -1) {
// printf ("\n[RECEIVER] Received termination signal. Exiting." ) ;
// break;
// }
// printf( "\n[RECEIVER] Received Counter: %d  | Message: %s", msg.counter, msg.buffer);
// }

// Cleanup: Close the message queue descriptor
close(client);
//mq_close(mqd);
// close(client);

return 0;
}
