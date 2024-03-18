#include <stdio.h>

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
};

char bufferConsume(struct PreprocessorContext *context);

void bufferInsert(struct PreprocessorContext *context, char c);

