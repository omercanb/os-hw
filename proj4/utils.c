#include "utils.h"
#include "limits.h"
#include <assert.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int fd_disk;

char inode_map[BLOCKSIZE];
bool is_inode_free(int inode_num) {
    // TODO make this more efficient
    // Also check if this always works correctly, like if inode map can be edited before it is read into
    read_block(&inode_map, BLOCK_INODE_MAP);
    assert(inode_num < MAX_NUM_FILES);
    return !_bit_get(inode_map, inode_num);
}

// superblock get_superblock() {
//     superblock block;
//     read_block(&block, BLOCK_SUPERBLOCK);
//     return block;
// }
// void set_superblock(superblock block) {
//     write_block(&block, BLOCK_SUPERBLOCK);
// }

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
    direntry *root_block = malloc_and_read_block(BLOCK_ROOT_DIR);
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
    int inode_num = _allocate_bit(BLOCK_INODE_MAP);
    if (inode_num == -1) {
        return -ENOSPC;
    }

    _create_direntry(filename, inode_num);

    int index_block_num = _create_index_block();
    inode f = _create_inode(inode_num, index_block_num);

    // Allocate an initial data block to start
    _allocate_data_block(f);
    return inode_num;
}

int delete_file(const char *filename) {
    // Only setting the used bit is enough to delete it
    int inode_num = _find_inode_num(filename);
    if (inode_num == -1) {
        return -1;
    }
    // Free the inode
    _free_bit(BLOCK_INODE_MAP, inode_num);

    // Free the index block
    inode inode = _get_inode(inode_num);
    _free_bit(BLOCK_BITMAP, inode.index_block_num);
    return 0;
}

int append_to_file(const char *filename, const char *buf, int size) {
    printf("Appending\n");
    int inode_num = _find_inode_num(filename);
    printf("inode num: %d\n", inode_num);
    if (inode_num == -1) {
        return -1;
    }
    inode inode = _get_inode(inode_num);
    int file_size = inode.st_size;
    // while there is remaining content to write
    int buf_offset = 0;
    char *cur_data_block = malloc_block();

    // Read index block
    assert(inode.index_block_num != -1);
    // TODO refactor this combo of malloc and read to do them together
    int *index_block = malloc_and_read_block(inode.index_block_num);

    // TODO remove
    int data_block_num = _allocate_data_block(inode);
    printf("One append done\n");
    assert(data_block_num != -1);

    int data_block_idx = file_size / BLOCKSIZE;
    int data_block_offset = file_size % BLOCKSIZE;
    // If there is a non-empty data file to write in
    // if (data_block_offset != 0) {
    // Get the last block
    int cur_data_block_num = index_block[data_block_idx];
    assert(cur_data_block_num != -1);
    read_block(cur_data_block, cur_data_block_num);
    // Calculate how much to write
    int free_bytes_in_data_block = BLOCKSIZE - data_block_offset;
    int write_amount = min(size, free_bytes_in_data_block);
    // Write it
    memcpy(&cur_data_block[data_block_offset], buf, write_amount);
    write_block(cur_data_block, cur_data_block_num);
    buf_offset += write_amount;
    file_size += write_amount;
    // }
    free(index_block);
    free(cur_data_block);
    inode.st_size = file_size;
    _save_inode(inode);
    int num_bytes_written = buf_offset;
    return num_bytes_written;

    // // If there is a data block available write into it as much as you can
    // // Continue allocating a data block and writing into it
    // while (bytes_written > 0) {
    //     int data_block_idx = file_size / BLOCKSIZE;
    //     int data_block_offset = file_size % BLOCKSIZE;
    //     int cur_data_block_num = index_block[data_block_idx];
    //     if (cur_data_block_num == -1) {
    //         cur_data_block_num = _allocate_data_block(inode);
    //         if (cur_data_block_num == -1) {
    //             free(index_block);
    //             free(cur_data_block);
    //             return -1;
    //         }
    //     }
    //     assert(cur_data_block_num != -1);
    //     read_block(cur_data_block, cur_data_block_num);
    //     int free_bytes_in_data_block = BLOCKSIZE - data_block_offset;
    //     int write_amount;
    //     if (bytes_written <= free_bytes_in_data_block) {
    //         write_amount = bytes_written;
    //     } else {
    //         write_amount = free_bytes_in_data_block;
    //     }
    //     memcpy(&cur_data_block[data_block_offset], &buf[buf_offset], write_amount);
    //     write_block(cur_data_block, cur_data_block_num);
    //     bytes_written -= write_amount;
    //     buf_offset += write_amount;
    //     file_size += write_amount;
    // }
    //
    // inode.st_size = file_size;
    // _save_inode(inode);
    //
    // free(index_block);
    // free(cur_data_block);
}

int read_file(const char *filename, char *buf, int size, int offset) {
    inode inode;
    int res = get_inode(filename, &inode);
    if (res == -1) {
        return -1;
    }
    char *data = malloc_block();
    int block_num = _get_data_block(inode, 0, data);
    memcpy(buf, data, size);
    free(data);
    return size;
    //
    // char *cur_data_block = malloc_block();
    //
    // // Read index block
    // assert(inode.index_block_num != -1);
    // int *index_block = malloc_block();
    // read_block(index_block, inode.index_block_num);
    //
    // int data_block_idx = offset / BLOCKSIZE;
    // int data_block_offset = offset % BLOCKSIZE;
    // int cur_data_block_num = index_block[data_block_idx];
    // assert(cur_data_block_num != -1);
    // read_block(cur_data_block, cur_data_block_num);
    // int read_amount = size;
    // memcpy(buf, cur_data_block, read_amount);
    // free(cur_data_block);
    // free(index_block);
    // return read_amount;
}

void _create_direntry(const char *filename, int inode_num) {
    direntry *root_block = malloc_and_read_block(BLOCK_ROOT_DIR);
    direntry dir = root_block[inode_num];
    strcpy(dir.filename, filename);
    dir.inode_num = inode_num;
    root_block[inode_num] = dir;
    write_block(root_block, BLOCK_ROOT_DIR);
    free(root_block);
}

inode _create_inode(int inode_num, int index_block_num) {
    int i = inode_num / sizeof(inode);
    int offset = inode_num % sizeof(inode);
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode *block = malloc_and_read_block(block_num);
    inode inode = block[offset];
    inode.inode_num = inode_num;
    inode.index_block_num = index_block_num;
    inode.st_size = 0;
    block[offset] = inode;
    write_block(block, block_num);
    free(block);
    return inode;
}

inode _get_inode(int inode_num) {
    int i = inode_num / sizeof(inode);
    int offset = inode_num % sizeof(inode);
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode *block = malloc_and_read_block(block_num);
    inode inode = block[offset];
    free(block);
    return inode;
}

void _save_inode(inode f) {
    int inode_num = f.inode_num;
    int i = inode_num / sizeof(inode);
    int offset = inode_num % sizeof(inode);
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode *block = malloc_and_read_block(block_num);
    block[offset] = f;
    write_block(block, block_num);
    free(block);
}

int _create_index_block() {
    int block_num = _allocate_bit(BLOCK_BITMAP);
    printf("block num: %d\n", block_num);
    if (block_num == -1) {
        return -1;
    }
    int *empty_index = malloc_block();
    for (int i = 0; i < INDEX_LENGTH; i++) {
        empty_index[i] = -1;
    }
    write_block(empty_index, block_num);
    free(empty_index);
    return block_num;
}

void print_root_dir() {
    direntry *root_block = malloc_and_read_block(BLOCK_ROOT_DIR);
    printf("Root Directory\n");
    for (int i = 0; i < MAX_NUM_FILES; i++) {
        direntry f = root_block[i];
        if (is_inode_free(i)) {
            continue;
        }
        inode inode = _get_inode(f.inode_num);
        printf("i: %d, inode_num: %d, name: %s | inode - inode_num: %d, size: %d\n", i, f.inode_num, f.filename, inode.inode_num, inode.st_size);
    }
    free(root_block);
}

void print_block_num(int block_num) {
    char *block = malloc_block();
    read_block(block, block_num);
    print_block(block);
    free(block);
}

void print_block(char *block) {
    for (int i = 0; i < BLOCKSIZE; i++) {
        if (block[i] == 0) {
            printf("\\0");
        } else {
            printf("%c", block[i]);
        }
    }
    printf("\n");
}

int get_inode(const char *filename, inode *out) {
    int i = _find_inode_num(filename);
    if (i == -1) {
        return -1;
    }
    *out = _get_inode(i);
    return 0;
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

int _get_data_block(inode f, int idx, void *out) {
    int *index_block = malloc_block();
    read_block(index_block, f.index_block_num);
    int data_block_num = index_block[idx];
    if (data_block_num == -1) {
        free(index_block);
        return -1;
    }
    read_block(out, data_block_num);
    free(index_block);
    return data_block_num;
}

bool _bit_get(const char *block, int i) {
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

void _bit_set(char *block, int i, bool val) {
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
// TODO remove
// int _get_first_free_bit(char *block, int bound) {
//     for (int i = 0; i < bound; i++) {
//         printf("%d: %d", i, _bit_get(block, i));
//         if (_bit_get(block, i) == false) {
//             return i;
//         }
//     }
//     return -1;
// }

// Find free bit set it to 1 and return its index, returns -1 on not finding a free bit
int _allocate_bit(int block_num) {
    assert(block_num == BLOCK_BITMAP || block_num == BLOCK_INODE_MAP);
    int bound;
    if (block_num == BLOCK_INODE_MAP) {
        bound = MAX_NUM_FILES;
    } else {
        bound = BLOCKSIZE * 8;
    }
    char *block = malloc_block();
    read_block(block, block_num);
    int free_idx = -1;
    for (int i = 0; i < bound; i++) {
        if (_bit_get(block, i) == false) {
            free_idx = i;
            break;
        }
    }
    if (free_idx == -1) {
        free(block);
        return -1;
    }
    _bit_set(block, free_idx, 1);
    write_block(block, block_num);
    free(block);
    return free_idx;
}

int _free_bit(int block_num, int idx) {
    assert(block_num == BLOCK_BITMAP || block_num == BLOCK_INODE_MAP);
    int bound;
    if (block_num == BLOCK_INODE_MAP) {
        bound = MAX_NUM_FILES;
    } else {
        bound = BLOCKSIZE * 8;
    }
    if (idx < 0 || idx >= bound) {
        return -1;
    }
    char *block = malloc_and_read_block(block_num);
    _bit_set(block, idx, 0);
    write_block(block, block_num);
    free(block);
    return 0;
}

// Find an empty index entry in the inode's index block, then find a free data block and assign the data block to the free entry in the index
// Returns a -1 when there is not enough space for one of the two operations
int _allocate_data_block(inode inode) {
    int index_block_num = inode.index_block_num;
    int *index_block = malloc_and_read_block(index_block_num);
    int free_idx = _get_free_index_entry(index_block);
    if (free_idx == -1) {
        free(index_block);
        return -1;
    }
    int free_data_block_num = _allocate_bit(BLOCK_BITMAP);
    if (free_data_block_num == -1) {
        free(index_block);
        return -1;
    }
    index_block[free_idx] = free_data_block_num;
    write_block(index_block, index_block_num);
    free(index_block);
    return free_data_block_num;
}

int _get_free_index_entry(int *index_block) {
    for (int i = 0; i < INDEX_LENGTH; i++) {
        if (index_block[i] == -1) {
            return i;
        }
    }
    return -1;
}

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

void *malloc_and_read_block(int k) {
    void *block = malloc_block();
    read_block(block, k);
    return block;
}

// Allocate space in memory for a block
void *malloc_block() {
    return calloc(BLOCKSIZE, sizeof(char));
}

int min(int a, int b) {
    if (a <= b) {
        return a;
    } else {
        return b;
    }
}
