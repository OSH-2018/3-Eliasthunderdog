#include "types.h"
#include <stdio.h>
#include <string.h>

int sfs_write(const char *path, const char *src, size_t size, off_t offset, 
                    struct fuse_file_info *info)
{
    // let's assume that path is root.

    off_t toffset = offset;
    printf("call write, path=%s, size=%d, offset=%d\n", path, size, offset);

    struct super_block *S = (struct super_block *)block[0];
    struct fileinfo *s = (struct fileinfo *)block[1];
    
    struct fileinfo *t;
    struct fileinfo *p;

    t = getfile(path, &p);

    if (t == NULL) {
        printf("write failed, file not found\n");//create new file? no
        return 0;
    } else {// I've found it ! I've found it !!
        int32_t byteOffset = 0;
        struct position p;
        int32_t start = getNumBlock(offset, t, &byteOffset, &p);
        int64_t byteWrote = 0;
        for (int j = 0; byteWrote < size; j++) {
            if (size < 8*1024 - byteOffset) {// can write in a block.
                if (start == 0) {//new block
                    newAlloBlock(t, &start);
                }

                memcpy((void *)((char *)block[start] + byteOffset),
                (void *)((char *)src + byteWrote), size);
                byteWrote += size;
                offset += size;
            }
            else { // just fill a block and go on.
                if (start == 0) {//new file or more size.
                    newAlloBlock(t, &start);
                }

                memcpy((void *)((char *)block[start] + byteOffset), 
                (void *)((char *)src + byteWrote), 8*1024 - byteOffset);
                byteWrote += 8*1024 - byteOffset;
                offset += 8*1024 - byteOffset;
                start = getNumBlock(offset, t, &byteOffset, &p);
            }
        }/*
        if (toffset + size > s[i].st.st_size) {
            s[i].st.st_size = toffset + size;
        }*/
        printf("wrote %d bytes to the file.\n", byteWrote);
        return byteWrote;
    }
}
