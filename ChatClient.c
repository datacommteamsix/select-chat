#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	255		//Buffer length
#define TRUE	1
#define LISTENQ	5
#define MAXLINE 4096

int main (int argc, char **argv)
{
	char *data;
	int port;
	data = (char*) malloc(sizeof(char) * strlen(argv[1]+1));

	switch(argc)
	{
		case 2:
			port = SERVER_TCP_PORT;	// Use the default port
			strcpy(data, argv[1]);
			fprintf(stderr, "data is %s" ,data);
		break;
		case 3:
			port = atoi(argv[2]);	// Get user specified port
			strcpy(data, argv[1]);
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}
}