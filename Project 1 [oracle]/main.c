#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash.h"
#include "oracle.h"
#include "oracleFunctions.h"

int size;

int main(int argc, const char * argv[])
{
    int depth, result, k;
    char *bloomFilter;
    pointer root;
    listPointer start;

    if (argc < 3)
    {
        printf("Not enough arguments to execute program.\n");
        exit(-1);
    }

    if (argc == 3)
    {
        size = atoi(argv[1]);
        depth = atoi(argv[2]);
        k = 3;      /* Default value for k hash functions */
    }
    else
    {
        k = atoi(argv[2]);
        size = atoi(argv[3]);
        depth = atoi(argv[4]);
    }

    initSeed(29);   /* Initialize oracle function(s) */

    root = malloc(sizeof(node));
    if (root == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }

    root->word = malloc(sizeof(char)*50);
    if (root->word == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }
    
    strcpy(root->word, "");     /* First word searched will be empty string */
    bloomFilter = malloc(sizeof(char)*size);
    if (bloomFilter == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }

    memset(bloomFilter, 0, sizeof(char)*size);      /* Initialize bloom filter to all 0s */
    start = NULL;
    /* Start searching for secret word */
    result = searchWords(root, depth, bloomFilter, k, &start);
    /* If result != 1, secret word wasn't found in tree, search list */
    if (result != 1)
    {
       result = searchList(&start, bloomFilter, k);
    }
    /* If searchList returns != 1, secret word couldn't be found */
    if (result != 1)
        printf("Could not find secret word :( \n");
    
    /* Print tree allocated in pre-order format */
    preorderTree(root);
    /* Deallocate memory */
    destroyTree(&root);
    destroyList(&start);
    free(bloomFilter);
    
    return 0;
}
