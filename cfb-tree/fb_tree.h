#ifndef CFB_TREE_H
#define CFB_TREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CFB_FOUND_NONE (1)
#define CFB_FOUND_RANGE (2)
#define CFB_FOUND_EXACT (4)

#define CFB_LEAF_TYPE_NULL (0)
#define CFB_LEAF_TYPE_BLOCK (2)
#define CFB_LEAF_TYPE_TUPLE (4)

#define CFB_SLOT_TYPE_NULL (0)
#define CFB_SLOT_TYPE_NODE (8)
#define CFB_SLOT_TYPE_CACHE (16)

#define CFB_BLOCK_TYPE_NULL (0)
#define CFB_BLOCK_TYPE_ROOT (32)
#define CFB_BLOCK_TYPE_INNER (64)
#define CFB_BLOCK_TYPE_LEAF (128)

typedef struct _fb_tuple fb_tuple;
typedef struct _fb_leaf fb_leaf;
typedef struct _fb_slot_h fb_slot_h;
typedef struct _fb_node_h fb_node_h;
typedef struct _fb_cache_h fb_cache_h;
typedef struct _fb_block_h fb_block_h;
typedef struct _fb_tree fb_tree;

/**
 * An index value
 */
typedef uint32_t fb_key;

/**
 * A data tuple
 */

struct _fb_tuple
{
	int32_t id;
	//char name[20];
	//char dob[8];

}
__attribute__((packed));

/**
 * A leaf value
 */

struct _fb_leaf
{
	// whether we point to another block or a tuple
	char type;
	size_t pos;
}
__attribute__((packed));


struct _fb_slot_h
{
	char type;
}
__attribute__((packed));

/**
 * A node slot
 */

struct _fb_node_h
{
	fb_slot_h slot;
	uint8_t keyc;
	fb_key keyv[0];
	// list of key values follows
}
__attribute__((packed));

/**
 * A cache slot
 */

struct _fb_cache_h
{
	fb_slot_h slot;
	uint8_t tuplec;
	fb_tuple tuplev[0];
	// list of cached tuples follows
}
__attribute__((packed));

/**
 * A block of slots
 */

struct _fb_block_h
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
struct _fb_tree
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
	fb_block_h *root;
}
__attribute__((packed));

/**
 * Initialize a tree, allocating its resources
 * @param[out] tree The tree being initialized
 * @param[in] file The file where to store the tree
 * @param[in] block_size The size of a block
 * @param[in] slot_size The size of a slot
 */
void fb_init_tree(
		fb_tree *tree,
		const char *file,
		size_t block_size,
		size_t slot_size);

/**
 * Destroy a tree, releasing all its resources
 * @param[in] tree The tree to be destroyed
 */
void fb_destr_tree(
		fb_tree *tree);

/**
 * Search for the file position of a tuple
 * @param[in] tree The tree to search
 * @param[in] key The key being searched for
 * @param[out] found Whether the key is in the tree
 * @param[out] value The position of the tuple for key 'key'
 */
void fb_get(
		fb_tree *tree,
		fb_key key,
		bool *found,
		size_t *value);

/*void fb_insert(
		fb_tree *tree,
		fb_key key,
		fb_tuple tuple);*/

#endif

