#include "macro-definitions.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

enum MacroTypes
{
    mt_textsub,  // macro as direct a->b text substitution
    mt_function, // macro as function
};

struct Macro
{
    char *inVal;
    char *outVal;

    // only valid if type == mt_function
    char **paramsList; // array of strings naming the params of this macro function
    unsigned nParams;  // size of the array of params

    enum MacroTypes type;
};

int longestKeyword = 0;

// recalculate which keyword is longest, regenerated the sorted list of keywords by length
void handleDefineChange(struct PreprocessorContext *c)
{
    longestKeyword = 0;
    while (c->keywordsByLength->size > 0)
    {
        Stack_Pop(c->keywordsByLength);
    }

    // scan through #define-d keywords
    for (int i = 0; i < c->defines->nBuckets; i++)
    {
        struct LinkedList *examinedBucket = c->defines->buckets[i];
        for (struct LinkedListNode *examinedDefine = examinedBucket->head; examinedDefine != NULL; examinedDefine = examinedDefine->next)
        {
            struct HashTableEntry *e = examinedDefine->data;
            struct Macro *m = e->value;
            Stack_Push(c->keywordsByLength, m->inVal);
        }
    }

    // bubble sort all keywords in descending order (longest at index 0 of data)
    for (int i = 0; i < c->keywordsByLength->size; i++)
    {
        for (int j = 0; j < c->keywordsByLength->size - i - 1; j++)
        {
            char *thisKw = c->keywordsByLength->data[j];

            int thisLen = strlen(thisKw);

            int compLen = strlen((char *)c->keywordsByLength->data[j + 1]);

            if (thisLen > compLen)
            {
                struct Lifetime *swap = c->keywordsByLength->data[j];
                c->keywordsByLength->data[j] = c->keywordsByLength->data[j + 1];
                c->keywordsByLength->data[j + 1] = swap;
            }
        }
    }

    if (c->keywordsByLength->size > 0)
    {
        longestKeyword = strlen(c->keywordsByLength->data[c->keywordsByLength->size - 1]);
    }
}

struct Macro *findMacro(struct PreprocessorContext *c)
{
    char *strBuf = malloc(c->bufLen + 1);
    memcpy(strBuf, c->inBuf, c->bufLen);
    strBuf[c->bufLen] = '\0';

    for (int i = 0; i < c->keywordsByLength->size; i++)
    {
        char *comparedKeyword = c->keywordsByLength->data[i];
        if (strncmp(c->inBuf, comparedKeyword, strlen(comparedKeyword)) == 0)
        {
            free(strBuf);
            return HashTable_Lookup(c->defines, comparedKeyword)->value;
        }
    }
    free(strBuf);
    return NULL;
}

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

void handleTextSubstitutionMacro(struct PreprocessorContext *c, struct Macro *toOutput)
{
    assert(toOutput->type == mt_textsub);

    int printedLen = strlen(toOutput->inVal);
    c->bufLen -= printedLen;
    memmove(c->inBuf, c->inBuf + printedLen, c->bufLen);
    bufferInsertFront(c, toOutput->outVal);
}

void handleFunctionMacro(struct PreprocessorContext *c, struct Macro *toOutput)
{
    assert(toOutput->type == mt_function);

    printf("inval:%s\n", toOutput->inVal);
    int inValLen = strlen(toOutput->inVal);
    while (inValLen-- > 0)
    {
        bufferConsume(c);
    }

    for (int i = 0; i < c->bufLen; i++)
    {
        printf("%c", c->inBuf[i]);
    }
    printf("\n");

    if (c->inBuf[0] != '(')
    {
        printf("Error expanding function macro %s - didn't see open paren!\n", toOutput->inVal);
    }
    bufferConsume(c); // grab the first open paren out of the buffer to start

    char *paramsListBuf = malloc(c->bufLen);
    unsigned paramsListLen = 0;

    int parenDepth = 1;
    while ((parenDepth > 0) && (c->bufLen > 0))
    {
        char fromBuffer = bufferConsume(c);
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

        printf("got character %c from buf - depth is %d\n", fromBuffer, parenDepth);
    }

    if (parenDepth != 0)
    {
        printf("unclosed parenthesis while expanding macro %s\n", toOutput->inVal);
        exit(1);
    }

    paramsListBuf[paramsListLen] = '\0';

    printf("paramslistbuf: %s\n", paramsListBuf);

    paramsListLen = 0;
    char **paramsList = spaceSeparatedParamsListToArray(paramsListBuf, &paramsListLen);

    if (paramsListLen != toOutput->nParams)
    {
        printf("Number of arguments (%d) provided to macro function %s doesn't match %d expected!\n", paramsListLen, toOutput->inVal, toOutput->nParams);
        exit(1);
    }

    char *oldBuf = strndup(c->inBuf, c->bufLen);
    unsigned oldBufLen = c->bufLen;
    unsigned oldBufCap = c->bufCap;
    struct HashTable *oldDefines = c->defines;
    struct Stack *oldKeywordsByLength = c->keywordsByLength;

    c->defines = HashTable_New(oldDefines->nBuckets);
    c->keywordsByLength = Stack_New();

    for (int i = 0; i < toOutput->nParams; i++)
    {
        defineTextSubMacro(c, toOutput->paramsList[i], paramsList[i]);
        printf("define sub %s->%s\n", toOutput->paramsList[i], paramsList[i]);
    }

    c->bufLen = 0;

    int outValLen = strlen(toOutput->outVal);
    printf("write outval %s to buffer\n", toOutput->outVal);
    for (int i = 0; i < outValLen; i++)
    {
        bufferInsert(c, toOutput->outVal[i]);
    }

    while (c->bufLen > 0)
    {
        attemptMacroSubstitution(c, 0);
        if (c->bufLen > 0)
        {
            fputc(bufferConsume(c), c->outFile);
        }
    }

    Stack_Free(c->keywordsByLength);
    c->keywordsByLength = oldKeywordsByLength;
    HashTable_Free(c->defines);
    c->defines = oldDefines;

    
    assert(c->bufCap >= oldBufCap); // make sure the buffer capacity didn't change
    memcpy(c->inBuf, oldBuf, oldBufLen);
    c->bufLen = oldBufLen;

}

void handleMacro(struct PreprocessorContext *c, struct Macro *toOutput)
{
    switch (toOutput->type)
    {
    case mt_textsub:
        handleTextSubstitutionMacro(c, toOutput);
        break;

    case mt_function:
        handleFunctionMacro(c, toOutput);
        break;
    }
}

// returns number of macros expanded
int attemptMacroSubstitutionRecursive(struct PreprocessorContext *c, char stillParsing, int depth)
{
    if (depth > 0xFF)
    {
        perror("Recursion limit on macro substitution!\n");
        abort();
    }

    // early return if there is more input (put it into the buffer so we don't accidentally greedily match a prefix of a keyword)
    if ((stillParsing && (c->bufLen < longestKeyword)) || (longestKeyword == 0))
    {
        return 0;
    }

    struct Macro *m = findMacro(c);

    // early return if no macro found
    if (m == NULL)
    {
        return 0;
    }

    handleMacro(c, m);
    return 1 + attemptMacroSubstitutionRecursive(c, stillParsing, depth + 1);
}

void attemptMacroSubstitution(struct PreprocessorContext *c, char stillParsing)
{
    int nExpansions = 0;
    int lastExpansions = 0;

    while ((nExpansions += attemptMacroSubstitutionRecursive(c, stillParsing, nExpansions)) > lastExpansions)
    {
        lastExpansions = nExpansions;
    }
}

// must strdup as outVal is passed directly in from packcc parser
void defineTextSubMacro(struct PreprocessorContext *c, char *token, char *outVal)
{
    struct Macro *newMacro = malloc(sizeof(struct Macro));
    newMacro->inVal = strdup(token);
    newMacro->outVal = strdup(outVal);
    newMacro->type = mt_textsub;
    HashTable_Insert(c->defines, newMacro->inVal, newMacro);
    handleDefineChange(c);
}

// no need to strdup spaceSeparatedParamsList as packcc parser manages allocation while converting from comma separated to space separated
void defineFunctionMacro(struct PreprocessorContext *c, char *token, char *spaceSeparatedParamsList, char *funcBody)
{
    struct Macro *newMacro = malloc(sizeof(struct Macro));
    memset(newMacro, 0, sizeof(struct Macro));
    newMacro->inVal = strdup(token);
    newMacro->outVal = strdup(funcBody);
    newMacro->type = mt_function;

    newMacro->paramsList = spaceSeparatedParamsListToArray(spaceSeparatedParamsList, &newMacro->nParams);

    HashTable_Insert(c->defines, newMacro->inVal, newMacro);
    handleDefineChange(c);
}

void undefineMacro(struct PreprocessorContext *c, char *token)
{
    struct HashTableEntry *e = HashTable_Lookup(c->defines, token);
    if (e != NULL)
    {
        HashTable_Remove(c->defines, token, free);
        handleDefineChange(c);
    }
}
