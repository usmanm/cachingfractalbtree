#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "db.h"

// 10K, 100K, 200K, 500k, 1M, 2M, 5M, 10M 

void get_diff(struct timespec *start, struct timespec *end, 
	      struct timespec *diff);

void insert_items(uint32_t start, uint32_t numofitems, 
		  bool random, bool cached);
void search_items(uint32_t numoflookups, uint32_t range, bool random, 
		  bool cached);

void _benchmark_inserts(bool random, bool cached);
void benchmark_inserts(size_t block_size, size_t slot_size, size_t bfactor);
void _bechmark_searches(bool random, bool cached);
void benchmark_searches(size_t block_size, size_t slot_size, size_t bfactor);

#endif
