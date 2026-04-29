#include "utils.h"
#include <assert.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int fd_disk;

// Read block k from disk (virtual disk) into buffer block.
// Size of the block is BLOCKSIZE.
// Memory space for block must be allocated outside of this function.
// Block numbers start from 0 in the virtual disk.
int read_block(void *block, int k) {
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
int write_block(void *block, int k) {
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

// Allocate space in memory for a block
void *malloc_block() {
    return calloc(BLOCKSIZE, sizeof(char));
}

bool bit_get(const char *block, int i) {
    assert(i / 8 < BLOCKSIZE);
    char num = block[i / 8];
    char offset = i % 8;
    char mask = (1 << 7) >> offset;
    if ((num & mask) == mask) {
        return true;
    } else {
        return false;
    }
}

void bit_set(char *block, int i, bool val) {
    assert(i / 8 < BLOCKSIZE);
    char num = block[i / 8];
    char offset = i % 8;
    if (val) {
        char mask = (1 << 7) >> offset;
        num = num | mask;
    } else {
        char mask = ~((1 << 7) >> offset);
        num = num & mask;
    }
    block[i / 8] = num;
}

// Free means 0 bit
int get_first_free_bit(char *block, int bound) {
    for (int i = 0; i < bound; i++) {
        printf("%d: %d", i, bit_get(block, i));
        if (bit_get(block, i) == false) {
            return i;
        }
    }
    return -1;
}

char inode_map[BLOCKSIZE];
bool inode_map_valid = false;
bool is_inode_free(int inode_num) {
    if (!inode_map_valid) {
        read_block(&inode_map, BLOCK_INODE_MAP);
        inode_map_valid = true;
    }
    // TODO remove to make more efficient
    // Maybe don't need at all if the in memory one is always kept updated
    read_block(&inode_map, BLOCK_INODE_MAP);
    assert(inode_num < MAX_NUM_FILES);
    return !bit_get(inode_map, inode_num);
}

superblock get_superblock() {
    superblock block;
    read_block(&block, BLOCK_SUPERBLOCK);
    return block;
}
void set_superblock(superblock block) {
    write_block(&block, BLOCK_SUPERBLOCK);
}

// Tries to find the file in root, returns -1 on error
int get_file(const char *filename, inode *out) {
    int inode_num = _find_inode_num(filename);
    if (inode_num == -1) {
        return -1;
    }
    inode inode = _get_inode(inode_num);
    memcpy(out, &inode, sizeof(inode));
    return 0;
}

bool file_exists(const char *filename) {
    direntry *root_block = malloc_block();
    read_block(root_block, BLOCK_ROOT_DIR);
    int inode_num = -1;
    for (int i = 0; i < MAX_NUM_FILES; i++) {
        direntry f = root_block[i];
        if (is_inode_free(i)) {
            continue;
        }
        if (strcmp(filename, f.filename) == 0) {
            inode_num = i;
            break;
        }
    }
    free(root_block);
    if (inode_num == -1) {
        return false;
    } else {
        return true;
    }
}

int create_file(const char *filename) {
    if (file_exists(filename)) {
        return -EEXIST;
    }

    // Search the inode map for an unused inode
    char *inode_map_block = malloc_block();
    read_block(inode_map_block, BLOCK_INODE_MAP);
    int inode_num = get_first_free_bit(inode_map_block, MAX_NUM_FILES);
    if (inode_num == -1) {
        free(inode_map_block);
        return -ENOSPC;
    }

    _create_direntry(filename, inode_num);
    _create_inode(inode_num);

    // Set the inode to use
    bit_set(inode_map_block, inode_num, true);
    write_block(inode_map_block, BLOCK_INODE_MAP);
    inode_map_valid = false;

    free(inode_map_block);
    return inode_num;
}

int delete_file(const char *filename) {
    // Only setting the used bit is enough to delete it
    int inode_num = _find_inode_num(filename);
    if (inode_num == -1) {
        return -1;
    }
    bit_set(inode_map, inode_num, 0);
    write_block(inode_map, BLOCK_INODE_MAP);
    return 0;
}

int _create_direntry(const char *filename, int inode_num) {
    direntry *root_block = malloc_block();
    read_block(root_block, BLOCK_ROOT_DIR);
    direntry dir = root_block[inode_num];
    assert(is_inode_free(inode_num) == true);
    strcpy(dir.filename, filename);
    dir.inode_num = inode_num;
    root_block[inode_num] = dir;
    write_block(root_block, BLOCK_ROOT_DIR);
    free(root_block);
}

void _create_inode(int inode_num) {
    int i = inode_num / sizeof(inode);
    int offset = inode_num % sizeof(inode);
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode *block = malloc_block();
    read_block(block, block_num);
    inode inode = block[offset];
    inode.inode_num = inode_num;
    inode.st_size = 0;
    block[offset] = inode;
    write_block(block, block_num);
    free(block);
}

inode _get_inode(int inode_num) {
    int i = inode_num / sizeof(inode);
    int offset = inode_num % sizeof(inode);
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode *block = malloc_block();
    read_block(block, block_num);
    inode inode = block[offset];
    return inode;
}

direntry *get_files() {
    direntry *root_block = malloc_block();
    printf("alloced\n");
    read_block(root_block, BLOCK_ROOT_DIR);
    return root_block;
}

void print_root_dir() {
    direntry *root_block = malloc_block();
    printf("Root Directory\n");
    read_block(root_block, BLOCK_ROOT_DIR);
    for (int i = 0; i < MAX_NUM_FILES; i++) {
        direntry f = root_block[i];
        if (is_inode_free(i)) {
            continue;
        }
        inode inode = _get_inode(f.inode_num);
        printf("i: %d, inode_num: %d, name: %s | inode - inode_num: %d, size: %d\n", i, f.inode_num, f.filename, inode.inode_num, inode.st_size);
    }
}

// Searches for filename in the root, returns -1 on error
int _find_inode_num(const char *filename) {
    direntry *root_block = malloc_block();
    read_block(root_block, BLOCK_ROOT_DIR);
    int inode_num = -1;
    for (int i = 0; i < MAX_NUM_FILES; i++) {
        direntry f = root_block[i];
        if (is_inode_free(i)) {
            continue;
        }
        if (strcmp(filename, f.filename) == 0) {
            inode_num = i;
            break;
        }
    }
    free(root_block);

    if (inode_num == -1) {
        // Not found
        return -1;
    }
    return inode_num;
}
