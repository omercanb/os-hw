#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fuse3/fuse.h>

#define BLOCKSIZE 16384 // bytes - 16 KB
#define MAXFILENAME 32  // maximum filename that can be stored by mfs


int mfs_getattr( const char *, struct stat *, struct fuse_file_info * );
int mfs_readdir( const char *, void *, fuse_fill_dir_t, off_t,
                 struct fuse_file_info *, enum fuse_readdir_flags);
int mfs_open( const char *, struct fuse_file_info * );
int mfs_read( const char *, char *, size_t, off_t,
              struct fuse_file_info * );
int mfs_release(const char *path, struct fuse_file_info *fi);
// ....






/* Utility functions */
int read_block (void *block, int k);
int write_block (void *block, int k);

static struct fuse_operations mfs_oper = {
    .getattr    = mfs_getattr,
    .readdir    = mfs_readdir,
    .mknod = NULL,
    
    .mkdir = NULL,
    .unlink = NULL,
    .rmdir = NULL,
    .truncate = NULL,
    .open    = mfs_open,
    .read    = mfs_read,
    .release = mfs_release,
    .write = NULL,
    .rename = NULL,
    .utimens = NULL
};


struct superblock
{
    int numblocks;
    // ....
};


struct direnty
{
    char filename[MAXFILENAME];
    // ....
};



// ********** Global Variables ***************************************
int fd_disk;


int mfs_getattr( const char *path, struct stat *stbuf, struct fuse_file_info *) {
    int res = 0;
    printf("getattr: (path=%s)\n", path);

    memset(stbuf, 0, sizeof(struct stat));
    if( strcmp( path, "/" ) == 0 ) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if( strcmp( path, "/hello" ) == 0 ) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = 12;
    } else
        res = -ENOENT;

    return res;
}



int mfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                struct fuse_file_info *fi, enum fuse_readdir_flags fl) {
    (void) offset;
    (void) fi;
    printf("readdir: (path=%s)\n", path);

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "hello ...", NULL, 0, 0);

    return 0;
}

int mfs_open( const char *path, struct fuse_file_info *fi ) {
    printf("open: (path=%s)\n", path);
    return 0;
}

int mfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    printf("read: (path=%s)\n", path);
    memcpy( buf, "Hello\n", 6 );
    return 6;
}

int mfs_release(const char *path, struct fuse_file_info *fi) {
    printf("release: (path=%s)\n", path);
    return 0;
}



int
mfs_init()
{
    char buffer[BLOCKSIZE];
    
    printf ("Initializing the file system...");
    //....
    bzero (buffer, BLOCKSIZE);
    write_block (buffer, 0); // write superblock info
    fsync (fd_disk);
    
    return (0);
}

int main(int argc, char *argv[]) {
    
    
    // initialize the  disk
    
    mfs_init();
    
    printf ("initialized the file system\n");
    fflush(stdout);
    
    fuse_main( argc, argv, &mfs_oper, NULL);

    return 0;
}







// Read block k from disk (virtual disk) into buffer block.
// Size of the block is BLOCKSIZE.
// Memory space for block must be allocated outside of this function.
// Block numbers start from 0 in the virtual disk.
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(fd_disk, (off_t) offset, SEEK_SET);
    n = read (fd_disk, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("read error\n");
        return -1;
    }
    return (0);
}

// write block k into the virtual disk.
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(fd_disk, (off_t) offset, SEEK_SET);
    n = write (fd_disk, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("write error\n");
        return (-1);
    }
    return 0;
}
