#include <stdio.h>

#include "util.h"

struct Dictionary *defineDict = NULL;

int main(int argc, char *argv[])
{
    defineDict = Dictionary_New(10);
    printf("sbpp\n");
}
