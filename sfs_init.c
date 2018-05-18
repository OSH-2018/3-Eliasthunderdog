#include "types.h"
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

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