#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "b_tree.h"
#include "benchmark.h"

#define NUMITEMS 10000
#define NUMLOOKUPS 2000

#define TEST 0
#define BENCHMARK 1

void test_heap() {
  int i;

  remove("table.db");

  printf("Testing HeapFile...\n");

  for (i = 1; i < 10; i++) {
    tuple_t t;
    pointer_t p;

    t.id = i;
    strncpy((char *) &t.name, "Usman Masood Tester", 19);
    t.name[19] = i;
    p = heap_insert(&t);
    assert(p == ((i-1) * sizeof(tuple_t)));
  }

  for (i = 1; i < 10; i++) {
    tuple_t t;
    char str[] = "Usman Masood Tester";
    heap_search(sizeof(tuple_t) * (i-1), &t);
    assert(t.id == (uint32_t) i);
    assert(t.name[19] == i);
    assert(memcmp(&t.name, &str, 19) == 0);
  }

  printf("HeapFile tests successful!\n");
}

void test_bt_no_split() {
  int i;

  remove("btree.db");

  printf("Testing B+ Tree (No split)...\n");

  for (i = 51; i <= 100; i++) {
    btree_insert(i, i);
  }

  for (i = 50; i > 0; i--) {
    btree_insert(i, i);
  }

  for (i = 1; i <= 100; i++) {
    assert(i == (int) btree_search(i));
  }

  printf("B+ Tree (No split) tests successful!\n");
}

void test_bt_split() {
  int i;

  remove("btree.db");

  printf("Testing B+ Tree (Split)...\n");

  for (i = 20000; i >= 1; i--) {
    if ((i % 3) != 0) continue;
    btree_insert(i, i);
  }

  for (i = 1; i <= 20000; i++) {
    if ((i % 3) == 0) continue;
    btree_insert(i, i);
  }

  for (i = 1; i <= 20000; i++) {
    assert(i == (int) btree_search(i));
  }

  // Check updates as well!
  for (i = 1; i <= 20000; i++) {
    if ((i % 3) == 0) continue;
    btree_insert(i, i + 1);
  }

  for (i = 1; i <= 20000; i++) {
    if ((i % 3) == 0) {
      assert(i == (int) btree_search(i));
    }
    else {
      assert((i + 1) == (int) btree_search(i));
    }
  }

  printf("B+ Tree (Split) tests successful!\n");
}

int main() {
  if (TEST) {
    test_heap();
    test_bt_no_split();
    test_bt_split();
  }

  if (BENCHMARK) {
    init_benchmark();
    load_tables(NUMITEMS);
    //    test_range(NUMLOOKUPS, NUMITEMS);
    test_lookups(NUMLOOKUPS, NUMITEMS);
  }

  return 0;
}
