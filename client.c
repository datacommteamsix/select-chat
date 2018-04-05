/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		client.c -   A simple chat program that uses tcp connection
--
--	PROGRAM:		client.exe
--
--	FUNCTIONS:		void *client_recv(void *ptr);
--					int client_send();
--					void sendPacket(char * sendbuf);
--					void intHandler(int sigint);
--					void endProgram();
--					static void SystemFatal(const char* message);
--
--	DATE:			April 04, 2018
--
--	REVISIONS:		NONE
--
--
--	DESIGNERS:		Angus Lam, Benny Wang, Roger Zhang
--
--
--	PROGRAMMER:		Angus Lam, Roger Zhang
--
--	NOTES:
--	The program will connect a server using tcp, then send and receive from all other
--  connected clients.
---------------------------------------------------------------------------------------*/
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
#include <signal.h>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	255		//Buffer length
#define HOST_LEN 128	//Host length
#define TRUE	1
#define LISTENQ	5
#define MAXLINE 4096

pthread_t clientThread;
void *client_recv(void *ptr);
int client_send();
void sendPacket(char * sendbuf);
void intHandler(int sigint);
void endProgram();
static void SystemFatal(const char* message);

int clientSocket;
int savelog = 0;	//save log flag
FILE *ofp;

/*------------------------------------------------------------------------------
-- FUNCTION:    main
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Angus Lam, Benny Wang, Roger Zhang
-- 
-- PROGRAMMER:  Roger Zhang, Benny Wang
-- 
-- INTERFACE:   int main(int, char)
-- 
-- RETURNS: int
--				0 to exit
-- 
-- NOTES: This is the main function that connects to the server. It also sets
-- up the signal handler and check for input parameters.
-- The main function also starts a separate thread to receive from clients.
------------------------------------------------------------------------------*/
int main (int argc, char **argv)
{
	char *data;
	int port, cmp;
	struct sockaddr_in clientInetAddress;
	if (argc > 1)
	{
		data = (char*) malloc(sizeof(char) * strlen(argv[1]+1));
	}

	signal(SIGINT, intHandler);

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
		case 4:
			port = atoi(argv[2]);	// Get user specified port
			strcpy(data, argv[1]);
			if ((cmp = strcmp(argv[3], "-s")) == 0)
			{
				savelog = 1;
				ofp = fopen("./chat.txt", "w");
				fprintf(stdout, "File will be saved to chat.txt\n");
			}
			break;
		default:
			fprintf(stderr, "Usage: %s [IP] [PORT] [Optional -s for save]\n", argv[0]);
			exit(1);
	}

	// Create a stream socket
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		SystemFatal("Cannot Create Socket!");

	struct hostent *Hostent;
	if ((Hostent = gethostbyname(data)) == NULL)
	{
		fprintf(stderr, "Bad Host Name.\n");
		exit(1);
	}

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

	fprintf(stdout, "Success!\n");
	fprintf(stdout, "Welcome to Chat!\nType in EXIT if you want to exit the program.\n");

	// Creats the client listen thread to listen for messages from the server
	pthread_create(&clientThread, NULL, &client_recv, (void *)NULL);

	while(client_send(clientSocket) == 1);
	pthread_join(clientThread, NULL);

	return 0;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    SystemFatal
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Benny Wang, Roger Zhang
-- 
-- PROGRAMMER:  Roger Zhang
-- 
-- INTERFACE:   SystemFatal(const char* message)
--							message : error messages
-- 
-- RETURNS: void
-- 
-- NOTES: This is the fatal messages that prints the error messages and exits
-- the program upon failure.
------------------------------------------------------------------------------*/
static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}

/*------------------------------------------------------------------------------
-- FUNCTION:    client_recv
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Roger Zhang
-- 
-- PROGRAMMER:  Roger Zhang
-- 
-- INTERFACE:   client_recv(void *ptr)
--							void ptr for pthread
-- 
-- RETURNS: void*
-- 
-- NOTES: This is the listening thread to listen for server messages. It also
-- prints to the text file if enabled by user.
------------------------------------------------------------------------------*/
void *client_recv(void *ptr) {
	char recvbuf[BUFLEN];
	int	r;

	for (;;) {
		if ((r = recv(clientSocket, recvbuf, BUFLEN, 0)) == -1) {
			perror("recv call() failed");
			continue;
		}
		if (r == 0)
		{
			endProgram();
		}
		fprintf(stdout, "\n%s", recvbuf);
		fprintf(stderr, "Me: "); //stdout does not work sometimes with this line.
		if (savelog == 1)
		{
			fprintf(ofp, "%s", recvbuf);
		}
	}
}

/*------------------------------------------------------------------------------
-- FUNCTION:    client_send
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Angus Lam
-- 
-- PROGRAMMER:  Angus Lam
-- 
-- INTERFACE:   client_send()
-- 
-- RETURNS: void
-- 
-- NOTES: This is the send message that clients sends to the user. It checks
-- for the input and upon certain command, it will exit properly.
------------------------------------------------------------------------------*/
int client_send()
{
	char *buffer;
    size_t bufsize = 255;
    size_t characters;
	int cmp = -1;

    buffer = (char *)malloc(bufsize * sizeof(char));
    if( buffer == NULL)
    {
        perror("Unable to allocate buffer");
        exit(1);
    }
    fprintf(stdout, "Me: ");
    characters = getline(&buffer,&bufsize,stdin);

	if ((cmp = strcmp(buffer, "EXIT\n")) == 0)
	{
		fprintf(stderr, "See you next time!\n");
		endProgram();
	}
    if ((buffer)[characters - 1] == '\n')
  	{
      (buffer)[characters - 1] = '\0';
      --characters;
  	}
    sendPacket(buffer);

    return 1;
}

/*------------------------------------------------------------------------------
-- FUNCTION:    sendPacket
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Angus Lam, Benny Wang
-- 
-- PROGRAMMER:  Angus Lam
-- 
-- INTERFACE:   sendPacket(char * sendbuf)
--						sendbuf : messages to send to the server
-- RETURNS: void
-- 
-- NOTES: This is the send function that sends the messages to the server.
-- Will save to the file if enabled by user. The function also properly adds
-- the host name to the message.
------------------------------------------------------------------------------*/
void sendPacket(char * sendbuf) {
	int r;
	char text_out[BUFLEN];
	char host[HOST_LEN];

	gethostname(host, HOST_LEN);

	sprintf(text_out, "%s : %s\n", host, sendbuf);

	if (savelog == 1)
	{
		fprintf(ofp, "%s\n", text_out);
	}
	if ((r = send(clientSocket, text_out, strlen(text_out), 0)) == -1) {
		perror("send() call failed");
	}
}

/*------------------------------------------------------------------------------
-- FUNCTION:    intHandler
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Angus Lam
-- 
-- PROGRAMMER:  Angus Lam
-- 
-- INTERFACE:   intHandler(int terminate)
-- 
-- RETURNS: void
-- 
-- NOTES: This is the signal handler to handle ctr-c termination. The thread
-- will exit, socket and file will be closed properly.
------------------------------------------------------------------------------*/
void intHandler(int terminate) {
    if (savelog == 1)
    {
    	fclose(ofp);
    }
    close(clientSocket);
    pthread_kill(clientThread, SIGUSR1);
	exit(1);
}

/*------------------------------------------------------------------------------
-- FUNCTION:    endProgram
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Angus Lam
-- 
-- PROGRAMMER:  Angus Lam
-- 
-- INTERFACE:   endProgram()
-- 
-- RETURNS: void
-- 
-- NOTES: This close the program properly with thread, files and socket all
-- terminates properly.
------------------------------------------------------------------------------*/
void endProgram()
{
	if (savelog == 1)
    {
    	fclose(ofp);
    }
    close(clientSocket);
    pthread_kill(clientThread, SIGUSR1);
	exit(1);
}
