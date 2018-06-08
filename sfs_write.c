  #include "types.h"
#include <stdio.h>
#include <string.h>

int sfs_write(const char *path, const char *src, size_t size, off_t offset,
                    struct fuse_file_info *info)
{
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
        int32_t end = getNumBlock(offset + size, t, &byteOffset, &p);
        int32_t start;
        int64_t byteWrote = 0;

        if(end == 0)  extend(t, offset + size);

        start = getNumBlock(offset, t, &byteOffset, &p);

        while (byteWrote < size) {

            if (size - byteWrote < 8*1024 - byteOffset) {// can write in a block.
                memcpy((void *)((char *)block[start] + byteOffset),
                  (void *)((char *)src + byteWrote), size - byteWrote);
                byteWrote += size - byteWrote;
                toffset += size - byteWrote;
            }
            else { // just fill a block and go on.
                memcpy((void *)((char *)block[start] + byteOffset),
                (void *)((char *)src + byteWrote), 8*1024 - byteOffset);
                byteWrote += 8*1024 - byteOffset;
                toffset += 8*1024 - byteOffset;
                start = getNumBlock(toffset, t, &byteOffset, &p);
            }
        }

        printf("wrote %d bytes to the file.\n", byteWrote);
        return byteWrote;
    }
}
