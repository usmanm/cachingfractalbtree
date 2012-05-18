#include <assert.h>
#include "benchmark.h"

void insert_items(uint32_t start, uint32_t numofitems, 
		  bool random, bool cached) {
  uint32_t i, j;
  //  time_t stime = time(NULL);

  //  printf("Inserting %u items...\n", numofitems);

  for (i = start; i < numofitems + start; i++) {
    fb_tuple t;

    t.id = random ? (uint32_t) (rand() + start) : i;
    strncpy((char *) &t.name, "Benchmarking Ninjas", 19);
    t.name[19] = (char) t.id;

    for (j = 0; j < 2; j++) {
      t.items[j] = (uint32_t) rand();
    }

    if (cached) {
      insert_cached(i, &t);
    }
    else {
      insert_uncached(i, &t);
    }
  }

  //  printf("%u items inserted in %lu s.\n", 
  //	 numofitems, (unsigned long) (time(NULL) - stime));
}

// Items must be packed in range [0:range]
void lookup_items(uint32_t numoflookups, uint32_t range, 
		  bool random, bool cached) {
  //  time_t stime = time(NULL);
  uint32_t i;

  //  printf("Looking up %u items...\n", numoflookups);

  for (i = 0; i < numoflookups; i++) {
    fb_key k;
    fb_tuple t;
    char str[] = "Benchmarking Ninjas";

    k = random ? (fb_key) (rand() % range) : i;

    if (cached) {
      search_cached(k, &t);
    }
    else {
      search_uncached(k, &t);
    }

    // Verify
    assert(t.id == (uint32_t) k);
    assert(memcmp(&t.name, &str, 19) == 0);
    assert(t.name[19] == (char) k);
  }

  //  printf("Looked up %u items in %lu s.\n", 
  //	 numoflookups, (unsigned long) (time(NULL) - stime));
}

/*
All benchmarks in 10K, 100K, 200K, 500k, 1M, 2M, 5M, 10M items range.
*/

void
_benchmark_inserts(bool random, bool cached) 
{
  struct timespec start, end, diff;
  uint32_t n, d, o;
  clock_gettime(CLOCK_REALTIME, &start);

  // Uncached first, then cached
  // Random first, sequential later

  // 10K
  o = 0;
  n = d = 10000;
  insert_items(0, d, random, cached);  
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 100K
  o = n;
  d = 100000 - n;
  n = 100000;
  insert_items(o, d, random, cached);  
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 200K
  o = n;
  d = 200000 - n;
  n = 200000;
  insert_items(o, d, random, cached);  
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 500K
  o = n;
  d = 500000 - n;
  n = 500000;
  insert_items(o, d, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 1M
  o = n;
  d = 1000000 - n;
  n = 1000000;
  insert_items(o, d, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 2M
  o = n;
  d = 2000000 - n;
  n = 2000000;
  insert_items(o, d, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 5M
  o = n;
  d = 5000000 - n;
  n = 5000000;
  insert_items(o, d, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 10M
  o = n;
  d = 10000000 - n;
  n = 10000000;
  insert_items(o, d, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Insert [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, 0, 0, (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);
}

void 
benchmark_inserts(size_t bs, size_t ss, size_t bf)
{
  int i;
  bool j = false;

  for (i = 0; i < 4; i++) {
    j = !j; 
    init(bs, ss, bf);
    _benchmark_inserts(j, i % 2);
    destr();
  }
}

void
_benchmark_searches(bool random, bool cached)
{
  struct timespec start, end, diff;
  uint32_t n, d;
  clock_gettime(CLOCK_REALTIME, &start);

  // Insert items, uncached
  insert_items(0, 10000000, false, false);  

  // Uncached first, then cached
  // Random first, sequential later
  // 10K
  n = d = 10000;
  lookup_items(n, 10000000, random, cached);  
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 100K
  d = 100000 - n;
  n = 100000;
  lookup_items(n, 10000000,random, cached);  
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 200K
  d = 200000 - n;
  n = 200000;
  lookup_items(n, 10000000, random, cached);  
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 500K
  d = 500000 - n;
  n = 500000;
  lookup_items(n, 10000000, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 1M
  d = 1000000 - n;
  n = 1000000;
  lookup_items(n, 10000000, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 2M
  d = 2000000 - n;
  n = 2000000;
  lookup_items(n, 10000000, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 5M
  d = 5000000 - n;
  n = 5000000;
  lookup_items(n, 10000000, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);

  // 10M
  d = 10000000 - n;
  n = 10000000;
  lookup_items(n, 10000000, random, cached);
  clock_gettime(CLOCK_REALTIME, &end);
  get_diff(&start, &end, &diff);
  printf("Search [N: %u, C: %u, R: %u] ==> %llu.%ld\n", 
	 n, cached, random, 
	 (unsigned long long) diff.tv_sec, diff.tv_nsec/1000);
}

void 
benchmark_searches(size_t bs, size_t ss, size_t bf)
{
  int i;
  bool j = false;

  for (i = 0; i < 4; i++) {
    j = !j; 
    init(bs, ss, bf);
    _benchmark_searches(j, i % 2);
    destr();
  }
}

inline void get_diff(struct timespec *start, struct timespec *end,
	      struct timespec *diff)
{
  if (end->tv_nsec < start->tv_nsec) {
    diff->tv_sec = end->tv_sec + 1 - start->tv_sec;
    diff->tv_nsec = start->tv_nsec - end->tv_nsec;
  }
  else {
    diff->tv_sec = end->tv_sec - start->tv_sec;
    diff->tv_nsec = end->tv_nsec - start->tv_nsec;
  }
}
