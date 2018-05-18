#include "types.h"
#include <errno.h>
int sfs_utimens(const char *path, const struct timespec tv[2]) {
    struct fileinfo * file = findFile(path);
    if (file == NULL) return -ENOENT;
    else {
        file->st.st_atime = tv[0].tv_sec;
        file->st.st_mtime = tv[1].tv_sec;
        return 0;
    }
}