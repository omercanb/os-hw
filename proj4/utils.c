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

bool is_inode_free(int inode_num) {
    char inode_map[BLOCKSIZE];
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
    direntry root_block[BLOCKSIZE / sizeof(direntry)];
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
    inode ino = _create_inode(inode_num, index_block_num);
    return inode_num;
}

int delete_file(const char *filename) {
    inode ino;
    if (get_inode(filename, &ino) == -1) return -1;

    // Free the inode number from the inode map
    _free_bit(BLOCK_INODE_MAP, ino.inode_num);

    // Load the index to free all data blocks
    int index[BLOCKSIZE / sizeof(int)];
    read_block(index, ino.index_block_num);

    // Load the bitmap to do frees in memory before writing
    char bitmap[BLOCKSIZE];
    read_block(bitmap, BLOCK_BITMAP);

    // Free data blocks
    int num_blocks = (ino.st_size + BLOCKSIZE - 1) / BLOCKSIZE;
    for (int i = 0; i < num_blocks; i++) {
        _free_entry(bitmap, index[i]);
    }
    // Free the index block itself
    _free_entry(bitmap, ino.index_block_num);
    write_block(bitmap, BLOCK_BITMAP);
    return 0;
}

int read_file(const char *filename, char *buf, int size, int offset) {
    inode ino;
    if (get_inode(filename, &ino) == -1) return -1; // File not found
    // Clamp the size to what exists in the file
    // Return EOF or shorten the request
    if (ino.st_size <= offset) return 0;
    if (ino.st_size < offset + size) size = ino.st_size - offset;

    // Load the file's index block
    int index[INDEX_LENGTH];
    read_block(index, ino.index_block_num);

    char block_buf[BLOCKSIZE];
    int bytes_done = 0;
    while (bytes_done < size) {
        int file_offset = offset + bytes_done;          // The place in the file to read
        int block_num = index[file_offset / BLOCKSIZE]; // The data block to read
        int block_offset = file_offset % BLOCKSIZE;     // Place inside block to start read from

        if (read_block(block_buf, block_num) < 0) return -1;

        int bytes_left_in_block = BLOCKSIZE - block_offset;
        int bytes_left_in_request = size - bytes_done;
        int chunk_size = min(bytes_left_in_block, bytes_left_in_request);
        memcpy(buf + bytes_done, block_buf + block_offset, chunk_size);
        bytes_done += chunk_size;
    }
    return bytes_done;
}

int write_file(const char *filename, const char *buf, int size, off_t offset) {
    inode ino;
    if (get_inode(filename, &ino) == -1) return -1; // File not found

    // Load the file's index block to be used later
    int index[INDEX_LENGTH];
    if (read_block(index, ino.index_block_num) == -1) return -EIO;

    int write_end = size + offset;
    int new_size = max(ino.st_size, write_end);
    if (new_size > MAX_FILE_SIZE) return -EFBIG;

    // If we need to write past where the file already exists, we may need to allocate more data blocks
    if (new_size > ino.st_size) {
        // Allocate all the data blocks needed
        int new_num_blocks = (new_size + BLOCKSIZE - 1) / BLOCKSIZE;
        int old_num_blocks = (ino.st_size + BLOCKSIZE - 1) / BLOCKSIZE;
        int blocks_to_alloc = new_num_blocks - old_num_blocks;

        // If we need to actually allocate more data blocks
        if (blocks_to_alloc > 0) {
            // Load the free data blocks
            char bitmap[BLOCKSIZE];
            if (read_block(bitmap, BLOCK_BITMAP) == -1) return -EIO;

            for (int i = old_num_blocks; i < new_num_blocks; i++) {
                int new_block_num = _allocate_entry(bitmap);
                if (new_block_num == -1) {
                    // Out of space, free entries
                    for (int j = old_num_blocks; j < i; j++) {
                        _free_entry(bitmap, index[j]);
                    }
                    return -ENOSPC;
                }
                index[i] = new_block_num;
            }
            write_block(index, ino.index_block_num);
            write_block(bitmap, BLOCK_BITMAP);
        }
    }
    // Finish allocating new data blocks

    char block_buf[BLOCKSIZE];
    int bytes_done = 0;
    while (bytes_done < size) {
        int file_offset = offset + bytes_done;          // The place in the logical file to write to
        int block_num = index[file_offset / BLOCKSIZE]; // The data block to write to
        int block_offset = file_offset % BLOCKSIZE;     // Place inside block to start writing to

        int bytes_left_in_block = BLOCKSIZE - block_offset;
        int bytes_left_in_request = size - bytes_done;
        int chunk_size = min(bytes_left_in_block, bytes_left_in_request);

        // If we will write a whole block we don't need to read first
        if (!(block_offset == 0 && chunk_size == BLOCKSIZE)) {
            if (read_block(block_buf, block_num) < 0) return -EIO;
        }

        memcpy(block_buf + block_offset, buf + bytes_done, chunk_size);
        write_block(block_buf, block_num);
        bytes_done += chunk_size;
    }
    if (new_size != ino.st_size) {
        ino.st_size = new_size;
        _save_inode(ino);
    }
    return bytes_done;
}

// Creates a new entry in the root directory by putting a
void _create_direntry(const char *filename, int inode_num) {
    direntry root_block[BLOCKSIZE / sizeof(direntry)];
    read_block(root_block, BLOCK_ROOT_DIR);
    direntry dir;
    strcpy(dir.filename, filename);
    root_block[inode_num] = dir;
    write_block(root_block, BLOCK_ROOT_DIR);
}

inode _create_inode(int inode_num, int index_block_num) {
    int inodes_per_block = BLOCKSIZE / sizeof(inode);
    int i = inode_num / inodes_per_block;
    int offset = inode_num % inodes_per_block;
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode block[BLOCKSIZE / sizeof(inode)];
    read_block(block, block_num);
    inode inode;
    inode.inode_num = inode_num;
    inode.index_block_num = index_block_num;
    inode.st_size = 0;
    block[offset] = inode;
    write_block(block, block_num);
    return inode;
}

inode _get_inode(int inode_num) {
    int inodes_per_block = BLOCKSIZE / sizeof(inode);
    int i = inode_num / inodes_per_block;
    int offset = inode_num % inodes_per_block;
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode block[BLOCKSIZE / sizeof(inode)];
    read_block(block, block_num);
    inode inode = block[offset];
    return inode;
}

void _save_inode(inode ino) {
    int inodes_per_block = BLOCKSIZE / sizeof(inode);
    int i = ino.inode_num / inodes_per_block;
    int offset = ino.inode_num % inodes_per_block;
    int block_num = BLOCK_INODE_TABLE_START + i;
    assert(block_num <= BLOCK_INODE_TABLE_END);
    inode block[BLOCKSIZE / sizeof(inode)];
    read_block(block, block_num);
    block[offset] = ino;
    write_block(block, block_num);
}

int _create_index_block() {
    int block_num = _allocate_bit(BLOCK_BITMAP);
    // printf("block num: %d\n", block_num);
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
    direntry root_block[BLOCKSIZE / sizeof(direntry)];
    read_block(root_block, BLOCK_ROOT_DIR);
    printf("Root Directory\n");
    for (int i = 0; i < MAX_NUM_FILES; i++) {
        direntry f = root_block[i];
        if (is_inode_free(i)) {
            continue;
        }
        inode inode = _get_inode(i);
        printf("i: %d, name: %s | inode - inode_num: %d, size: %d\n", i, f.filename, inode.inode_num, inode.st_size);
    }
}

void print_block_num(int block_num) {
    char *block = malloc_block();
    read_block(block, block_num);
    print_block(block);
    free(block);
}

void print_block(char *block) {
    for (int i = 0; i < 1000; i++) {
        // for (int i = 0; i < BLOCKSIZE; i++) {
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

// Write contents of the buffer to the data block, starting from block offset. Writes up to size or the number of free bytes in the data block, returns the number of bytes written.
int _write_data_block(int data_block_num, int block_offset, const char *buf, int size) {
    char cur_data_block[BLOCKSIZE];
    read_block(cur_data_block, data_block_num);
    int num_free_bytes = BLOCKSIZE - block_offset;
    int write_amount = min(size, num_free_bytes);
    // Write it
    memcpy(&cur_data_block[block_offset], buf, write_amount);
    write_block(cur_data_block, data_block_num);
    return write_amount;
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
    char block[BLOCKSIZE / sizeof(char)];
    read_block(block, block_num);
    _bit_set(block, idx, 0);
    write_block(block, block_num);
    return 0;
}

// Find an empty index entry in the inode's index block, then find a free data block and assign the data block to the free entry in the index
// Returns a -1 when there is not enough space for one of the two operations
int _allocate_data_block(inode inode) {
    int index_block_num = inode.index_block_num;
    int index_block[BLOCKSIZE / sizeof(int)];
    read_block(index_block, index_block_num);
    int free_idx = _get_free_index_entry(index_block);
    if (free_idx == -1) {
        return -1;
    }
    int free_data_block_num = _allocate_bit(BLOCK_BITMAP);
    if (free_data_block_num == -1) {
        return -1;
    }
    index_block[free_idx] = free_data_block_num;
    write_block(index_block, index_block_num);
    return free_data_block_num;
}

// Find a zero bit in the bitmap, set it to 1 and return the index
// Returns -1 when there's no free entry
// The size of the bitmap should be BLOCKSIZE
// Does not save to disk
int _allocate_entry(char *bitmap) {
    for (int i = 0; i < BLOCKSIZE * 8; i++) {
        if (_bit_get(bitmap, i) == false) {
            _bit_set(bitmap, i, 1);
            return i;
        }
    }
    return -1;
}

// Frees an entry in the bitmap by setting it to 1
// Does not save on disk
void _free_entry(char *bitmap, int i) {
    _bit_set(bitmap, i, 0);
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

int max(int a, int b) {
    if (a >= b) {
        return a;
    } else {
        return b;
    }
}
