#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "oracleFunctions.h"
#include "hash.h"
#include "oracle.h"

/*** (morfopoihmenes) synarthseis apo to mathhma Domes Dedomenwn ***/
void preorderTree(pointer root)
{
    int i;
    
    if (root != NULL)
    {
        printf("%s\n", root->word);
        for (i = 0; i < root->children; i++)
            preorderTree(root->array[i]);
    }
}

void destroyTree(pointer *root)
{
    int i;
    
    if (*root == NULL)
        return;
    
    for (i = 0; i < (*root)->children; i++)
        destroyTree(&(*root)->array[i]);
    free((*root)->array);
    free((*root)->word);
    free(*root);
    *root = NULL;
}

void insertStart(listPointer *start, char *w)
{
    listPointer temp;
    
    temp = malloc(sizeof(listNode));
    if (temp == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }
    
    temp->word = malloc((strlen(w)+1)*sizeof(char));
    if (temp->word == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }

    strcpy(temp->word, w);
    temp->nextNode = *start;
    *start = temp;
}

/* Deletes node but keeps/returns word in node */
char* deleteStart(listPointer *start)
{
    char *w;
    listPointer temp;
    
    temp = *start;
    w = temp->word;
    *start = temp->nextNode;
    free(temp);
    return w;
}

void destroyList(listPointer *start)
{
    char *w;
    
    while (*start != NULL)
    {
        w = deleteStart(start);
        free(w);
    }
}
/*********************************************************/

int exists(char *word, char *bloomFilter, int k)
{
    int i, result;
    uint64_t *hashResults;
    
    result = 0;
    hashResults = malloc(sizeof(uint64_t)*k);
    if (hashResults == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }
    
    /* Pass word through all k hash functions */
    for (i = 0; i < k; i++)
    {
        hashResults[i] = hash_by(i, word)%(size*8);
        if (bloomFilter[hashResults[i]/8] & (1 << (7-hashResults[i]%8)))
            result++;     /* exists */
    }
    
    free(hashResults);
    if (result == k)
        return 1;   /* all bits for word exist in bloom filter */
    
    return 0;   /* at least one bit for word was zero in bloom filter */
}

int searchWords(pointer p, int depth, char *bloomFilter, int k, listPointer *start)
{
    int i, j, oracleReturn, counter, index, result, stringSize, existsResult;
    uint64_t hashResults;
    char **words;
    
    if (p == NULL)
        return -1;
    if (depth == 0)
        return -1;
    
    words = (char**)oracle(p->word);
    printf("Tree::Trying word: %s - ", p->word);
    if (words == NULL)
    {
        printf("\nSecret word is %s.\n\n", p->word);
        return 1;
    }
    
    printf("not the secret word.\n");
    
    if (depth == 1)
    {
        p->children = 0;
        
        /* Maximum tree depth reached, add words returned by oracle to linked list */
        for (i = 0; words[i] != NULL; i++)
        {
            insertStart(start, words[i]);
        }
        return -1;
    }
    
    for (i = 0; words[i] != NULL; i++)  /* Count number of words returned by oracle */
        ;
    oracleReturn = i;
    
    counter = 0;
    /* Count number of words that haven't already been checked or added to tree */
    for (i = 0; i < oracleReturn; i++)
    {
        existsResult = exists(words[i], bloomFilter, k);
        if (existsResult == 0)  /* Word hasn't been checked/added before */
            counter++;
    }
    
    p->children = counter;
    p->array = malloc(sizeof(pointer)*counter);
    if (p->array == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }
    
    index = 0; /* p->array's index to insert next word, because 'i' may increase past p->array's bounds */
    for (i = 0; i < oracleReturn; i++)
    {
        existsResult = exists(words[i], bloomFilter, k);
        if (existsResult == 0) /* If word hasn't been checked/added before, add to tree */
        {
            p->array[index] = malloc(sizeof(node));
            if (p->array[index] == NULL)
            {
                perror("malloc failed");
                exit(-1);
            }
            
            stringSize = (int)strlen(words[i]);
            p->array[index]->word = malloc(stringSize+1);   /* free */
            if (p->array[index]->word == NULL)
            {
                perror("malloc failed");
                exit(-1);
            }
            
            strcpy(p->array[index]->word, words[i]);
            
            for (j = 0; j < k; j++)
            {
                hashResults = hash_by(j, p->array[index]->word) % (size*8);
                bloomFilter[hashResults/8] |= 1 << (7-hashResults%8);
            }
            
            index++;
        }
    }
    
    /* Call searchWords recursively for each of your children, decreasing depth by 1 */
    for (i = 0; i < counter; i++)
    {
        result = searchWords(p->array[i], depth-1, bloomFilter, k, start);
        if (result == 1)
            return 1;   /* Secret word has been found */
    }
    
    return -1;  /* If reached end of function and secret word hasn't been found, return -1 */
}

int searchList(listPointer *start, char *bloomFilter, int k)
{
    int i, existsResult, counter = 0;
    char **words;
    char *w;
    uint64_t hashResults;
    
    while (*start != NULL)
    {
        w = deleteStart(start);     /* delete node, but keep word to check */
        existsResult = exists(w, bloomFilter, k);
        if (existsResult == 0)      /* If word hasn't been checked/added */
        {
            for (i = 0; i < k; i++) /* Pass word through k hash functions and update bloom filter */
            {
                hashResults = hash_by(i, w) % (size*8);
                bloomFilter[hashResults/8] |= 1 << (7-hashResults%8);
            }
            
            words = (char**)oracle(w);      /* Call oracle function for above word */
            printf("List::Trying word: %s - ", w);
            if (words == NULL)
            {
                printf("\nSecret word is %s.\n\n", w);
                free(w);
                return 1;
            }
            else
            {
                free(w);
                printf("not the secret word.\n");
                for (i = 0; words[i] != NULL; i++)  /* Add all words returned by oracle to list, at start, emulating DFS */
                {
                    insertStart(start, words[i]);
                }
            }
        }
        counter++;
    }
    
    return -1;
}