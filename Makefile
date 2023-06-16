all:
	gcc xvmalloc.c test.c
	./a.out
	./a.out -s 32:4088
	./a.out -s 32:4088 -c