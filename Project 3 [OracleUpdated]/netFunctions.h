#ifndef __netFunctions__
#define __netFunctions__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //inet_addr
#include <netdb.h>

#include "oracleFunctions.h"

// Global parameters
int portNo;
char *address;

// tcpThread
int argcCopy;
char **argvCopy;

int socketDesc;

void *serverThread(void *argp);
void *tcpThread(void *argp);
int client(void);

int writeAll(int fd, void *buff, size_t size);
int readAll(int fd, void *buff, size_t size);


#endif /* defined(__netFunctions__) */
