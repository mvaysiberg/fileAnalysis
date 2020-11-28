#include <string.h>
#include <stdlib.h>
#include "structures.h"

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

void insertToken(node* token, int totalTokens, parentNode** head) {
	node* ptr = (*head)->firstChild;
	node* prev = NULL;
	node* newNode = malloc(sizeof(node));
	newNode->string = token->string;
	newNode->count = token->count / totalTokens;
	while (ptr != NULL &&  strcmp(token->string, ptr->string) > 0){
	    prev = ptr;
	    ptr = ptr->next;
    }
    if(prev == NULL) {
	    newNode->next = (*head)->firstChild;
        (*head)->firstChild = newNode;
    }
    else {
        newNode->next = ptr;
        prev->next = newNode;
    }
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
            free(childptr->string);
	        free(childptr);
            childptr = temp;
        }
        parentNode* temp = head->next;
        free(head);
        head = temp;
    }
}

void freeNode(node* ptr){
    while (ptr != NULL){
        node* temp = ptr->next;
        free(ptr);
        ptr = temp;
    }
}
