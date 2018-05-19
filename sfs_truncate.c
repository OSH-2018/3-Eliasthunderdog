#include "types.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int sfs_truncate(const char *path, off_t offset) {
    printf("call truncate, path=%s, offset=%d", path, offset);
    struct super_block *s = (struct super_block *)block[0];
    struct fileinfo *fi = (struct fileinfo *)block[1];

    struct fileinfo *t;
    struct fileinfo *p;

    t = getfile(path, &p);

    if (t != NULL) {
        if(t->st.st_size > offset) erase(t, offset);
        else extend(t, offset);
        return 0;
    }
    else return -ENOENT;
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

void releaseBlock(int blockNum) {

    if(blockNum == 0 | blockNum == 1) {
        printf("are you crazy? release block num = %d\n", blockNum);
        return;
    }

    munmap(block[blockNum], 8 * 1024);

    struct super_block *s = (struct super_block *)block[0];
    int temp = blockNum / 32;
    int bitOffset = blockNum - 32 * temp;
    // set that bit to 1
    s->theMap.map[temp] |= (unsigned int)(1 << (31 - bitOffset));
    
    if(blockNum < s->theMap.fristUnused)
        s->theMap.fristUnused = blockNum;
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
        releaseBlock(l1->l0_block[i]);
        l1->l0_block[i] = 0;
    }
    if (l0_offset == 0) {
        releaseBlock(l1_addr);
    }
    return;
}