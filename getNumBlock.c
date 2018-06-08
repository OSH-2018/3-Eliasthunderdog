# include "types.h"
int32_t getNumBlock(off_t offset, struct fileinfo *info, int32_t *byteOffset, struct position *p) {

    if(offset > info->st.st_size || offset < 0) return 0;

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
