
CC = gcc

CFLAGS = -Wall -Wextra -std=c99 -fPIC

DEBUG = -g

DEFS = -D_POSIX_C_SOURCE=200112L

PERF = -O3 -march=native

LIBS = -lm

test: fb_tree.c fb_tree.h test.c db.h db.c
	$(CC) $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -o test test.c

fb_tree.o: fb_tree.c fb_tree.h
	$(CC) $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -c -o fb_tree.o fb_tree.c
	$(CC) -shared $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -o fb_tree.so fb_tree.c


.PHONY: clean
clean:
	rm -fR *.o
	rm -fR *.so
