#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BLOCKSIZE 16384 // bytes - 16 KB
#define MAXFILENAME 32  // maximum filename that can be stored by SFS

/* Utility functions */
int read_block(int fd_disk, void *block, int k);
int write_block(int fd_disk, void *block, int k);

struct superblock {
    int num_blocks;
    // ....
} superblock;

struct direnty {
    char filename[MAXFILENAME];
    // ....
};

// ********** Global Variables ***************************************
int disk;
char diskname[64];

int mfs_format() {
    char buffer[BLOCKSIZE];
    // struct superblock sb;

    printf("formatting disk %s with the file system...\n", diskname);
    bzero(buffer, BLOCKSIZE);
    write_block(disk, buffer, 0); // write superblock info
    // init superblock
    // init bitmap
    // init inodemap
    // init inode tables
    // init root directory

    fsync(disk);
    return (0);
}

int main(int argc, char *argv[]) {

    // initialize the  disk
    if (argc != 2) {
        printf("usage: make_sfs <diskname>\n");
        exit(0);
    }

    strcpy(diskname, argv[1]);

    disk = open(diskname, O_RDWR);
    printf("opened disk...\n");

    mfs_format();

    printf("closing disk...\n");
    close(disk);

    return 0;
}

// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk.
int read_block(int fd_disk, void *block, int k) {
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(fd_disk, (off_t)offset, SEEK_SET);
    n = read(fd_disk, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf("read error\n");
        return -1;
    }
    return (0);
}

// write block k into the virtual disk.
int write_block(int fd_disk, void *block, int k) {
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(fd_disk, (off_t)offset, SEEK_SET);
    n = write(fd_disk, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf("write error\n");
        return (-1);
    }
    return 0;
}
