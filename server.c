/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		server.c -   A simple chat program that uses tcp connection
--
--	PROGRAM:		server.exe
--
--	FUNCTIONS:		int main()
--					static void SystemFatal(const char* );
--
--	DATE:			April 04, 2018
--
--	REVISIONS:		NONE
--
--
--	DESIGNERS:		Angus Lam, Benny Wang, Roger Zhang
--
--
--	PROGRAMMER:		Angus Lam, Benny Wang
--
--	NOTES:
--	This is the server program that uses select to find read ready client socket and
-- 	will echo everything back to all the other clients.
---------------------------------------------------------------------------------------*/
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

// Function Prototypes
static void SystemFatal(const char* );

/*------------------------------------------------------------------------------
-- FUNCTION:    main
-- 
-- DATE:        April 4th, 2018
-- 
-- REVISIONS:   
-- 
-- DESIGNER:    Angus Lam, Benny Wang, Roger Zhang
-- 
-- PROGRAMMER:  Angus Lam, Benny Wang
-- 
-- INTERFACE:   int main(int, char)
-- 
-- RETURNS: int
--				0 to exit
-- 
-- NOTES: This is the main function that establish the connection to the clients
-- Adds new client to the set and filter them based on read ready by using select().
-- Read the ready socket and echo them back to all the other clients.
------------------------------------------------------------------------------*/
int main (int argc, char **argv)
{
	int i, maxi, nready, bytes_to_read, arg;
	int listen_sd, new_sd, sockfd, port, maxfd, client[FD_SETSIZE];
	unsigned int client_len;
	struct sockaddr_in server, client_addr, client_addresses[FD_SETSIZE];
	char buf[BUFLEN];
   	ssize_t n;
   	fd_set rset, allset;

	switch(argc)
	{
		case 1:
			port = SERVER_TCP_PORT;	// Use the default port
		break;
		case 2:
			port = atoi(argv[1]);	// Get user specified port
			if(port == 0)
			{
				fprintf(stderr, "invalid port, default port used.\n");
				port = SERVER_TCP_PORT;
			}
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	// Create a stream socket
	if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		SystemFatal("Cannot Create Socket!");

	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
    arg = 1;
    if (setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
    {
        SystemFatal("setsockopt");
    }

	// Bind an address to the socket
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

	if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
		SystemFatal("bind error");

	// Listen for connections
	listen(listen_sd, LISTENQ);

	maxfd	= listen_sd;
   	maxi	= -1;

	for (i = 0; i < FD_SETSIZE; i++)
        // -1 indicates available entry
		client[i] = -1;

 	FD_ZERO(&allset);
   	FD_SET(listen_sd, &allset);

	while (TRUE)
	{
   		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listen_sd, &rset)) // new client connection
		{
			client_len = sizeof(client_addr);
			if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, &client_len)) == -1)
			{
				SystemFatal("accept error");
			}

		    printf(" Remote Address:  %s\n", inet_ntoa(client_addr.sin_addr));

		    for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0)
				{
					// save the new sockets and addresses to the client array
					client[i] = new_sd;
					client_addresses[i] = client_addr;
					break;
				}
			if (i == FD_SETSIZE)
			{
				printf ("Too many clients\n");
				exit(1);
			}

			FD_SET (new_sd, &allset);     // add new descriptor to set
			if (new_sd > maxfd)
				maxfd = new_sd;	// for select

			if (i > maxi)
				maxi = i;	// new max index in client[] array

			if (--nready <= 0)
				continue;	// no more readable descriptors
		}

		for (i = 0; i <= maxi; i++)	// check all clients for data
		{
			if ((sockfd = client[i]) < 0)
				continue;

			if (FD_ISSET(sockfd, &rset))
			{
				bytes_to_read = BUFLEN;
				n = read(sockfd, &buf, bytes_to_read);

				if(n != 0)
				{
					for (int j = 0; j <= maxi; j++)
					{
						if (client[j] != sockfd)
						{
							// Echo to all the other clients
							write(client[j], &buf, BUFLEN);
						}
					}

					memset(buf, 0, sizeof(buf));
				}

				// connection closed by client
				if (n == 0)
				{
					printf(" Remote Address:  %s closed connection\n", inet_ntoa(client_addresses[i].sin_addr));
					strcpy(buf, inet_ntoa(client_addresses[i].sin_addr));
					strcat(buf, " has disconnected.\n");

					for (int j = 0; j <= maxi; j++)
					{
						if (client[j] != sockfd)
						{
							// Sends the disconnected messages to all the other clients.
							write(client[j], &buf, BUFLEN);
						}
					}
					memset(buf, 0, sizeof(buf));
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				}

				// If there are no more ready descriptors. break and exit.
				if (--nready <= 0)
					break;
			}
		}
	}
	return(0);
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
