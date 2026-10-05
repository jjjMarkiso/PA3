#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_ADDR "127.0.0.1"
#define PORT 23422

#define NIST_SERVER "time.nist.gov"
#define NIST_PORT "13"

#define NUM_CONNECTIONS 5
#define BUFFER_SIZE 1024

#define FALSE 0
#define TRUE !FALSE

void *handle_client(void *arg);

#endif

