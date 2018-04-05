# Makefile for Assignment #3 - Chat program

CC=gcc
CFLAGS=-c -Wall -pedantic
LDFLAGS=-lpthread

all: client server

client: client.o
		${CC} ${LDFLAGS} client.o -o client

server: server.o
		${CC} ${LDFLAGS} server.o -o server

client.o: client.c
		  ${CC} ${CFLAGS} client.c

server.o: server.c
		  ${CC} ${CFLAGS} server.c

clean:
		rm -rf *.o server client