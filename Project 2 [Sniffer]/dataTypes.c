#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include "dataTypes.h"

void insertStart(workerPtr *start, worker tempWorker, char *pipeName)
{
    workerPtr temp;
    
    temp = malloc(sizeof(worker));
    if (temp == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    
    temp->available = 0; /* is not available */
    temp->workerPID = tempWorker.workerPID;
    temp->writeFD = tempWorker.writeFD;
    
    temp->pipeName = malloc(strlen(pipeName)+1);
    if (temp->pipeName == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    strcpy(temp->pipeName, pipeName);
    
    temp->nextWorker = *start;
    *start = temp;
}

void deleteStart(workerPtr *start)
{
    workerPtr temp;
    
    temp = *start;
    *start = temp->nextWorker;
    free(temp);
}

void destroyList(workerPtr *start)
{
    while (*start != NULL)
    {
        deleteStart(start);
    }
}

int findAvailableWorker(workerPtr *start, worker *tempWorker)
{
    workerPtr temp;
    
    temp = *start;
    while (temp != NULL)
    {
        if (temp->available == 0)   /* If WORKER is not available, go to next one */
            temp = temp->nextWorker;
        else /* Found available worker */
        {
            temp->available = 0;    /* WORKER is no longer available */
            tempWorker = temp;
            return 1;
        }
    }
    
    return 0;  /* No available worker found */
}

int findSpecificWorker(workerPtr *start, pid_t pid)
{
    workerPtr temp;
    
    temp = *start;
    while (temp != NULL)
    {
        if (temp->workerPID != pid)
            temp = temp->nextWorker;
        else
        {
            temp->available = 1;    /* WORKER is now available */
            return 1;
        }
    }
 
    return 0;  /* Specific WORKER not found */
}

void killAllWorkers(workerPtr *start)
{
    workerPtr temp;
    int status;
    
    printf("All WORKERs will be killed\n");
    
    temp = *start;
    while (temp != NULL)
    {
        kill(temp->workerPID, SIGUSR1);
        waitpid(temp->workerPID, &status, WNOHANG);
        
        close(temp->writeFD);
        unlink(temp->pipeName);
        
        free(temp->pipeName);
        deleteStart(start);
        temp = temp->nextWorker;
    }
    printf("Finished killing WORKERs\n");
}