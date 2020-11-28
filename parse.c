#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include "structures.h"
#include "parse.h"

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
        filePath[curDirL+1] = '\0';
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
            file_args* fileArgs = malloc(sizeof(file_args));
            fileArgs->dirName = filePath;
            fileArgs->lock = parameters->lock;
            fileArgs->distributions = parameters->distributions;
            
            pthread_mutex_lock(parameters->lock);
            threadNode* newNode = malloc(sizeof(threadNode));
            newNode->next = NULL;
            (parameters->tail)->next = newNode;
            parameters->tail = newNode;
            pthread_mutex_unlock(parameters->lock);

            pthread_create(&((parameters->tail)->thread), NULL, fileHandle, (void*)fileArgs);
        }
        else {
            free(filePath);
        }
    }
    free(parameters->dirName);
    free(parameters);
    pthread_exit(NULL);
}

void* fileHandle(void* input) {
	file_args *parameters = (file_args*)input;
    FILE* fp = fopen(parameters->dirName, "r");
    if(fp== NULL) {
        printf("File not accessible: %s\n", parameters->dirName);
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
	        if (i == maxSize-1){
		        token = realloc(token, 2*maxSize);
		        maxSize *= 2;
            }
		    if (isalpha(c) || c == '-'){
			    token[i] = tolower(c);
			    ++i;
            }
            c = fgetc(fp);
            if(feof(fp)) {
                break;
            }
        }
        token[i] = '\0';
        if(strcmp(token, "") != 0) {
	        ++totalTokens;
            node* repeatToken = searchHash(hashTable, token);
            if (repeatToken == NULL){
                int tokenLength = strlen(token);
                char* tokenCopy = malloc(tokenLength + 1);
                strcpy(tokenCopy, token);
                tokenCopy[tokenLength] = '\0';
                insertHash(hashTable, tokenCopy);
            }
            else{
                repeatToken->count += 1;
            }
        }
    }
    free(token);
    fclose(fp);
    
    pthread_mutex_lock(parameters->lock);
    parentNode** curFile;
    if(*(parameters->distributions) == NULL) {
        *(parameters->distributions) = malloc(sizeof(parentNode));
        (*(parameters->distributions))->next = NULL;
        (*(parameters->distributions))->string = parameters->dirName;
        (*(parameters->distributions))->count = totalTokens;
        (*(parameters->distributions))->firstChild = NULL;
        curFile = parameters->distributions;
    }
    else {
        parentNode* ptr = *(parameters->distributions);
        parentNode* prev = NULL;
        parentNode* newNode = malloc(sizeof(parentNode));
        newNode->string = parameters->dirName;
        newNode->count = totalTokens;
        newNode->firstChild = NULL;
        while(ptr != NULL && ptr->count < newNode->count){
            prev = ptr;
            ptr = ptr->next;
        }
        if(prev == NULL) {
            newNode->next = *(parameters->distributions);
            *(parameters->distributions) = newNode;
        }
        else
        {
            newNode->next = ptr;
            prev->next = newNode;
        }
        curFile = &newNode;
    }
    pthread_mutex_unlock(parameters->lock);
    
	for(int i = 0; i < 1000; i++){
		node* ptr = hashTable[i];
		while (ptr != NULL){
	        insertToken(ptr, totalTokens, curFile);
            ptr = ptr->next;
        }
    }
    free(parameters);
    freeHash(hashTable);
    pthread_exit(NULL);
}
