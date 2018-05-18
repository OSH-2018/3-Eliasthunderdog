#include "types.h"
#include <errno.h>

int sfs_unlink(const char *path) {
    printf("call unlink, path = %s\n", path);
    char *name = (char *)(path + 1);
    struct fileinfo *theFile = (struct fileinfo *)block[1];

    struct super_block *s = (struct super_block *)block[0];
    int flag = 0;
    int i = 0;
    for (i = 0; i < s->numOfFile; i++) {
        if (strcmp(theFile[i].filename, name) == 0) {flag = 1; break;}
    }

    if (flag == 1) {
        sfs_truncate(path, 0);
        for(int l = i; l < s->numOfFile; l++) {
            memcpy(&theFile[l], &theFile[l+1], sizeof(struct fileinfo));
        }
        s->numOfFile--;
        return 0;
    }

    else return -ENOENT;
}