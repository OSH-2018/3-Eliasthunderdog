#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include "types.h"
/*-----------------------functions-----------------------------------*/

const size_t BLOCKSIZE = 8 * 1024;
const size_t SIZE = 4 * 1024 * 1024 * (size_t) 1024;
struct super_block S;
const size_t size = SIZE;
const unsigned int blockSize = BLOCKSIZE; // a block is 8k in size.
void *block[4096*128];

static const struct fuse_operations op = {
    .init = sfs_init,
    .getattr = sfs_getattr,
    .readdir = sfs_readdir,
    .open = sfs_open,
    .unlink = sfs_unlink,
    .mknod = sfs_mknod,
    .read = sfs_read,
    .write = sfs_write,
    .truncate = sfs_truncate,
    .utimens = sfs_utimens,
    .mkdir = sfs_mkdir,
    .opendir = sfs_opendir,
    .rmdir = sfs_rmdir
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &op, NULL);
}