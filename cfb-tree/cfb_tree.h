#ifndef CFB_TREE_H
#define CFB_TREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CFB_VALUE_TYPE_NULL (0)
#define CFB_VALUE_TYPE_NODE (1)
#define CFB_VALUE_TYPE_BLOCK (2)
#define CFB_VALUE_TYPE_CNTNT (4)

#define CFB_SLOT_TYPE_CACHE (8)
#define CFB_SLOT_TYPE_NODE (16)

#define CFB_BLOCK_TYPE_INNER (0)
#define CFB_BLOCK_TYPE_ROOT (32)
#define CFB_BLOCK_TYPE_LEAF (128)

typedef struct _fb_val fb_val;
typedef struct _fb_tuple fb_tuple;
typedef struct _fb_slot_h fb_slot_h;
typedef struct _fb_inner_h fb_inner_h;
typedef struct _fb_leaf_h fb_leaf_h;
typedef struct _fb_cache_h fb_cache_h;
typedef struct _fb_block_h fb_block_h;
typedef struct _fb_tree fb_tree;

/**
 * An index / leaf value
 */
typedef uint32_t fb_key;
typedef uint32_t fb_pos;

struct _fb_val {
	uint8_t type;
	union {
		fb_pos node_pos;
		fb_pos block_pos;
		uint32_t value;
	};
}__attribute__((packed));


/**
 * A data tuple
 */

struct _fb_tuple
{
	uint32_t id;
	char name[20];
	uint32_t items[2];

}
__attribute__((packed));

/**
 * A slot
 */
struct _fb_slot_h
{
	uint8_t type;
	uint8_t cont;
	fb_pos parent;
	uint8_t body[0];
} __attribute__((packed));

/**
 * A block of slots
 */
struct _fb_block_h
{
	uint8_t type;
	uint8_t cont;
	fb_pos parent;
	fb_pos root;
	fb_pos height;

	char body[0];

	// list of slots follows
}
__attribute__((packed));


typedef struct _fb_block_data fb_block_data;
struct _fb_block_data
{
	fb_block_h *block;
	fb_pos pos;
	char *mptr;
	size_t off;
};

/**
 * The cached B+tree
 */
struct _fb_tree
{
	// the descriptor of the index file
	int index_fd;

	// the size of one big node
	size_t block_size;

	// the size of a small node
	size_t slot_size;

	// max number of slots in a block,
	// including slots used only as cache
	size_t block_slots;

	// number of usable nodes
	// does not include cache slots
	size_t block_nodes;

	// max number of tuples in a cache
	size_t cache_tuples;

	// max number of children for each node
	size_t bfactor;

	// max number of keys for each node
	size_t kfactor;

	// number of tree levels in each block
	size_t block_height;

	// number of items contained in the tree
	size_t content;

	// the number of allocated contiguous blocks
	// some may be unused when removed
	// suitable to decide where to store next block
	size_t blocks_alloc;

	// the root block
	fb_pos root;
}
__attribute__((packed));

/**
 * Initialize a tree, allocating its resources
 * @param[out] tree The tree being initialized
 * @param[in] file The file where to store the tree
 * @param[in] block_size The size of a block
 * @param[in] slot_size The size of a slot
 * @param[in] bfactor The branching factor of the tree
 */
void fb_init_tree(
		fb_tree *tree,
		const char *file,
		size_t block_size,
		size_t slot_size,
		size_t bfactor);

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
void fb_retrieve(
		fb_tree *tree,
		fb_key key,
		bool *exact,
		fb_val *result);

void _fb_retrieve(
		fb_tree *tree,
		fb_key key,
		bool *exact,
		fb_val *result,
		fb_pos *block_pos,
		fb_pos *node_pos);

/**
 * Insert a new value in the tree
 * @param[in] tree The tree to which we are adding the value
 * @param[in] key The key to insert
 * @param[in] value The value corresponding to the key
 */
void fb_insert(
		fb_tree *tree,
		fb_key key,
		fb_val value);

void _fb_insert(
		fb_tree *tree,
		fb_key key,
		fb_val value,
		bool exact,
		fb_pos block_pos,
		fb_pos node_pos);
/**
 * Try to add an entry to a block cache
 * @param[in] tree The tree to use
 * @param[in] block_pos The block whose cache to access
 * @param[in] key The key to try to insert
 * @param[in] tuple The value corresponding to the key
 */
void fb_cache_add(
		fb_tree *tree,
		fb_pos block_pos,
		fb_key key,
		fb_tuple *tuple);

/**
 * Check whether an entry is cached
 * @param[in] tree The tree to use
 * @param[in] block_pos The block whose cache to access
 * @param[in] key The key to probe
 * @param[out] tuple The value corresponding to the key, if found
 * @return True if the value was found and tuple was set
 */
bool fb_cache_probe(
		fb_tree *tree,
		fb_pos block_pos,
		fb_key key,
		fb_tuple *tuple);

/**
 * Try to replace an existing cache entry on tree insertion
 * @param[in] tree The tree to use
 * @param[in] block_pos The block whose cache to access
 * @param[in] key The key to try to replace, if cached
 * @param[in] tuple The new value to assign to the key
 */
void fb_cache_replace(
		fb_tree *tree,
		fb_pos block_pos,
		fb_key key,
		fb_tuple *tuple);

#endif

