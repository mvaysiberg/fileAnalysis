#include <string.h>
#include <stdlib.h>
#include "structures.h"

//Calculate the bucket in the hash table that corresponds to the string
//It is assumed that string != NULL and len >= 0
//Returns the bucket for string in the hash table
int getBucket(char string[]) {
	int sum = 0;
	for(int i = 0; i < strlen(string); i++) {
		sum += string[i];
    }
	return sum % 1000;
}
//Insert the string into the hash table
//It is assumed that hashTable != NULL (guaranteed by fileHandle) and string != NULL (guaranteed by fileHandle)
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
///Searches hash table for string
//It is assumed that hashTable != NULL (guaranteed by fileHandle) and string != NULL (guaranteed by fileHandle)
//Returns the node if the token is in the hashtable and NULL otherwise
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
//Free all nodes in hash table
//It is assumed that hashTable != NULL (guaranteed by fileHandle)
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
//Inserts a token into a linked list in alphabetical order
//It is assumed that token is not NULL (guaranteed by fileHandle)
void insertToken(node* token, int totalTokens, node** head) {
	node* ptr = *head;
	node* prev = NULL;
	node* newNode = malloc(sizeof(node));
	newNode->string = token->string;
	newNode->count = token->count / totalTokens;
	while (ptr != NULL &&  strcmp(token->string, ptr->string) > 0){
	    prev = ptr;
	    ptr = ptr->next;
    }
    if(prev == NULL) {
	    newNode->next = *head;
        *head = newNode;
    }
    else {
        newNode->next = ptr;
        prev->next = newNode;
    }
}
//Inserts a node consisting of two files into a linked list which is sorted in increasing order by the nodes' total tokens
//It is assumed that first and second are not NULL (guaranteed by sortTotalTokens)
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
        else {
            *tail = newNode;
        }
    }
}
//Frees and joins all threads
void freeThread(threadNode* head){
    while(head!=NULL) {
        pthread_join(head->thread, NULL);
        threadNode* temp = head;
        head = head->next;
        free(temp);
    }
}
//Frees the 2-d linked list
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
//Frees a node
void freeNode(node* ptr){
    while (ptr != NULL){
        node* temp = ptr->next;
        free(ptr);
        ptr = temp;
    }
}
//Creates a linked list with every combination of two files in ascending order by the nodes' total tokens
//Assumed that ptr is not NULL (guaranteed by main)
void sortTotalTokens(nodePair ** totalTokens, parentNode* ptr){
        nodePair* totalTokensTail = NULL;
        while(ptr->next != NULL) { 
            parentNode* ptrNext = ptr->next;
            while(ptrNext != NULL) {
                int sum = ptr->count + ptrNext->count;
                insertPair(&totalTokensTail, sum, ptr, ptrNext);
                ptrNext = ptrNext->next;
                if(*totalTokens == NULL) {
                    *totalTokens = totalTokensTail;
                }
            }
            ptr = ptr->next;
        }
}