
CC = gcc

CFLAGS = -Wall -Wextra -std=c99 -fPIC

DEBUG = -g

DEFS = -D_POSIX_C_SOURCE=200112L

PERF = -O3 -march=native

LIBS = -lm -lrt

test: cfb_tree.c cfb_tree.h test.c db.h db.c benchmark.c benchmark.h
	$(CC) $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -o test test.c benchmark.c db.c cfb_tree.c

#fb_tree.o: cfb_tree.c cfb_tree.h fb_tree.c fb_tree.h
#	$(CC) $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -c -o fb_tree.o fb_tree.c
#	$(CC) -shared $(CFLAGS) $(DEBUG) $(PERF) $(LIBS) $(DEFS) -o fb_tree.so fb_tree.c


.PHONY: clean
clean:
	rm -fR *.o
	rm -fR *.so
