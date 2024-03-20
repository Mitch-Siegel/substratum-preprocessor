#include <stdio.h>
#include "hash-table.h"

#ifndef _PREPROCESSOR_BUFFERING_H_
#define _PREPROCESSOR_BUFFERING_H_

struct PreprocessorContext
{
    char *inBuf;
    size_t bufLen; // number of characters currently in the buffer
    size_t bufCap; // max capacity of the buffer
    FILE *inFile;
    FILE *outFile;
    unsigned int curLine;
    unsigned int curCol;
    unsigned int curLineRaw;
    unsigned int curColRaw;
    char *curFileName;
    struct HashTable *defines;
    struct Stack *keywordsByLength;
    int includeDepth;
    struct PreprocessorContext *includedFrom;
};

char bufferConsume(struct PreprocessorContext *context);

void bufferInsert(struct PreprocessorContext *context, char c);

void bufferInsertFront(struct PreprocessorContext *context, char *s);

void includeFile(struct PreprocessorContext *context, char *s);

#endif

