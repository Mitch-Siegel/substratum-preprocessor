#include <stdlib.h>
#include <string.h>

#include "preprocessor-buffering.h"

char *macroParamTokenize(char *start, char sep, char **lasts)
{
    if (!start)
    {
        start = *lasts;
    }

    if(!start)
    {
        return NULL;
    }

    int len = strlen(start);
    int parenDepth = 0;

    char found = 0;
    int i = 0;
    for (i = 0; (i < len) && (!found); i++)
    {
        switch (start[i])
        {
        case '(':
            parenDepth++;
            break;

        case ')':
            parenDepth--;
            break;

        default:
            if ((parenDepth == 0) && (start[i] == sep))
            {
                found = 1;
            }
            break;
        }
    }

    if ((!found && (i < len)) || (parenDepth > 0))
    {
        printf("Couldn't split macro params string on token '%c' - found: %d, parenDepth: %d\n", sep, found, parenDepth);
        abort();
    }

    if(found)
    {
        start[i - 1] = '\0';
        *lasts = &start[i];
    }
    else
    {
        *lasts = NULL;
    }

    return start;
}

char **spaceSeparatedParamsListToArray(char *list, unsigned int *arraySize)
{
    char **array = NULL;
    char *dupPl = strdup(list);

    char *lasts;
    char *tok = macroParamTokenize(dupPl, ' ', &lasts);

    while (tok != NULL)
    {
        array = realloc(array, ((*arraySize) + 1) * sizeof(char *));

        array[*arraySize] = strdup(tok);
        (*arraySize)++;

        tok = macroParamTokenize(NULL, ' ', &lasts);
    }

    free(dupPl);

    for(int i = 0; i < *arraySize; i++)
    {
        printf("%d:%s\n", i, array[i]);
    }

    return array;
}

char *removeFirstLayerCommasFromMatchedParens(struct TextBuffer *b)
{
    char *paramsListBuf = malloc(b->size);
    unsigned paramsListLen = 0;

    // grab the first open paren out of the buffer to start
    if (textBuffer_consume(b) != '(')
    {
        printf("Error expanding function macro - didn't see open paren!\n");
    }

    int parenDepth = 1;
    while ((parenDepth > 0) && (b->size > 0))
    {
        char fromBuffer = textBuffer_consume(b);
        switch (fromBuffer)
        {
        case ',':
            // strip commas in the macro at the first level - so (second_function(a, b), c) becomes (second_function(a, b) c)
            if (parenDepth > 1)
            {
                paramsListBuf[paramsListLen++] = fromBuffer;
            }
            break;

        case ')':
            parenDepth--;
            if (parenDepth == 0)
            {
                break;
            }
            // add non-final rparen to buffer
            paramsListBuf[paramsListLen++] = fromBuffer;
            break;

        case '(':
            parenDepth++;
            paramsListBuf[paramsListLen++] = fromBuffer;
            break;

        default:
            paramsListBuf[paramsListLen++] = fromBuffer;
            break;
        }
    }

    if (parenDepth != 0)
    {
        printf("unclosed parenthesis while expanding macro\n");
        exit(1);
    }

    paramsListBuf[paramsListLen] = '\0';
    return paramsListBuf;
}
