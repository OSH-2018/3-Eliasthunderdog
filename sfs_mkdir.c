#include "types.h"
#include <string.h>

struct fileinfo * getfile(const char *path, struct fileinfo **parent) {//super fucking important
    
    char pathtmp[200];
    char name[32];
    strcpy(pathtmp, path);
    struct fileinfo *root = (struct fileinfo *)block[1];
    struct fileinfo *cur = root;
    struct fileinfo *infotmp;
    struct fileinfo *parentmp = root;

    if (strcmp(path, "/") == 0) {
        *parent = parentmp;
        return cur;
    }

    char *t = strchr(pathtmp, '/');
    *t = 0;
    // /0dir1/dir2/dir3/file
    while (t != NULL) { // doubt
        char *tmp = t + 1;
        t = strchr(t + 1, '/');
        if(t != NULL) *t = 0;
        strcpy(name, tmp);
        parentmp = cur;
        cur = findCurDir(cur, name);
        if (cur == NULL){ *parent = parentmp; return NULL;}
    }

    *parent = parentmp;
    return cur;
}

struct fileinfo * findCurDir(struct fileinfo *info, const char *name) {
    if(info->type != DIR) return NULL;
    else {
        struct fileinfo *f = (struct fileinfo *)block[1];
        int i = 0;
        for (i = 0; i < 32; i++) {
            int m = info->l0_block[i];
            if(strcmp(name, f[m].filename) == 0) return &f[m];
        }
        return NULL;
    }
}

int sfs_mkdir(const char* path, mode_t mode) {
    struct fileinfo * t;
    struct fileinfo * p;

    t = getfile(path, &p);

    struct super_block *s = (struct super_block *)block[0];
    struct fileinfo *f = (struct fileinfo *)block[1];

    int i = 0;
    for (i = 0; f[i].filename[0]; i++)
    ; // find an empty space.

    t = &f[i];
    char *names1 = strrchr(path, '/');

    strcpy(t->filename, names1 + 1);

    t->st.st_mode = __S_IFDIR | mode;
    t->st.st_ctime = time(NULL);
    t->st.st_atime = time(NULL);
    t->st.st_mtime = time(NULL);
    t->st.st_uid = fuse_get_context()->uid;
    t->st.st_gid = fuse_get_context()->gid;
    t->st.st_size = 8*1024; // doubt.
    t->st.st_nlink = 2;
    s->numOfFile++;

    int j = 0;

    for (j = 0; p->l0_block[j] != 0; j++)
    ;

    p->l0_block[j] = i;
    return 0;
};