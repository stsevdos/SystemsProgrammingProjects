#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "oracleFunctions.h"
#include "hash.h"
#include "oracle.h"

#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

// Synarthseis diaxeirishs listwn, morfopoihmenes apo to mathhma Domes Dedomenwn
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

// Deletes node but keeps/returns word in node
char* deleteStart(listPointer *start)
{
    char *w;
    listPointer temp;
    
    temp = *start;
    if (temp != NULL)
    {
        w = malloc(strlen(temp->word) + 1);
        strcpy(w , temp->word);
        *start = temp->nextNode;
        free(temp->word);
        free(temp);
        return w;
    }
    else
        return NULL;
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

// Used to sort hash_by results in order to lock bloom filter sections
void selectionSort(uint64_t *hashResults, int k)
{
    int i, j;
    uint64_t temp;
    
    for (i = 0; i < k - 1; i++)
    {
        for (j = i+1; j < k; j++)
        {
            if (hashResults[i] > hashResults[j])
            {
                temp = hashResults[i];
                hashResults[i] = hashResults[j];
                hashResults[j] = temp;
            }
        }
    }
}

// Checks if a word (specific combination of bits - hash_by) has been passed through bloom filter
int exists(char *word, char *bloomFilter, int k)
{
    int i, result;
    uint64_t *hashResults;
    int mutexNumber = -1;
    int currentMutexNumber;
    
    result = 0;
    hashResults = malloc(sizeof(uint64_t)*k);
    if (hashResults == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }
    
    // Pass word through all k hash functions
    for (i = 0; i < k; i++)
    {
        hashResults[i] = hash_by(i, word)%(size*8);
    }
    
    selectionSort(hashResults, k);
    
    // Pass word through all k hash functions
    for (i = 0; i < k; i++)
    {
        currentMutexNumber = (int)hashResults[i]/(64*8);    // Which mutex to lock
        if (currentMutexNumber != mutexNumber)
        {
            if (mutexNumber != -1)  // Unlock mutex
                pthread_mutex_unlock(&mutexArray[mutexNumber]);
            pthread_mutex_lock(&mutexArray[currentMutexNumber]);
            mutexNumber = currentMutexNumber;
        }
        
        if (bloomFilter[hashResults[i]/8] & (1 << (7-hashResults[i]%8)))
            result++;     // bit exists (is 1)
        else    // bit is 0, change it to 1
            bloomFilter[hashResults[i]/8] = bloomFilter[hashResults[i]/8] | (1 << (7-hashResults[i]%8));
    }
    
    if (mutexNumber != -1)
        pthread_mutex_unlock(&mutexArray[mutexNumber]);
    
    free(hashResults);
    
    if (result == k)
        return 1;   // all bits for word exist in bloom filter
    
    return 0;   // at least one bit for word was zero in bloom filter
}

// Creates initial word for each thread to start searching
char *initialWord()
{
    char *word;
    int i;
    
    word = malloc(sizeof(char)*(INITIALSIZE+1));
    
    if (word == NULL)
    {
        perror("malloc failed");
        exit(-1);
    }
    
    for (i = 0; i < INITIALSIZE; i++)
    {
        word[i] = rand_r(&rseed)%26 + 65; //[65-90] only capital letters, piazza
    }
    word[INITIALSIZE] = '\0';
    
    //printf("Thread's [%d] initialWord is: %s\n", (int)pthread_self(), word);
    
    return word;
}

char *searchList(listPointer *start, char *bloomFilter, int k)
{
    int i, j, existsResult, counter;
    int checkStop, wordsChecked, wordsInFilter;
    char **words, *word, *w;
    
    j = 0; counter = 0;
    checkStop = 0; wordsChecked = 0; wordsInFilter = 0;
    
    for (j = 0; j < threadRestart; j++)
    {
        if (checkStop == WHENTOCHECK)   // Check if secret word has been found by another thread
        {
            checkStop = 0;
            if (stopThreads == 1)  // Thread must write to file and stop searching
            {
                pthread_mutex_lock(&mutexForFile);
                fprintf(fp, "Thread [%d] [Stopped]\n", (int)pthread_self());
                fprintf(fp, "Words Tried: %d\n", wordsChecked);
                fprintf(fp, "Percentage in BF %f\%%\n", ((float)wordsInFilter/(float)wordsChecked)*100.00);
                pthread_mutex_unlock(&mutexForFile);
                return NULL;
            }
        }
        else
            checkStop++;
        
        word = initialWord();   // Get (new) initial word
        wordsChecked++;
        
        existsResult = exists(word, bloomFilter, k);
        if (existsResult != 0)  // Word has been passed through BF
        {
            wordsInFilter++;
            words = NULL;
        }
        else    // Word hasn't been passed through BF, call oracle for this word
            words = (char**)oracle(word);
        
        if (words == NULL ) // Found secret word // || words[0] == NULL
        {
            printf("\nThread [%d] found the secret word (%s).\n\n", (int)pthread_self(), word);
            pthread_mutex_lock(&mutexForFile);
            fprintf(fp, "Thread [%d] [Winner]\n", (int)pthread_self());
            fprintf(fp, "Words Tried: %d\n", wordsChecked);
            fprintf(fp, "Percentage in BF %f\%%\n", ((float)wordsInFilter/(float)wordsChecked)*100.00);
            pthread_mutex_unlock(&mutexForFile);
            stopThreads = 1;   // All threads must stop searching
            return word;
        }
        else
            printf("Random %s is not the secret word.\n", word);
        
        free(word);
        
        for (i = 0; words[i] != NULL; i++)
        {
            wordsChecked++;
            existsResult = exists(words[i], bloomFilter, k);
            if (existsResult == 0)
                insertStart(start, words[i]); // If word hasn't been checked/added
            else
                wordsInFilter++;
            
            free(words[i]);
        }
        
        free(words);
        
        while (*start != NULL)  // *** SEARCH WORDS IN LIST ***
        {
            if (checkStop == WHENTOCHECK)   // Check if secret word has been found by another thread
            {
                checkStop = 0;
                if (stopThreads == 1)  // Thread must write to file and stop searching
                {
                    pthread_mutex_lock(&mutexForFile);
                    fprintf(fp, "Thread [%d] [Stopped]\n", (int)pthread_self());
                    fprintf(fp, "Words Tried: %d\n", wordsChecked);
                    fprintf(fp, "Percentage in BF %f\%%\n", ((float)wordsInFilter/(float)wordsChecked)*100.00);
                    pthread_mutex_unlock(&mutexForFile);
                    return NULL;
                }
            }
            else
                checkStop++;
            
            w = deleteStart(start);     // delete node, but keep word to check
            if (w == NULL)
                printf("Something went wrong...\n");

            words = (char**)oracle(w);      // Call oracle function for above word
            
            if (words == NULL ) //|| words[0] == NULL
            {
                pthread_mutex_lock(&mutexForFile);
                fprintf(fp, "Thread [%d] [Winner]\n", (int)pthread_self());
                fprintf(fp, "Words Tried: %d\n", wordsChecked);
                fprintf(fp, "Percentage in BF %f\%%\n", ((float)wordsInFilter/(float)wordsChecked)*100.00);
                pthread_mutex_unlock(&mutexForFile);
                stopThreads = 1;   // All threads must stop searching
                return w;
            }
            else
            {
                printf("Thread [%d] tried %s and is not the secret word \n", (int)pthread_self(), w);
                free(w);
                
                for (i = 0; words[i] != NULL; i++)
                {
                    wordsChecked++;
                    existsResult = exists(words[i], bloomFilter, k);
                    if(existsResult == 0){
                        insertStart(start, words[i]);
                    }
                    else
                        wordsInFilter++;
                    free(words[i]);
                }
                free(words);
            }
            
            counter++;
        }
    }

    // At this point, thread has restarted L (threadRestart) times and hasn't been able to find secret word
    pthread_mutex_lock(&mutexForFile);
    //printf("wordChecked = %d\nwordsInFilter = %d\n", wordsChecked, wordsInFilter);
    fprintf(fp, "Thread [%d] [Loser]\n", (int)pthread_self());
    fprintf(fp, "Words Tried: %d\n", wordsChecked);
    fprintf(fp, "Percentage in BF %f\%%\n", ((float)wordsInFilter/(float)wordsChecked)*100.00);
    pthread_mutex_unlock(&mutexForFile);
    return NULL;
}