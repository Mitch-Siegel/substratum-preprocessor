#include "macro-definitions.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "text-manipulation.h"
#include "macro.h"


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

            if (thisLen < compLen)
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
    for (int i = 0; i < c->keywordsByLength->size; i++)
    {
        char *comparedKeyword = c->keywordsByLength->data[i];
        int comparedLen = strlen(comparedKeyword);
        if ((comparedLen <= c->inBuf->size) && (strncmp(c->inBuf->data, comparedKeyword, comparedLen) == 0))
        {
            struct Macro *matchedMacro = HashTable_Lookup(c->defines, comparedKeyword)->value;

            // if we match a function macro, it can only be expanded if the next character in the buffer is a paren
            if ((matchedMacro->type == mt_function) &&
                ((c->inBuf->size <= comparedLen) || (c->inBuf->data[comparedLen] != '(')))
            {
                return NULL;
            }

            return matchedMacro;
        }
    }
    return NULL;
}

void handleTextSubstitutionMacro(struct PreprocessorContext *c, struct Macro *toOutput, struct TextBuffer *expandToBuf)
{
    assert(toOutput->type == mt_textsub);
    textBuffer_erase(c->inBuf, strlen(toOutput->inVal));

    if (c->inBuf == expandToBuf)
    {
        textBuffer_insertFront(expandToBuf, toOutput->outVal);
    }
    else
    {
        int outLen = strlen(toOutput->outVal);
        for (int i = 0; i < outLen; i++)
        {
            textBuffer_insert(expandToBuf, toOutput->outVal[i]);
        }
    }
}

void handleFunctionMacro(struct PreprocessorContext *c, struct Macro *toOutput, struct TextBuffer *expandToBuf)
{
    assert(toOutput->type == mt_function);

    textBuffer_erase(c->inBuf, strlen(toOutput->inVal));

    char *paramsListBuf = removeFirstLayerCommasFromMatchedParens(c->inBuf);

    unsigned int paramsListLen = 0;
    char **paramsList = spaceSeparatedParamsListToArray(paramsListBuf, &paramsListLen);

    if (paramsListLen != toOutput->nParams)
    {
        printf("Number of arguments (%d) provided to macro function %s doesn't match %d expected!\n", paramsListLen, toOutput->inVal, toOutput->nParams);
        exit(1);
    }

    struct TextBuffer *oldBuf = c->inBuf;
    struct HashTable *oldDefines = c->defines;
    struct Stack *oldKeywordsByLength = c->keywordsByLength;

    c->defines = HashTable_New(oldDefines->nBuckets);
    c->keywordsByLength = Stack_New();

    // define text substitutions for the values of each macro parameter
    for (int i = 0; i < toOutput->nParams; i++)
    {
        defineTextSubMacro(c, toOutput->paramsList[i], paramsList[i]);
        free(paramsList[i]);
    }
    // clean up our intermediate string handling buffers
    free(paramsList);
    free(paramsListBuf);

    c->inBuf = textBuffer_new();
    struct TextBuffer *intermediateOutBuf = textBuffer_new();

    textBuffer_insertFront(c->inBuf, toOutput->outVal);

    // process the entire macro function into the old context's buffer
    preprocessUntilBufferEmpty(c, intermediateOutBuf, 0);

    if (oldBuf == expandToBuf)
    {
        textBuffer_insert(intermediateOutBuf, '\0');
        textBuffer_insertFront(expandToBuf, intermediateOutBuf->data);
    }
    else
    {
        for (int i = 0; i < intermediateOutBuf->size; i++)
        {
            textBuffer_insert(expandToBuf, intermediateOutBuf->data[i]);
        }
    }

    textBuffer_free(intermediateOutBuf);

    Stack_Free(c->keywordsByLength);
    c->keywordsByLength = oldKeywordsByLength;
    HashTable_Free(c->defines, (void (*)(void *))macro_free);
    c->defines = oldDefines;

    textBuffer_free(c->inBuf);
    c->inBuf = oldBuf;
}

void handleMacro(struct PreprocessorContext *c, struct Macro *toOutput, struct TextBuffer *expandToBuf)
{
    switch (toOutput->type)
    {
    case mt_textsub:
        handleTextSubstitutionMacro(c, toOutput, expandToBuf);
        break;

    case mt_function:
        handleFunctionMacro(c, toOutput, expandToBuf);
        break;
    }
}

// returns number of macros expanded
int attemptMacroSubstitutionRecursive(struct PreprocessorContext *c, struct TextBuffer *expandToBuf, char stillParsing, int depth)
{
    if (depth > 0xFF)
    {
        perror("Recursion limit on macro substitution!\n");
        abort();
    }

    // early return if there is more input (put it into the buffer so we don't accidentally greedily match a prefix of a keyword)
    if ((stillParsing && (c->inBuf->size < longestKeyword)) || (longestKeyword == 0))
    {
        return 0;
    }

    struct Macro *m = findMacro(c);

    // early return if no macro found
    if (m == NULL)
    {
        return 0;
    }

    handleMacro(c, m, expandToBuf);
    return 1 + attemptMacroSubstitutionRecursive(c, expandToBuf, stillParsing, depth + 1);
}

// entry function for macro substitution attempts
// will continually try to expand macros as long as the previous run was able to expand something
// expands macros into TextBuffer b
void attemptMacroSubstitutionToBuffer(struct PreprocessorContext *c, struct TextBuffer *expandToBuf, char stillParsing)
{
    int nExpansions = 0;
    int lastExpansions = 0;

    while ((nExpansions += attemptMacroSubstitutionRecursive(c, expandToBuf, stillParsing, nExpansions)) > lastExpansions)
    {
        lastExpansions = nExpansions;
    }
}

// entry function for macro substitution attempts
// will continually try to expand macros as long as the previous run was able to expand something
// expands macros back into c's input buffer
void attemptMacroSubstitution(struct PreprocessorContext *c, char stillParsing)
{
    attemptMacroSubstitutionToBuffer(c, c->inBuf, stillParsing);
}

// must strdup as outVal is passed directly in from packcc parser
void defineTextSubMacro(struct PreprocessorContext *c, char *token, char *outVal)
{
    struct Macro *newMacro = macro_new(token, outVal, mt_textsub);
    HashTable_Insert(c->defines, newMacro->inVal, newMacro);
    handleDefineChange(c);
}

// no need to strdup spaceSeparatedParamsList as packcc parser manages allocation while converting from comma separated to space separated
void defineFunctionMacro(struct PreprocessorContext *c, char *token, char *spaceSeparatedParamsList, char *funcBody)
{
    struct Macro *newMacro = macro_new(token, funcBody, mt_function);
    
    newMacro->paramsList = spaceSeparatedParamsListToArray(spaceSeparatedParamsList, &newMacro->nParams);
    free(spaceSeparatedParamsList);

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
