#include "types.h"
#include <stdio.h>
int sfs_open(const char *path, struct fuse_file_info *info) {
    printf("call open, flags = %o\n", info->flags);
    return 0;
}