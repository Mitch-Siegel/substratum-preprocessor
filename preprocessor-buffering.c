#include "preprocessor-buffering.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

char bufferConsume(struct PreprocessorContext *context)
{
    assert(context->bufLen > 0);
    char toReturn = context->inBuf[0];
    --context->bufLen;
    memmove(context->inBuf, context->inBuf + 1, context->bufLen);
    return toReturn;
}

void bufferInsert(struct PreprocessorContext *context, char c)
{
    if((context->bufLen + 1) >= context->bufCap)
    {
        context->bufCap++;
        context->inBuf = realloc(context->inBuf, context->bufCap);
    }

    context->inBuf[context->bufLen++] = c;
}

void bufferInsertFront(struct PreprocessorContext *context, char *s)
{
    int insertedStringLength = strlen(s);
    if((context->bufLen + insertedStringLength) >= context->bufCap)
    {
        context->bufCap += insertedStringLength;
        context->inBuf = realloc(context->inBuf, context->bufCap);
    }

    memmove(context->inBuf + insertedStringLength, context->inBuf, context->bufLen);
    
    memcpy(context->inBuf, s, insertedStringLength);
    context->bufLen+= insertedStringLength;
}


