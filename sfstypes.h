#ifndef SFSTYPES_H
#define SFSTYPES_H

#define FILE 0
#define DIR 1
#define SIZE 4 * 1024 * 1024 * (size_t) 1024
#define BLOCKSIZE 8 * 1024 //every block is 8k in size.
#define MAX_FILE_NUM 2000
#include <inttypes.h>
#include <sys/types.h>
struct fileinfo {
    unsigned int type;
    char filename[32];
    struct stat *st;
    int32_t l0_block[32];
};

struct l1_block {
    int32_t l0_block[BLOCKSIZE/sizeof(int32_t)];
};

struct l2_block {
    int32_t l1_block[BLOCKSIZE/sizeof(int32_t)];
};


struct blockBitmap {
    int32_t map[SIZE/BLOCKSIZE/32];
    int32_t fristUnused;
};


#endif