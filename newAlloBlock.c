#include "types.h"
#include <stdbool.h>
#include <sys/mman.h>

int32_t getNextEmptyBlock(struct blockBitmap* theBlockBitmap) {

    int temp = theBlockBitmap->fristUnused;
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
            break;
        }
    }

    if (i == SIZE/BLOCKSIZE){
        return 0; //indicate that all blocks are used.
    }
    block[temp] = mmap(NULL, sizeof(struct super_block), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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
            return 0;
        }
        else {
            // allocate an l0_block
            *the_block = getNextEmptyBlock(&s->theMap);
            info->l0_block[info->l0_offset] = *the_block;
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
            return 0;
        }
        else {
            // allocate a l0_block.
            int32_t new_l0 = getNextEmptyBlock(&s->theMap);
            int32_t l1_addr = info->l0_block[31];
            struct l1_block * l1 = (struct l1_block *)block[l1_addr];
            l1->l0_block[info->l0_offset] = new_l0;
            info->l0_offset++;
            *the_block = new_l0;
            return 0;
        }
    }
    else { // from l2
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
            return 0;
        }
    }
}