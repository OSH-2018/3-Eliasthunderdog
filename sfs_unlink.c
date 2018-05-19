#include "types.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int sfs_unlink(const char *path) {
    printf("call unlink, path = %s\n", path);

    struct super_block *s = (struct super_block *)block[0];
    struct fileinfo *f = (struct fileinfo *)block[1];

    struct fileinfo *t;
    struct fileinfo *p;

    t = getfile(path, &p);

    if (t != NULL) {
        erase(t, 0);
        
        s->numOfFile--;
        int i;
        for (i = 0; i < 32; i++) {
            int m = p->l0_block[i];
            if (strcmp(t->filename, f[m].filename) == 0) {
                p->l0_block[i] = 0;
                break;
            }
        }
        memset(t, 0, sizeof(struct fileinfo));
        return 0;
    }

    else return -ENOENT;
}