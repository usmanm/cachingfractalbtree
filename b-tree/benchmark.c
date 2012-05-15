#include <assert.h>
#include "benchmark.h"

void init_benchmark() {
  unsigned int s = (unsigned int) time(NULL);
  srand(s);
  remove("btree.db");
  remove("table.db");
}

void load_tables(uint32_t numofitems) {
  uint32_t i, j;
  time_t stime = time(NULL);

  printf("Loading table [%u]...\n", numofitems);

  for (i = 0; i < numofitems; i++) {
    tuple_t t;

    t.id = i;
    strncpy((char *) &t.name, "Benchmarking Ninjas", 19);
    t.name[19] = (char) i;

    for (j = 0; j < 5; j++) {
      t.items[j] = (uint32_t) (rand() % numofitems);
    }

    insert(i, &t);
  }

  printf("Table loaded in %lu s.\n", (unsigned long) (time(NULL) - stime));
}

void test_range(uint32_t rangesize, uint32_t numofitems) {
  uint32_t i = rand();
  time_t stime = time(NULL);

  printf("Testing range [%u]...\n", rangesize);

  while (rangesize != 0) {
    tuple_t t;
    char str[] = "Benchmarking Ninjas";

    search(i % numofitems, &t);

    // Verify
    assert(t.id == (uint32_t) (i % numofitems));
    assert(memcmp(&t.name, &str, 19) == 0);
    assert(t.name[19] == (char) (i % numofitems));
    i++;
    rangesize--;
  }

  printf("Range tested in %lu s.\n", (unsigned long) (time(NULL) - stime));
}

void test_lookups(uint32_t numoflookups, uint32_t numofitems) {
  time_t stime = time(NULL);

  printf("Testing lookups [%u]...\n", numoflookups);

  while (numoflookups != 0) {
    uint32_t i;
    tuple_t t;
    char str[] = "Benchmarking Ninjas";

    i = rand() % numofitems;
    search(i, &t);

    // Verify
    assert(t.id == (uint32_t) i);
    assert(memcmp(&t.name, &str, 19) == 0);
    assert(t.name[19] == (char) i);
    numoflookups--;
  }

  printf("Lookups tested in %lu s.\n", (unsigned long) (time(NULL) - stime));
}
