#include "types.h"
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void* sfs_init(struct fuse_conn_info *conn) {
    printf("call init\n");
   int blockSize = 8 * 1024;
   int blockNum = 512 * 1024; // the number of blocks
   //printf("unmap!\n");
   block[1] = mmap(NULL, 4*128*1024, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   block[0] = mmap(NULL, sizeof(struct super_block), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   //init S
   struct super_block* S = (struct super_block*) block[0];
   S->blockUsed = 2;
   S->blockRemain = blockNum - 2;
   S->theMap.fristUnused = 2;
   struct fileinfo *root = (struct fileinfo *)block[1];
   root->type = DIR;
   root->filename[0] = '/';
   root->st.st_mode = __S_IFDIR | 0755; // there are no limitation except for execution of directory.
   root->st.st_nlink = 2;
   root->st.st_ctime = time(NULL);
   root->st.st_atime = time(NULL);
   root->st.st_mtime = time(NULL);

   memset(S->theMap.map, -1, sizeof(S->theMap.map)); //doubt.
   S->theMap.map[0] &= 0x3fffffff;
   return NULL;
}