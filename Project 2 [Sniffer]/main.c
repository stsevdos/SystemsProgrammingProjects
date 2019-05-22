#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "dataTypes.h"
#include "dataTypesTLD.h"
#include "parser.h"
#include "handlers.h"

#define MAX_BUFFER 1000
#define READ 0
#define WRITE 1

int childrenCount;
workerPtr start;
pid_t listenerPID;
int workerReadFD;

char* inotParser(char *whatReads);

int main(int argc, const char * argv[])
{
    short check;
    int i, j, fd[2], bytes, workerAvailability;
    char whatReads[MAX_BUFFER], outFile[5];
    char *pipeName, *fileName, *path;
    struct stat fileInfo;
    worker tempWorker;
    TLDPtr filesList;
    
    static struct sigaction act, act2;
    
    if (argc > 1)   /* If argc > 1, user has defined a path */
    {
        path = malloc(sizeof(char)*(strlen(argv[2])+1));
        if (path == NULL)
        {
            perror("malloc");
            exit(-1);
        }
        strcpy(path, argv[2]);
    }
    else if (argc == 1)    /* else default path is current directory */
    {
        path = malloc(sizeof(char)*2);
        if (path == NULL)
        {
            perror("malloc");
            exit(-1);
        }
        strcpy(path, ".");
    }
    else
    {
        printf("Too many parameters to run program\n");
        exit(-1);
    }
    
    /* Redefining behaviour for MANAGER (main) process, when specific signals are received */
    act.sa_handler = handlerSIGINTmanager;
    sigfillset(&(act.sa_mask));
    sigaction(SIGINT, &act, NULL);
    
    act2.sa_handler = handlerSIGCHLDmanager;
    sigfillset(&(act2.sa_mask));
    sigaction(SIGCHLD, &act2, NULL);
    
    tempWorker.workerPID = 0;
    childrenCount = 0;
    
    /* Creating base name for each of the named pipes that will be created (e.g. fifo_1, fifo_2) */
    pipeName = malloc(sizeof("fifo_")+10);
    if (pipeName == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    
    check = pipe(fd);   /* Simple pipe for MANAGER's communication with LISTENER */
    if (check == -1)
    {
        perror("pipe");
        exit(-1);
    }
    
    start = NULL;   /* Initializing WORKER list */
    
    listenerPID = fork();
    if (listenerPID == -1)
    {
        perror("fork");
        exit(-1);
    }
    
    if (listenerPID) /* MANAGER (Reader) */
    {
        close(fd[WRITE]);
        dup2(fd[READ], STDIN_FILENO);
        close(fd[READ]);
    }
    else    /* LISTERNER (Writer) */
    {
        /* Redefining behaviour for LISTENER process, when specific signals are received */
        act.sa_handler = handlerSIGINTlistener;
        sigfillset(&(act.sa_mask));
        sigaction(SIGINT, &act, NULL);
        
        act2.sa_handler = handlerSIGUSR1listener;
        sigfillset(&(act2.sa_mask));
        sigaction(SIGUSR1, &act2, NULL);
        
        close(fd[READ]);
        dup2(fd[WRITE], STDOUT_FILENO);
        close(fd[WRITE]);
        
        execlp("inotifywait", "inotifywait", "-m", "-e", "create", path, NULL);
    }
    
    if (listenerPID)    /* MANAGER reads from buffer */
    {
        for ( ; ; )
        {
            printf("MANAGER is ready to read from pipe\n");
            
            memset(whatReads, 0, MAX_BUFFER);
            bytes = (int)read(STDIN_FILENO, whatReads, MAX_BUFFER);
            
            if (bytes > 0)
            {
                printf("MANAGER read: %s\t", whatReads);
                check = write(STDOUT_FILENO, whatReads, MAX_BUFFER);
                if (check < 0)
                {
                    perror("write");
                    exit(-1);
                }
                
                fileName = inotParser(whatReads);    /* inotifywait outputs "./ CREATE file", need to parse it */
                
                /* Looking at file's extension, if it's a ".out" file, don't feed to worker */
                for (i = (int)strlen(fileName)-1, j = 3; j >= 0; i--, j--)
                    outFile[j] = fileName[i];
                outFile[4] = '\0';

                if (!strcmp(outFile, ".out"))
                    continue;
                /* Also, don't feed to worker if it's a named pipe */
                stat(fileName, &fileInfo);
                if ((fileInfo.st_mode & S_IFMT) == S_IFIFO)
                    continue;
                
                printf("MANAGER will feed %s to WORKER\n", fileName);
                
                workerAvailability = findAvailableWorker(&start, &tempWorker);
                if (workerAvailability) /* Available worker found & is tempWorker */
                {
                    printf("WORKER (pid: %d) is available\n", tempWorker.workerPID);
                    check = write(tempWorker.writeFD, fileName, strlen(fileName)+1);
                    if (check == -1)
                    {
                        perror("write");
                        exit(-1);
                    }
                    
                    kill(tempWorker.workerPID, SIGCONT);    /* Tell WORKER to start working again */
                }
                else    /* No available WORKER found, create a new one */
                {
                    printf("MANAGER will create WORKER\n");
                    /* Parent process creates new WORKER and named pipe, and adds worker to list */
                    sprintf(pipeName, "fifo_%d", childrenCount);
                    check = mkfifo(pipeName, 0666);
                    
                    tempWorker.writeFD = open(pipeName, O_RDWR | O_NONBLOCK);   /* Open newly created named pipe */
                    if (tempWorker.writeFD == -1)
                    {
                        perror("open");
                        exit(-1);
                    }
                    
                    check = write(tempWorker.writeFD, fileName, strlen(fileName)+1); /* Write filename to it */
                    if (check == -1)
                    {
                        perror("write");
                        exit(-1);
                    }
                    
                    tempWorker.workerPID = fork();  /* Create new WORKER */
                    if (tempWorker.workerPID < 0)
                    {
                        perror("fork");
                        exit(-1);
                    }
                    
                    if (tempWorker.workerPID) /* MANAGER */
                    {
                        childrenCount++;

                        printf("MANAGER created WORKER (pid: %d)\n", tempWorker.workerPID);
                        printf("(Total children count is %d)\n", childrenCount);

                        insertStart(&start, tempWorker, pipeName);  /* Insert WORKER's data to list */
                    }
                    else    /* WORKER */
                    {
                        printf("WORKER (pid: %d) has been created\n", getpid());
                        
                        sleep(2); /* In order to have time to create more WORKERs, but not needed */
                        
                        /* Redefining behaviour for WORKER process(es), when specific signals are received */
                        act.sa_handler = handlerSIGINTworker;
                        sigfillset(&(act.sa_mask));
                        sigaction(SIGINT, &act, NULL);
                        
                        act2.sa_handler = handlerSIGUSR1worker;
                        sigfillset(&(act2.sa_mask));
                        sigaction(SIGUSR1, &act2, NULL);
                        
                        workerReadFD = open(pipeName, O_RDWR | O_NONBLOCK); /* WORKER opens named pipe for reading */
                        if (workerReadFD == -1)
                        {
                            perror("open");
                            exit(-1);
                        }
                        
                        while(1)
                        {
                            memset(whatReads, 0, MAX_BUFFER);   /* whatReads buffer should be empty in case WORKER reads < MAX_BUFFER */
                            
                            printf("WORKER (pid: %d) will read\n", getpid());
                            
                            bytes = (int)read(workerReadFD, whatReads, MAX_BUFFER); /* whatReads will only contain a filename */
                            if (bytes < 0)
                            {
                                perror("read");
                                exit(-1);
                            }
                            
                            printf("WORKER (pid: %d) read %s from MANAGER (main)\n", getpid(), whatReads);
                            
                            filesList = parser(path, whatReads);    /* Parse file and create list with domains & hits */
                            writer(filesList, whatReads);   /* Write above information to .out file */
                            
                            printf("WORKER (pid: %d) will now stop\n", getpid());
                            fflush(NULL);
                            raise(SIGSTOP); /* WORKER sends SIGSTOP to self */
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

char* inotParser(char *whatReads)
{
    int i, spaceCounter;
    char *fileName;
    
    fileName = NULL;
    spaceCounter = 0;
    
    /* inotifywait's output will contain a '\n', replace with '\0' to treat as string */
    whatReads[strlen(whatReads)-1] = '\0';
    
    /* "./ CREATE file" the file's name will be right after the second space */
    for (i = 0; i < strlen(whatReads); i++)
    {
        if (spaceCounter == 2)
        {
            fileName = malloc(sizeof(char)*(strlen(whatReads)-i+1));
            if (fileName == NULL)
            {
                perror("malloc");
                exit(-1);
            }
            
            strcpy(fileName, whatReads+i);
            break;
        }
        
        if (whatReads[i] == ' ')
            spaceCounter++;
        
    }
    
    return fileName;
}
