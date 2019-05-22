#ifndef __parser__dataTypes2__
#define __parser__dataTypes2__

#include <stdio.h>

typedef struct TLD* TLDPtr;
typedef struct TLD
{
    char *TLDname;
    int TLDcounter;
    TLDPtr next;
    
}TLD;

int searchTLD(TLDPtr start, char *TLDsearch);
void insertTLDStart(TLDPtr *start, char *domain);
void deleteTLDStart(TLDPtr *start);
void destroyTLDList(TLDPtr *start);

#endif /* defined(__parser__dataTypes2__) */
