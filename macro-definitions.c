#include "macro-definitions.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "text-manipulation.h"

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
    for (int i = 0; i < c->keywordsByLength->size; i++)
    {
        char *comparedKeyword = c->keywordsByLength->data[i];
        int comparedLen = strlen(comparedKeyword);
        if ((comparedLen <= c->inBuf->size) && (strncmp(c->inBuf->data, comparedKeyword, comparedLen) == 0))
        {
            return HashTable_Lookup(c->defines, comparedKeyword)->value;
        }
    }
    return NULL;
}

void handleTextSubstitutionMacro(struct PreprocessorContext *c, struct Macro *toOutput)
{
    assert(toOutput->type == mt_textsub);

    textBuffer_erase(c->inBuf, strlen(toOutput->inVal));
    textBuffer_insertFront(c->inBuf, toOutput->outVal);
}

void handleFunctionMacro(struct PreprocessorContext *c, struct Macro *toOutput)
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

    for (int i = 0; i < toOutput->nParams; i++)
    {
        defineTextSubMacro(c, toOutput->paramsList[i], paramsList[i]);
    }

    c->inBuf = textBuffer_new();

    int outValLen = strlen(toOutput->outVal);
    for (int i = 0; i < outValLen; i++)
    {
        textBuffer_insert(c->inBuf, toOutput->outVal[i]);
    }

    while (c->inBuf->size > 0)
    {
        attemptMacroSubstitution(c, 0);
        if (c->inBuf->size > 0)
        {
            fputc(textBuffer_consume(c->inBuf), c->outFile);
        }
    }

    Stack_Free(c->keywordsByLength);
    c->keywordsByLength = oldKeywordsByLength;
    HashTable_Free(c->defines);
    c->defines = oldDefines;

    textBuffer_free(c->inBuf);
    c->inBuf = oldBuf;
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
    newMacro->type = mt_function;

    struct TextBuffer *macroBodyBuffer = textBuffer_new();
    textBuffer_insertFront(macroBodyBuffer, funcBody);
    textBuffer_insertFront(macroBodyBuffer, "(");
    textBuffer_insert(macroBodyBuffer, ')');
    newMacro->outVal = removeFirstLayerCommasFromMatchedParens(macroBodyBuffer);
    textBuffer_free(macroBodyBuffer);

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
