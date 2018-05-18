#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED
#include <inttypes.h>
#include <sys/types.h>
#include <fuse.h>
struct fileinfo {
    char filename[32];
    struct stat st;
    int32_t l0_block[32];
    int32_t l2_addr;
    int32_t l1_offset;
    int32_t l0_offset;
};

struct l1_block {
    int32_t l0_block[8 * 1024/sizeof(int32_t)];
};

struct l2_block {
    int32_t l1_block[8 * 1024/sizeof(int32_t)];
};

struct blockBitmap {
    int32_t map[4 * 1024 * 1024 * (size_t) 1024/8 / 1024/32];
    int32_t fristUnused;
};

struct position {
    int32_t l2;
    int32_t l1;
    int32_t l0;
};

struct super_block {
    int numOfFile;
    unsigned int blockUsed;
    size_t blockRemain;
    struct blockBitmap theMap;
};

int32_t getNextEmptyBlock(struct blockBitmap* theBlockBitmap);
int32_t newAlloBlock(struct fileinfo *info, int32_t *the_block);
struct fileinfo * findFile(const char *path);
void releaseBlock(int blockNum);
void releasel2(int32_t l2_addr, int32_t l1_offset, int32_t l0_offset);
void releasel1(int32_t l1_addr, int32_t l0_offset);
int32_t getNumBlock(off_t offset, struct fileinfo *info, int32_t *byteOffset, struct position *p);
void erase(struct fileinfo *info, off_t offset);
void extend(struct fileinfo *info, off_t offset);
void* sfs_init(struct fuse_conn_info *conn);
int sfs_getattr(const char *path, struct stat *st);
int sfs_readdir(const char *path, void *buffer, 
    fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info);
int sfs_open(const char *path, struct fuse_file_info *info);
int sfs_truncate(const char *path, off_t offset);
int sfs_unlink(const char *path);
void mkFileInfo(const char *filename, struct stat *thestat);
int sfs_mknod(const char *path, mode_t mode, dev_t dev);
int sfs_read(const char *path, char *buffer, size_t size, off_t offset, 
                    struct fuse_file_info *info);
int sfs_write(const char *path, const char *src, size_t size, off_t offset, 
                    struct fuse_file_info *info);
int sfs_utimens(const char *path, const struct timespec tv[2]);


const size_t BLOCKSIZE = 8 * 1024;
const size_t SIZE = 4 * 1024 * 1024 * (size_t) 1024;
struct super_block S;
const size_t size = SIZE;
const unsigned int blockSize = BLOCKSIZE; // a block is 8k in size.
void *block[4096*128];



#endif