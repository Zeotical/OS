
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
//#include <cstring>
//#include <cstdlib>
//using namespace std;
const long MAX_MSG_SIZE = 256; // Must match sender's size
// Define the data structure for the message (must be identical)
struct Message {
int counter;
char buffer[252];
};
int main(int argc, char* argv[]) {
if (argc != 2) {
    char* c = argv[0];
fprintf(stderr ,"Usage: %c <MQ_NAME>" , *c);
return 1;
}
const char* MQ_NAME = argv[1];
mqd_t mqd;
struct Message msg;
// 1. Open the existing message queue (read-only)
// Note: We don't need the attributes struct here, as we are not creating it.
mqd = mq_open(MQ_NAME, O_RDONLY);
if (mqd == (mqd_t)-1) {
perror("[RECEIVER] mq_open failed");
return 1;
}
printf ("[RECEIVER] Child Process (PID: " );
//int pid = getpid();
printf( "%c" , getpid()) ;
printf ( ") connected to message queue." );
// 2. Continuously receive messages until termination signal (-1)
while (1) {
// Receive the message. It blocks until a message is available.
ssize_t bytes_read = mq_receive(mqd, (char*)&msg, MAX_MSG_SIZE, NULL);
if (bytes_read == -1) {
perror("[RECEIVER] mq_receive failed");
break;
}
if (msg.counter == -1) {
printf ("[RECEIVER] Received termination signal. Exiting." ) ;
break;
}
printf( "[RECEIVER] Received Counter: " );
// int msg_counter = msg.counter;
printf ( "%c", msg.counter) ;
printf(" | Message: ");
printf (msg.buffer );
}
// Cleanup: Close the message queue descriptor
mq_close(mqd);
return 0;
}
