#include <stdlib.h>
#include <string.h>

#include "preprocessor-buffering.h"

char **spaceSeparatedParamsListToArray(char *list, unsigned int *arraySize)
{
    char **array = NULL;
    char *dupPl = strdup(list);

    char *lasts;
    char *tok = strtok_r(dupPl, " ", &lasts);

    while (tok != NULL)
    {
        array = realloc(array, ((*arraySize) + 1) * sizeof(char *));

        array[*arraySize] = strdup(tok);
        (*arraySize)++;

        tok = strtok_r(NULL, " ", &lasts);
    }

    free(dupPl);

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
