
CC = gcc

CFLAGS = -Wall -Wextra -std=c99 -fPIC

DEBUG = -g

DEFS = -D_POSIX_C_SOURCE=200112L

PERF = -O3 -march=native

LIBS = -lm

test: b_tree.o benchmark.o test.c
	$(CC) $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -o test test.c benchmark.o b_tree.o

b_tree.o: b_tree.c b_tree.h
	$(CC) $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -c -o b_tree.o b_tree.c
	$(CC) -shared $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -o b_tree.so b_tree.c

benchmark.o: benchmark.c 
	$(CC) $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -c -o benchmark.o benchmark.c
	$(CC) -shared $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -o benchmark.so benchmark.c

.PHONY: clean
clean:
	rm -fR *.o
	rm -fR *.so
	rm test
	rm *.db