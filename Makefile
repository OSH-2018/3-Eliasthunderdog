sfs : main.c
	gcc -O -D_FILE_OFFSET_BITS=64 -o ssfs main.c `pkg-config fuse --cflags --libs`
clean :
	rm ssfs