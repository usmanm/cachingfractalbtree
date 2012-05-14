#ifndef B_TREE_H
#define B_TREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define BTREEFILE "btree.db" // B+ tree file name
#define HEAPFILE "table.db" // Data file name
#define NODESIZE 4096 // Node size

// Types of node
#define INNER 0
#define LEAF 1

// Max # of keys in one node
#define MAXKEYS ((NODESIZE - sizeof(char) - sizeof(uint32_t) - 2*sizeof(pointer_t)) / (sizeof(key_t) + sizeof(pointer_t))) 

// Key
typedef uint32_t key_t;

// Pointer 
typedef uint32_t pointer_t;

// Tuple
struct _tuple_t {
  int32_t id;
  char name[20];
  char dob[8];
}
__attribute__((packed));
typedef struct _tuple_t tuple_t;

// B+ Tree Node
struct _node_t {
  char type; // inner or leaf?
  pointer_t offset;
  uint32_t num_keys;
  key_t keys[MAXKEYS];
  pointer_t values[MAXKEYS + 1];
}
__attribute__((packed));
typedef struct _node_t node_t;

// B+ Tree Metadata
struct _btree_t {
  uint32_t num_items;
  pointer_t root;
}
__attribute__((packed));
typedef struct _btree_t btree_t;

// Load/save b+ tree metadata
void load_btree(btree_t *btree);
void save_btree(btree_t *btree);

// Insert ops
int insert(key_t key, tuple_t *tuple);
int btree_insert(key_t key, pointer_t value);
pointer_t heap_insert(tuple_t *t);

// Search ops
int search(key_t key, tuple_t *t);
pointer_t btree_search(key_t key);
void heap_search(pointer_t offset, tuple_t *t);

#endif

