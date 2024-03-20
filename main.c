#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "hash-table.h"
#include "macro-definitions.h"

struct LinkedList *includePath;

void usage()
{
	printf("Substratum preprocessor: Usage\n");
	printf("(-i) infile : specify input substratum file to compile - default: stdin\n");
	printf("(-o) outfile: specify output file to generate object code to - default: stdout\n");
    printf("(-I) dir: include 'dir' in search for #include-s\n");
	printf("\n");
}


int main(int argc, char *argv[])
{
    includePath = LinkedList_New();

    char *inFileName = "stdin";
    char *outFileName = "stdout";

    int option;
	while ((option = getopt(argc, argv, "i:o:I:")) != EOF)
	{
		switch (option)
		{
		case 'i':
			inFileName = optarg;
			break;

		case 'o':
			outFileName = optarg;
			break;

		case 'I':
		{
			LinkedList_Append(includePath, realpath(optarg, NULL));
		}
			break;

		default:
			usage();
            exit(-1);
		}
	}


    struct PreprocessorContext c;
    memset(&c, 0, sizeof(struct PreprocessorContext));

    if(strcmp(outFileName, "stdout"))
    {
        c.outFile = fopen(outFileName, "wb");
    }
    else
    {
        c.outFile = stdout;
    }

    includeFile(&c, inFileName);

    LinkedList_Free(includePath, free);
}
