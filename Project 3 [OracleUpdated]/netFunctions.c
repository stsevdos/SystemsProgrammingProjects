#include "netFunctions.h"

void *workerThread(void *argp)
{
    listPointer start = NULL;
    char *word = NULL;
    
    // Search for secret word
    word = searchList(&start, bloomFilter, k);
    
    destroyList(&start);
    pthread_exit((void*)word);
}

void *serverThread(void *argp)
{
    int clientSocket , c;
    struct sockaddr_in server , client;
    pthread_t threadID;
    
    // Create socket
    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDesc == -1)
    {
        printf("Server Thread [%d]: Could not create socket\n", (int)pthread_self());
        pthread_exit(NULL);
    }
    else
        printf("Server Thread [%d]: Socket created\n", (int)pthread_self());
    
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( portNo );
    
    // Bind socket to server
    if (bind(socketDesc, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("Server Thread [%d]: Could not bind\n", (int)pthread_self());
        pthread_exit(NULL);
    }
    else
        printf("Server Thread [%d]: Bind done\n", (int)pthread_self());
    
    // Listen from socket
    listen(socketDesc, 3);
    
    // Accept any incoming connection
    printf("Server Thread [%d]: Waiting for incoming connections...\n", (int)pthread_self());
    c = sizeof(struct sockaddr_in);
    
    while ((clientSocket = accept(socketDesc, (struct sockaddr *)&client, (socklen_t*)&c)) > 0)
    {
        printf("Server Thread [%d]: Connection accepted\n", (int)pthread_self());
        
        // Create thread that will copy parameters from server to client
        if (pthread_create(&threadID, NULL, tcpThread, (void*)&clientSocket) < 0)
        {
            printf("Server Thread [%d]: Could not creade thread\n", (int)pthread_self());
            pthread_exit(NULL);
        }
        
        //pthread_join(threadID, NULL);
        printf("Server Thread [%d]: Handler assigned\n", (int)pthread_self());
    }
    
    if (clientSocket < 0)
    {
        printf("Server Thread [%d]: Accept failed\n", (int)pthread_self());
        pthread_exit(NULL);
    }
    
    pthread_exit(NULL);
}

void *tcpThread(void *argp)
{
    int i;
    int sock;
    char buffer[20];
    
    pthread_detach(pthread_self()); // Detaching self to avoid join
    
    sock = *(int*)argp;
    
    sprintf(buffer, "%d ", OSEED);
    
    printf("Thread [%d]: This is tcpThread\n", (int)pthread_self());
    
    // Writing seed to socket to check compatibility with client program
    if (write(sock, buffer, strlen(buffer)) < strlen(buffer))
    {
        printf("Thread [%d] could not write seed to socket\n", (int)pthread_self());
        pthread_exit(NULL);
    }
    
    // Writing program arguments to socket
    for (i = 1; i < argcCopy; i++)
    {
        write(sock, argvCopy[i], strlen(argvCopy[i]));  // ADD IF CHECK
        write(sock, " ", strlen(" "));  //ADD IF CHECK
    }
    
    write(sock, " ", strlen(" "));
    
    //for (i = 1; i < argcCopy; i++)
    //{
    //  printf("%s ", argvCopy[i]);
    //	fflush(NULL);
    //}
    
    // Locking mutexes and writing/copying current bloom filter to socket
    for (i = 0; i < mutexCount; i++)
        pthread_mutex_lock(&mutexArray[i]);
    
    if (writeAll(sock , bloomFilter , size) == -1)
    {
        printf("Thread [%d] writeAll failed\n", (int)pthread_self());
        exit(-1);
    }
    
    for (i = 0; i < mutexCount; i++)
        pthread_mutex_unlock(&mutexArray[i]);
    
    //printf(" <%s>", bloomFilter);fflush(NULL);
    
    close(sock);
    pthread_exit(NULL);
}

int client(void)
{
    int i, j, sock;
    char character;
    char buffer[20];
    struct sockaddr_in server;
    struct sockaddr *serverPointer = (struct sockaddr*)&server;
    struct hostent *rem;
    
    sprintf(buffer, "%d ", OSEED);
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Thread [%d] could not create socket\n", (int)pthread_self());
        pthread_exit(NULL);
    }
    
    // Find server address
    if ((rem = gethostbyname(address)) == NULL)
    {
        printf("++ %d) Could not gethostbyname\n", (int)pthread_self());
        pthread_exit(NULL);
    }
    
    server.sin_family = AF_INET;    // Internet domain
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(portNo); // Server port
    
    // Initiate connection
    if (connect(sock, serverPointer, sizeof(server)) < 0)
    {
        printf("Thread [%d] could not connect to server [portNo = %d, address = %s]\n", (int)pthread_self(), portNo, address);
        pthread_exit(NULL);
    }
    
    printf("Thread [%d] connecting to %s port %d.\n Must check %d arguments and then get the bloom filter\n", (int)pthread_self(), address, portNo, argcCopy-1);
    
    // First, checking if seed is same
    for (j = 0; j < strlen(buffer); j++)
    {
        printf("%c == ", buffer[j]); fflush(NULL);
        if (read(sock, &character, 1) < 0) // Reading from socket
        {
            printf("Thread [%d] could not read from socket\n", (int)pthread_self());
            close(sock);
            return 1; // Failed
        }
        
        printf("%c,", character);fflush(NULL);
        
        if (character != buffer[j])    // Character read is different from what should have been
        {
            printf("(%c) != (%c)\n", character, buffer[j]);
            close(sock);
            return 1; // Failed
        }
    }
    
    printf("Thread [%d] Seed OK!\n", (int)pthread_self());
    
    // Checking if execution parameters are same
    for (i = 1; i < argcCopy-2; i++)
    {
        // Won't check logfile name or -h ADDRESS
        printf("argument[%d]: ", i); fflush(NULL);
        
        for (j = 0; j < strlen(argvCopy[i]); j++)
        {
            printf("%c == ", argvCopy[i][j]); fflush(NULL);
            if (read(sock, &character, 1) < 0) // Reading from socket
            {
                printf("Thread [%d] could not read from socket\n", (int)pthread_self());
                close(sock);
                return 1; // Failed
            }
            
            printf("%c,", character);fflush(NULL);
            
            if (character != argvCopy[i][j])    // Character read is different from what should have been
            {
                printf("(%c) != (%c)\n", character, argvCopy[i][j]);
                printf("SOMETHING WENT REALLY WRONG\n");
                close(sock);
                return 1; // Failed
            }
        }
        
        if (read(sock, &character, 1) < 0)
        {
            printf("Thread [%d] could not read from socket\n", (int)pthread_self());
            pthread_exit(NULL);
        }
        
        printf("(%c) == space\n", character);fflush(NULL);
        
        if (character != ' ')
        {
            close(sock);
            return 1; // Failed
        }
    }
    
    if (read(sock, &character, 1) < 0)
    {
        printf("Thread [%d] could not read from socket\n", (int)pthread_self());
        pthread_exit(NULL);
    }
    
    if (character != ' ' ) // Delimiter is ' '
    {
        close(sock);
        return 1; // Failed
    }
    
    printf("(%c) == space\n", character); fflush(NULL);
    printf("Thread [%d] Parameters OK!\n", (int)pthread_self());
    
    // After checking for parameter sameness, we copy the bloom filter from the server
    /*for (i = 0; i < size; i++)
    {
        if (read(sock, &character, 1) < 0)
        {
            printf("Thread [%d] could not read from socket\n", (int)pthread_self());
            pthread_exit(NULL);
        }
        bloomFilter[i] = character;
    }*/
    
    // Instead we'll use readAll function
    if (readAll(sock, bloomFilter, size) == -1)
    {
        printf("readAll failed\n");
        exit(-1);
    }
    
    printf("Thread [%d] Bloom Filter OK!\n", (int)pthread_self());
    
    close(sock);
    
    return 0; // Success
}

// apo diafaneies k. Smaragdakh
int writeAll(int fd, void *buff, size_t size)
{
    int sent, n;
    
    for(sent = 0; sent < size; sent+=n)
    {
        if ((n = (int)write(fd, buff+sent, size-sent)) == -1)
            return -1; /* error */
    }
    return sent;
}

int readAll(int fd, void *buff, size_t size)
{
    int received, n;
    
    for (received = 0; received < size; received+=n)
    {
        if ((n = (int)read(fd, buff+received, size-received)) == -1)
            return -1; /* error */
    }
    return received;
}