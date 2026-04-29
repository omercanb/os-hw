#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int mfs_getattr(const char *, struct stat *, struct fuse_file_info *);
int mfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
                struct fuse_file_info *, enum fuse_readdir_flags);
int mfs_open(const char *, struct fuse_file_info *);
int mfs_read(const char *, char *, size_t, off_t,
             struct fuse_file_info *);
int mfs_release(const char *path, struct fuse_file_info *fi);
int mfs_create(const char *path, mode_t, struct fuse_file_info *);
int mfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi);
int mfs_unlink(const char *path);

// ....

/* Utility functions */
int read_block(void *block, int k);
int write_block(void *block, int k);

static struct fuse_operations mfs_oper = {
    .getattr = mfs_getattr,
    .readdir = mfs_readdir,
    .mknod = NULL,

    .mkdir = NULL,
    .unlink = mfs_unlink,
    .rmdir = NULL,
    .truncate = NULL,
    .create = mfs_create,
    .open = mfs_open,
    .read = mfs_read,
    .release = mfs_release,
    .write = NULL,
    .rename = NULL,
    .utimens = mfs_utimens};

// Fake the utimens so that the touch command does not error
int mfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    return 0;
}

int mfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *_) {
    int res = 0;
    // printf("getattr: (path=%s)\n", path);
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (strcmp(path, "/hello") == 0) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = 20;
        return 0;
    }
    const char *filename = &path[1];
    inode f;
    res = get_file(filename, &f);
    if (res < 0) {
        return -ENOENT;
    }
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = 10;
    return 0;
}

int mfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                struct fuse_file_info *fi, enum fuse_readdir_flags fl) {
    (void)offset;
    (void)fi;
    printf("readdir: (path=%s)\n", path);

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    // If . and .. are removed cd .. etc still works, they just won't be displayed
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "hello", NULL, 0, 0);
    direntry *root = get_files();
    for (int i = 0; i < MAX_NUM_FILES; i++) {
        direntry f = root[i];
        if (is_inode_free(i)) {
            continue;
        }
        printf("file: %s\n", f.filename);
        filler(buf, f.filename, NULL, 0, 0);
    }
    free(root);
    print_root_dir();

    return 0;
}

int mfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf("create: (path=%s)\n", path);
    const char *filename = &path[1];
    int inode_num = create_file(filename);
    if (inode_num < 0) {
        // inode num is an error
        return inode_num;
    }
    // Set the file handle to be used later
    fi->fh = inode_num;
    return 0;
}

int mfs_unlink(const char *path) {
    printf("delete: (path=%s)\n", path);
    const char *filename = &path[1];
    int res = delete_file(filename);
    if (res == -1) {
        return -ENOENT;
    }
    return 0;
}

int mfs_open(const char *path, struct fuse_file_info *fi) {
    printf("open: (path=%s)\n", path);
    // create_file(path);
    return 0;
}

int mfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("read: (path=%s)\n", path);
    memcpy(buf, "Hello\n", 6);
    return 6;
}

int mfs_release(const char *path, struct fuse_file_info *fi) {
    printf("release: (path=%s)\n", path);
    return 0;
}

int mfs_init() {
    char buffer[BLOCKSIZE];

    printf("Initializing the file system...");
    //....
    bzero(buffer, BLOCKSIZE);
    write_block(buffer, 0); // write superblock info
                            // TODO remove
    for (int i = 0; i < BLOCK_DATA_START; i++) {
        write_block(buffer, i); // write superblock info
    }
    fsync(fd_disk);
    printf("fd disk: %d \n", fd_disk);

    return (0);
}

int main(int argc, char *argv[]) {

    // initialize the  disk
    char disk_path[256];
    strncpy(disk_path, argv[argc - 1], sizeof(disk_path) - 1);
    argv[argc - 1] = NULL;
    argc--;
    fd_disk = open(disk_path, O_RDWR);
    if (fd_disk < 0) {
        perror("open disk");
        return 1;
    }

    mfs_init();

    printf("initialized the file system\n");
    fflush(stdout);

    int i = create_file("ZUBIZUBIZOO");
    i = create_file("DUBIDUBIDUBI");
    printf("new file: %d\n", i);

    fuse_main(argc, argv, &mfs_oper, NULL);

    return 0;
}
