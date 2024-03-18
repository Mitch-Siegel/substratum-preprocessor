#include "preprocessor-buffering.h"

#include <stdlib.h>
#include <assert.h>

char bufferConsume(struct PreprocessorContext *context)
{
    assert(context->bufLen > 0);
    char toReturn = context->inBuf[--context->bufLen];
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


