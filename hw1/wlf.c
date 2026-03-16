#include "dll.c"
#include <bits/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * TODOs
 * Read words from an input stream (file or stdio) (whitespace sep)
 * Put these strings into a doubly linked list
 * Measure the time to read and make the linked list
 * Sort using strcmp
 * Delete duplicates
 *
 *
 */

#define wordLen 64

int main(int argc, char *argv[]) {
    FILE *fp;
    if (argc == 1) {
        fp = stdin;
    } else if (argc == 2) {
        // Using file
        char *filename = argv[1];
        fp = fopen(filename, "r");
        if (fp == NULL) {
            fprintf(stderr, "Error: file '%s' can't be opened\n", filename);
            exit(1);
        }
    } else {
        fprintf(stderr, "Error: usage: %s [filename]\n", argv[0]);
        exit(1);
    }

    struct timespec start, finish;

    clock_gettime(CLOCK_MONOTONIC, &start);
    Dll *list = newDll();
    char buf[256];
    while (fscanf(fp, "%s", buf) == 1) {
        dllAppend(list, buf);
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    long sec = finish.tv_sec - start.tv_sec;
    long nanosec = finish.tv_nsec - start.tv_nsec;

    printf("Time taken to read and insert into doubly linked list: %ld sec, "
           "%ld nanosec\n",
           sec, nanosec);

    dllSort(list);
    (void)dllDeleteDuplicatesSorted(list);

    // Count the number of words with length 1 to 64
    size_t counts[wordLen] = {0};
    DllNode *cur = list->head;
    while (cur != NULL) {
        size_t len = strlen(cur->word);
        if (1 <= len && len <= wordLen) {
            counts[len - 1] += 1;
        }
        cur = cur->next;
    }

    for (int i = 0; i < wordLen; i++) {
        printf("%d %zd\n", i + 1, counts[i]);
    }

    if (fp != stdin) {
        fclose(fp);
    }
}
