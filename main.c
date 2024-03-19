#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "hash-table.h"
#include "preprocessor-parser.h"
#include "macro-definitions.h"



int main(int argc, char *argv[])
{
    struct PreprocessorContext c;
    memset(&c, 0, sizeof(struct PreprocessorContext));
    c.defines = HashTable_New(10);
    c.keywordsByLength = Stack_New();

    if(argc > 1)
    {
        c.inFile = fopen(argv[1], "rb");
        if(c.inFile == NULL)
        {
            printf("Unable to open file %s\n", argv[1]);
            abort();
        }
    }
    else
    {
        c.inFile = stdin;
    }

    c.outFile = stdout;
    char *ret;

    pcc_context_t *parseContext = pcc_create(&c);

    while (pcc_parse(parseContext, &ret))
    {
        attemptMacroSubstitution(&c, 1);
    }

    while(c.bufLen > 0)
    {
        attemptMacroSubstitution(&c, 0);
    }

    pcc_destroy(parseContext);
    printf("sbpp\n");
}
