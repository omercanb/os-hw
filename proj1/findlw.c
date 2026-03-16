#include "buffer.c"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * First parse arguments
 * For each file name
 *  Get the long words
 *  Append them to a common list
 *  Sort and format
 * */

FILE *getFile(char *filePrefix, int fileNo);
WordList processFile(char *filePrefix, int fileNo, int longWordLimit);
WordList getLongWords(FILE *fp, int fileNo, int longWordLimit);
WordList processFiles(char *filePrefix, int numFiles, int longWordLimit);
void printLongWords(WordList list, FILE *outfile);

// list needs to be sorted to be able to count
void printLongWords(WordList list, FILE *outfile) {
    /* Format
     * wordx (count=3): 1-34, 1-67, 3-15
     * wordyz (count=4): 2-76, 2-102, 3-102, 3-145
     */
    int i = 0;
    while (i < list.size) {
        char *word = list.data[i].word;
        int wordCount = 1;
        // Scan forward to count occurances (works because sorted)
        for (int j = i + 1; j < list.size; j++) {
            if (strcmp(word, list.data[j].word) == 0) {
                wordCount++;
            } else {
                break;
            }
        }
        // Print the word and count
        fprintf(outfile, "%s (count=%d): ", word, wordCount);

        // Scan forward again to print the file and line nos
        int end = i + wordCount;
        for (; i < end; i++) {
            // Print the file and line nos
            WordOccurance occurance = list.data[i];
            fprintf(outfile, "%d-%d ", occurance.fileNo, occurance.lineNo);
        }
        fprintf(outfile, "\n");
    }
}

WordList processFiles(char *filePrefix, int numFiles, int longWordLimit) {
    WordList longWords;
    listInit(&longWords);
    for (int fileNo = 1; fileNo <= numFiles; fileNo++) {
        WordList list = processFile(filePrefix, fileNo, longWordLimit);
        for (int i = 0; i < list.size; i++) {
            listAppend(&longWords, list.data[i].word, list.data[i].fileNo,
                       list.data[i].lineNo);
        }
        listFree(&list);
    }
    return longWords;
}

// Returns the address of a long word array on the heap
WordList processFile(char *filePrefix, int fileNo, int longWordLimit) {
    FILE *fp = getFile(filePrefix, fileNo);
    WordList list = getLongWords(fp, fileNo, longWordLimit);
    return list;
}

WordList getLongWords(FILE *fp, int fileNo, int longWordLimit) {
    WordList longWords;
    listInit(&longWords);
    int c;
    int lineNo = 1;

    // Find a long word
    while (1) {
        int wordLineNo = lineNo;
        Buffer buf;
        bufInit(&buf);
        // Find the next word
        while ((c = getc(fp)) != EOF) {
            if (isspace(c)) {
                if (c == '\n') {
                    lineNo++;
                }
                break;
            }
            bufAppend(&buf, c);
        }

        // Read the remaining spaces
        while ((c = getc(fp)) != EOF) {
            if (c == '\n') {
                lineNo++;
            }
            if (!isspace(c)) {
                ungetc(c, fp);
                break;
            }
        }

        // Check if long word and add to result
        // buf.size > 0 because of a bug I discovered
        // if the longWordLimit is given as 0, on an empty file this will cause
        // it to append an empty buffer
        if (buf.size >= longWordLimit && buf.size > 0) {
            bufAppend(&buf, '\0');
            listAppend(&longWords, buf.data, fileNo, wordLineNo);
        }

        bufFree(&buf);
        if (c == EOF) {
            break;
        }
    }
    return longWords;
}

FILE *getFile(char *filePrefix, int fileNo) {
    int numLength = snprintf(NULL, 0, "%d", fileNo);
    int totalLength = numLength + strlen(filePrefix) + 1;
    char fileName[totalLength];
    snprintf(fileName, totalLength, "%s%d", filePrefix, fileNo);
    FILE *fp = fopen(fileName, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: no file '%s'\n", fileName);
        exit(1);
    }
    return fp;
}
