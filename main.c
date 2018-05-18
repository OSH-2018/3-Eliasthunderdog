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
#define MAX_FILE_NUM 100

/*----------------data structrues---------------------------*/
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
/*--------------------global variables-----------------------*/

const size_t BLOCKSIZE = 8 * 1024;
const size_t SIZE = 4 * 1024 * 1024 * (size_t) 1024;
static struct super_block S;
static const size_t size = SIZE;
static const unsigned int blockSize = BLOCKSIZE; // a block is 8k in size.
static void *block[4096*128];

static bool watchpoint = false;
static bool watchi = false;
static bool watchwrite = false;
static bool watchalloc = false;
static bool watchm = false;
static bool getLarge = false;
static bool watchbyte = false;


static unsigned int l0_num = 0;
static unsigned int l1_num = 0;
static unsigned int l2_num = 0;

/*-----------------------functions-----------------------------------*/

int32_t getNextEmptyBlock(struct blockBitmap* theBlockBitmap) {

    int temp = theBlockBitmap->fristUnused;
    if(theBlockBitmap->fristUnused = 0) watchi = true;
    int intoffset = temp / 32;
    int bitOffset = temp - 32 * intoffset;
    theBlockBitmap->map[intoffset] &= ~(1 << (31 - bitOffset)); // set the bit to zero. 
    int i;
    for (i = temp + 1; i < SIZE/BLOCKSIZE; i++) {
        intoffset = i / 32;
        bitOffset = i - 32 * intoffset;
        
        bool m = ((unsigned int)(theBlockBitmap->map[intoffset] & (unsigned int) (1 << (31 - bitOffset))) != 0);
        
        if (m) {
            theBlockBitmap->fristUnused = i;
            if(theBlockBitmap->fristUnused == 0) watchi = true;
            break;
        }
    }

    if (i == SIZE/BLOCKSIZE){
        watchi = true;
        return 0; //indicate that all blocks are used.
    }
    return temp;
}

int32_t newAlloBlock(struct fileinfo *info, int32_t *the_block) {

    struct super_block * s = (struct super_block *)block[0];
    info->st.st_size += BLOCKSIZE;
    //from the smallest one to the largest one.
    if(info->l2_addr == 0 && info->l1_offset == 0) {// only the l0
        if(info->l0_offset == 31) {
            // allocate an l1_block and update.
            info->l0_block[31] = getNextEmptyBlock(&s->theMap);
            info->l1_offset = 1;
            info->l0_offset = 1;
            *the_block = getNextEmptyBlock(&s->theMap);
            ((struct l1_block *)block[info->l0_block[31]])->l0_block[0] = *the_block;
            if (*the_block < 0 || *the_block > 524288) watchpoint = true;
            return 0;
        }
        else {
            // allocate an l0_block
            *the_block = getNextEmptyBlock(&s->theMap);
            info->l0_block[info->l0_offset] = *the_block;
            if (*the_block < 0 || *the_block > 524288) watchpoint = true;
            info->l0_offset++;
            return 0;
        }
    }
    else if(info->l2_addr == 0) { // l0 and l1
        if(info->l0_offset == 2047) {
            // allocate a l2_block and update
            int32_t new_l2 = getNextEmptyBlock(&s->theMap);
            int32_t new_l1 = getNextEmptyBlock(&s->theMap);
            int32_t new_l0 = getNextEmptyBlock(&s->theMap);

            int32_t l1_addr = info->l0_block[31];
            struct l1_block * l1 = (struct l1_block *)block[l1_addr];
            l1->l0_block[2047] = new_l2;
            struct l2_block *l2 = (struct l2_block *)block[new_l2];
            l2->l1_block[0] = new_l1;
            struct l1_block *newl1 = (struct l1_block *)block[new_l1];
            newl1->l0_block[0] = new_l0;
            info->l2_addr = new_l2;
            info->l1_offset = 1;
            info->l0_offset = 1;
            *the_block = new_l0;
            if (*the_block < 0 || *the_block > 524288) watchpoint = true;
            return 0;
        }
        else {
            // allocate a l0_block.
            getLarge = true;
            int32_t new_l0 = getNextEmptyBlock(&s->theMap);
            int32_t l1_addr = info->l0_block[31];
            struct l1_block * l1 = (struct l1_block *)block[l1_addr];
            l1->l0_block[info->l0_offset] = new_l0;
            info->l0_offset++;
            *the_block = new_l0;
            if (*the_block < 0 || *the_block > 524288) watchpoint = true;
            return 0;
        }
    }
    else { // from l2
/*
        if (info->l1_offset == 2047 && info->l0_offset == 2048) {
            // allocate a l2_block and update.
                    getLarge = true;
            int32_t new_l2 = getNextEmptyBlock(&s->theMap);
            int32_t new_l1 = getNextEmptyBlock(&s->theMap);
            int32_t new_l0 = getNextEmptyBlock(&s->theMap);

            struct l2_block *l2 = (struct l2_block *)block[info->l2_addr];
            l2->l1_block[2047] = new_l2;
            struct l2_block *n_l2 = (struct l2_block *)block[new_l2];
            n_l2->l1_block[0] = new_l1;
            struct l1_block *l1 = (struct l1_block *)block[new_l1];
            l1->l0_block[0] = new_l0;
            info->l2_addr = new_l2;
            info->l1_offset = 1;
            info->l0_offset = 1;
            *the_block = new_l0;
            return 0;
        }*/
        if (info->l1_offset < 2047 && info->l0_offset == 2048) {
            // allocate a l1_block and update.
            int32_t new_l1 = getNextEmptyBlock(&s->theMap);
            int32_t new_l0 = getNextEmptyBlock(&s->theMap);

            struct l2_block *l2 = (struct l2_block *)block[info->l2_addr];
            l2->l1_block[info->l1_offset] = new_l1;
            info->l1_offset++;
            struct l1_block *l1 = (struct l1_block *)block[new_l1];
            l1->l0_block[0] = new_l0;
            info->l0_offset = 1;
            *the_block = new_l0;
            if (*the_block < 0 || *the_block > 524288) watchpoint = true;
            return 0;
        }
        else {
            // allocate a l0_block and update.
            int32_t new_l0 = getNextEmptyBlock(&s->theMap);
            struct l2_block *l2 = (struct l2_block *)block[info->l2_addr];
            int32_t l1_addr = l2->l1_block[info->l1_offset-1];
            struct l1_block *l1 = (struct l1_block *)block[l1_addr];
            l1->l0_block[info->l0_offset] = new_l0;
            info->l0_offset++;
            *the_block = new_l0;
            if (*the_block < 0 || *the_block > 524288) watchpoint = true;
            return 0;
        }
    }
}

struct fileinfo * findFile(const char *path) {
    struct super_block *s = (struct super_block *)block[0];
    struct fileinfo *file = (struct fileinfo *)block[1];

    int flag = 0;
    int i = 0;
    for (i = 0; i < s->numOfFile; i++) {
        if(strcmp(path + 1, file[i].filename) == 0) {
            flag = 1;
            break;
        }
    }
    return (flag == 1) ? &file[i] : NULL;
}

void releaseBlock(int blockNum) {

    if(blockNum == 0 | blockNum == 1) {
        printf("are you crazy? release block num = %d\n", blockNum);
        return;
    }

    struct super_block *s = (struct super_block *)block[0];
    int temp = blockNum / 32;
    int bitOffset = blockNum - 32 * temp;
    // set that bit to 1
    s->theMap.map[temp] |= (unsigned int)(1 << (31 - bitOffset));
    
    if(blockNum < s->theMap.fristUnused)
        s->theMap.fristUnused = blockNum;
    memset(block[blockNum], 0, BLOCKSIZE);
    return;
}

void releasel2(int32_t l2_addr, int32_t l1_offset, int32_t l0_offset) {

    if (l0_offset == 2048) {
        l1_offset += 1;
        l0_offset = 0;
    }
    struct l2_block *l2 = (struct l2_block *)block[l2_addr];
    for (int i = l1_offset; l2->l1_block[i] != 0 && i < 2048; i++) {
        struct l1_block *l1 = (struct l1_block *)block[l2->l1_block[i]];
                
        for (int j = l0_offset; l1->l0_block[j]!= 0 && j <= 2047; j++) {
            releaseBlock(l1->l0_block[j]);
            l1->l0_block[j] = 0;// doubt will it stop the cycle ? 
        }

        if (l0_offset == 0) {
            releaseBlock(l2->l1_block[i]);
            l2->l1_block[i] = 0;
        }

        l0_offset = 0;
    }
}

void releasel1(int32_t l1_addr, int32_t l0_offset) {
    
    if (l0_offset == 2047) return;
    struct l1_block *l1 = (struct l1_block *)block[l1_addr];
    for (int i = l0_offset; l1->l0_block[i] != 0 && i < 2048; i++) {
        if (l1->l0_block[i] < 0 || l1->l0_block[i] > 3000) watchpoint = true;
        releaseBlock(l1->l0_block[i]);
        l1->l0_block[i] = 0;
    }
    if (l0_offset == 0) {
        releaseBlock(l1_addr);
    }
    return;
}

int32_t getNumBlock(off_t offset, struct fileinfo *info, int32_t *byteOffset, struct position *p) {

    if(offset >= info->st.st_size) return 0;

    off_t l0 = offset / BLOCKSIZE;
    if (l0 <= 30) {
        *byteOffset = offset - l0 * BLOCKSIZE;
        p->l0 = l0;
        p->l1 = 0;
        p->l2 = 0;
        return info->l0_block[l0];
    }
    else {
        int l1 = (offset - BLOCKSIZE * 31) / BLOCKSIZE;
        if (l1 < BLOCKSIZE / sizeof(int32_t) - 1) {//get help in l1
            off_t l0_offset = offset - 31 * BLOCKSIZE;
            *byteOffset = l0_offset - l1 * BLOCKSIZE;
            int32_t t1 = info->l0_block[31];
            int32_t l0 = l1;
            int32_t t2 = ((struct l1_block *)block[t1])->l0_block[l1];
            p->l0 = l0;
            p->l1 = 1;
            p->l2 = 0;
            return t2;
        } else { // time to get help from l2
            int32_t t1 = info->l0_block[31];
            off_t l2_offset = offset - BLOCKSIZE * (31 + 2047);
            int32_t l2_addr = ((struct l1_block *)block[t1])->l0_block[2047];
            int temp;

            p->l2 = 1;

            off_t l1_offset = l2_offset;

            off_t l1_temp = l1_offset / (2048 * BLOCKSIZE); // which l1_block?
            p->l1 = l1_temp;
            int32_t l1_target = ((struct l2_block *)block[l2_addr])->l1_block[l1_temp];// locate in block[l1_target]

            off_t l0_offset = l1_offset - (l1_temp * 2048 * BLOCKSIZE);
            off_t l0_temp = l0_offset / BLOCKSIZE;
            p->l0 = l0_temp;
            *byteOffset = l0_offset - l0_temp * BLOCKSIZE;
            int32_t l0_target = ((struct l1_block *)block[l1_target])->l0_block[l0_temp];
            return l0_target;
        }
    }
}

void erase(struct fileinfo *info, off_t offset) {
    // delete all the contents after the offset.
    if (info->st.st_size < offset) {
        printf("erase error: offset > size\n");
        return;
    }
    else {
        struct position p;
        int32_t byte_offset;
        int32_t s_block = getNumBlock(offset, info, &byte_offset, &p);
        memset(block[s_block]+ byte_offset, 0, BLOCKSIZE - byte_offset);
        if (byte_offset == 0) releaseBlock(s_block);
        // and the next?
        struct l2_block *l2 = NULL;
        struct l1_block *l1 = NULL;
        
        if (p.l2 > 0) {
            l1 = (struct l1_block *)block[info->l0_block[31]];
            l2 = (struct l2_block *)block[l1->l0_block[2047]];
            int32_t l2_addr = l1->l0_block[2047];
            releasel2(l2_addr, p.l1, p.l0 + 1);//doubt p.l0 = 2047

        }
        else if (p.l2 == 0 && p.l1 == 1) {
            //releasel1(info->l0_block[31], p.l0 + 1); // doubt.
            int32_t l1_addr = info->l0_block[31];
            int32_t l2_addr = ((struct l1_block*)block[l1_addr])->l0_block[2047];
            if (l2_addr > 1) releasel2(l2_addr, 0, 0);
            releasel1(info->l0_block[31], p.l0 + 1);
            return;
        }
        else if (p.l2 == 0 && p.l1 == 0) {

            int32_t l1_addr = info->l0_block[31];
            if (l1_addr > 1) {
                int32_t l2_addr = ((struct l1_block *)block[l1_addr])->l0_block[2047];
                if (l2_addr > 1) releasel2(l2_addr, 0, 0);
                releasel1(l1_addr, 0);
                info->l0_block[31] = 0;
            }

            for (int i = p.l0 + 1; i <= 30 && info->l0_block[i] != 0; i++) {
                releaseBlock(info->l0_block[i]);
                info->l0_block[i] = 0;
            }
            
        }

        info->st.st_size = offset;
        info->l2_addr = p.l2;
        info->l1_offset = p.l1;
        info->l0_offset = p.l0;
    }
}

void extend(struct fileinfo *info, off_t offset) {
    int32_t temp = info->st.st_size;
    if (info->st.st_size > offset) {
        printf("extend error: offset = %d while size = %d, offset < size\n", offset, info->st.st_size);
        return;
    }
    else {
        while (info->st.st_size < offset) {
            int the_block;
            newAlloBlock(info, &the_block);
            if(the_block == 0){
                printf("no more space , quit\n");
                info->st.st_size = temp;
                return;
            }
        }//while
        info->st.st_size = offset;
    }
}

void* sfs_init(struct fuse_conn_info *conn) {
    printf("call init\n");
   int blockSize = 8 * 1024;
   int blockNum = 512 * 1024; // the number of blocks
   //printf("unmap!\n");
   block[1] = mmap(NULL, 4*128*1024, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   for (int i = 0; i < blockNum; i++) {
       if(i == 1){
           memset(block[i], 0, 4*128*1024); // doubt
           continue;
       }
       else if(i == 0) {
           block[0] = mmap(NULL, sizeof(struct super_block), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
       }
       else {
           block[i] = mmap(NULL, blockSize, PROT_READ | PROT_WRITE, 
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
           memset(block[i], 0, BLOCKSIZE); // doubt
       }
   }

   //init S
   struct super_block* S = (struct super_block*) block[0];
   S->blockUsed = 2;
   S->blockRemain = blockNum - 2;
   S->theMap.fristUnused = 2;

   memset(S->theMap.map, -1, sizeof(S->theMap.map)); //doubt.

   void *m = block[0];
   
   return NULL;
}

int sfs_getattr(const char *path, struct stat *st) {
    
    printf("call getattr, path = %s\n", path);
    
    if(strcmp(path, "/") == 0) {
        printf("It's root !\n");
        st->st_mode = __S_IFDIR | 0755;
        st->st_nlink = 2; // the root directory has 2 hard links
    } else {
        printf("called else, path=%s\n", path);
        int found = 0;
        int i = 0;
        struct fileinfo *f = (struct fileinfo *)block[1];
        struct super_block *s = (struct super_block *)block[0];
        printf("numofFile=%d\n", s->numOfFile);
        for (i = 0; i <= s->numOfFile - 1; i++) {
            if(strcmp(path + 1, f[i].filename) == 0)
            { found = 1; break;}
        }
        if (found == 1) {
            memcpy(st, &(f[i].st), sizeof(struct stat));
        }
        else {
            printf("no file found, return %d\n", -ENOENT);
            return -ENOENT;
        } 
    }
    return 0;
}

int sfs_readdir(const char *path, void *buffer, 
    fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info) {

    printf("call readdir, path=%s, offset=%d\n", path, offset);
    
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    
    struct fileinfo * theFile = (struct fileinfo *)block[1];
    struct super_block * s = (struct super_block *)block[0];

    for(int i = 0; i < s->numOfFile; i++) {
        filler(buffer, theFile[i].filename, NULL, 0);
    }

    return 0;
}

int sfs_open(const char *path, struct fuse_file_info *info) {
    printf("call open, flags = %o\n", info->flags);
    return 0;
}

int sfs_truncate(const char *path, off_t offset) {
    printf("call truncate, path=%s, offset=%d", path, offset);
    struct super_block *s = (struct super_block *)block[0];
    struct fileinfo *fi = (struct fileinfo *)block[1];
    int i = 0;
    int flag = 0;
    int32_t byteOffset = 0;
    for (i = 0; i < s->numOfFile; i++) {
        if(strcmp(fi[i].filename, path+1) == 0) {flag = 1; break;}
    }
    if (flag == 1) {
        struct fileinfo *s = &fi[i];
        if(s->st.st_size > offset) erase(s, offset);
        else extend(s, offset);
        return 0;
    }
    else return -ENOENT;
}

int sfs_unlink(const char *path) {
    printf("call unlink, path = %s\n", path);
    char *name = (char *)(path + 1);
    struct fileinfo *theFile = (struct fileinfo *)block[1];

    struct super_block *s = (struct super_block *)block[0];
    int flag = 0;
    int i = 0;
    for (i = 0; i < s->numOfFile; i++) {
        if (strcmp(theFile[i].filename, name) == 0) {flag = 1; break;}
    }

    if (flag == 1) {
        sfs_truncate(path, 0);
        for(int l = i; l < s->numOfFile; l++) {
            memcpy(&theFile[l], &theFile[l+1], sizeof(struct fileinfo));
        }
        s->numOfFile--;
        return 0;
    }

    else return -ENOENT;
}

void mkFileInfo(const char *filename, struct stat *thestat) {

    struct super_block *s = (struct super_block *)block[0];

    struct fileinfo * theFile = (struct fileinfo *)block[1];
    struct fileinfo * newNode = &theFile[s->numOfFile++];
    strcpy(newNode->filename, filename);

    memcpy(&(newNode->st), thestat, sizeof(struct stat));
    
    return;
}

int sfs_utimens(const char *path, const struct timespec tv[2]) {
    struct fileinfo * file = findFile(path);
    if (file == NULL) return -ENOENT;
    else {
        file->st.st_atime = tv[0].tv_sec;
        file->st.st_mtime = tv[1].tv_sec;
        return 0;
    }
}

int sfs_mknod(const char *path, mode_t mode, dev_t dev) {
    printf("mknod, path = %s\n, mode = %o", path, mode);
    struct stat st;
    st.st_mode = __S_IFREG | 0666;
    st.st_uid = fuse_get_context()->uid;
    st.st_gid = fuse_get_context()->gid;
    st.st_nlink = 1;
    st.st_size = 0;
    st.st_atime = time(NULL);
    st.st_mtime = time(NULL);
    st.st_ctime = time(NULL);
    mkFileInfo(path + 1, &st);
    return 0;
}

int sfs_read(const char *path, char *buffer, size_t size, off_t offset, 
                    struct fuse_file_info *info) {
    //return the number of bytes it has read.
    //
    // now le's assume that it operates on the root directory.
    printf("call read, path=%s, size=%d, offset=%d\n", path, size, offset);
    struct fileinfo *s = (struct fileinfo *)block[1];
    int flag = 1;
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++) {
        if(strcmp(path + 1, s[i].filename) == 0) {flag = 0; break;}
    }

    if (flag == 1) {
        printf("read failed, file not found\n");
        return 0;
    } else {
        printf("I have found the file, and it says:\n");
        struct position p;

        if(offset >= s[i].st.st_size || s[i].st.st_size == 0) return 0;

        int32_t byteOffset;
        int start = getNumBlock(offset, &s[i], &byteOffset, &p);
        int byteRead = 0;
        int sizeRemain = size;
        for (int j = 0; byteRead < size; j++) {
            if(byteRead + BLOCKSIZE - byteOffset >= size){ // we can read less than one block.
                if(offset + byteRead + sizeRemain > s[i].st.st_size) {// the byte to read is more than the file size, so we just read to the end of the file.
                    memcpy(buffer + byteRead, (void *)((char *)block[start] + byteOffset), s[i].st.st_size - (byteRead + offset));
                    byteRead += s[i].st.st_size - (byteRead + offset);
                    sizeRemain = size - byteRead;
                    return byteRead;
                }
                else {// the byte to read is less than the file size.
                    memcpy(buffer + byteRead,(void *) ((char *)block[start]+byteOffset), sizeRemain);
                    byteRead += sizeRemain;
                    sizeRemain = size - byteRead;
                    return byteRead;
                }
            }
            else{ // we must read the whole block and go on.
                if(byteRead + BLOCKSIZE - byteOffset + offset > s[i].st.st_size) { // the byte to read is more than the file size, read to the end and return.
                    if(offset > s[i].st.st_size || s[i].st.st_size == 0) return 0;
                    memcpy(buffer + byteRead,(void *) ((char *)block[start]+byteOffset), s[i].st.st_size - (offset + byteRead));
                    byteRead += s[i].st.st_size - (offset + byteRead);
                    return byteRead;
                }
                else { // the byte to read is less than the file size, read the whole block and go on.
                    memcpy(buffer + byteRead,(void *) ((char *)block[start]+byteOffset), BLOCKSIZE - byteOffset);
                    byteRead += BLOCKSIZE - byteOffset;
                    sizeRemain = size - byteRead;
                }
            }
            start = getNumBlock(offset + byteRead, &s[i], &byteOffset, &p);
        }
        printf("%d bytes read by me\n", byteRead);
        return byteRead;
    }
}

int sfs_write(const char *path, const char *src, size_t size, off_t offset, 
                    struct fuse_file_info *info)
{
    // let's assume that path is root.

    off_t toffset = offset;
    printf("call write, path=%s, size=%d, offset=%d\n", path, size, offset);

    struct super_block *S = (struct super_block *)block[0];
    struct fileinfo *s = (struct fileinfo *)block[1];
    int flag = 1;
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++) {
        if(strcmp(path + 1, s[i].filename) == 0) {flag = 0; break;}
    }

    if (flag == 1) {
        watchwrite = true;
        printf("write failed, file not found\n");//create new file? no
        return 0;
    } else {// I've found it ! I've found it !!
        int32_t byteOffset = 0;
        struct position p;
        int32_t start = getNumBlock(offset, &s[i], &byteOffset, &p);
        if (start < 0 || start > 524288) watchpoint = true;
        if (byteOffset > 0) watchbyte = true;
        int64_t byteWrote = 0;
        for (int j = 0; byteWrote < size; j++) {
            if (size < 8*1024 - byteOffset) {// can write in a block.
                if (start == 0) {//new block
                    newAlloBlock(&s[i], &start);
                }
                
                if(start == 0 || start == 1) watchpoint = true;

                memcpy((void *)((char *)block[start] + byteOffset),
                (void *)((char *)src + byteWrote), size);
                byteWrote += size;
                offset += size;
            }
            else { // just fill a block and go on.
                if (start == 0) {//new file or more size.
                    newAlloBlock(&s[i], &start);
                }

                if(start == 0 || start == 1) watchpoint = true;

                memcpy((void *)((char *)block[start] + byteOffset), 
                (void *)((char *)src + byteWrote), 8*1024 - byteOffset);
                byteWrote += 8*1024 - byteOffset;
                offset += 8*1024 - byteOffset;
                start = getNumBlock(offset, &s[i], &byteOffset, &p);
                if (start < 0 || start > 524288) watchpoint = true;
            }
        }/*
        if (toffset + size > s[i].st.st_size) {
            s[i].st.st_size = toffset + size;
        }*/
        printf("wrote %d bytes to the file.\n", byteWrote);
        return byteWrote;
    }
}

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
    .utimens = sfs_utimens
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &op, NULL);
}