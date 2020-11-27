#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include "structures.h"


void* directoryHandle(void* input) {
    args *parameters = (args*)input;
    //burn through . and ..
    readdir(parameters->currDir);
    readdir(parameters->currDir);
    struct dirent* dir = NULL;
    while ((dir = readdir(parameters->currDir)) != NULL) {
        //concatenate previous file path to current file 
        int curDirL = strlen(parameters->dirName);
        char* filePath = malloc(curDirL + strlen(dir->d_name) + 2);
        filePath = strcpy(filePath,parameters->dirName);
        filePath[curDirL] = '/';
        strcat(filePath, dir->d_name);
        if (dir->d_type == DT_DIR) {
            args* arguments = malloc(sizeof(args));
            arguments->currDir = opendir(filePath);
            arguments->dirName = filePath;
            arguments->lock = parameters->lock;
            arguments->distributions = parameters->distributions;
            
            pthread_mutex_lock(parameters->lock);
            threadNode* newNode = malloc(sizeof(threadNode));
            newNode->next = NULL;
            (parameters->tail)->next = newNode;
            parameters->tail = newNode;
            arguments->tail = newNode;
            pthread_mutex_unlock(parameters->lock);
            pthread_create(&((parameters->tail)->thread), NULL, directoryHandle, (void*)arguments);
        }
        else if(dir->d_type == DT_REG) {
            pthread_mutex_lock(parameters->lock);
            threadNode* newNode = malloc(sizeof(threadNode));
            newNode->next = NULL;
            (parameters->tail)->next = newNode;
            parameters->tail = newNode;
            pthread_mutex_unlock(parameters->lock);
            file_args* fileArgs = malloc(sizeof(file_args));
            fileArgs->dirName = filePath;
            fileArgs->lock = parameters->lock;
            fileArgs->distributions = parameters->distributions;
            pthread_create(&((parameters->tail)->thread), NULL, fileHandle, (void*)fileArgs);
        }
    }
    free(parameters->dirName);
    free(parameters);
    pthread_exit(NULL);
}

int getBucket(char string[]) {
	int sum = 0;
	for(int i = 0; i < strlen(string); i++) {
		sum += string[i];
    }
	return sum % 1000;
}

void insertHash(node** hashTable, char string[]) {
	int bucket = getBucket(string);
    node* newNode = malloc(sizeof(node));
	newNode->string = string;
	newNode->count = 1;
	if(hashTable[bucket] == NULL) {
		newNode->next = NULL;
		hashTable[bucket] = newNode;
    }
	else {
		newNode->next = hashTable[bucket];
		hashTable[bucket] = newNode;
    }
}

node* searchHash(node** hashTable, char string[]) {
	int bucket = getBucket(string);
	node* ptr = hashTable[bucket];
	while (ptr != NULL) {
		if (strcmp(ptr->string, string) == 0) {
			return ptr;
        }
		ptr = ptr->next;
    }
	return NULL;
}

void freeHash(node** hashTable) {
	for (int i = 0; i < 1000; i++) {
		node* freePtr = hashTable[i];
		while(freePtr != NULL) {
			node* temp = freePtr->next;
			free(freePtr);
			freePtr = temp;
       	}
	}
}

void insertToken(node* token, int totalTokens, parentNode* head) {
	node* ptr = head->firstChild;
	node* prev = NULL;
	node* newNode = malloc(sizeof(node));
	newNode->string = token->string;
	newNode->count = token->count / totalTokens;
	while (ptr != NULL &&  strcmp(token->string, ptr->string) > 0){
	    prev = ptr;
	    ptr = ptr->next;
    }
    if(prev == NULL) {
	    newNode->next = head->firstChild;
        head->firstChild = newNode;
    }
    else {
        newNode->next = ptr;
        prev->next = newNode;
    }
}

void* fileHandle(void* input) {
	file_args *parameters = (file_args*)input;
    FILE* fp = fopen(parameters->dirName, "r");
    if(fp== NULL) {
        printf("File not accessible\n");
        free(parameters->dirName);
        free(parameters);
        pthread_exit(NULL);
    }
    node* hashTable[1000];
    for (int i = 0; i < 1000; i++) {
        hashTable[i] = NULL;
    }
    int totalTokens = 0;
    int maxSize = 10;
    char* token = malloc(maxSize);
    while(!feof(fp)){
        char c = fgetc(fp);
        token[0] = '\0';
	    int i = 0;
	    while(!isspace(c)){
		    i = 0;
	        if (i == maxSize-1){
		        realloc(token, 2*maxSize);
		        maxSize *= 2;
            }
		    if (isalpha(c) || c == '-'){
			    token[i] = tolower(c);
			    ++i;
            }
            c = fgetc(fp);
        }
        token[i+1] = '\0';
        if(strcmp(token, "") != 0) {
	        ++totalTokens;
            node* repeatToken = searchHash(hashTable, token);
            if (repeatToken == NULL){
                insertHash(hashTable, token);
            }
            else{
                repeatToken->count += 1;
            }
        }
    }
    free(token);
    fclose(fp);
    mutex_lock(parameters->lock);
    parentNode* curFile;
    if (totalTokens == 0){
	    printf("No tokens in file: %s",parameters->dirName);
	    free(parameters->dirName);
    }
    else {
        if(parameters->distributions == NULL) {
            parameters->distributions = malloc(sizeof(parentNode));
            (parameters->distributions)->next = NULL;
            (parameters->distributions)->string = parameters->dirName;
            (parameters->distributions)->count = totalTokens;
            (parameters->distributions)->firstChild = NULL;
            curFile = parameters->distributions;
        }
        else {
            parentNode* ptr = parameters->distributions;
            parentNode* prev = NULL;
            parentNode* newNode = malloc(sizeof(node*));
            newNode->string = parameters->dirName;
            newNode->count = totalTokens;
            newNode->firstChild = NULL;
            while(ptr != NULL && ptr->count < newNode->count){
                prev = ptr;
                ptr = ptr->next;
            }
            if(prev == NULL) {
                newNode->next = parameters->distributions;
                parameters->distributions = newNode;
            }
            else
            {
                newNode->next = ptr;
                prev->next = newNode;
            }
            curFile = newNode;
        }
    }
    mutex_unlock(parameters->lock);
	for (int i = 0; i < 1000; i++){
		node* ptr = hashTable[i];
		while (ptr != NULL){
	        insertToken(ptr, totalTokens, curFile);
        }
    }
    free(parameters);
    freeHash(hashTable);
    pthread_exit(NULL);
}

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

void insertPair(nodePair** tail, int sum, parentNode* first, parentNode* second) {

    nodePair* newNode = malloc(sizeof(nodePair));
    newNode->sum = sum;
    newNode->first = first;
    newNode->second = second;
    if (*tail == NULL){
        newNode->prev = NULL;
        newNode->next = NULL;
        *tail = newNode;
    }else{
        nodePair* prevPtr = *tail;
        while (prevPtr != NULL && newNode->sum < prevPtr->sum){
            prevPtr = prevPtr->prev;
        }
        nodePair* temp = prevPtr->next;
        prevPtr->next = newNode;
        newNode->next = temp;
        newNode->prev = prevPtr;
        if(temp != NULL){
            temp->prev = newNode;
        }
    }
}
node* mean(parentNode* file1, parentNode* file2) {
    node* output = NULL;
    node* ptr = NULL;
    node* ptr1 = file1->firstChild;
    node* ptr2 = file2->firstChild;
    while(ptr1 != NULL && ptr2 != NULL) {
        node* newNode = malloc(sizeof(node));
        newNode->next = NULL;
        if(output = NULL) {
            output = newNode;
            ptr = newNode;
        }
        else {
            ptr->next = newNode;
            ptr = ptr->next;
        }
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
    }
    while(ptr1 != NULL) {
        node* newNode = malloc(sizeof(node));
        newNode->next = NULL;
        ptr->next = newNode;
        ptr = ptr->next;
        newNode->string = ptr1->string;
        newNode->count = 0.5*ptr1->count;
        ptr1 = ptr1->next;
    }
    while(ptr2 != NULL) {
        node* newNode = malloc(sizeof(node));
        newNode->next = NULL;
        ptr->next = newNode;
        ptr = ptr->next;
        newNode->string = ptr2->string;
        newNode->count = 0.5*ptr2->count;
        ptr2 = ptr2->next;
    }
    return output;
}

double KLD(node* mean, parentNode* file) {
    double sum = 0.0;
    node* ptr = file->firstChild;
    while (ptr != NULL){
        while(strcmp(mean->string, ptr->string) != 0){
            mean = mean->next;
        }
        sum += ptr->count*log(ptr->count/mean->count);
        ptr = ptr->next;
    }
    return sum;
}

double JSD(parentNode* file1, parentNode* file2){
    node* meanptr = mean(file1, file2);
    double kld1 = kld(meanptr, file1);
    double kld2 = kld(meanptr, file2);
    return 0.5*(kld1 + kld2);
}

void freeThread(threadNode* head){
    while(head!=NULL) {
        pthread_join(head->thread, NULL);
        threadNode* temp = head;
        head = head->next;
        free(temp);
    }
}

void freeDistributions(parentNode* head){
    while (head != NULL){
        free(head->string);
        node* childptr = head->firstChild;
        while (childptr != NULL){
            node* temp = childptr->next;
            free(childptr);
            free(childptr->string);
            childptr = temp;
        }
        parentNode* temp = head->next;
        free(head);
        head = temp;
    }
}


int main(int argc, char* argv[]){
    if (argc == 1){
        printf("No input");
        exit(0);
    }else if (argc > 2){
        printf("More than one directory");
        exit(0);
    }
    DIR* currDir = opendir(argv[1]);
    if (currDir == NULL){
        printf("invalid file\n");
        exit(0);
    }
    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);

    threadNode* head = malloc(sizeof(threadNode));
    head->next = NULL;
    args* arguments = malloc(sizeof(args));
    parentNode* distributions = NULL;

    arguments->currDir = currDir;
    arguments->dirName = malloc(strlen(argv[1]) + 1);
    strcpy(arguments->dirName, argv[1]);
    arguments->lock = mutex;
    arguments->tail = head;
    arguments->distributions = distributions;

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
        nodePair* totalTokensTail = NULL;
        parentNode* ptr = distributions;
        while(ptr->next != NULL) { 
            parentNode* ptrNext = ptr->next;
            while(ptrNext != NULL) {
                int sum = ptr->count + ptrNext->count;
                insertPair(&totalTokensTail, sum, ptr, ptrNext);
                ptrNext = ptrNext->next;
                if(totalTokens == NULL) {
                    totalTokens = totalTokensTail;
                }
            }
            ptr = ptr->next;
        }
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
}