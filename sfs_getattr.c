#include "types.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
int sfs_getattr(const char *path, struct stat *st) {
    
    printf("call getattr, path = %s\n", path);
    
    if(strcmp(path, "/") == 0) {
        printf("It's root !\n");
        st->st_mode = __S_IFDIR | 0755;
        st->st_nlink = 2; // the root directory has 2 hard links
    } else {
        printf("called else, path=%s\n", path);
        
        struct fileinfo *t;
        struct fileinfo *p;

        t = getfile(path, &p);
        if (t != NULL)
            memcpy(st, &(t->st), sizeof(struct stat));
        else{
            printf("no file found, return %d\n", -ENOENT);
            return -ENOENT;
        }
    }
    return 0;
}