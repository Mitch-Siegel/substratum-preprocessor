#include "macro-definitions.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct Macro
{
    char *inVal;
    char *outVal;
};

int longestKeyword = 0;

// recalculate which keyword is longest, regenerated the sorted list of keywords by length
void handleDefineChange(struct PreprocessorContext *c)
{
    longestKeyword = 0;
    while (c->keywordsByLength->size > 0)
    {
        Stack_Pop(c->keywordsByLength);
    }

    // scan through #define-d keywords
    for (int i = 0; i < c->defines->nBuckets; i++)
    {
        struct LinkedList *examinedBucket = c->defines->buckets[i];
        for (struct LinkedListNode *examinedDefine = examinedBucket->head; examinedDefine != NULL; examinedDefine = examinedDefine->next)
        {
            struct HashTableEntry *e = examinedDefine->data;
            struct Macro *m = e->value;
            Stack_Push(c->keywordsByLength, m->inVal);
        }
    }

    // bubble sort all keywords in descending order (longest at index 0 of data)
    for (int i = 0; i < c->keywordsByLength->size; i++)
    {
        for (int j = 0; j < c->keywordsByLength->size - i - 1; j++)
        {
            char *thisKw = c->keywordsByLength->data[j];

            int thisLen = strlen(thisKw);

            if (thisLen > longestKeyword)
            {
                longestKeyword = thisLen;
            }

            int compLen = strlen((char *)c->keywordsByLength->data[j + 1]);

            if (thisLen > compLen)
            {
                struct Lifetime *swap = c->keywordsByLength->data[j];
                c->keywordsByLength->data[j] = c->keywordsByLength->data[j + 1];
                c->keywordsByLength->data[j + 1] = swap;
            }
        }
    }
}

struct Macro *findMacro(struct PreprocessorContext *c)
{
    char *strBuf = malloc(c->bufLen + 1);
    memcpy(strBuf, c->inBuf, c->bufLen);
    strBuf[c->bufLen] = '\0';

    for (int i = 0; i < c->keywordsByLength->size; i++)
    {
        char *comparedKeyword = c->keywordsByLength->data[i];
        if (strncmp(c->inBuf, comparedKeyword, strlen(comparedKeyword)) == 0)
        {
            free(strBuf);
            return HashTable_Lookup(c->defines, comparedKeyword)->value;
        }
    }
    free(strBuf);
    return NULL;
}

void handleMacro(struct PreprocessorContext *c, struct Macro *toOutput)
{
    int printedLen = strlen(toOutput->inVal);
    c->bufLen -= printedLen;
    memmove(c->inBuf, c->inBuf + printedLen, c->bufLen);
    bufferInsertFront(c, toOutput->outVal);
}

// returns number of macros expanded
int attemptMacroSubstitutionRecursive(struct PreprocessorContext *c, char stillParsing, int depth)
{
    if(depth > 0xFF)
    {
        perror("Recursion limit on macro substitution!\n");
        abort();
    }

    // early return if there is more input (put it into the buffer so we don't accidentally greedily match a prefix of a keyword)
    if (stillParsing && (c->bufLen < longestKeyword))
    {
        return 0;
    }

    struct Macro *m = findMacro(c);

    // early return if no macro found
    if (m == NULL)
    {
        fputc(bufferConsume(c), c->outFile);
        return 0;
    }

    handleMacro(c, m);
    return 1 + attemptMacroSubstitutionRecursive(c, stillParsing, depth + 1);
}

void attemptMacroSubstitution(struct PreprocessorContext *c, char stillParsing)
{
    int nExpansions = 0;
    int lastExpansions = 0;

    while((nExpansions += attemptMacroSubstitutionRecursive(c, stillParsing, nExpansions)) > lastExpansions)
    {
        lastExpansions = nExpansions;
    }
}

void defineMacro(struct PreprocessorContext *c, char *token, char *outVal)
{
    struct Macro *newMacro = malloc(sizeof(struct Macro));
    newMacro->inVal = strdup(token);
    newMacro->outVal = strdup(outVal);
    HashTable_Insert(c->defines, newMacro->inVal, newMacro);
    handleDefineChange(c);
}

void undefineMacro(struct PreprocessorContext *c, char *token)
{
    struct HashTableEntry *e = HashTable_Lookup(c->defines, token);
    if(e != NULL)
    {
        HashTable_Remove(c->defines, token, free);
        handleDefineChange(c);
    }
}
