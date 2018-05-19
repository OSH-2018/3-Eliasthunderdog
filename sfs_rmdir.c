#include "types.h"
#include <fuse.h>
#include <errno.h>
#include <string.h>

int isempty(struct fileinfo *info) {
    if(info->type != DIR) return -1;
    else {
        int i = 0;
        for (i = 0; i <= 31; i++) {
            if (info->l0_block[i] != 0) return 1; // not empty.
        }
        return 0; // empty.
    }
}

int sfs_rmdir(const char * path) {

    struct fileinfo *t;
    struct fileinfo *p;

    t = getfile(path, &p);
    struct fileinfo *f = (struct fileinfo *)block[1];
    struct super_block *s = (struct super_block *)block[0];

    if (t == NULL) return -ENOENT;
    else {
        int m = isempty(t);
        if (m == 1) return -ENOTEMPTY;
        else if (m == 0) {
            int i = 0;
            for (i = 0; i <= 31; i++) {
                int j = p->l0_block[i];
                if (strcmp(t->filename, f[j].filename) == 0){
                    p->l0_block[i] = 0;//release the reference.
                    break;
                }
            }
            s->numOfFile--;
            memset(t, 0, sizeof(struct fileinfo));//release the filenode.
            return 0;
        }
    }
}