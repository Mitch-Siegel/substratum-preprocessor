
#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

struct LinkedListNode
{
	struct LinkedListNode *next;
	struct LinkedListNode *prev;
	void *data;
};

struct LinkedList
{
	struct LinkedListNode *head;
	struct LinkedListNode *tail;
	int size;
};

struct HashTableEntry
{
	char *key;
	void *value;
	unsigned hash;
};

struct HashTable
{
	struct LinkedList **buckets;
	int nBuckets;
};

struct Stack
{
	void **data;
	int size;
	int allocated;
};

struct Stack *Stack_New();

void Stack_Free(struct Stack *s);

void Stack_Push(struct Stack *s, void *data);

void *Stack_Pop(struct Stack *s);

struct LinkedList *LinkedList_New();

void LinkedList_Free(struct LinkedList *l, void (*dataFreeFunction)(void *));

void LinkedList_Append(struct LinkedList *l, void *element);

struct HashTable *HashTable_New(int nBuckets);

struct HashTableEntry *HashTable_Insert(struct HashTable *ht, char *key, void *value);

struct HashTableEntry *HashTable_Lookup(struct HashTable *ht, char *key);

void HashTable_Free(struct HashTable *ht);

#endif
