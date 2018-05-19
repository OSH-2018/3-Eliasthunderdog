objects = main.o getNumBlock.o newAlloBlock.o sfs_getattr.o sfs_init.o sfs_mknod.o sfs_open.o sfs_read.o \
	sfs_readdir.o sfs_truncate.o sfs_unlink.o sfs_utimens.o sfs_write.o sfs_mkdir.o sfs_opendir.o sfs_rmdir.o
FLAGS = -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs` -g

sfs : $(objects)
	cc -o sfs $(objects) $(FLAGS)
getNumBlock.o : getNumBlock.c types.h
	cc -c getNumBlock.c $(FLAGS)
newAlloBlock.o : newAlloBlock.c types.h
	cc -c newAlloBlock.c $(FLAGS)
sfs_getattr.o : sfs_getattr.c types.h
	cc -c sfs_getattr.c  $(FLAGS)
sfs_init.o : sfs_init.c types.h
	cc -c sfs_init.c  $(FLAGS)
sfs_mknod.o : sfs_mknod.c types.h
	cc -c sfs_mknod.c  $(FLAGS)
sfs_open.o : sfs_open.c types.h
	cc -c sfs_open.c  $(FLAGS)
sfs_read.o : sfs_read.c types.h
	cc -c sfs_read.c  $(FLAGS)
sfs_readdir.o : sfs_readdir.c types.h
	cc -c sfs_readdir.c  $(FLAGS)
sfs_truncate.o : sfs_truncate.c types.h
	cc -c sfs_truncate.c  $(FLAGS)
sfs_unlink.o : sfs_unlink.c types.h
	cc -c sfs_unlink.c  $(FLAGS)
sfs_utimens.o : sfs_utimens.c types.h
	cc -c sfs_utimens.c  $(FLAGS)
sfs_write.o : sfs_write.c types.h
	cc -c sfs_write.c  $(FLAGS)
sfs_mkdir.o : sfs_mkdir.c types.h
	cc -c sfs_mkdir.c $(FLAGS)
sfs_opendir.o : sfs_opendir.c types.h
	cc -c sfs_opendir.c $(FLAGS)
sfs_rmdir.o : sfs_rmdir.c types.h
	cc -c sfs_rmdir.c $(FLAGS)
main.o : main.c types.h
	cc -c main.c $(FLAGS)
clean :
	rm $(objects)
