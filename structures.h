#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
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
    parentNode** distributions;
} args;

typedef struct _file_args {
    char* dirName;
    pthread_mutex_t* lock;
    parentNode** distributions;
} file_args;
int getBucket(char string[]);
void insertHash(node** hashTable, char string[]);
node* searchHash(node** hashTable, char string[]);
void freeHash(node** hashTable);
void insertToken(node* token, int totalTokens, node** head);
void insertPair(nodePair** tail, int sum, parentNode* first, parentNode* second);
void freeThread(threadNode* head);
void freeDistributions(parentNode* head);
void freeNode(node* ptr);

#endif