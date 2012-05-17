#ifndef DB_H
#define DB_H

#include <unistd.h>
#include <stdint.h>

#include "cfb_tree.h"

void init(size_t block_size, size_t slot_size, size_t bfactor);

void destr();

int insert_cached(fb_key key, fb_tuple *tuple);
int insert_uncached(fb_key key, fb_tuple *tuple);

int search_cached(fb_key key, fb_tuple *t);
int search_uncached(fb_key key, fb_tuple *t);

#endif

