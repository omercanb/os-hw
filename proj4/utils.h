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
    int inode_num;
    // We can set nlink to 1 and st_mode to S_IFREG | 0777
    char _padding[28];
} direntry;
static_assert(sizeof(direntry) == 64, "direntry should be size 64");

typedef struct {
    int inode_num; // The index in the inode map
    int index_block_num;
    int st_size;
    char _padding[116];
} inode;
static_assert(sizeof(inode) == 128, "inode should be size 128");

int read_block(void *block, int k);
int write_block(void *block, int k);
void *malloc_block();

int get_first_free_bit(char *block, int bound);
bool bit_get(const char *block, int i);
void bit_set(char *block, int i, bool val);

bool is_inode_free(int inode_num);

superblock get_superblock();
void set_superblock(superblock s);

int get_file(const char *filename, inode *out);
bool file_exists(const char *filename);
direntry *get_files();
int create_file(const char *filename);
int delete_file(const char *filename);

void print_root_dir();

int _create_direntry(const char *filename, int inode_num);
void _create_inode(int inode_num);
int _find_inode_num(const char *filename);
inode _get_inode(int inode_num);
