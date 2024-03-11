#include <stdio.h>

#include "util.h"

struct Dictionary *defineDict = NULL;

enum PreprocessorKeywords
{
    kw_define,       // #define
    kw_undef,        // #undef
    kw_user_defined, // something that the user #define-d
    kw_max,
};

struct Macro
{
    char *outVal;
};

char *preprocessorKeywordValues[kw_max] = {"#define",
                                           "#undef"};

int longestKeyword = 0;
int tokenLengths[kw_max] = {0};
struct Stack *keywordsByLength = NULL;
char *inBuf = NULL;
int inBufLen = 0;

void sortKeywordsByLength()
{
    while (keywordsByLength->size > 0)
    {
        Stack_Pop(keywordsByLength);
    }

    // scan through hardcoded keywords
    for (int i = 0; i < kw_max; i++)
    {
        Stack_Push(keywordsByLength, preprocessorKeywordValues[i]);
    }

    // scan through #define-d keywords
    for (int i = 0; i < defineDict->nBuckets; i++)
    {
        struct LinkedList *examinedBucket = defineDict->buckets[i];
        for (struct LinkedListNode *examinedDefine = examinedBucket->head; examinedDefine != NULL; examinedDefine = examinedDefine->next)
        {
            Stack_Push(keywordsByLength, examinedDefine->data);
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
    // reset longest keyword, scan internal keywords
    longestKeyword = 0;
    for (int i = 0; i < kw_max; i++)
    {
        tokenLengths[i] = strlen(preprocessorKeywordValues[i]);
        if (tokenLengths[i] > longestKeyword)
        {
            longestKeyword = tokenLengths[i];
        }
    }

    // scan through #define-d keywords
    for (int i = 0; i < defineDict->nBuckets; i++)
    {
        struct LinkedList *examinedBucket = defineDict->buckets[i];
        for (struct LinkedListNode *examinedDefine = examinedBucket->head; examinedDefine != NULL; examinedDefine = examinedDefine->next)
        {
            char *examinedDefinedKeyword = examinedDefine->data;
            if (strlen(examinedDefinedKeyword) > longestKeyword)
            {
                longestKeyword = strlen(examinedDefinedKeyword);
            }
        }
    }

    // reallocate the buffer to account for the new longest keyword
    inBuf = realloc(inBuf, longestKeyword);

}

void addCharToBuf(int c, char *buf, int *bufLen)
{
    buf[*bufLen++] = c;
}

void writeFromBufferToOutput(FILE *outFile, char *buf, int *bufLen)
{
    fputc(buf[0], outFile);
    *bufLen--;
    memmove(buf, buf + 1, *bufLen);
}

struct Macro *findMacro(char *buf, int buflen)
{
    for(int i = 0; i < keywordsByLength->size; i++)
    {
        char *comparedKeyword = keywordsByLength->data[i];
        if((strlen(comparedKeyword) <= buflen) && strcmp(buf, comparedKeyword) == 0)
        {
            return Dictionary_Lookup(defineDict, comparedKeyword);
        }
    }
    return NULL;
}

void handleMacro(FILE *outFile, struct Macro *toOutput, char *buf, int *bufLen)
{
    *bufLen = 0;
    fputs(outFile, toOutput->outVal);
}

int handleCharacter(FILE *inFile, FILE *outFile)
{
    int gotten = fgetc(inFile);
    // if we get an EOF, return
    if (gotten == EOF)
    {
        for (int i = 0; i < inBufLen; i++)
        {
            fputc(inBuf[i], outFile);
        }
        return 1;
    }

    addCharToBuf(gotten, inBuf, &inBufLen);

    while (inBufLen >= longestKeyword)
    {
        struct Macro *foundMacro = NULL;
        if ((foundMacro = findMacro(inBuf, inBufLen)) != NULL)
        {
            handleMacro(outFile, foundMacro, inBuf, &inBufLen);
        }
        else
        {
            writeFromBufferToOutput(outFile, inBuf, &inBufLen);
        }
    }

    return 0;
}

void setupBuiltinKeywords()
{
    for(int i = 0; i < kw_user_defined; i++)
    {
        struct Macro *builtinMacro = malloc(sizeof(struct Macro));
        builtinMacro->outVal = "";
        // TODO: hashtable insert here (need to actually implement hashtable)
    }
}

int main(int argc, char *argv[])
{
    keywordsByLength = Stack_New();
    defineDict = Dictionary_New(10);
    setupBuiltinKeywords();
    calculateTokenLengths();
    sortKeywordsByLength();

    printf("sbpp\n");
}
