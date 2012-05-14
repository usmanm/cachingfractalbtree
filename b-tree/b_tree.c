#include "b_tree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEBUG 0
#define DEBUGX 0

void load_btree(btree_t *bt) {
  size_t size;
  FILE *fp;

  fp = fopen(BTREEFILE, "r");
  size = fread(bt, 1, sizeof(btree_t), fp);
  fclose(fp);

  if (DEBUG) printf("Loading btree_t [%d, %d]...\n", bt->num_items, bt->root);
  assert(size == sizeof(btree_t));
}

void save_btree(btree_t *bt) {
  if (DEBUG) printf("Saving btree_t [%d]...\n", bt->num_items);
  size_t size;
  int fp;

  fp = open(BTREEFILE, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
  size = write(fp, bt, sizeof(btree_t));
  close(fp);

  assert(size == sizeof(btree_t));
}

int insert(key_t key, tuple_t *tuple) {
  pointer_t p = heap_insert(tuple);
  return btree_insert(key, p);
}

void new_node(node_t *n, char type) {
  if (DEBUG) printf("New node_t (%d) [%d]...\n", sizeof(node_t), type);
  struct stat st;
  FILE *fp;
  size_t size;

  stat(BTREEFILE, &st);

  n->type = type;
  n->num_keys = 0;
  n->offset = (pointer_t) st.st_size;

  memset((void *) n->keys, 0, sizeof(key_t) * MAXKEYS);
  memset((void *) n->values, 0, sizeof(pointer_t) * (MAXKEYS + 1));

  fp = fopen(BTREEFILE, "a+");
  size = fwrite(n, 1, sizeof(node_t), fp);

  assert(size == sizeof(node_t));

  // Put garbage in the end
  fwrite(n, 1, NODESIZE - sizeof(node_t), fp);

  fclose(fp);
}

void flush_node(node_t *n) {
  if (DEBUG) printf("Flush node_t...[%d, %d]\n", n->offset, n->num_keys);
  int fp;
  size_t size;

  fp = open(BTREEFILE, O_WRONLY);
  lseek(fp, n->offset, SEEK_SET);
  size = write(fp, n, sizeof(node_t));
  close(fp);

  if (DEBUGX) {
    printf("FN [%d]: ", n->offset);
    uint32_t j = 0;
    for (j = 0; j < MAXKEYS; j++) {
      printf("(%d, %d) ", n->keys[j], n->values[j]);
    }
    printf("(-, %d)\n", n->values[MAXKEYS]);
  }

  assert(size == sizeof(node_t));
}

void get_node(node_t *n, pointer_t offset) {
  if (DEBUG) printf("Get node_t...[%d]\n", offset);
  struct stat st;
  FILE *fp;
  size_t size;

  stat(BTREEFILE, &st);
  size = st.st_size;

  assert(size  >= (offset + sizeof(node_t)));

  fp = fopen(BTREEFILE, "r");
  fseek(fp, offset, SEEK_SET);
  size = fread(n, 1, sizeof(node_t), fp);
  fclose(fp);

  if (DEBUGX) {
    printf("GN [%d]: ", n->offset);
    uint32_t j = 0;
    for (j = 0; j < MAXKEYS; j++) {
      printf("(%d, %d) ", n->keys[j], n->values[j]);
    }
    printf("(-, %d)\n", n->values[MAXKEYS]);
  }

  assert(size == sizeof(node_t));
}

// Find slot that fits key (linear)
uint32_t find_slot(key_t key, node_t *n, int exact) {
  uint32_t i = 0;
  key_t prev;

  for (i = 0; i <= n->num_keys; i++) {
    // Last slot
    if (i == n->num_keys) break;

    if (i != 0) {
      if (DEBUG) {
	if (n->keys[i] <= prev) {
	  printf("Oops... {%d, %d, %d}\n", prev, n->keys[i], n->num_keys);
	}
      }
      assert(n->keys[i] > prev); // assert sorted!
    }

    prev = n->keys[i];

    // Get first slot whose key is >= mine
    if (exact) {
      if (key == n->keys[i]) {
	break;
      }
    }
    else {
      if (key < n->keys[i]) {
	break;
      }
    }
  }

  if (DEBUG) printf("find_slot(%d) => {%d, %d}\n", key, i, n->num_keys);
  return i;
}

// Only for non-full leaf nodes
void insert_to_leaf(node_t *node, key_t key, pointer_t value) {
  uint32_t n;
  if (DEBUG) printf("Insert to leaf... [%d, %d]...\n", key, value);
  n = find_slot(key, node, 0);
  memmove(&node->keys[n+1], &node->keys[n], 
	  (MAXKEYS-n-1) * sizeof(key_t));
  memmove(&node->values[n+1], &node->values[n], 
	  (MAXKEYS-n-1) * sizeof(pointer_t));
  node->keys[n] = key;
  node->values[n] = value;
  node->num_keys++;
}

// Only for non-full leaf nodes
void insert_to_node_left(node_t *node, key_t key, pointer_t value) {
  assert(node->type == INNER);
  if (DEBUG) printf("Insert to inner node... [%d, %d]...\n", key, value);
  uint32_t n;
  n = find_slot(key, node, 0);
  memmove(&node->keys[n+1], &node->keys[n], 
	  (MAXKEYS-n-1) * sizeof(key_t));
  memmove(&node->values[n+2], &node->values[n+1], 
	  (MAXKEYS-n-1) * sizeof(pointer_t));
  node->keys[n] = key;
  node->values[n+1] = value;
  node->num_keys++;
}

pointer_t split_node(node_t *node, node_t **path, int i, key_t key, 
		     pointer_t value) {
  assert(node->num_keys == MAXKEYS);
  assert(path[i] == node);

  if (DEBUG) {
    printf("Splitting node [%d, %d, %d]...\n", node->offset, 
	   node->num_keys, i);
  }

  pointer_t r = 0;
  key_t k;
  pointer_t p;

  node_t *succ = (node_t *) malloc(sizeof(node_t));

  new_node(succ, node->type);

  if (node->type == LEAF) {
    memcpy(&succ->keys[0], &node->keys[MAXKEYS/2], 
	   (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
    memset(&node->keys[MAXKEYS/2], 0, 
	   (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
    memcpy(&succ->values[0], &node->values[MAXKEYS/2], 
	   (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
    memset(&node->values[MAXKEYS/2], 0, 
	   (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));

    node->num_keys = MAXKEYS/2;
    succ->num_keys = MAXKEYS/2 + (MAXKEYS % 2);

    // Insert to node/succ
    if (key >= succ->keys[0]) {
      insert_to_leaf(succ, key, value);
    }
    else {
      insert_to_leaf(node, key, value);
    }

    k = succ->keys[0];
    p = succ->offset;
  }
  else { // inner node
    int direction = 0;
    key_t median = key;

    get_node(node, node->offset);

    if (key > node->keys[MAXKEYS/2]) {
      direction = 1;
      median = node->keys[MAXKEYS/2];
    }
    else if (key < node->keys[MAXKEYS/2 - 1]) {
      direction = -1;
      median = node->keys[MAXKEYS/2 - 1];
    }

    if (direction == 0) {
      if (DEBUG) {
	printf("Case 0 Median, K, V =  %d, %d, %d\n", median, key, value);
      }
      memcpy(&succ->keys[0], &node->keys[MAXKEYS/2], 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      memset(&node->keys[MAXKEYS/2], 0, 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      memcpy(&succ->values[1], &node->values[MAXKEYS/2 + 1], 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      memset(&node->values[MAXKEYS/2 + 1], 0, 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      succ->values[0] = value;
      node->num_keys = MAXKEYS/2;
      succ->num_keys = MAXKEYS/2 + (MAXKEYS % 2);
    }
    else if (direction == 1) {
      if (DEBUG) {
	printf("Case 1 Median, K, V =  %d, %d, %d\n", median, key, value);
      }
      memcpy(&succ->keys[0], &node->keys[MAXKEYS/2 + 1], 
	     (MAXKEYS/2 + (MAXKEYS % 2) - 1) * sizeof(key_t));
      memset(&node->keys[MAXKEYS/2], 0, 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      memcpy(&succ->values[0], &node->values[MAXKEYS/2 + 1], 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      memset(&node->values[MAXKEYS/2 + 1], 0, 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      node->num_keys = MAXKEYS/2;
      succ->num_keys = MAXKEYS/2 + (MAXKEYS % 2) - 1;
      insert_to_node_left(succ, key, value);
    }
    else {
      if (DEBUG) {
	printf("Case -1 Median, K, V =  %d, %d, %d\n", median, key, value);
      }
      memcpy(&succ->keys[0], &node->keys[MAXKEYS/2], 
	     (MAXKEYS/2 + (MAXKEYS % 2)) * sizeof(key_t));
      memset(&node->keys[MAXKEYS/2 - 1], 0, 
	     (MAXKEYS/2 + (MAXKEYS % 2) + 1) * sizeof(key_t));
      memcpy(&succ->values[0], &node->values[MAXKEYS/2], 
	     (MAXKEYS/2 + (MAXKEYS % 2) + 1) * sizeof(key_t));
      memset(&node->values[MAXKEYS/2], 0, 
	     (MAXKEYS/2 + (MAXKEYS % 2) + 1) * sizeof(key_t));

      if (DEBUGX) {
	printf("GN [%d]: ", node->offset);
	uint32_t j = 0;
	for (j = 0; j < MAXKEYS; j++) {
	  printf("(%d, %d) ", node->keys[j], node->values[j]);
	}
	printf("(-, %d)\n", node->values[MAXKEYS]);
      }

      if (DEBUGX) {
	printf("GN [%d]: ", succ->offset);
	uint32_t j = 0;
	for (j = 0; j < MAXKEYS; j++) {
	  printf("(%d, %d) ", succ->keys[j], succ->values[j]);
	}
	printf("(-, %d)\n", succ->values[MAXKEYS]);
      }
      node->num_keys = MAXKEYS/2 - 1;
      succ->num_keys = MAXKEYS/2 + (MAXKEYS % 2);      
      insert_to_node_left(node, key, value);
    }

    k = median;
    p = succ->offset;
  }

  flush_node(node);
  flush_node(succ);
  free(succ);

  i--;
  if (i == -1) { // new root!
    node_t *root = (node_t *) malloc(sizeof(node_t));
    new_node(root, INNER);
    if (DEBUG) printf("New root time! %d...\n", root->offset);
    r = root->offset;
    root->num_keys = 1;
    root->keys[0] = k;
    root->values[0] = path[0]->offset;
    root->values[1] = p;
    flush_node(root);
    free(root);
  }
  else {
    node_t *parent = path[i];    
    if (parent->num_keys < MAXKEYS) {
      insert_to_node_left(parent, k, p);
      flush_node(parent);
    }
    else { // split parent
      r = split_node(parent, path, i, k, p);
    }
  }

  return r;
}

int btree_insert(key_t key, pointer_t value) {
  struct stat st;
  btree_t *bt;
  node_t *root, *node;
  uint32_t depth, n, i;
  pointer_t r;

  bt = (btree_t *) malloc(sizeof(btree_t));

  if (stat(BTREEFILE, &st) != 0) {
    if (DEBUG) printf("Creating new btree_t...\n");
    bt->num_items = 0;
    bt->root = sizeof(btree_t);
    save_btree(bt);
  }
  else {
    load_btree(bt);
  }

  root = (node_t *) malloc(sizeof(node_t));

  // Create new root
  if (bt->num_items == 0) {
    new_node(root, LEAF);
    flush_node(root);
  }
  else {
    get_node(root, bt->root);
  }

  n = bt->num_items;
  depth = 1;
  while ((n = n/(MAXKEYS/2)) != 0) {
    depth++;
  }

  node_t *path[depth + 2]; // HACK to store path from root
  node = root;
  path[0] = root;
  i = 0;

  // Get leaf to add key, value to
  while (node->type != LEAF) {
    i++;
    pointer_t p = node->values[find_slot(key, node, 0)];
    node = (node_t *) malloc(sizeof(node_t));
    get_node(node, p);
    path[i] = node;
  }

  n = find_slot(key, node, 0);

  // Update case
  if (n > 0 && node->keys[n-1] == key) {
    node->values[n-1] = value;
    flush_node(node);
    goto end;
  }

  bt->num_items++;

  // No split case
  if (node->num_keys < MAXKEYS) {
    insert_to_leaf(node, key, value);
    flush_node(node);
    goto end;
  }

  // Split leaf (also inserts kv as side effect)
  r = split_node(node, path, i, key, value);

  if (r != 0) {
    bt->root = r;
  }

 end:
  save_btree(bt);

  for (; (int) i > -1; i--) {
    free(path[i]);
  } 

  free(bt);
  return 0;
}

pointer_t heap_insert(tuple_t *t) {
  struct stat st;
  pointer_t p;
  FILE *fp;
  size_t size;

  if (stat(HEAPFILE, &st) == 0) {
    p = st.st_size;
  }
  else {
    p = 0;
  }

  fp = fopen(HEAPFILE, "a+");
  size = fwrite(t, 1, sizeof(tuple_t), fp);
  fclose(fp);

  assert(size == sizeof(tuple_t)); // must write entire tuple

  return p;
}

int search(key_t key, tuple_t *t) {
  pointer_t p = btree_search(key);
  if ((int32_t) p == -1) return -1;
  heap_search(p, t);
  return 0;
}

pointer_t btree_search(key_t key) {
  btree_t *bt;
  node_t *node;
  uint32_t n;

  bt = (btree_t *) malloc(sizeof(btree_t));
  load_btree(bt);

  node = (node_t *) malloc(sizeof(node_t));
  get_node(node, bt->root);

  // Get leaf which might have key
  while (node->type != LEAF) {
    pointer_t p = node->values[find_slot(key, node, 0)];
    get_node(node, p);
  }

  n = find_slot(key, node, 1); // exact

  // Not found!
  if (n == node->num_keys) {
    return -1;
  }
  
  return node->values[n];
}

void heap_search(pointer_t offset, tuple_t *t) {
  struct stat st;
  FILE *fp;
  size_t size;

  stat(HEAPFILE, &st);
  size = st.st_size;

  assert(((uintptr_t) offset % sizeof(tuple_t)) == 0); // offset is aligned?
  assert(size >= offset + sizeof(tuple_t)); // offset must be within file

  fp = fopen(HEAPFILE, "r");
  fseek(fp, offset, SEEK_SET);
  size = fread(t, 1, sizeof(tuple_t), fp);
  fclose(fp);
}
