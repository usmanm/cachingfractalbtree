#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "b_tree.h"

void init_benchmark();
void load_tables(uint32_t numofitems);
void test_lookups(uint32_t numoflookups, uint32_t numofitems);

#endif
