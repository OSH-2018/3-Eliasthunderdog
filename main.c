#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <inttypes.h>
#include "sfstypes.h"

struct super_block {
    unsigned int numOfFile;
    unsigned int blockUsed;
    unsigned int blockRemain;
    struct blockBitmap theMap;
};
static struct super_block S = {
    .numOfFile = 0,
    .blockUsed = 0,
    .blockRemain = SIZE/BLOCKSIZE - 2,
    .theMap = {
    .map = {0},
    .fristUnused = 2
    }
};

int32_t getNextEmptyBlock(struct blockBitmap* theBlockBitmap) {
    int temp = theBlockBitmap->fristUnused;
    int intoffset = temp / 32;
    int bitOffset = temp - 32 * intoffset;
    theBlockBitmap->map[intoffset] &= ~(1 << bitOffset); // set the bit to zero.
    int i;
    for (i = temp + 1; i < SIZE/BLOCKSIZE; i++) {
        intoffset = i / 32;
        bitOffset = i - 32 * intoffset;
        if (theBlockBitmap->map[intoffset] & (1 << bitOffset) > 0) {
            theBlockBitmap->fristUnused = i;
            break;
        }
    }

    if (i == SIZE/BLOCKSIZE) theBlockBitmap->fristUnused = 0; //indicate that all blocks are used.

    return temp;
}

int32_t getNumBlock(off_t offset, struct fileinfo *info, int32_t *byteOffset) {
    int32_t l0 = offset / BLOCKSIZE;
    if (l0 <= 30) {
        *byteOffset = offset - l0 * BLOCKSIZE;
        return info->l0_block[l0];
    }
    else {
        int l1 = (offset - BLOCKSIZE * 31) / BLOCKSIZE;
        if (l1 < BLOCKSIZE / sizeof(int32_t)) {//get help in l1
            int32_t l0_offset = offset - 31 * BLOCKSIZE;
            *byteOffset = l0_offset - l1 * BLOCKSIZE;
            int32_t t1 = info->l0_block[31];
            int32_t t2 = ((struct l1_block *)block[t1])->l0_block[l1];
            return t2;
        } else { // time to get help from l2
            int32_t t1 = info->l0_block[31]; 
            int32_t l2_offset = offset - BLOCKSIZE * (31 + 255);
            int32_t l2_temp = l2_offset / (255 * 256 * BLOCKSIZE);
            int temp;
            int32_t target = ((struct l1_block *)block[t1])->l0_block[255];
            for (temp = 0; temp < l2_temp; temp++) {
                target = ((struct l2_block *)block[target])->l1_block[255];
            }// now the result is in block[target]

            int32_t l1_offset = l2_offset - (l2_temp * 255 * 256 * BLOCKSIZE);

            int32_t l1_temp = l1_offset / (256 * BLOCKSIZE); // which l1_block?
            int32_t l1_target = ((struct l2_block *)block[target])->l1_block[l1_temp];// locate in block[l1_target]

            int32_t l0_offset = l1_offset - (l1_temp * 256 * BLOCKSIZE);
            int32_t l0_temp = l0_offset / BLOCKSIZE;
            *byteOffset = l0_offset - l0_temp * BLOCKSIZE;
            int32_t l0_target = ((struct l1_block *)block[l1_target])->l0_block[l0_temp];

            return l0_target;
        }
    }
}

static const size_t size = SIZE;
static const unsigned int blockSize = BLOCKSIZE; // a block is 8k in size.
static void *block[SIZE/BLOCKSIZE];

static void* sfs_init(struct fuse_conn_info *conn) {
   
   int blockNum = size / blockSize;
   block[1] = mmap(NULL, 4*128*1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   for (int i = 0; i < blockNum; i++) {
       if(i == 1){
           memset(block[i], 0, 4*128*1024); 
           continue;
       }
       else {
           block[i] = mmap(NULL, blockSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
           memset(block[i], 0, BLOCKSIZE);
       }
   }


   struct fileinfo node[2000];

   memcpy(block[0], &S, sizeof(S));
   memcpy(block[1], node, sizeof(node));//some problems.
   
   return NULL;
}

static int sfs_getattr(const char *path, struct stat *st) {

    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);
    
    if(strcmp(path, "/") == 0) {
        st->st_mode = __S_IFDIR | 0755;
        st->st_nlink = 2; // the root directory has 2 hard links
    }
    else {
        st->st_mode = __S_IFREG | 0644;
        st->st_nlink = 1;
    }
    return 0;
}

static int sfs_readdir(const char *path, void *buffer, 
    fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info) {
    
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    
    struct fileinfo * theFile = (struct fileinfo *)block[1];

    for(int i = 0; i < S.numOfFile; i++) {
        filler(buffer, theFile[i].filename, NULL, 0);
    }

    return 0;
}

static int sfs_open(const char *path, struct fuse_file_info *info) {
    return 0;
}

static void mkFileInfo(const char *filename, struct stat *thestat) {
    struct fileinfo * theFile = (struct fileinfo *)block[1];
    struct fileinfo newNode = theFile[S.numOfFile++];
    strcpy(newNode.filename, filename);

    newNode.type = FILE;
    memcpy(newNode.st, thestat, sizeof(struct stat));
    
    return;
}

static int sfs_mknod(const char *path, mode_t mode, dev_t dev) {
    
    struct stat st;
    st.st_mode = __S_IFREG | 0644;
    st.st_uid = fuse_get_context()->uid;
    st.st_gid = fuse_get_context()->gid;
    st.st_nlink = 1;
    st.st_size = 0;

    mkFileInfo(path + 1, &st);

    return 0;
}

static int sfs_read(char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *info) {
    //return the number of bytes it has read.
    //
    // now le's assume that it operates on the root directory.

    struct fileinfo *s = (struct fileinfo *)block[1];
    int flag;
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++) {
        if(s[i].filename == NULL) { flag = 1; break; }
        if(strcmp(path, s[i].filename) == 0) {flag = 0; break;}
    }

    if (flag == 1) {
        printf("read failed, file not found\n");
        return 0;
    } else {
        printf("I have found the file, and it says:\n");
        int32_t byteOffset;
        int start = getNumBlock(offset, &s[i], &byteOffset);
        int byteRead = 0;
        for (int i = 0; byteRead < size; i++) {
            if(byteRead + BLOCKSIZE - byteOffset > size){
                memcpy(buffer,(void *) ((int32_t)block[start]+byteOffset), size);
                byteRead += size;
            }
            else{
                memcpy(buffer,(void *) ((int32_t)block[start]+byteOffset), BLOCKSIZE - byteOffset);
                byteRead += BLOCKSIZE - byteOffset;
            }
            start = getNumBlock(offset + byteRead, &s[i], &byteOffset);
        }
    }
}

static int sfs_write(char *path, char *src, size_t size, off_t offset, struct fuse_file_info *info) {

    // let's assume that path is root.
    struct fileinfo *s = (struct fileinfo *)block[1];
    int flag;
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++) {
        if(s[i].filename == NULL) { flag = 1; break; }
        if(strcmp(path, s[i].filename) == 0) {flag = 0; break;}
    }

    if (flag == 1) {
        printf("write failed, file not found\n");//create new file?
        return 0;
    } else {// I've found it ! I've found it !!
        int32_t byteOffset;
        int32_t start = getNumBlock(offset, &s[i], &byteOffset);
        int64_t byteWrote = 0;
        for (int i = 0; byteWrote < size; i++) {
            if (size < BLOCKSIZE - byteOffset) {
                memcpy((void *)((int32_t)block[start] + byteOffset), (void *)((int32_t)src + byteWrote), size);
                byteWrote += size;
                offset += size;
            }
            else {
                memcpy((void *)((int32_t)block[start] + byteOffset), (void *)((int32_t)src + byteWrote), BLOCKSIZE - byteOffset);
                byteWrote += BLOCKSIZE - byteOffset;
                offset += BLOCKSIZE - byteOffset;
                start = getNumBlock(offset, &s[i], &byteOffset);
            }
        }
    }
}

static const struct fuse_operations op = {

}; 

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &op, NULL);
}