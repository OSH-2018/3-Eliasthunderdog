#include "types.h"
#include <stdio.h>
#include <errno.h>
int sfs_readdir(const char *path, void *buffer, 
    fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info) {

    printf("call readdir, path=%s, offset=%d\n", path, offset);

    struct fileinfo *f = (struct fileinfo *)block[1];

    struct fileinfo *t;
    struct fileinfo *p;
    t = getfile(path, &p);
    
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    if (t != NULL && t->type == DIR)
    for (int i = 0; i < 32; i++) {
        int m = t->l0_block[i];
        if (m != 0)
            filler(buffer, f[m].filename, NULL, 0);
    }
    else if (t == NULL) {
        return -ENONET;
    }
    else if (t->type == DIR) {
        printf("not a dir\n");
        return -ENOENT;
    }

    return 0;
}