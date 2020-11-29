#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "structures.h"
#include "parse.h"

double JSD(parentNode* file1, parentNode* file2);
void printJSD(double value, char* file1, char* file2);
node* mean(parentNode* file1, parentNode* file2);
double KLD(node* mean, parentNode* file);


//Takes a directory from STDIN, initializes the token handling process, and calls functions to print the output
//Checks the following errors: no input, more than one input, and invalid input, only 0 or 1 readable files in the directory structure, and prints a warning
int main(int argc, char* argv[]){
    if (argc == 1){
        printf("No input\n");
        exit(0);
    }else if (argc > 2){
        printf("More than one directory\n");
        exit(0);
    }
    DIR* currDir = opendir(argv[1]);
    if (currDir == NULL){
        printf("Invalid directory\n");
        exit(0);
    }
    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    pthread_mutex_t *distributionsMutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(distributionsMutex, NULL);

    threadNode* head = malloc(sizeof(threadNode));
    head->next = NULL;
    args* arguments = malloc(sizeof(args));
    parentNode* distributions = NULL;

    arguments->currDir = currDir;
    arguments->dirName = malloc(strlen(argv[1]) + 1);
    if (argv[1][0] == '\"'){
    	strncpy(arguments->dirName, argv[1] + 1, strlen(argv[1])-2);
    }else{
    	strcpy(arguments->dirName, argv[1]);
    }
    arguments->lock = mutex;
    arguments->distributionsLock = distributionsMutex;
    arguments->tail = head;
    arguments->distributions = &distributions;

    pthread_create(&(head->thread), NULL, directoryHandle, (void*)arguments);
    
    freeThread(head);


    if(distributions == NULL) {
        printf("No files\n");
    }
    else if(distributions->next == NULL) {
        printf("Only one file\n");
    }
    else {
        nodePair* totalTokens = NULL;
        sortTotalTokens(&totalTokens,distributions);
        while(totalTokens != NULL) {
            printJSD(JSD(totalTokens->first, totalTokens->second), totalTokens->first->string, totalTokens->second->string);
	        nodePair* temp = totalTokens->next;
            free(totalTokens);
            totalTokens = temp;
        }
    }
    freeDistributions(distributions);
    pthread_mutex_destroy(mutex);
    free(mutex);
    pthread_mutex_destroy(distributionsMutex);
    free(distributionsMutex);
}
//Computes JSD of the two files
//It is assumed that file1 and file2 are valid linked lists if they are not NULL(guaranteed by fileHandle)
double JSD(parentNode* file1, parentNode* file2){
    node* meanptr = mean(file1, file2);
    double kld1 = KLD(meanptr, file1);
    double kld2 = KLD(meanptr, file2);
    freeNode(meanptr);
    return 0.5*(kld1 + kld2);
}
//Prints formatted JSD
//Assumed file1 and file2 are not NULL(guaranteed by directoryHandle)
void printJSD(double value, char* file1, char* file2) {
    if(value >= 0 && value <= 0.1) {
        printf("\033[0;31m");
    }
    else if(value <= 0.15) {
        printf("\033[0;33m");
    }
    else if(value <= 0.2) {
        printf("\033[0;32m");
    }
    else if(value <= 0.25) {
        printf("\033[0;36m");
    }
    else if(value <= 0.3) {
        printf("\033[0;34m");
    }
    printf("%lf\033[0m \"%s\" and \"%s\"\n", value, file1, file2);

}
//Creates a sorted linked list in increasing order by total tokens that contains the mean of two token distributions
//It is assumed that file1 and file2 are valid linked lists if they are not NULL(guaranteed by fileHandle)
node* mean(parentNode* file1, parentNode* file2) {
    node* output = NULL;
    node* ptr = NULL;
    node* ptr1 = file1->firstChild;
    node* ptr2 = file2->firstChild;
    while(ptr1 != NULL && ptr2 != NULL) {
        node* newNode = malloc(sizeof(node));
        newNode->next = NULL;
        if(strcmp(ptr1->string, ptr2->string) == 0) {
            newNode->string = ptr1->string;
            newNode->count = 0.5*(ptr1->count + ptr2->count);
            ptr1 = ptr1->next;
            ptr2 = ptr2->next;

        }
        else if(strcmp(ptr1->string, ptr2->string) < 0) {
            newNode->string = ptr1->string;
            newNode->count = 0.5*ptr1->count;
            ptr1 = ptr1->next;
        }
        else {
            newNode->string = ptr2->string;
            newNode->count = 0.5*ptr2->count;
            ptr2 = ptr2->next;
        }
        if(output == NULL) {
            output = newNode;
            ptr = newNode;
        }
        else {
            ptr->next = newNode;
            ptr = ptr->next;
        }

    }
    while(ptr1 != NULL) {
        node* newNode = malloc(sizeof(node));
        newNode->next = NULL;
        newNode->string = ptr1->string;
        newNode->count = 0.5*ptr1->count;
        ptr1 = ptr1->next;
        if(output == NULL) {
            output = newNode;
            ptr = newNode;
        }
        else {
            ptr->next = newNode;
            ptr = ptr->next;
        }
    }
    while(ptr2 != NULL) {
        node* newNode = malloc(sizeof(node));
        newNode->next = NULL;
        newNode->string = ptr2->string;
        newNode->count = 0.5*ptr2->count;
        ptr2 = ptr2->next;
        if(output == NULL) {
            output = newNode;
            ptr = newNode;
        }
        else {
            ptr->next = newNode;
            ptr = ptr->next;
        }
    }
    return output;
}
//Computes the KLD for each file
//It is assumed that file is not NULL (guaranteed by fileHandle)
double KLD(node* mean, parentNode* file) {
    if(mean == NULL) {
        return 0;
    }
    double sum = 0.0;
    node* ptr = file->firstChild;
    while (ptr != NULL){
        while(strcmp(mean->string, ptr->string) != 0){
            mean = mean->next;
        }
        sum += ptr->count*log10(ptr->count/mean->count);
        ptr = ptr->next;
    }
    return sum;
}
