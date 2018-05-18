objects = main.o getNumBlock.o newAlloBlock.o sfs_getattr.o sfs_init.o sfs_mknod.o sfs_open.o sfs_read.o \
	sfs_readdir.o sfs_truncate.o sfs_unlink.o sfs_utimens.o sfs_write.o
FLAGS = -D FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`

sfs : $(objects)
	cc -o sfs $(objects)
getNumBlock.o : getNumBlock.c types.h
	cc $(FLAGS) -c getNumBlock.c
newAlloBlock.o : newAlloBlock.c types.h
	cc $(FLAGS) -c newAlloBlock.c
sfs_getattr.o : sfs_getattr.c types.h
	cc $(FLAGS) -c sfs_getattr.c
sfs_init.o : sfs_init.c types.h
	cc $(FLAGS) -c sfs_init.c
sfs_mknod.o : sfs_mknod.c types.h
	cc $(FLAGS) -c sfs_mknod.c
sfs_open.o : sfs_open.c types.h
	cc $(FLAGS) -c sfs_open.c
sfs_read.o : sfs_read.c types.h
	cc $(FLAGS) -c sfs_read.c
sfs_readdir.o : sfs_readdir.c types.h
	cc $(FLAGS) -c sfs_readdir.c
sfs_truncate.o : sfs_truncate.c types.h
	cc $(FLAGS) -c sfs_truncate.c
sfs_unlink.o : sfs_unlink.c types.h
	cc $(FLAGS) -c sfs_unlink.c
sfs_utimens.o : sfs_utimens.c types.h
	cc $(FLAGS) -c sfs_utimens.c
sfs_write.o : sfs_write.c types.h
	cc $(FLAGS) -c sfs_write.c
main.o : main.c types.h
	cc $(FLAGS) -c main.c 

clean :
	rm ssfs $(objects)
