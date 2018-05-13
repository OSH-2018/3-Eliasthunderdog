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
    unsigned int type;
    char filename[32];
    struct stat st;
    int32_t l0_block[32];
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


static unsigned int l0_num = 0;
static unsigned int l1_num = 0;
static unsigned int l2_num = 0;

/*-----------------------functions-----------------------------------*/

int32_t getNextEmptyBlock(struct blockBitmap* theBlockBitmap) {

    int temp = theBlockBitmap->fristUnused;
    if(theBlockBitmap->fristUnused = 0) watchi = true;
    int intoffset = temp / 32;
    int bitOffset = temp - 32 * intoffset;
    theBlockBitmap->map[intoffset] &= ~(1 << (32 - bitOffset)); // set the bit to zero. 
    int i;
    for (i = temp + 1; i < SIZE/BLOCKSIZE; i++) {
        if(i == 33) watchi = true;
        intoffset = i / 32;
        bitOffset = i - 32 * intoffset;
        
        bool m = ((unsigned int)(theBlockBitmap->map[intoffset] & (unsigned int) (1 << (32 - bitOffset))) != 0);
        
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
    if(temp > 546) getLarge = true;
    return temp;
}

int32_t alloBlock(struct fileinfo *info, int32_t *theblock) {
    
    struct super_block *s = (struct super_block *) block[0];

    int32_t st_block = info->l0_block[0];
    int32_t cur_block = st_block;

    if (st_block == 0) {//new file?
        info->l0_block[0] = getNextEmptyBlock(&s->theMap);
        l0_num++;
        s->blockRemain--;
        s->blockUsed++;
        *theblock = info->l0_block[0];
        if(*theblock == 0 || *theblock == 1) watchalloc = true;
        return 0;
    }

    int32_t i = 0;
    int32_t level = 0;
    int32_t l1_block_addr = 0;
    int32_t l2_block_addr = 0;
    while(1) {
        // search in l0 block;
        if (level == 0) {
            cur_block = info->l0_block[++i];
            if (i == 31 && cur_block == 0) {
                l1_block_addr = getNextEmptyBlock(&s->theMap);
                l1_num++;
                s->blockUsed++;
                s->blockRemain--;
                info->l0_block[i] = l1_block_addr;
                int32_t new_block = getNextEmptyBlock(&s->theMap);
                l0_num++;
                s->blockUsed++;
                s->blockRemain--;
                ((struct l1_block *)block[l1_block_addr])->l0_block[0] = new_block;
                *theblock = new_block;
                if(*theblock == 0 || *theblock == 1) watchalloc = true;
                break;
            }
            else if (i != 31 && cur_block == 0) {
                int32_t new_block = getNextEmptyBlock(&s->theMap);
                l0_num++;
                s->blockUsed++;
                s->blockRemain--;
                info->l0_block[i] = new_block;
                *theblock = new_block;
                if(*theblock == 0 || *theblock == 1) watchalloc = true;
                break;
            }

            else if (i == 31 && cur_block != 0)
                { level++; i = 0; l1_block_addr = cur_block; continue;}
            else if (i != 31 && cur_block != 0) {
                continue;
            }
        }

        if (level == 1) {
            cur_block = ((struct l1_block *) block[l1_block_addr])->l0_block[i];
            i++;

            if (cur_block == 0 && i == 256) {
                l2_block_addr = getNextEmptyBlock(&s->theMap);
                l2_num++;
                s->blockUsed++;
                s->blockRemain--;
                ((struct l1_block *) block[l1_block_addr])->l0_block[255] = l2_block_addr;
                int32_t new_l1_block = getNextEmptyBlock(&s->theMap);
                l1_num++;
                s->blockUsed++;
                s->blockRemain--;
                ((struct l2_block *) block[l2_block_addr])->l1_block[0] = new_l1_block;
                int32_t new_l0_block = getNextEmptyBlock(&s->theMap);
                l0_num++;
                s->blockUsed++;
                s->blockRemain--;
                ((struct l1_block *)block[new_l1_block])->l0_block[0] = new_l0_block;
                *theblock = new_l0_block;
                if(*theblock == 0 || *theblock == 1) watchalloc = true;
                break;
            }
            else if (cur_block == 0 && i != 256) {
                int32_t new_l0_block = getNextEmptyBlock(&s->theMap);
                s->blockUsed++;
                s->blockRemain--;
                l0_num++;
                ((struct l1_block *) block[l1_block_addr])->l0_block[i-1] = new_l0_block;
                *theblock = new_l0_block;
                if(*theblock == 0 || *theblock == 1) watchalloc = true;
                break;
            }
            else if (cur_block != 0 && i == 256) {
                level++;
                i = 0;
                l2_block_addr = cur_block;
                l1_block_addr = ((struct l2_block *)block[cur_block])->l1_block[0];
                continue;
            }
            else if (cur_block != 0 && i != 256) {
                continue;
            }
        }

        if (level == 2) {
            struct l2_block *l2 = (struct l2_block *)block[cur_block];
            struct l1_block *l1;
            int m = 0, n = 0;
            int flag = 0;
            for (m = 0; m <= 254 & flag != 1; m++)
                for(n = 0; n <= 255 & flag != 1; n++) {//optimizable
                    l1_block_addr = l2->l1_block[m];
                    if (l1_block_addr == 0) {flag = 1; break;}
                    l1 = (struct l1_block *)block[l1_block_addr];
                    cur_block = l1->l0_block[n];
                    if (cur_block == 0) {flag = 1; break;}
                }
            if (l1_block_addr == 0) {
                l2->l1_block[m - 1] = getNextEmptyBlock(&s->theMap);
                l1_num++;
                if(l2->l1_block[m - 1] == 0) watchm = true;
                s->blockUsed++;
                s->blockRemain--;
                int t = l2->l1_block[m - 1];
                ((struct l1_block *)block[t])->l0_block[0] = getNextEmptyBlock(&s->theMap);
                l0_num++;
                s->blockUsed++;
                s->blockRemain--;
                *theblock = ((struct l1_block *)block[t])->l0_block[0];
                if(*theblock == 0 || *theblock == 1) watchalloc = true;
                break;
            }
            else if(l1_block_addr != 0 && cur_block == 0) { // level 3 not full level 2 not full level 1 not full
                l1->l0_block[n] = getNextEmptyBlock(&s->theMap);
                l0_num++;
                s->blockUsed++;
                s->blockRemain--;
                *theblock = l1->l0_block[n];
                if(*theblock == 0 || *theblock == 1) watchalloc = true;
                break;
            }
            else if(l2->l1_block[255] == 0) { // level 3 full
                l2->l1_block[255] = getNextEmptyBlock(&s->theMap);
                l2_num++;
                s->blockUsed++;
                s->blockRemain--;
                int t2 = l2->l1_block[255];
                struct l2_block *t_l2 = (struct l2_block *)block[t2];
                t_l2->l1_block[0] = getNextEmptyBlock(&s->theMap);
                l1_num++;
                s->blockUsed++;
                s->blockRemain--;
                struct l1_block *t_l1 = (struct l1_block *)block[t_l2->l1_block[0]];
                t_l1->l0_block[0] = getNextEmptyBlock(&s->theMap);
                l0_num++;
                s->blockUsed++;
                s->blockRemain--;
                *theblock = t_l1->l0_block[0];
                if(*theblock == 0 || *theblock == 1) watchalloc = true;
                break;
            }
            else {
                cur_block = l2->l1_block[255];
                continue;
            }
        }
    }
}

int32_t getNumBlock(off_t offset, struct fileinfo *info, int32_t *byteOffset) {

    if(offset >= info->st.st_size) return 0;

    off_t l0 = offset / BLOCKSIZE;
    if (l0 <= 30) {
        *byteOffset = offset - l0 * BLOCKSIZE;
        return info->l0_block[l0];
    }
    else {
        int l1 = (offset - BLOCKSIZE * 31) / BLOCKSIZE;
        if (l1 < BLOCKSIZE / sizeof(int32_t)) {//get help in l1
            off_t l0_offset = offset - 31 * BLOCKSIZE;
            *byteOffset = l0_offset - l1 * BLOCKSIZE;
            int32_t t1 = info->l0_block[31];
            int32_t t2 = ((struct l1_block *)block[t1])->l0_block[l1];
            return t2;
        } else { // time to get help from l2
            int32_t t1 = info->l0_block[31]; 
            off_t l2_offset = offset - BLOCKSIZE * (31 + 255);
            int32_t l2_temp = l2_offset / (255 * 256 * BLOCKSIZE);
            int temp;
            int32_t target = ((struct l1_block *)block[t1])->l0_block[255];
            for (temp = 0; temp < l2_temp; temp++) {
                target = ((struct l2_block *)block[target])->l1_block[255];
            }// now the result is in block[target]

            off_t l1_offset = l2_offset - (l2_temp * 255 * 256 * BLOCKSIZE);

            off_t l1_temp = l1_offset / (256 * BLOCKSIZE); // which l1_block?
            int32_t l1_target = ((struct l2_block *)block[target])->l1_block[l1_temp];// locate in block[l1_target]

            off_t l0_offset = l1_offset - (l1_temp * 256 * BLOCKSIZE);
            off_t l0_temp = l0_offset / BLOCKSIZE;
            *byteOffset = l0_offset - l0_temp * BLOCKSIZE;
            int32_t l0_target = ((struct l1_block *)block[l1_target])->l0_block[l0_temp];

            return l0_target;
        }
    }
}

static void* sfs_init(struct fuse_conn_info *conn) {
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
    //printf("%d, %d, %d, %d\n", ((struct super_block *)m)->blockUsed, ((struct super_block *)m)->blockRemain, 
    //((struct super_block *)m)->numOfFile, ((struct super_block *)m)->theMap.map[0]);
   
   return NULL;
}

static int sfs_getattr(const char *path, struct stat *st) {
    
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

static int sfs_readdir(const char *path, void *buffer, 
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

static int sfs_open(const char *path, struct fuse_file_info *info) {
    printf("call open, flags = %o\n", info->flags);
    return 0;
}

static void releaseBlock(int blockNum) {

    if(blockNum == 0 | blockNum == 1) {
        printf("are you crazy? release block num = %d\n", blockNum);
        return;
    }

    struct super_block *s = (struct super_block *)block[0];
    int temp = blockNum / 32;
    int bitOffset = 32 - 32 * temp;
    // set that bit to 1
    s->theMap.map[temp] |= (unsigned int)(1 << bitOffset);
    
    if(blockNum < s->theMap.fristUnused)
        s->theMap.fristUnused = blockNum;
    memset(block[blockNum], 0, BLOCKSIZE);
    return;
}

static int sfs_truncate(const char *path, off_t offset) {
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
        // delete the file?
        fi[i].st.st_size = offset;
    }
    return 0;
}

static int sfs_chmod(const char *path, mode_t mode, struct fileinfo *info) {
    return 0;
}

static int sfs_chown(const char *path, uid_t uid, gid_t gid, struct fileinfo *info) {
    return 0;
}

static int sfs_unlink(const char *path) {
    printf("call unlink, path = %s\n", path);
    char *name = (char *)(path + 1);
    struct fileinfo *theFile = (struct fileinfo *)block[1];

    struct super_block *s = (struct super_block *)block[0];

    int i = 0;
    for (i = 0; i < s->numOfFile; i++) {
        if (strcmp(theFile[i].filename, name) == 0) break;
    }

    int stop = 0;
    // now release the block, move the array up.
    // release l0;
    for (int j = 0; j < 31; j++) {
        if(theFile[i].l0_block[j] == 0){ stop = 1; break;}
        else {
            releaseBlock(theFile[i].l0_block[j]);
        }
    }
    if (stop != 1) { // release l1
        int32_t l1_block_num = theFile[i].l0_block[31];
        if (l1_block_num == 0) stop = 1;
        else { // release l1
            struct l1_block *l1 = (struct l1_block *)block[l1_block_num];
            for (int32_t j = 0; j < 255; j++) {
                if(l1->l0_block[j] == 0) { stop = 1; break;}
                else releaseBlock(l1->l0_block[j]);
            }
        }
    }
    if (stop != 1) {
        int32_t l1_block_num = theFile[i].l0_block[31];
        int32_t l2_block_num = ((struct l1_block *)block[l1_block_num])->l0_block[255];
        struct l2_block *l2_temp = (struct l2_block *)block[l2_block_num];
        while(stop != 1) { // release l2
            int32_t m = 0;
            int32_t n = 0;

            for (m = 0; m <= 254 & stop != 1; m++) {
                int32_t l1_num = l2_temp->l1_block[m];
                struct l1_block *l1_temp = (struct l1_block *)block[l1_num];
                for (n = 0; n <= 255; n++) {
                    if(l1_temp->l0_block[n] == 0) {stop = 1; break;}
                    else releaseBlock(l1_temp->l0_block[n]);
                }
            }

            if (l2_temp->l1_block[255] == 0) {stop = 1; break;}
            else 
                l2_block_num = l2_temp->l1_block[255];
        }
    }

    // now release the fileinfo.

    for(int l = i; l < s->numOfFile - 1; l++) {
        memcpy(&theFile[l], &theFile[l+1], sizeof(struct fileinfo));
    }

    s->numOfFile--;

    return 0;
}

static void mkFileInfo(const char *filename, struct stat *thestat) {

    struct super_block *s = (struct super_block *)block[0];

    struct fileinfo * theFile = (struct fileinfo *)block[1];
    struct fileinfo * newNode = &theFile[s->numOfFile++];
    strcpy(newNode->filename, filename);

    memcpy(&(newNode->st), thestat, sizeof(struct stat));
    
    return;
}

static int sfs_mknod(const char *path, mode_t mode, dev_t dev) {
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

static int sfs_read(const char *path, char *buffer, size_t size, off_t offset, 
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

        if(offset > s[i].st.st_size || s[i].st.st_size == 0) return 0;

        int32_t byteOffset;
        int start = getNumBlock(offset, &s[i], &byteOffset);
        int byteRead = 0;
        for (int i = 0; byteRead < size; i++) {
            if(byteRead + BLOCKSIZE - byteOffset > size){
                if(offset + byteRead + size > s[i].st.st_size) {
                    memcpy(buffer, (void *)((char *)block[start] + byteOffset), s[i].st.st_size - (byteRead + offset));
                    byteRead += s[i].st.st_size - (byteRead + offset);
                    return byteRead;
                }
                else {
                    memcpy(buffer,(void *) ((char *)block[start]+byteOffset), size);
                    byteRead += size;
                }
            }
            else{
                if(byteRead + BLOCKSIZE - byteOffset + offset > s[i].st.st_size) {
                    if(offset > s[i].st.st_size || s[i].st.st_size == 0) return 0;
                    memcpy(buffer,(void *) ((char *)block[start]+byteOffset), s[i].st.st_size - (offset + byteRead));
                    byteRead += s[i].st.st_size - (offset + byteRead);
                    return byteRead;
                }
                else {
                    memcpy(buffer,(void *) ((char *)block[start]+byteOffset), BLOCKSIZE - byteOffset);
                    byteRead += BLOCKSIZE - byteOffset;
                }
            }
            start = getNumBlock(offset + byteRead, &s[i], &byteOffset);
        }
        printf("%d bytes read by me\n", byteRead);
        return byteRead;
    }
}

static int sfs_write(const char *path, const char *src, size_t size, off_t offset, 
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
        int32_t start = getNumBlock(offset, &s[i], &byteOffset);
        int64_t byteWrote = 0;
        for (int i = 0; byteWrote < size; i++) {
            if (size < 8*1024 - byteOffset) {// can write in a block.
                if (start == 0) {//new block
                    alloBlock(&s[i], &start);
                }
                
                if(start == 0 || start == 1) watchpoint = true;

                memcpy((void *)((char *)block[start] + byteOffset),
                (void *)((char *)src + byteWrote), size);
                byteWrote += size;
                offset += size;
            }
            else { // just fill a block and go on.
                if (start == 0) {//new file or more size.
                    alloBlock(&s[i], &start);
                }

                if(start == 0 || start == 1) watchpoint = true;

                memcpy((void *)((char *)block[start] + byteOffset), 
                (void *)((char *)src + byteWrote), 8*1024 - byteOffset);
                byteWrote += 8*1024 - byteOffset;
                offset += 8*1024 - byteOffset;
                start = getNumBlock(offset, &s[i], &byteOffset);
            }
        }

        if (toffset + size > s[i].st.st_size) {
            s[i].st.st_size = toffset + size;
        }
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
    .truncate = sfs_truncate
}; 

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &op, NULL);
}