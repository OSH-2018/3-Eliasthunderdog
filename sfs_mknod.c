#include "types.h"
#include <stdio.h>
#include <string.h>
#include <fuse.h>

void mkFileInfo(const char *filename, struct stat *thestat) {

    struct super_block *s = (struct super_block *)block[0];

    struct fileinfo * theFile = (struct fileinfo *)block[1];
    struct fileinfo * newNode = &theFile[s->numOfFile++];
    strcpy(newNode->filename, filename);

    memcpy(&(newNode->st), thestat, sizeof(struct stat));
    
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
    mkFileInfo(path + 1, &st);
    return 0;
}