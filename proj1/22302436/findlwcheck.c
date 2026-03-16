#include "findlw.c"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    // findlw fileprefix, n, k, outfile
    if (argc != 5) {
        fprintf(stderr, "usage:  findlw fileprefix, n, k, outfile\n");
        exit(1);
    }

    char *fileprefix = argv[1];
    int numFiles = atoi(argv[2]);
    int longWordLimit = atoi(argv[3]);
    char *outfileName = argv[4];

    // For all n we process a file
    WordList longWords = processFiles(fileprefix, numFiles, longWordLimit);
    listSort(&longWords);

    FILE *outfile = fopen(outfileName, "w");
    printLongWords(longWords, outfile);
    return 0;
}
