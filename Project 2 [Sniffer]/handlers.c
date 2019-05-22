#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "handlers.h"
#include "dataTypes.h"

extern int childrenCount;
extern workerPtr start;
extern pid_t listenerPID;
extern int workerReadFD;

/* *** MANAGER's signal handlers *** */
void handlerSIGCHLDmanager(int sigNo)
{
    pid_t pid;
    int status;
    int check;
    
    printf("MANAGER rcvd SIGCHLD\n");
    fflush(NULL);
    
    while ((pid = waitpid(-1, &status, WUNTRACED)) != -1) /* If any child process has stopped */
    {
        if (pid != listenerPID) /* and said process isn't the LISTENER */
        {
            check = findSpecificWorker(&start, pid);
            if (check == 0 )
            {
                printf("Could not find WORKER (pid: %d)\n", pid);
                exit(-10);
            }
            else
                break;
        }
    }
}

void handlerSIGINTmanager(int sigNo)
{
    int status;
    
    printf("MANAGER rcvd SIGINT\n");
    fflush(NULL);
    
    killAllWorkers(&start);
    kill(listenerPID, SIGUSR1);
    waitpid(listenerPID, &status, WNOHANG);
    
    printf("MANAGER cleared everything\n");
    exit(0);
    
}
/*************************************/

/* *** WORKER's signal handlers *** */
void handlerSIGUSR1worker(int sigNo)
{
    printf("WORKER (pid: %d) rcvd SIGUSR1\n", getpid());
    close(workerReadFD);
    printf("WORKER (pid: %d) will now exit\n", getpid());
    exit(0);
}

void handlerSIGINTworker(int sigNo)
{
    printf("WORKER (pid: %d) rcvd SIGINT\n", getpid());
    fflush(NULL);
    kill(getppid(), SIGINT); /* Forwards signal to parent process (MANAGER) */
    
}
/*************************************/

/* *** LISTENER's signal handlers *** */
void handlerSIGUSR1listener(int sigNo)
{
    printf("LISTENER rcvd SIGUSR1\n");
    printf("LISTENER will now exit\n");
    exit(0);
}

void handlerSIGINTlistener(int sigNo)
{
    printf("LISTENER rcvd SIGINT\n");
    fflush(NULL);
    kill(getppid(), SIGINT); /* Forwards signal to parent process (MANAGER) */
}
/*************************************/