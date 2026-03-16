#include "findlw.c"
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void processFileMultiProcess(int fileNo, int pipeWriteFd);
void processFilesMultiProcess();
int (*getPipeFileDescriptors(int numPipes))[2];
void createProcesses(int (*pipeFileDescriptors)[2]);
void waitForProcesses(int (*pipeFileDescriptors)[2]);
Buffer serialize(WordList list);
WordList deserialize(Buffer buf);
void sendListToParent(WordList list, int pipeWriteFd);
void readFromChild(Buffer *collectBuf, int pipeReadFd);
void sendLongWordsToParent(FILE *fp, int pipeWriteFd);
Buffer serializeWord(char *str, int lineNo);
void sendLongWordToParent(char *word, int lineNo, int pipeWriteFd);

char *filePrefix;
int numFiles;
int longWordLimit;
int dataLen;
char *outfileName;

int main(int argc, char **argv) {
    // First create a program that will just transmit the number of long
    // words in each file findlw fileprefix, n, k, datalen, outfile
    if (argc != 6) {
        fprintf(stderr, "usage:  findlw fileprefix, n, k, datalen, outfile\n");
        exit(1);
    }

    filePrefix = argv[1];
    numFiles = atoi(argv[2]);
    longWordLimit = atoi(argv[3]);
    dataLen = atoi(argv[4]);
    outfileName = argv[5];

    processFilesMultiProcess();
    return 0;
}

void processFilesMultiProcess() {
    int (*pfds)[2] = getPipeFileDescriptors(numFiles);
    createProcesses(pfds);
    waitForProcesses(pfds);
}

// Get the long words in file and send it through pipe
void processFileMultiProcess(int fileNo, int pipeWriteFd) {
    FILE *fp = getFile(filePrefix, fileNo);
    sendLongWordsToParent(fp, pipeWriteFd);
    // WordList list = getLongWords(fp, fileNo, longWordLimit);
    // sendListToParent(list, pipeWriteFd);
}

void waitForProcesses(int (*pipeFileDescriptors)[2]) {
    struct pollfd *pollFd;
    pollFd = calloc(numFiles, sizeof(*pollFd));
    for (int i = 0; i < numFiles; i++) {
        pollFd[i].fd = pipeFileDescriptors[i][0];
        pollFd[i].events = POLLIN | POLLHUP;
    }

    int hangups = 0;
    // Create buffers to collect from children
    // Collect buffers are the buffers we use to collect the serialized lists
    // from the children
    Buffer collectBuffers[numFiles];
    for (int i = 0; i < numFiles; i++) {
        bufInit(&collectBuffers[i]);
    }
    while (hangups < numFiles) {
        int ready = poll(pollFd, numFiles, -1);
        if (ready == -1) {
            exit(-1);
        }
        // Loop over and check the return events
        // Need to hold numFiles buffers
        for (int i = 0; i < numFiles; i++) {
            if (pollFd[i].revents & POLLIN) {
                readFromChild(&collectBuffers[i], pollFd[i].fd);
            }
            if (pollFd[i].revents & POLLHUP) {
                int len;
                char readBuf[dataLen];
                while ((len = read(pollFd[i].fd, readBuf, dataLen)) > 0) {
                    for (int k = 0; k < len; k++)
                        bufAppend(&collectBuffers[i], readBuf[k]);
                }
                hangups++;
                close(pollFd[i].fd);
                pollFd[i].fd = -1;
                waitpid(-1, NULL, WNOHANG);
            }
        }
    }

    // Deserialize all lists
    WordList deserializedLists[numFiles];
    for (int i = 0; i < numFiles; i++) {
        deserializedLists[i] = deserialize(collectBuffers[i]);
    }

    // Merge all lists
    WordList completeList;
    listInit(&completeList);
    for (int i = 0; i < numFiles; i++) {
        WordList currentList = deserializedLists[i];
        for (int j = 0; j < currentList.size; j++) {
            int fileNo = i + 1;
            listAppend(&completeList, currentList.data[j].word, fileNo,
                       currentList.data[j].lineNo);
        }
    }
    listSort(&completeList);
    FILE *outfile = fopen(outfileName, "w");
    printLongWords(completeList, outfile);
    for (int i = 0; i < numFiles; i++) {
        bufFree(&collectBuffers[i]);
    }
}

void sendLongWordsToParent(FILE *fp, int pipeWriteFd) {
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
        // if the longWordLimit is given as 0, on an empty file this will
        // cause it to append an empty buffer
        if (buf.size >= longWordLimit && buf.size > 0) {
            bufAppend(&buf, '\0');
            sendLongWordToParent(buf.data, wordLineNo, pipeWriteFd);
        }

        bufFree(&buf);
        if (c == EOF) {
            break;
        }
    }
}

// Read some chunks from the child (not full serialized list)
void readFromChild(Buffer *collectBuf, int pipeReadFd) {
    char readBuf[dataLen];
    int len = read(pipeReadFd, readBuf, dataLen);
    // printf("Read %d from %d \n", len, pipeReadFd);
    if (len == -1) {
        printf("Issue in reading from child pipe\n");
        exit(-1);
    }
    for (int i = 0; i < len; i++) {
        bufAppend(collectBuf, readBuf[i]);
    }
}

void sendLongWordToParent(char *word, int lineNo, int pipeWriteFd) {
    Buffer serialized = serializeWord(word, lineNo);
    // Send in chunks of dataLen
    // Send all except the last chunk that is less than dataLen
    int i = 0;
    for (; i < serialized.size - dataLen; i += dataLen) {
        char *dataStart = &serialized.data[i];
        int bytes = write(pipeWriteFd, dataStart, dataLen);
        if (bytes == -1) {
            printf("An issue writing to parent from pipe\n");
            exit(-1);
        }
    }
    // Calculate the last chunk using where we left off
    int lastChunkSize = serialized.size - i;
    char *lastChunk = &serialized.data[i];
    write(pipeWriteFd, lastChunk, lastChunkSize);
}

// Serializes and sends the list in chunks of dataLen
void sendListToParent(WordList list, int pipeWriteFd) {
    Buffer serialized = serialize(list);
    // Send in chunks of dataLen
    // Send all except the last chunk that is less than dataLen
    int i = 0;
    for (; i < serialized.size - dataLen; i += dataLen) {
        char *dataStart = &serialized.data[i];
        int bytes = write(pipeWriteFd, dataStart, dataLen);
        if (bytes == -1) {
            printf("An issue writing to parent from pipe\n");
            exit(-1);
        }
    }
    // Calculate the last chunk using where we left off
    int lastChunkSize = serialized.size - i;
    char *lastChunk = &serialized.data[i];
    printf("Pipe %d done writing\n", pipeWriteFd);
    write(pipeWriteFd, lastChunk, lastChunkSize);
    close(pipeWriteFd);
}

// Format: array of (string lineNo)
// We don't have to send fileNo because the parent keeps track
Buffer serialize(WordList list) {
    Buffer buf;
    bufInit(&buf);
    for (int i = 0; i < list.size; i++) {
        char *str = list.data[i].word;
        int n = strlen(str);
        for (int j = 0; j < n; j++) {
            bufAppend(&buf, str[j]);
        }
        bufAppend(&buf, '\0');

        int lineNo = list.data[i].lineNo;
        bufAppend(&buf, (lineNo >> 24) & 0xFF);
        bufAppend(&buf, (lineNo >> 16) & 0xFF);
        bufAppend(&buf, (lineNo >> 8) & 0xFF);
        bufAppend(&buf, lineNo & 0xFF);
    }
    return buf;
};

Buffer serializeWord(char *str, int lineNo) {
    Buffer buf;
    bufInit(&buf);
    int n = strlen(str);
    for (int j = 0; j < n; j++) {
        bufAppend(&buf, str[j]);
    }
    bufAppend(&buf, '\0');

    bufAppend(&buf, (lineNo >> 24) & 0xFF);
    bufAppend(&buf, (lineNo >> 16) & 0xFF);
    bufAppend(&buf, (lineNo >> 8) & 0xFF);
    bufAppend(&buf, lineNo & 0xFF);
    return buf;
}

// HAS NO FILENO
WordList deserialize(Buffer buf) {
    WordList list;
    listInit(&list);
    int i = 0;
    while (i < buf.size) {
        // Read in string
        Buffer str;
        bufInit(&str);
        char data;
        while ((data = buf.data[i]) != '\0') {
            bufAppend(&str, data);
            i++;
        }
        bufAppend(&str, '\0');
        // Skip '\0'
        i++;
        int lineNo = 0;
        lineNo = lineNo | (buf.data[i] << 24);
        i++;
        lineNo = lineNo | (buf.data[i] << 16);
        i++;
        lineNo = lineNo | (buf.data[i] << 8);
        i++;
        lineNo = lineNo | buf.data[i];
        i++;
        listAppend(&list, str.data, -100000, lineNo);
        bufFree(&str);
    }
    return list;
}

int (*getPipeFileDescriptors(int numPipes))[2] {
    int (*pfds)[2];
    pfds = calloc(numPipes, sizeof(int[2]));
    for (int i = 0; i < numPipes; i++) {
        int res = pipe(pfds[i]);
        if (res == -1)
            exit(-1);
        // Make the read ends non blocking
        int flags = fcntl(pfds[i][0], F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(pfds[i][0], F_SETFL, flags);
    }
    return pfds;
}

void createProcesses(int (*pipeFileDescriptors)[2]) {
    for (int fileNo = 1; fileNo <= numFiles; fileNo++) {
        switch (fork()) {
        case -1:
            exit(-1);
        case 0: // Child
            close(pipeFileDescriptors[fileNo - 1][0]);
            processFileMultiProcess(fileNo, pipeFileDescriptors[fileNo - 1][1]);
            exit(0);
        default: // Parent
            close(pipeFileDescriptors[fileNo - 1][1]);
            break;
        }
    }
}
