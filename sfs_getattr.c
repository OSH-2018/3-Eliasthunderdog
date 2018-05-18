#include "types.h"
#include <errno.h>
int sfs_getattr(const char *path, struct stat *st) {
    
    printf("call getattr, path = %s\n", path);
    
    if(strcmp(path, "/") == 0) {
        printf("It's root !\n");
        st->st_mode = __S_IFDIR | 0755;
        st->st_nlink = 2; // the root directory has 2 hard links
    } else {
        printf("called else, path=%s\n", path);
        int found = 0;
        int i = 0;
        struct fileinfo *f = (struct fileinfo *)block[1];
        struct super_block *s = (struct super_block *)block[0];
        printf("numofFile=%d\n", s->numOfFile);
        for (i = 0; i <= s->numOfFile - 1; i++) {
            if(strcmp(path + 1, f[i].filename) == 0)
            { found = 1; break;}
        }
        if (found == 1) {
            memcpy(st, &(f[i].st), sizeof(struct stat));
        }
        else {
            printf("no file found, return %d\n", -ENOENT);
            return -ENOENT;
        } 
    }
    return 0;
}