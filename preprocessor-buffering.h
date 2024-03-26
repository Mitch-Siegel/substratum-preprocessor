#include <stdio.h>
#include "hash-table.h"

#ifndef _PREPROCESSOR_BUFFERING_H_
#define _PREPROCESSOR_BUFFERING_H_

struct TextBuffer
{
    char *data;
    size_t size;     // number of characters currently in the buffer
    size_t capacity; // max capacity of the buffer
};

struct TextBuffer *textBuffer_new();

void textBuffer_free(struct TextBuffer *b);

struct ParserPlace
{
    unsigned int line;
    unsigned int col;
};

struct PreprocessorContext
{
    struct TextBuffer *inBuf;
    struct TextBuffer *outBuf;
    FILE *inFile;
    FILE *outFile;
    unsigned int lastMatchedPosition; // index of character (from start of pcc parse context) where the last match occurred
    struct ParserPlace matchedPlace;  // character position up to which we have matched
    struct ParserPlace rawPlace;      // character position up to which the parser has read (incl. lookahead) from input
    char *curFileName;
    struct HashTable *defines;
    struct Stack *keywordsByLength;
    struct Stack *ifdefDepth; // stack of raw data - 0 if ignoring input at this ifdef depth, 1 if not
    int includeDepth;
    struct PreprocessorContext *includedFrom;
};

char textBuffer_consume(struct TextBuffer *b);

void textBuffer_insert(struct TextBuffer *b, char c);

void textBuffer_insertFront(struct TextBuffer *b, char *s);

// erase n characters from the front of the buffer
void textBuffer_erase(struct TextBuffer *b, unsigned n);

void preprocessUntilBufferEmpty(struct PreprocessorContext *context, struct TextBuffer *outBuf, char expandToInput);

void emitPlace(struct PreprocessorContext *context);

void includeFile(struct PreprocessorContext *context, char *s);

#endif
