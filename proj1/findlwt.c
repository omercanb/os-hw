#include "findlw.c"
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void processFileMultiProcess(int fileNo);
void processFilesMultiProcess();
void *processFileWorker(void *arg);

char *filePrefix;
int numFiles;
int longWordLimit;
int dataLen;
char *outfileName;

WordList *lists;

int main(int argc, char **argv) {
    // findlwt fileprefix, n, k, outfile
    if (argc != 5) {
        fprintf(stderr, "usage:  findlw fileprefix, n, k, outfile\n");
        exit(1);
    }

    filePrefix = argv[1];
    numFiles = atoi(argv[2]);
    longWordLimit = atoi(argv[3]);
    outfileName = argv[4];

    lists = calloc(sizeof(WordList), numFiles);

    processFilesMultiProcess();
    return 0;
}

void processFilesMultiProcess() {
    pthread_t threads[numFiles];
    for (int i = 0; i < numFiles; i++) {
        int fileNo = i + 1;
        pthread_create(&threads[i], NULL, processFileWorker, (void *)fileNo);
    }
    for (int i = 0; i < numFiles; i++) {
        pthread_join(threads[i], NULL);
    }

    // Merge all lists
    WordList completeList;
    listInit(&completeList);
    for (int i = 0; i < numFiles; i++) {
        WordList currentList = lists[i];
        for (int j = 0; j < currentList.size; j++) {
            listAppend(&completeList, currentList.data[j].word,
                       currentList.data[j].fileNo, currentList.data[j].lineNo);
        }
    }
    listSort(&completeList);
    FILE *outfile = fopen(outfileName, "w");
    printLongWords(completeList, outfile);

    for (int i = 0; i < numFiles; i++) {
        listFree(&lists[i]);
    }
    free(lists);
}

void *processFileWorker(void *arg) {
    int fileNo = (int)arg;
    processFileMultiProcess(fileNo);
    return NULL;
}

// Get the long words in file and send it through pipe
void processFileMultiProcess(int fileNo) {
    FILE *fp = getFile(filePrefix, fileNo);
    WordList list = getLongWords(fp, fileNo, longWordLimit);
    listSort(&list);
    int idx = fileNo - 1;
    lists[idx] = list;
}
