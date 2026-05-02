#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BLOCKSIZE 16384 // bytes - 16 KB
#define MAXFILENAME 32  // maximum filename that can be stored by mfs
#define INDEX_LENGTH (BLOCKSIZE / 4)
#define MAX_FILE_SIZE (BLOCKSIZE * INDEX_LENGTH)
#define MAX_NUM_FILES 254
// Blocks given in diagram
#define BLOCK_SUPERBLOCK 0
#define BLOCK_BITMAP 1
#define BLOCK_INODE_MAP 2
#define BLOCK_INODE_TABLE_START 3
#define BLOCK_INODE_TABLE_END 4
#define BLOCK_ROOT_DIR 5
#define BLOCK_DATA_START 6

extern int fd_disk;

typedef struct superblock {
    int numblocks;
    int num_files;
    // ....
    char _padding[16376];
} superblock;
static_assert(sizeof(superblock) == BLOCKSIZE, "superblock should be a block");

typedef struct {
    char filename[MAXFILENAME];
    // We can set nlink to 1 and st_mode to S_IFREG | 0777
    char _padding[32];
} direntry;
static_assert(sizeof(direntry) == 64, "direntry should be size 64");

typedef struct {
    int inode_num; // The index in the inode map
    int index_block_num;
    int st_size; // Size of the data in bytes
    char _padding[116];
} inode;
static_assert(sizeof(inode) == 128, "inode should be size 128");

int read_block(void *block, int k);
int write_block(void *block, int k);
void *malloc_block();
void *malloc_and_read_block(int k);

int _allocate_bit(int block_num);
int _free_bit(int block_num, int idx);
// int _get_first_free_bit(char *block, int bound);
bool _bit_get(const char *block, int i);
void _bit_set(char *block, int i, bool val);

superblock get_superblock();
void set_superblock(superblock s);

void print_root_dir();
void print_block_num(int block_num);
void print_block(char *block);

bool is_inode_free(int inode_num);
int get_file(const char *filename, inode *out);
bool file_exists(const char *filename);
int create_file(const char *filename);
int delete_file(const char *filename);
int append_to_file(const char *filename, const char *buf, int size);
int read_file(const char *filename, char *buf, int size, int offset);
int write_file(const char *filename, const char *buf, int size, off_t offset);

void _create_direntry(const char *filename, int inode_num);
inode _create_inode(int inode_num, int index_block_num);
int _find_inode_num(const char *filename);
inode _get_inode(int inode_num);
int get_inode(const char *filename, inode *out);
void _save_inode(inode inode);
int _create_index_block();
int _get_free_index_entry(int *index_block);
int _allocate_data_block(inode inode);
int _get_data_block(inode f, int idx, void *out);
int _write_data_block(int data_block_num, int block_offset, const char *buf, int size);
int _allocate_data_blocks(inode ino, int index_block, size_t new_size);
int _allocate_free_entry(char *bitmap);
int _allocate_entry(char *bitmap);
void _free_entry(char *bitmap, int i);

int min(int a, int b);
int max(int a, int b);
