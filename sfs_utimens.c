#include "types.h"
#include <errno.h>
int sfs_utimens(const char *path, const struct timespec tv[2]) {
    
    struct fileinfo *t;
    struct fileinfo *p;

    t = getfile(path, &p);

    if (t == NULL) return -ENOENT;
    else {
        t->st.st_atime = tv[0].tv_sec;
        t->st.st_mtime = tv[1].tv_sec;
        return 0;
    }
}