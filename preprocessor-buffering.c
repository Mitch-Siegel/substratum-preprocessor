#include "preprocessor-buffering.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

#include "preprocessor-parser.h"

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
    if(n >= b->size)
    {
        b->size = 0;
    }
    else
    {
        memmove(b->data, b->data + n, b->size - n);
        b->size-= n;
    }
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

    context.inBuf = textBuffer_new();

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
        while (context.inBuf->size > 0)
        {
            attemptMacroSubstitution(&context, 0);
            if(context.inBuf->size > 0)
            {
                fputc(textBuffer_consume(context.inBuf), context.outFile);
            }
        }
    }

    while (context.inBuf->size > 0)
    {
        attemptMacroSubstitution(&context, 0);
        if(context.inBuf->size > 0)
        {
            fputc(textBuffer_consume(context.inBuf), context.outFile);
        }
    }

    if (oldContext->includeDepth == 0)
    {
        HashTable_Free(context.defines);
        Stack_Free(context.keywordsByLength);
        if (!strcmp(s, "stdin"))
        {
            readingStdin = 1;
        }
    }

    textBuffer_free(context.inBuf);

    pcc_destroy(parseContext);

    chdir(oldWd);
    free(oldWd);
}
