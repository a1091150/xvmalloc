all:
	gcc xvmalloc.c test.c

test: all
	./a.out
	./a.out -s 32:4088
	./a.out -s 32:4088 -c

perf: all
	perf record -g --call-graph dwarf ./a.out -s 32:4088 -c
	perf report --stdio -g graph,0.5,caller > perf.txt