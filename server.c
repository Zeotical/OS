#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
// #include <cstring>
// #include <cstdlib>
//using namespace std;

#include <string.h>
#include <stdlib.h>
//TODO 
#define PORT 8080; // Mr sharaf
typedef struct {
int client_sockets[2]; // num of players ig
} GameState;
////////////////

// Define constants for the message queue
const char* MQ_NAME = "/my_ipc_mq";
const long MAX_MSG_SIZE = 256;
const long MAX_MESSAGES = 10;
const char* MESSAGE_PREFIX = "Data cycle: ";
// Define the data structure for the message (must be identical in sender and receiver)
struct Message {
int counter;
char buffer[252]; // Max size is 256, leaving 4 bytes for the int
};
void run_sender(mqd_t mqd, pid_t receiver_pid) {
printf("\n[SENDER] Parent Process (PID: %d) started.",  getpid());
printf("\n[SENDER] Sending 3 messages to Child (PID: %d) via Queue...", receiver_pid);
struct Message msg;
for (int i = 1; i <= 3; ++i) {
// 1. Prepare the message
msg.counter = i;
char str[20]; // Buffer to hold the string
sprintf(str, "%d", i); // Converts int to string
// strcat(MESSAGE_PREFIX, str) ;
// char full_msg_str[] = {MESSAGE_PREFIX};
// strncpy(msg.buffer, full_msg_str, sizeof(msg.buffer) - 1);
//char iTostr = i;
const char* full_msg_str = MESSAGE_PREFIX ;
strncpy(msg.buffer, full_msg_str, sizeof(msg.buffer) - 1);
strncpy(msg.buffer+ strlen(msg.buffer), str, strlen(str)+1);

msg.buffer[sizeof(msg.buffer) - 1] = '\0';
// 2. Send the message
if (mq_send(mqd, (const char*)&msg, sizeof(msg), 0) == -1) {
perror("n[SENDER] mq_send failed");
} else {
printf ("\n[SENDER] Sent message %d. Pausing...", i);
}
sleep(2);
}
// 3. Send a termination signal message
msg.counter = -1;
if (mq_send(mqd, (const char*)&msg, sizeof(msg), 0) == -1) {
perror("\n[SENDER] mq_send failed for termination signal");
} else {
printf("\n[SENDER] Sent termination signal (-1). Waiting for receiver...");
//TO DO fix sync problem here
}
}
int main() {
mqd_t mqd; // Message Queue Descriptor
// Attributes structure for the message queue
struct mq_attr attr;
attr.mq_flags = 0;
attr.mq_maxmsg = MAX_MESSAGES;
attr.mq_msgsize = MAX_MSG_SIZE;
attr.mq_curmsgs = 0; // Current number of messages
// 1. Create and open the message queue
mqd = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
if (mqd == (mqd_t)-1) {
perror("mq_open failed");
return 1;
}
// --- Fork the Child (Receiver) Process ---
pid_t pid = fork();
if (pid < 0) {
perror("fork failed");
mq_close(mqd);
mq_unlink(MQ_NAME);
return 1;
} else if (pid == 0) {
// Child Process: Execute the Receiver Program
execl("./client", "client", MQ_NAME, NULL);
// execl only returns if an error occurred
perror("execl failed");
mq_close(mqd);
mq_unlink(MQ_NAME);
exit(EXIT_FAILURE);
} else {
// Parent Process: Run the Sender logic
run_sender(mqd, pid);
// Wait for the child to finish
wait(NULL);
// Cleanup: Close and unlink the message queue
mq_close(mqd);
mq_unlink(MQ_NAME);
printf("\n[SENDER] Parent finished and cleaned up message queue.");
}
return 0;
}