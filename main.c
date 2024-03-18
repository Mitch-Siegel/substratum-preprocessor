#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash-table.h"
#include "preprocessor-parser.h"

struct HashTable *defines = NULL;

struct Macro
{
    char *inVal;
    char *outVal;
};

int longestKeyword = 0;
int shortestKeyword = INT32_MAX;
struct Stack *keywordsByLength = NULL;

void sortKeywordsByLength()
{
    while (keywordsByLength->size > 0)
    {
        Stack_Pop(keywordsByLength);
    }

    // scan through #define-d keywords
    for (int i = 0; i < defines->nBuckets; i++)
    {
        struct LinkedList *examinedBucket = defines->buckets[i];
        for (struct LinkedListNode *examinedDefine = examinedBucket->head; examinedDefine != NULL; examinedDefine = examinedDefine->next)
        {
            struct HashTableEntry *e = examinedDefine->data;
            Stack_Push(keywordsByLength, e->key);
        }
    }

    // bubble sort all keywords in descending order (longest at index 0 of data)
    for (int i = 0; i < keywordsByLength->size; i++)
    {
        for (int j = 0; j < keywordsByLength->size - i - 1; j++)
        {
            char *thisKw = keywordsByLength->data[j];

            int thisLen = strlen(thisKw);
            int compLen = strlen((char *)keywordsByLength->data[j + 1]);

            if (thisLen > compLen)
            {
                struct Lifetime *swap = keywordsByLength->data[j];
                keywordsByLength->data[j] = keywordsByLength->data[j + 1];
                keywordsByLength->data[j + 1] = swap;
            }
        }
    }
}

void calculateTokenLengths()
{
    // reset longest keyword
    longestKeyword = 0;
    shortestKeyword = INT32_MAX;

    // scan through #define-d keywords
    for (int i = 0; i < defines->nBuckets; i++)
    {
        struct LinkedList *examinedBucket = defines->buckets[i];
        for (struct LinkedListNode *examinedDefine = examinedBucket->head; examinedDefine != NULL; examinedDefine = examinedDefine->next)
        {
            char *examinedDefinedKeyword = examinedDefine->data;
            if (strlen(examinedDefinedKeyword) > longestKeyword)
            {
                longestKeyword = strlen(examinedDefinedKeyword);
            }
        }
    }

}

struct Macro *findMacro(char *buf, int bufLen)
{
    char *strBuf = malloc(bufLen + 1);
    memcpy(strBuf, buf, bufLen);
    strBuf[bufLen] = '\0';

    int strBufLen = strlen(strBuf);

    for(int i = 0; i < keywordsByLength->size; i++)
    {
        char *comparedKeyword = keywordsByLength->data[i];
        if((strlen(comparedKeyword) == strBufLen) && strcmp(buf, comparedKeyword) == 0)
        {
            return HashTable_Lookup(defines, comparedKeyword)->value;
        }
    }
    return NULL;
}

void handleMacro(struct PreprocessorContext *c, struct Macro *toOutput)
{
    c->bufLen = 0;
    fputs(toOutput->outVal, c->outFile);
}

void attemptMacroSubstitution(struct PreprocessorContext *c)
{
    struct Macro *m = findMacro(c->inBuf, c->bufLen);

    // early return if no macro found
    if(m == NULL)
    {
        return;
    }

    handleMacro(c, m);
}


int main(int argc, char *argv[])
{
    keywordsByLength = Stack_New();
    defines = HashTable_New(10);
    calculateTokenLengths();
    sortKeywordsByLength();

    struct PreprocessorContext c;
    memset(&c, 0, sizeof(struct PreprocessorContext));
    if(argc > 1)
    {
        c.inFile = fopen(argv[1], "rb");
        if(c.inFile == NULL)
        {
            printf("Unable to open file %s\n", argv[1]);
            abort();
        }
    }
    else
    {
        c.inFile = stdin;
    }

    c.outFile = stdout;
    int ret;

    pcc_context_t *parseContext = pcc_create(&c);

    while (pcc_parse(parseContext, &ret))
    {
        if(c.bufLen >= longestKeyword)
        {
            attemptMacroSubstitution(&c);
        }
    }

    pcc_destroy(parseContext);
    printf("sbpp\n");
}
