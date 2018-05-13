sfs : main.c
	gcc -D_FILE_OFFSET_BITS=64 -o ssfs main.c `pkg-config fuse --cflags --libs` -g
clean :
	rm ssfs
