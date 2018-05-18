#include "types.h"
#include <stdio.h>
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