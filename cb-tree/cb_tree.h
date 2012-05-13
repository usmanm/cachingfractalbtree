#ifndef CB_TREE_H
#define CB_TREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CB_FOUND_NONE (1)
#define CB_FOUND_RANGE (2)
#define CB_FOUND_EXACT (4)

#define CB_LEAF_TYPE_NULL (0)
#define CB_LEAF_TYPE_BLOCK (1)
#define CB_LEAF_TYPE_TUPLE (2)

#define CB_SLOT_TYPE_NULL (0)
#define CB_SLOT_TYPE_NODE (1)
#define CB_SLOT_TYPE_CACHE (2)

#define CB_BLOCK_TYPE_NULL (0)
#define CB_BLOCK_TYPE_ROOT (0xAA)
#define CB_BLOCK_TYPE_INNER (0xBB)
#define CB_BLOCK_TYPE_LEAF (0xCC)

typedef struct _cb_tuple cb_tuple;
typedef struct _cb_leaf cb_leaf;
typedef struct _cb_slot_h cb_slot_h;
typedef struct _cb_node_h cb_node_h;
typedef struct _cb_cache_h cb_cache_h;
typedef struct _cb_block_h cb_block_h;
typedef struct _cb_tree cb_tree;

/**
 * An index value
 */
typedef uint32_t cb_key;

/**
 * A data tuple
 */

struct _cb_tuple
{
	int32_t id;
	//char name[20];
	//char dob[8];

}
__attribute__((packed));

/**
 * A leaf value
 */

struct _cb_leaf
{
	// whether we point to another block or a tuple
	char type;
	size_t pos;
}
__attribute__((packed));


struct _cb_slot_h
{
	char type;
}
__attribute__((packed));

/**
 * A node slot
 */

struct _cb_node_h
{
	cb_slot_h slot;
	uint8_t keyc;
	cb_key keyv[0];
	// list of key values follows
}
__attribute__((packed));

/**
 * A cache slot
 */

struct _cb_cache_h
{
	cb_slot_h slot;
	uint8_t tuplec;
	cb_tuple tuplev[0];
	// list of cached tuples follows
}
__attribute__((packed));

/**
 * A block of slots
 */

struct _cb_block_h
{
	char type;
	size_t parent;

	char body[0];

	// list of slots follows
	// list of leaves follows
}
__attribute__((packed));


/**
 * The cached B+tree
 */
struct _cb_tree
{
	// the descriptor of the index file
	int index_fd;

	// the size of one big node, including a B+tree
	size_t block_size;

	// the size of a small node
	size_t slot_size;

	// number of blocks in memory, including deleted ones
	size_t blocks_alloc;
	
	// number of blocks currently used
	size_t blocks_used;

	// max number of slots in a block,
	// including slots used only as cache
	size_t block_slots;

	// number of nodes in a block
	size_t block_nodes;

	// number of leaves in a block
	size_t block_leaves;

	// max number of keys in a node
	size_t node_keys;

	// max number of tuples in a cache
	size_t cache_tuples;

	// number of children for each node in a block
	size_t block_bfactor;

	// number of tree levels in each block
	size_t block_height;

	// offset from beginning of nodes to beginning of leaves
	size_t block_leaves_off;

	// the root block
	cb_block_h *root;
}
__attribute__((packed));

void cb_init_tree(
		cb_tree *tree,
		const char *file,
		size_t block_size,
		size_t slot_size);

void cb_destr_tree(
		cb_tree *tree);

/*void cb_insert(
		cb_tree *tree,
		cb_key key,
		cb_tuple tuple);*/

#endif

