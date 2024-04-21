all: lock

sparse-gen: lock.o
	    gcc -o lock lock.o

sparse-gen.o: lock.c
	      gcc -c lock.c

.PHONY: all