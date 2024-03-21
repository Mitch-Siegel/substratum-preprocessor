#include <stdio.h>
#include "hash-table.h"

#ifndef _PREPROCESSOR_BUFFERING_H_
#define _PREPROCESSOR_BUFFERING_H_

struct TextBuffer
{
    char *data;
    size_t size;      // number of characters currently in the buffer
    size_t capacity; // max capacity of the buffer
};

struct TextBuffer *textBuffer_new();

void textBuffer_free(struct TextBuffer *b);

struct PreprocessorContext
{
    struct TextBuffer *inBuf;
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

char textBuffer_consume(struct TextBuffer *b);

void textBuffer_insert(struct TextBuffer *b, char c);

void textBuffer_insertFront(struct TextBuffer *b, char *s);

 // erase n characters from the front of the buffer
void textBuffer_erase(struct TextBuffer *b, unsigned n);

void includeFile(struct PreprocessorContext *context, char *s);

#endif

