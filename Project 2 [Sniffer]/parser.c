#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "dataTypesTLD.h"
#include "parser.h"

#define BUFFER_SIZE 512
#define MAX_DOMAIN_LENGTH 64
#define MAX_URL_SIZE 256

TLDPtr parser(char *path, char *fileName)
{
    int i, fd, urlStart, urlEnd, dots;
    int bytes;
    char *filePath, fileBuffer[2*BUFFER_SIZE], *url;     /* fileBuffer is size of two buffers in case url is split between read calls */
    TLDPtr listStart = NULL;

    i = 0;
    
    filePath = malloc(sizeof(char)*(strlen(path)+strlen(fileName)+2));
    if (filePath == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    sprintf(filePath, "%s/%s", path, fileName);     /* Create output file name */
    
    fd = open(filePath, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        exit(-1);
    }
    
    bytes = (int)read(fd, fileBuffer+BUFFER_SIZE, BUFFER_SIZE);
    if (bytes <= 0)
    {
        printf("File empty\n");
        exit(-1);
    }
    
    while (1)
    {
        if (i == bytes) /* If reached end of buffer */
        {
            /* Copy to "old" buffer and read again */
            memcpy(fileBuffer, fileBuffer+BUFFER_SIZE, BUFFER_SIZE);
            bytes = (int) read(fd, fileBuffer+BUFFER_SIZE, BUFFER_SIZE);
            if (bytes <= 0)
                break;
            i = 0;
        }
        
        if (strncmp("http://", fileBuffer+BUFFER_SIZE+i, 7) == 0)
        {
            urlStart = i+7;
            i=i+7;
        
            while (fileBuffer[BUFFER_SIZE+i] != '/')    /* Searching for '/' after "http://" */
            {
                i++;
                if (i == bytes) /* If reached end of buffer */
                {
                    /* Copy to "old" buffer and read again */
                    memcpy(fileBuffer, fileBuffer+BUFFER_SIZE, BUFFER_SIZE);
                    bytes = (int) read(fd, fileBuffer+BUFFER_SIZE, BUFFER_SIZE);
                    if (bytes <= 0)
                        break;
                    i = 0;
                }
            }
            
            if (bytes <= 0)
                break;
            
            urlEnd = i; /* Where we found the '/' after "http://" */
            
            dots = 0;
            while (dots < 2) /* Going backwards to count size of string needed */
            {
                i--;
                /* Maybe the url is already ready, e.g. "uoa.gr" and no second dot is encountered */
                if (fileBuffer[BUFFER_SIZE+i] == '.' || fileBuffer[BUFFER_SIZE+i] == '/')
                    dots++;
            }
            
            urlStart = i+1;
            url = (char*)malloc((urlEnd-urlStart+1)*sizeof(char));
            if (url == NULL)
            {
                perror("malloc");
                exit(-10);
            }
            
            for (i = urlStart; i < urlEnd; i++)
                url[i-urlStart] = fileBuffer[BUFFER_SIZE+i];
            url[urlEnd-urlStart] = '\0';
            
            /* Insert to list or, if already in list, just increment counter */
            if (!searchTLD(listStart, url))
                insertTLDStart(&listStart, url);
            
            free(url);
            i = urlEnd;
        }
        i++;
    }
    
    close(fd);
    
    return listStart;
}

void writer(TLDPtr listStart, char *fileName)
{
    int fd, bytes;
    char *outFileName, TLDcounterStr[100];
    
    outFileName = malloc(sizeof(char)*strlen(fileName)+5);
    if (outFileName == NULL)
    {
        perror("malloc");
        exit(-10);
    }
    
    sprintf(outFileName, "%s%s", fileName, ".out"); /* Create output filename name as <filename>.out */
    fd = open(outFileName, O_CREAT|O_WRONLY, 0666);
    if (fd < 0)
    {
        perror("open");
        exit(-1);
    }
    
    while (listStart != NULL)   /* Write each domain and times found in output file */
    {
        bytes = (int) write(fd, listStart->TLDname, strlen(listStart->TLDname));
        if (bytes != strlen(listStart->TLDname))
        {
            perror("write (1)");
            exit(-1);
        }
        
        bytes = (int) write(fd, " ", 1);
        if (bytes != 1)
        {
            perror("write (2)");
            exit(-1);
        }
        
        sprintf(TLDcounterStr, "%d", listStart->TLDcounter);
        bytes = (int) write(fd, TLDcounterStr, strlen(TLDcounterStr));
        if (bytes != strlen(TLDcounterStr))
        {
            perror("write (3)");
            exit(-1);
        }
        
        bytes = (int) write(fd, "\n", 1);
        if (bytes != 1)
        {
            perror("write (4)");
            exit(-1);
        }
        
        listStart = listStart->next;
    }
    
    close(fd);

    destroyTLDList(&listStart);     /* Domains' list is no longer needed */
}