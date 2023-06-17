all:
	gcc xvmalloc.c test.c

test: all
	./a.out
	./a.out -s 32:4088
	./a.out -s 32:4088 -c