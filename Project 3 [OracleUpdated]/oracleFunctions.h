#ifndef __oracle__oracleFunctions__
#define __oracle__oracleFunctions__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

#define OSEED 10        // Oracle initial seed
#define WHENTOCHECK 1   // Check if secret word found, after that many tries
#define INITIALSIZE 20  // Word length

pthread_mutex_t *mutexArray, mutexForFile;

// Global parameters
int size, threadRestart, k;
char *logfileName;

int mutexCount;
int stopThreads;
unsigned int rseed;
char *bloomFilter;
FILE *fp;

typedef struct listNode* listPointer;
typedef struct listNode
{
    char *word;
    listPointer nextNode;
}listNode;

// List management functions
void insertStart(listPointer *start, char *w);
char* deleteStart(listPointer *start);
void destroyList(listPointer *start);

// Thread & Searching functions
char *searchList(listPointer *start, char *bloomFilter, int k);
int exists(char *word, char *bloomFilter, int k);
void *workerThread(void *argp);

#endif /* defined(__oracle__oracleFunctions__) */
