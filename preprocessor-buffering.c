#include "preprocessor-buffering.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

#include "preprocessor-parser.h"

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
    if ((context->bufLen + 1) >= context->bufCap)
    {
        context->bufCap++;
        context->inBuf = realloc(context->inBuf, context->bufCap);
    }

    context->inBuf[context->bufLen++] = c;
}

void bufferInsertFront(struct PreprocessorContext *context, char *s)
{
    int insertedStringLength = strlen(s);
    if ((context->bufLen + insertedStringLength) >= context->bufCap)
    {
        context->bufCap += insertedStringLength;
        context->inBuf = realloc(context->inBuf, context->bufCap);
    }

    memmove(context->inBuf + insertedStringLength, context->inBuf, context->bufLen);

    memcpy(context->inBuf, s, insertedStringLength);
    context->bufLen += insertedStringLength;
}

extern struct LinkedList *includePath;

FILE *searchIncludeToOpen(char *fileName)
{
    FILE *opened = NULL;

    opened = fopen(fileName, "rb");

    if (opened != NULL)
    {
        return opened;
    }

    for (struct LinkedListNode *includePathRunner = includePath->head; includePathRunner != NULL; includePathRunner = includePathRunner->next)
    {
        char *pathPrefix = includePathRunner->data;
        char *prefixedPath = malloc(strlen(fileName) + strlen(pathPrefix) + 2);
        strcpy(prefixedPath, pathPrefix);
        strcat(prefixedPath, "/");
        strcat(prefixedPath, fileName);

        printf("try %s\n", prefixedPath);
        opened = fopen(prefixedPath, "rb");
        free(prefixedPath);

        if (opened != NULL)
        {
            return opened;
        }
    }

    return NULL;
}

void includeFile(struct PreprocessorContext *oldContext, char *s)
{
    struct PreprocessorContext context;
    memset(&context, 0, sizeof(struct PreprocessorContext));
    char readingStdin = 0;

    char *oldWd = getcwd(NULL, 0);

    if (oldContext->includeDepth == 0)
    {
        context.defines = HashTable_New(10);
        context.keywordsByLength = Stack_New();
        if (!strcmp(s, "stdin"))
        {
            readingStdin = 1;
        }
    }
    else
    {
        context.defines = oldContext->defines;
        context.keywordsByLength = oldContext->keywordsByLength;
    }

    context.includeDepth = oldContext->includeDepth + 1;
    context.includedFrom = oldContext;
    context.outFile = oldContext->outFile;

    if (readingStdin)
    {
        context.inFile = stdin;
    }
    else
    {
        context.inFile = searchIncludeToOpen(s);
        if (context.inFile == NULL)
        {
            printf("Unable to open file %s\n", s);
            abort();
        }

        if (chdir(dirname(s)))
        {
            perror(strerror(errno));
        }
        printf("chdir to %s\n", dirname(s));
    }

    char *ret;
    pcc_context_t *parseContext = pcc_create(&context);

    while (pcc_parse(parseContext, &ret))
    {
        attemptMacroSubstitution(&context, 1);
    }

    while (context.bufLen > 0)
    {
        attemptMacroSubstitution(&context, 0);
    }

    free(context.inBuf);
    if (oldContext->includeDepth == 0)
    {
        HashTable_Free(context.defines);
        Stack_Free(context.keywordsByLength);
        if (!strcmp(s, "stdin"))
        {
            readingStdin = 1;
        }
    }

    pcc_destroy(parseContext);

    chdir(oldWd);
    free(oldWd);
}
