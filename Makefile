all: lock

lock: lock.o
	    gcc -o lock lock.o

lock.o: lock.c
	      gcc -c lock.c

.PHONY: all