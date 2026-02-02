CC = gcc
CFLAGS=-pthread

all: server client
server:
	$(CC) $(CFLAGS) server.c -o server
client:
	$(CC) client.c -o client
clean:
	rm -rf *o server client