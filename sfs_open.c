#include "types.h"
#include <stdio.h>
int sfs_open(const char *path, struct fuse_file_info *info) {
    printf("call open,path=%s flags = %o\n",path, info->flags);
    return 0;
}