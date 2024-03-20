#include "hash-table.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


unsigned int hash(char *str)
{
	unsigned int hash = 5381;
	unsigned int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

/*
 * LINKED LIST FUNCTIONS
 *
 */

struct LinkedList *LinkedList_New()
{
	struct LinkedList *wip = malloc(sizeof(struct LinkedList));
	wip->head = NULL;
	wip->tail = NULL;
	wip->size = 0;
	return wip;
}

void LinkedList_Free(struct LinkedList *l, void (*dataFreeFunction)(void *))
{
	struct LinkedListNode *runner = l->head;
	while (runner != NULL)
	{
		if (dataFreeFunction != NULL)
		{
			dataFreeFunction(runner->data);
		}
		struct LinkedListNode *old = runner;
		runner = runner->next;
		free(old);
	}
	free(l);
}

void LinkedList_Append(struct LinkedList *l, void *element)
{
	if (element == NULL)
	{
		perror("Attempt to append data with null pointer into LinkedList!");
		exit(1);
	}

	struct LinkedListNode *newNode = malloc(sizeof(struct LinkedListNode));
	newNode->data = element;
	if (l->size == 0)
	{
		newNode->next = NULL;
		newNode->prev = NULL;
		l->head = newNode;
		l->tail = newNode;
	}
	else
	{
		l->tail->next = newNode;
		newNode->prev = l->tail;
		newNode->next = NULL;
		l->tail = newNode;
	}
	l->size++;
}

void LinkedList_Prepend(struct LinkedList *l, void *element)
{
	if (element == NULL)
	{
		perror("Attempt to prepend data with null pointer into LinkedList!");
		exit(1);
	}

	struct LinkedListNode *newNode = malloc(sizeof(struct LinkedListNode));
	newNode->data = element;
	if (l->size == 0)
	{
		newNode->next = NULL;
		newNode->prev = NULL;
		l->head = newNode;
		l->tail = newNode;
	}
	else
	{
		l->head->prev = newNode;
		newNode->next = l->head;
		l->head = newNode;
	}
	l->size++;
}

void LinkedList_Join(struct LinkedList *before, struct LinkedList *after)
{
	for (struct LinkedListNode *runner = after->head; runner != NULL; runner = runner->next)
	{
		LinkedList_Append(before, runner->data);
	}
}

void *LinkedList_Delete(struct LinkedList *l, int (*compareFunction)(void *, void *), void *element)
{
	for (struct LinkedListNode *runner = l->head; runner != NULL; runner = runner->next)
	{
		if (!compareFunction(runner->data, element))
		{
			if (l->size > 1)
			{
				if (runner == l->head)
				{
					l->head = runner->next;
					runner->next->prev = NULL;
				}
				else
				{
					if (runner == l->tail)
					{
						l->tail = runner->prev;
						runner->prev->next = NULL;
					}
					else
					{
						runner->prev->next = runner->next;
						runner->next->prev = runner->prev;
					}
				}
			}
			else
			{
				l->head = NULL;
				l->tail = NULL;
			}
			void *data = runner->data;
			free(runner);
			l->size--;
			return data;
		}
	}
	perror("Couldn't delete element from linked list!\n");
    abort();
}

void *LinkedList_Find(struct LinkedList *l, int (*compareFunction)(void *, void *), void *element)
{
	for (struct LinkedListNode *runner = l->head; runner != NULL; runner = runner->next)
	{
		if (!compareFunction(runner->data, element))
		{
			return runner->data;
		}
	}
	return NULL;
}

void *LinkedList_PopFront(struct LinkedList *l)
{
	if (l->size == 0)
	{
		perror("Unable to pop front from empty linkedlist!\n");
        abort();
	}
	struct LinkedListNode *popped = l->head;

	l->head = l->head->next;
	if (l->head != NULL)
	{
		l->head->prev = NULL;
	}
	else
	{
		l->tail = NULL;
	}
	l->size--;

	void *poppedData = popped->data;
	free(popped);

	return poppedData;
}

void *LinkedList_PopBack(struct LinkedList *l)
{
	if (l->size == 0)
	{
        perror("Unable to pop front from empty linkedlist!\n");
        abort();
	}
	struct LinkedListNode *popped = l->tail;

	l->tail = l->tail->prev;
	l->tail->next = NULL;
	l->size--;

	void *poppedData = popped->data;
	free(popped);

	return poppedData;
}
/*

STACK FUNCTIONS

*/

struct Stack *Stack_New()
{
	struct Stack *wip = malloc(sizeof(struct Stack));
	wip->data = malloc(20 * sizeof(void *));
	wip->size = 0;
	wip->allocated = 20;
	return wip;
}

void Stack_Free(struct Stack *s)
{
	free(s->data);
	free(s);
}

void Stack_Push(struct Stack *s, void *data)
{
	if (s->size >= s->allocated)
	{
		void **newData = malloc((int)(s->allocated * 1.5) * sizeof(void *));
		memcpy(newData, s->data, s->allocated * sizeof(void *));
		free(s->data);
		s->data = newData;
		s->allocated = (int)(s->allocated * 1.5);
	}

	s->data[s->size++] = data;
}

void *Stack_Pop(struct Stack *s)
{
	if (s->size > 0)
	{
		return s->data[--s->size];
	}
	else
	{
		printf("Error - attempted to pop from empty stack!\n");
		exit(1);
	}
}

void *Stack_Peek(struct Stack *s)
{
	if (s->size > 0)
	{
		return s->data[s->size - 1];
	}
	else
	{
		printf("Error - attempted to peek empty stack!\n");
		exit(1);
	}
}

// hash table
struct HashTable *HashTable_New(int nBuckets)
{
	struct HashTable *wip = malloc(sizeof(struct HashTable));
	wip->nBuckets = nBuckets;
	wip->buckets = malloc(nBuckets * sizeof(struct LinkedList *));

	for (int i = 0; i < nBuckets; i++)
	{
		wip->buckets[i] = LinkedList_New();
	}

	return wip;
}

struct HashTableEntry *HashTable_Insert(struct HashTable *ht, char *key, void *value)
{
    struct HashTableEntry *newEntry = malloc(sizeof(struct HashTableEntry));
	newEntry->key = key;
    newEntry->value = value;
    newEntry->hash = hash(key);

	LinkedList_Append(ht->buckets[newEntry->hash % ht->nBuckets], newEntry);

	return newEntry;
}

struct HashTableEntry *HashTable_Lookup(struct HashTable *ht, char *key)
{
	unsigned int strHash = hash(key);

	struct LinkedList *bucket = ht->buckets[strHash % ht->nBuckets];
	if (bucket->size == 0)
	{
		return NULL;
	}

	struct LinkedListNode *runner = bucket->head;

	while (runner != NULL)
	{
        struct HashTableEntry *examinedEntry = runner->data;
		if ((examinedEntry->hash == strHash) && (strcmp(examinedEntry->key, key) == 0))
		{
			return examinedEntry;
		}

		runner = runner->next;
	}
	return NULL;
}

// struct HashTableEntry *HashTable_LookupOrInsert(struct HashTable *ht, char *value)
// {
// 	struct HashTableEntry *lookupResult = HashTable_Lookup(ht, value);
// 	if (lookupResult != NULL)
// 	{
// 		return lookupResult;
// 	}
// 	else
// 	{
// 		return HashTable_Insert(ht, key, value);
// 	}
// }

int compareHashTableEntries(void *a, void *b)
{
	return strcmp((((struct HashTableEntry *)a)->key), b);
}

void HashTable_Remove(struct HashTable *ht, char *key, void(*freeDataFunction)(void *))
{
	unsigned int strHash = hash(key);

	struct LinkedList *bucket = ht->buckets[strHash % ht->nBuckets];
	if (bucket->size == 0)
	{
		printf("attempt to delete nonexistent hash table element with key %s\n", key);
	}

	void *data = LinkedList_Delete(bucket, compareHashTableEntries, key);
	if(freeDataFunction != NULL)
	{
		freeDataFunction(data);
	}
}

void HashTable_Free(struct HashTable *ht)
{
	for (int i = 0; i < ht->nBuckets; i++)
	{
		LinkedList_Free(ht->buckets[i], free);
	}
	free(ht->buckets);
	free(ht);
}
