test_write:	all newTest.c
	gcc -Wall -fPIC -I. -o msh testWrite.c -L. -lFAT32 -g
all_static: main.c fat32.c utils32.c fat16_32.h fat32api.h utils32.h
	gcc main.c fat32.c utils32.c -g -o main
all: fat32.c utils32.c fat16_32.h fat32api.h utils32.h
	gcc fat32.c utils32.c -fPIC -shared -o libFAT32.so -g
