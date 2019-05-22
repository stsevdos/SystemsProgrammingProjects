#ifndef __inotify__dataTypes__
#define __inotify__dataTypes__

#include <stdio.h>

typedef struct worker* workerPtr;
typedef struct worker
{
    pid_t workerPID;
    int writeFD;
    short available;
    workerPtr nextWorker;
    char *pipeName;
}worker;

void insertStart(workerPtr *start, worker tempWorker, char *pipeName);
void deleteStart(workerPtr *start);
void destroyList(workerPtr *start);
int findAvailableWorker(workerPtr *start, worker *tempWorker);
int findSpecificWorker(workerPtr *start, pid_t pid);
void killAllWorkers(workerPtr *start);

#endif