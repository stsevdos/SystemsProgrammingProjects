#ifndef __oracle__oracleFunctions__
#define __oracle__oracleFunctions__

#include <stdio.h>

extern int size;

typedef struct node* pointer;
typedef struct node
{
    char *word;
    int children;
    pointer *array;
}node;

typedef struct listNode* listPointer;
typedef struct listNode
{
    char *word;
    listPointer nextNode;
}listNode;

void preorderTree(pointer root);
void destroyTree(pointer *root);

void insertStart(listPointer *start, char *w);
char* deleteStart(listPointer *start);
void destroyList(listPointer *start);

int searchWords(pointer p, int depth, char *bloomFilter, int k, listPointer *start);
int searchList(listPointer *start, char *bloomFilter, int k);

int exists(char *word, char *bloomFilter, int k);

#endif /* defined(__oracle__oracleFunctions__) */
