sfs : main.c
	gcc main.c -o ssfs `pkg-config fuse --cflags --libs`
