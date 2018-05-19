#include "types.h"
#include <stdio.h>
#include <string.h>
#include <fuse.h>

void mkFileInfo(const char *filename, struct fileinfo *parent, struct stat *thestat) {

    struct super_block *s = (struct super_block *)block[0];

    struct fileinfo * theFile = (struct fileinfo *)block[1];
    
    int i = 0;

    for (i = 0; theFile[i].filename[0]; i++)
    ; // find the empty space.

    strcpy(theFile[i].filename, filename);

    theFile[i].type = REG;

    memcpy(&(theFile[i].st), thestat, sizeof(struct stat));

    for (int j = 0; j < 32; j++) {
        if (parent->l0_block[j] == 0) {
            (parent->l0_block[j]) = i;
            break;
        }
    }
    
    return;
}

int sfs_mknod(const char *path, mode_t mode, dev_t dev) {
    printf("mknod, path = %s, mode = %o", path, mode);
    struct stat st;
    st.st_mode = __S_IFREG | 0666;
    st.st_uid = fuse_get_context()->uid;
    st.st_gid = fuse_get_context()->gid;
    st.st_nlink = 1;
    st.st_size = 0;
    st.st_atime = time(NULL);
    st.st_mtime = time(NULL);
    st.st_ctime = time(NULL);
    
    struct fileinfo *t;
    struct fileinfo *p;

    t = getfile(path, &p);

    char *names1 = strrchr(path, '/');

    mkFileInfo(names1 + 1, p, &st);

    return 0;
}