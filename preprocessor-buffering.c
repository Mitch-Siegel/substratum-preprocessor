#include "preprocessor-buffering.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

#include "preprocessor-parser.h"
#include "macro.h"

struct TextBuffer *textBuffer_new()
{
    struct TextBuffer *b = malloc(sizeof(struct TextBuffer));
    memset(b, 0, sizeof(struct TextBuffer));
    return b;
}

void textBuffer_free(struct TextBuffer *b)
{
    free(b->data);
    free(b);
}

char textBuffer_consume(struct TextBuffer *b)
{
    assert(b->size > 0);
    char toReturn = b->data[0];
    --b->size;
    memmove(b->data, b->data + 1, b->size);
    return toReturn;
}

void textBuffer_insert(struct TextBuffer *b, char c)
{
    if ((b->size + 1) >= b->capacity)
    {
        b->capacity++;
        b->data = realloc(b->data, b->capacity);
    }

    b->data[b->size++] = c;
}

void textBuffer_insertFront(struct TextBuffer *b, char *s)
{
    int insertedStringLength = strlen(s);
    if ((b->size + insertedStringLength) >= b->capacity)
    {
        b->capacity += insertedStringLength;
        b->data = realloc(b->data, b->capacity);
    }

    memmove(b->data + insertedStringLength, b->data, b->size);

    memcpy(b->data, s, insertedStringLength);
    b->size += insertedStringLength;
}

void textBuffer_erase(struct TextBuffer *b, unsigned n)
{
    if (n >= b->size)
    {
        b->size = 0;
    }
    else
    {
        memmove(b->data, b->data + n, b->size - n);
        b->size -= n;
    }
}

extern struct LinkedList *includePath;

// attempt to open fileName in the current directory. If that fails, traverse the include path attempting to open fileName under the include path directories
FILE *searchIncludeToOpen(char *fileName)
{
    FILE *opened = NULL;
    printf("attempt to open %s\n", fileName);
    opened = fopen(fileName, "rb");

    if (opened != NULL)
    {
        char *dupedFileName = strdup(fileName);
        if (chdir(dirname(dupedFileName)))
        {
            fprintf(stderr, "Found file \"%s\" but failed to change to its directory!\n", fileName);
            perror(strerror(errno));
            exit(1);
        }
        free(dupedFileName);

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

        if (opened != NULL)
        {
            char *dupedFileName = strdup(prefixedPath);
            if (chdir(dirname(dupedFileName)))
            {
                fprintf(stderr, "Found file \"%s\" but failed to change to its directory!\n", prefixedPath);
                perror(strerror(errno));
                exit(1);
            }
            free(dupedFileName);
            free(prefixedPath);

            return opened;
        }
        free(prefixedPath);
    }

    return NULL;
}

// attempt macro substitution on the full buffer contained within 'context', expanding any macros as we go
// context: preprocessor context containing input buffer and macro definitions
// outBuf: text buffer to output will be directed
// expandToInput: if nonzero, expansion will loop to context->inBuf, otherwise macros will be expanded to outBuf
void preprocessUntilBufferEmpty(struct PreprocessorContext *context, struct TextBuffer *outBuf, char expandToInput)
{
    // until the buffer is empty
    while (context->inBuf->size > 0)
    {
        // try and expand any (all) macros at the start of the buffer
        if (expandToInput)
        {
            attemptMacroSubstitutionToBuffer(context, context->inBuf, 0);
        }
        else
        {
            attemptMacroSubstitutionToBuffer(context, outBuf, 0);
        }

        // return from above call means done expanding, nothing else to expand at front of buffer
        if (context->inBuf->size > 0)
        {
            // consume the first character in the buffer, loop again to attempt more expansion
            textBuffer_insert(outBuf, textBuffer_consume(context->inBuf));
        }
    }
}

void includeFile(struct PreprocessorContext *oldContext, char *s)
{
    struct PreprocessorContext context;
    memset(&context, 0, sizeof(struct PreprocessorContext));

    char readingStdin = 0;

    context.inBuf = textBuffer_new();
    context.outBuf = textBuffer_new();

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
        char *newCwd = getcwd(NULL, 0);
        printf("old wd [%s], new [%s]\n", oldWd, newCwd);
        free(newCwd);
    }

    char *ret;
    pcc_context_t *parseContext = pcc_create(&context);

    while (pcc_parse(parseContext, &ret))
    {
        preprocessUntilBufferEmpty(&context, context.outBuf, 1);
        while (context.outBuf->size > 0)
        {
            fputc(textBuffer_consume(context.outBuf), context.outFile);
        }
    }

    preprocessUntilBufferEmpty(&context, context.outBuf, 1);
    while (context.outBuf->size > 0)
    {
        fputc(textBuffer_consume(context.outBuf), context.outFile);
    }

    if (oldContext->includeDepth == 0)
    {
        HashTable_Free(context.defines, (void (*)(void *))macro_free);
        Stack_Free(context.keywordsByLength);
        if (!strcmp(s, "stdin"))
        {
            readingStdin = 1;
        }
    }

    textBuffer_free(context.inBuf);
    textBuffer_free(context.outBuf);

    pcc_destroy(parseContext);

    chdir(oldWd);
    free(oldWd);
}
