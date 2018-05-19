#include "types.h"
#include <fuse.h>
#include <stdio.h>

int sfs_opendir(const char *path, struct fuse_file_info * info) {
    printf("called opendir,path=%s\n", path);
    return 0;
}