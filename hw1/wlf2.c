#include "dll.c"
#include <bits/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

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

// This file uses the APIs for io given in the stdlib like open, read, write,
// and close

#define wordLen 64

Dll *scanToList(int);

int main(int argc, char *argv[]) {
    char buf[256];
    int fd;
    if (argc == 1) {
        fd = STDIN_FILENO;
    } else if (argc == 2) {
        // Using file
        char *filename = argv[1];
        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            sprintf(buf, "Error: file '%s' can't be opened\n", filename);
            write(STDERR_FILENO, buf, strlen(buf));
            exit(1);
        }
    } else {
        char buf[256];
        sprintf(buf, "Error: usage: %s [filename]\n", argv[0]);
        write(STDERR_FILENO, buf, strlen(buf));
        exit(1);
    }

    struct timespec start, finish;

    clock_gettime(CLOCK_MONOTONIC, &start);
    Dll *list = scanToList(fd);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    long sec = finish.tv_sec - start.tv_sec;
    long nanosec = finish.tv_nsec - start.tv_nsec;

    sprintf(buf,
            "Time taken to read and insert into doubly linked list: %ld sec, "
            "%ld nanosec\n",
            sec, nanosec);
    write(STDOUT_FILENO, buf, strlen(buf));

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
        sprintf(buf, "%d %zd\n", i + 1, counts[i]);
        write(STDOUT_FILENO, buf, strlen(buf));
    }

    if (fd != STDIN_FILENO) {
        close(fd);
    }
}

Dll *scanToList(int fd) {
    Dll *list = newDll();
    // Buffer is short to make sure that word boundaries work between buffers
    char buf[4];
    char *curWord = NULL;
    int curLen = 0;
    int capacity = 10;
    int n;

    while ((n = read(fd, buf, 4)) > 0) {
        for (int i = 0; i < n; i++) {
            if (isspace(buf[i])) {
                if (curWord != NULL) {
                    curWord[curLen] = '\0';
                    dllAppend(list, curWord);
                    free(curWord);
                    curWord = NULL;
                    curLen = 0;
                    capacity = 10;
                }
            } else {
                // Resize if needed
                if (curLen + 1 >= capacity) {
                    capacity *= 2;
                    curWord = (char *)realloc(curWord, capacity);
                }
                if (curWord == NULL) {
                    curWord = (char *)malloc(capacity);
                }
                curWord[curLen] = buf[i];
                curLen++;
            }
        }
    }

    if (curWord != NULL) {
        curWord[curLen] = '\0';
        dllAppend(list, curWord);
        free(curWord);
    }

    return list;
}
