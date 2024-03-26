#include <stdio.h>

#define UPDATE_PLACE(p, c) \
    {                      \
        if ((c) == '\n')   \
        {                  \
            (p)->line++;   \
            (p)->col = 0;  \
        }                  \
        else               \
        {                  \
            (p)->col++;    \
        }                  \
    }

#define PCC_GETCHAR(auxil) ({               \
    int inChar = fgetc(auxil->inFile);      \
    UPDATE_PLACE(&auxil->rawPlace, inChar); \
    inChar;                                 \
})

/*
#define PCC_DEBUG(auxil, event, rule, level, pos, buffer, length)                                                                      \
    {                                                                                                                                  \
            for (size_t i = 0; i < level; i++)                                                                                         \
            {                                                                                                                          \
                printf("-   ");                                                                                                        \
            }                                                                                                                          \
            printf("PCC @ %s:%d:%2d - %s:%s %lu", auxil->curFileName, auxil->curLine, auxil->curCol, dbgEventNames[event], rule, pos); \
            printf("[");                                                                                                               \
            for (size_t i = 0; i < length; i++)                                                                                        \
            {                                                                                                                          \
                if (buffer[i] == '\n')                                                                                                 \
                {                                                                                                                      \
                    printf("\\n");                                                                                                     \
                }                                                                                                                      \
                else                                                                                                                   \
                {                                                                                                                      \
                    printf("%c", buffer[i]);                                                                                           \
                }                                                                                                                      \
            }                                                                                                                          \
            printf("]\n");                                                                                                             \
    }
 */