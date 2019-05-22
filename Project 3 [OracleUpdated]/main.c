#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hash.h"
#include "oracle.h"
#include "oracleFunctions.h"
#include "netFunctions.h"

int main(int argc, const char * argv[])
{
    int i, threadCount, errorCheck, result;
    double elapsedTime;
    char *word;
    pthread_t *threads;
    pthread_t sThread = 0;
    struct timeval time1, time2;
    
    // Parameters for tcpThread
    address = NULL;
    argcCopy = argc;
    argvCopy = (char **)argv;
    
    if (argc < 6)
    {
        printf("Not enough arguments to execute program.\n");
        exit(-1);
    }
    else if (argc > 10)
    {
        printf("Too many arguments to execute program.\n");
        exit(-1);
    }
    else if (argc == 6)
    {
        size = atoi(argv[1]);
        threadCount = atoi(argv[2]);
        threadRestart = atoi(argv[3]);
        portNo = atoi(argv[4]);
        logfileName = (char*)argv[5];
        
        k = 3;      // Default value for k hash functions
    }
    else if (argc == 8)
    {
        size = atoi(argv[1]);
        threadCount = atoi(argv[2]);
        threadRestart = atoi(argv[3]);
        portNo = atoi(argv[4]);
        logfileName = (char*)argv[5];
        
        if (argv[6][1] == 'k')  // -k option passed
            k = atoi(argv[7]);
        else    // -h option passed
        {
            k = 3 ;
            address = (char*)argv[7];
        }
    }
    else if (argc == 10)
    {
        size = atoi(argv[1]);
        threadCount = atoi(argv[2]);
        threadRestart = atoi(argv[3]);
        portNo = atoi(argv[4]);
        logfileName = (char*)argv[5];
        
        k = atoi(argv[7]);  // Default value for k hash functions
        address = (char*)argv[9];   // address
    }
    else    // argc == 7 || 9
    {
        printf("Invalid options\n");
        exit(-1);
    }
    
    srand((unsigned int)time(NULL));
    
    // Additional parameter checking
    if (size <= 0 || threadCount <= 0 || threadRestart <= 0 || portNo < 0 || k <= 0)
    {
        printf("All parameters must be > 0.\n");
        exit(-1);
    }
    
    bloomFilter = malloc(sizeof(char)*size);
    if (bloomFilter == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    
    if (address == NULL) // SERVER
        memset(bloomFilter, 0, sizeof(char)*size);  // Initialize bloom filter to all 0s
    else    // CLIENT
    {
        result = client();
        if (result != 0)
        {
            free(bloomFilter);
            printf("Server-Client parameters incompatible. Will exit.\n");
            exit(-1);
        }
    }
    
    stopThreads = 0; // 0 = word not found so don't exit yet
    threads = malloc(sizeof(pthread_t)*threadCount);
    if (threads == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    
    fp = fopen(logfileName, "w");
    
    // Calculating mutexes needed, one mutex for each of bloom filter's sections
    mutexCount = size/64;
    if (size % 64 > 0)
        mutexCount++;
    
    mutexArray = malloc(sizeof(pthread_mutex_t)*mutexCount);
    if (mutexArray == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    
    // Creating mutexes for bloom filter access
    for (i = 0; i < mutexCount; i++)
    {
        errorCheck = pthread_mutex_init(&mutexArray[i], NULL);
        if (errorCheck != 0){
            printf("pthread_mutex_init\n");
            exit(-1);
        }
    }
    
    // Creating mutex for logfile access
    errorCheck = pthread_mutex_init(&mutexForFile, NULL);
    if (errorCheck != 0)
    {
        printf("pthread_mutex_init\n");
        exit(-1);
    }
    
    initAlloc(malloc);
    // setEasyMode();
    // setHardMode();
    initSeed(OSEED);   // Initialize oracle function(s)
    rseed = (unsigned int)time(NULL);   // Get seed for rand_r
    
    gettimeofday(&time1, NULL); // Start Timer
    
    // Creating threads for word search
    for (i = 0; i < threadCount; i++)
    {
        errorCheck = pthread_create(&threads[i], NULL, workerThread, NULL);
        if (errorCheck != 0)
        {
            printf("pthread_create failed\n");
            exit(-1);
        }
    }
    
    // Create thread that will listen for TCP connection(s)
    errorCheck = pthread_create(&sThread, NULL, serverThread, NULL);
    if (errorCheck != 0)
    {
        printf("pthread_create for server thread failed\n");
        exit(-1);
    }
    
    // At this point, threads will have stopped their search
    for (i = 0; i < threadCount; i++)
    {
        errorCheck = pthread_join(threads[i], (void**)&word);
        if (errorCheck != 0)
        {
            printf("pthread_join failed\n");
            exit(-1);
        }
        
        printf("Thread [%d] exited.\n", (int)threads[i]);
        
        if (word != NULL) //found
        {
            printf("Thread [%d] found the secret word %s\n", (int)threads[i], word);
            free(word);
        }
    }
    
    // Shutting down socket
    if (shutdown(socketDesc, SHUT_RDWR))
    {
        printf("Socket shutdown failed\n");
        exit(-1);
    }
    
    gettimeofday(&time2, NULL); // Stop Timer
    elapsedTime = (time2.tv_sec - time1.tv_sec) * 1000000.0;
    elapsedTime += (time2.tv_usec - time1.tv_usec) / 1000000.0;
    
    fprintf(fp, "Execution Time: %.2lf ms\n", elapsedTime);
    
    // No longer need threads array or write anything more to logfile
    free(threads);
    fclose(fp);
    
    errorCheck = pthread_join(sThread, NULL);
    if (errorCheck != 0){
        printf("pthread_join for server thread failed\n");
        exit(-1);
    }
    
    // Destroy mutexes for bloom filter & logfile access, cannot destroy before TCP server ends its jobs
    for (i = 0; i < mutexCount; i++)
    {
        errorCheck = pthread_mutex_destroy(&mutexArray[i]);
        if (errorCheck != 0){
            printf("pthread_mutex_destroy failed\n");
            exit(-1);
        }
    }
    
    errorCheck = pthread_mutex_destroy(&mutexForFile);
    if (errorCheck != 0)
    {
        printf("pthread_mutex_destroy \n");
        exit(-1);
    }
    
    free(bloomFilter);
    free(mutexArray);
    
    return 0;
}
