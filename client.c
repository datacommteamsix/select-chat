#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/select.h>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	255		//Buffer length
#define TRUE	1
#define LISTENQ	5
#define MAXLINE 4096

pthread_t clientThread;
void *client_recv(void *ptr);
int client_send();
void sendPacket(char * sendbuf);

static void SystemFatal(const char* message);
int clientSocket;

int main (int argc, char **argv)
{
	char *data;
	int port;
	struct sockaddr_in clientInetAddress;
	data = (char*) malloc(sizeof(char) * strlen(argv[1]+1));

	switch(argc)
	{
		case 2:
			port = SERVER_TCP_PORT;	// Use the default port
			strcpy(data, argv[1]);
		break;
		case 3:
			port = atoi(argv[2]);	// Get user specified port
			strcpy(data, argv[1]);
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	// Create a stream socket
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		SystemFatal("Cannot Create Socket!");

	struct hostent *Hostent;
	if ((Hostent = gethostbyname(data)) == NULL)
		SystemFatal("Bad Host Name.");

	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
    int arg = 1;
    if (setsockopt (clientSocket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
            SystemFatal("setsockopt");

	memset(&clientInetAddress, 0, sizeof(struct sockaddr_in));
	clientInetAddress.sin_family = AF_INET;
	clientInetAddress.sin_port = htons(port);
	clientInetAddress.sin_addr.s_addr = inet_addr(data);

	// Connecting to the server
	if (connect (clientSocket, (struct sockaddr *)&clientInetAddress, sizeof(clientInetAddress)) == -1)
		SystemFatal("Failed to connect to server");

	fcntl(clientSocket, F_SETFL, O_NONBLOCK);
	fprintf(stderr, "Success!\n");

	//pthread_create(&clientThread, NULL, &client_recv, (void *)NULL);

	while(client_send(clientSocket) == 1);
	//clientThread.join();
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}

void *client_recv(void *ptr) {
	char recvbuf[BUFLEN];
	int	r;

	for (;;) {
		if ((r = recv(clientSocket, recvbuf, BUFLEN, 0)) == -1) {
			perror("recv call() failed");
			continue;
		}
		recvbuf[r] = '\n';
		recvbuf[r+1] = '\0';
		fprintf(stderr, "%s\n", &recvbuf);;
	}
}

int client_send()
{
	char *buffer;
    size_t bufsize = 32;
    size_t characters;

    buffer = (char *)malloc(bufsize * sizeof(char));
    if( buffer == NULL)
    {
        perror("Unable to allocate buffer");
        exit(1);
    }

    printf("Me: ");
    characters = getline(&buffer,&bufsize,stdin);

    if ((buffer)[characters - 1] == '\n') 
  	{
      (buffer)[characters - 1] = '\0';
      --characters;
  	}
    //printf("%zu characters were read.\n",characters);
    printf("You typed: '%s'\n",buffer);
    sendPacket(buffer);

    char recvbuf[BUFLEN];
	int	r;
	if ((r = recv(clientSocket, recvbuf, BUFLEN, 0)) == -1) {
		perror("recv call() failed");
	}
	fprintf(stderr, "%s\n", &recvbuf);;

    return 1;
}

void sendPacket(char * sendbuf) {
	int r;
	char  text_out[BUFLEN];
	char host[128];

	gethostname(host, 128);

	sprintf(text_out, "%s > %s", host, sendbuf);

	if ((r = send(clientSocket, text_out, strlen(text_out), 0)) == -1) {
		perror("send() call failed");
	}
}

// char *GetTimeString()
// {
// 	char buffer[64];
// 	memset(buffer, 0, sizeof(buffer));
// 	time_t rawtime;
// 	time(&rawtime);
// 	const auto timeinfo = localtime(&rawtime);
// 	strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo); //additional info %d-%m-%Y 
// 	return buffer;
// }