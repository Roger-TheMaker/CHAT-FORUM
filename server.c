#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXIMUM_NUMBER_OF_CLIENTS 100
#define BUFFER_SIZE 2048

/* The Client structure */
typedef struct{
	struct sockaddr_in address;
	int socketFileDescriptor;
	int userId;
	char userName[40];
}clientType;

clientType *clients[MAXIMUM_NUMBER_OF_CLIENTS];

int main(int argc, char **argv){



}
