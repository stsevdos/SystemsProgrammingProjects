#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dataTypesTLD.h"

int searchTLD(TLDPtr start, char *TLDsearch)
{
    TLDPtr temp;
    
    temp = start;
    while (temp != NULL)
    {
        if (strcmp(temp->TLDname, TLDsearch) == 0)
        {
            temp->TLDcounter++;
            return 1;   /* Found and increased */
        }
        else
            temp = temp->next;
    }
    
    return 0;   /* Returns 0 if not found */
}

void insertTLDStart(TLDPtr *start, char *domain)
{
    TLDPtr temp;
    
    temp = malloc(sizeof(TLD));
    if (temp == NULL)
    {
        perror("malloc");
        exit(-1);
    }

    temp->TLDname = malloc(sizeof(char)*(strlen(domain)+1));
    if (temp->TLDname == NULL)
    {
        perror("malloc");
        exit(-1);
    }
    strcpy(temp->TLDname, domain);
    temp->next = *start;
    temp->TLDcounter = 1;

    *start = temp;
}

void deleteTLDStart(TLDPtr *start)
{
    TLDPtr temp;
    
    temp = *start;
    *start = temp->next;
    free(temp);
}

void destroyTLDList(TLDPtr *start)
{
    while (*start != NULL)
    {
        deleteTLDStart(start);
    }
}