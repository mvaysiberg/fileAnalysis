#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <cmath.h>

typedef struct _node{
   char* string;
   double count;
   struct _node* next;
} node;

typedef struct _parentNode{
   char* string;
   int count;
   struct _parentNode* next;
  node* firstChild; 
} parentNode;

typedef struct _nodePair{
	parentNode* first;
	parentNode* second;
	int sum;
	struct _nodePair* next;
	struct _nodePair* prev; 
} nodePair;

typedef struct _threadNode{
	struct _threadNode* next;
	pthread_t thread;
} threadNode;

typedef struct arg_struct {
    DIR* currDir;
    char* dirName;
    pthread_mutex_t* lock;
    threadNode* tail;
    parentNode* distributions;
} args;

typedef struct _file_args {
    char* dirName;
    pthread_mutex_t* lock;
    parentNode* distributions;
} file_args;

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
	int bucket = getBucket(string, strlen(string));
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
