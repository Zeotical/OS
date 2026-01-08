
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>

#include<stdbool.h> // header file to use bool variables in C. else would use 1 or ;;.

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#define PORT 8080

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

valread = read(sock, buffer,1024 - 1); 
printf("%s\n", buffer);
char myNum[100];

// Ask the user to type a number
printf("Type a letter: \n");

// Get and save the number the user types
scanf("%s", &myNum);

send(sock, myNum, strlen(myNum), 0);

// Cleanup: Close the message queue descriptor
close(sock);
return 0;
}
