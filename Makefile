all: bmalloc.h bmalloc.c test1.c test2.c test3.c
	gcc -o test1 test1.c bmalloc.c -fpack-struct
	gcc -o test2 test2.c bmalloc.c -fpack-struct
	gcc -o test3 test3.c bmalloc.c -fpack-struct
	gcc -o test4 test4.c bmalloc.c -fpack-struct
clean:
	rm -rf test1 test2 test3 test4 bmalloc.o
