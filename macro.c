#include "macro.h"

#include <stdlib.h>
#include <string.h>

struct Macro *macro_new(char *inVal, char *outVal, enum MacroTypes type)
{
    struct Macro *m = malloc(sizeof(struct Macro));
    memset(m, 0, sizeof(struct Macro));
    m->inVal = strdup(inVal);
    m->outVal = strdup(outVal);
    m->type = type;

    return m;
}

void macro_free(struct Macro *m)
{
    free(m->inVal);
    free(m->outVal);

    if(m->type == mt_function)
    {
        for(int i = 0; i < m->nParams; i++)
        {
            free(m->paramsList[i]);
        }
        free(m->paramsList);
    }

    free(m);
}
