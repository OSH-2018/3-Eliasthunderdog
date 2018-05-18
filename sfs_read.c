#include "types.h"
#include <stdio.h>
#include <string.h>

int sfs_read(const char *path, char *buffer, size_t size, off_t offset, 
                    struct fuse_file_info *info) {
    //return the number of bytes it has read.
    //
    // now le's assume that it operates on the root directory.
    printf("call read, path=%s, size=%d, offset=%d\n", path, size, offset);
    struct super_block *S = (struct super_block *)block[0];
    struct fileinfo *s = (struct fileinfo *)block[1];
    int flag = 1;
    int i;
    for (i = 0; i < S->numOfFile; i++) {
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