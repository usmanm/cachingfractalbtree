#include "fb_tree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

static void _fb_init_block(fb_tree *tree, fb_block_h* block, char type, size_t parent);

void fb_init_tree(
		fb_tree *tree,
		const char *file,
		size_t block_size,
		size_t slot_size)
{
	assert(slot_size >= sizeof(fb_node_h));
	assert(slot_size >= sizeof(fb_cache_h));
	assert(block_size >= sizeof(fb_block_h));

	tree->block_size = block_size;
	tree->slot_size = slot_size;

	size_t node_avail_body = slot_size - sizeof(fb_node_h);
	size_t node_keys = node_avail_body / sizeof(fb_key);
	size_t max_branching_factor = node_keys + 1 < UINT8_MAX ? node_keys + 1 : UINT8_MAX;

	size_t cache_avail_body = slot_size - sizeof(fb_node_h);
	size_t cache_keys = cache_avail_body / sizeof(fb_tuple);

	assert(node_keys > 0);
	assert(cache_keys > 0);
	tree->node_keys = node_keys;
	tree->cache_tuples = cache_keys;

	// empirically find the branching factor leading to least waste
	// space is @f$\sum_{i=0}^h b^i \cdot \text{ (slot_size) } + b^{h+1} \cdot \text{ (leaf_size) }$
	size_t best_b = 2;
	size_t best_h = 1;
	size_t best_req = 0;
	size_t best_used = 0;
	size_t best_nodes = 0;
	size_t best_leaves = 0;
	// set a brancing factor
	for (size_t b = 2; b <= max_branching_factor; ++b)
	{
		// set a height
		size_t req_bytes = 0;
		size_t used_bytes = 0;
		size_t num_nodes = 0;
		size_t num_leaves = 0;
		for (size_t h = 1; used_bytes <= block_size; ++h)
		{
			// add space for the slots
			req_bytes = sizeof(fb_block_h);
			used_bytes = sizeof(fb_block_h);
			for (size_t i = 0; i <= h; ++i)
			{
				req_bytes += pow(b, i) * slot_size;
				used_bytes += pow(b, i) * (sizeof(fb_node_h) + sizeof(fb_key)*(b - 1));
				num_nodes += pow(b, i);
			}
			// add space for the leaves
			req_bytes += pow(b, h + 1) * sizeof(fb_leaf);
			used_bytes += pow(b, h + 1) * sizeof(fb_leaf);
			num_leaves = pow(b, h + 1);

			//printf("b, h, req, used, density: %zu %zu %zu %zu %f\n",
			//		b, h, req_bytes, used_bytes, used_bytes / (float)block_size);
			if (req_bytes <= block_size)
			{
				// this (b,h) pair is a candidate
				if (used_bytes > best_used)
				{
					best_b = b;
					best_h = h;
					best_req = req_bytes;
					best_used = used_bytes;
					best_nodes = num_nodes;
					best_leaves = num_leaves;
				}
			}
		}
	}

	tree->block_bfactor = best_b;
	tree->block_height = best_h;
	tree->block_nodes = best_nodes;
	tree->block_leaves = best_leaves;
	tree->block_leaves_off = block_size - best_leaves * sizeof(fb_leaf) - sizeof (fb_block_h);
	tree->block_slots = tree->block_leaves_off / slot_size;
	tree->blocks_alloc = 1;
	tree->blocks_used = 1;

	tree->index_fd = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(tree->index_fd != -1);

	ftruncate(tree->index_fd, block_size);
	tree->root = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, tree->index_fd, 0);
	_fb_init_block(tree, tree->root, CFB_BLOCK_TYPE_ROOT | CFB_BLOCK_TYPE_LEAF, 0);

	float usage_density = best_used / (float)block_size;
	printf("index_fd %i\n"
			"block_size %zu\n"
			"slot_size %zu\n"
			"blocks_alloc %zu\n"
			"blocks_used %zu\n"
			"block_slots %zu\n"
			"block_nodes %zu\n"
			"block_leaves %zu\n"
			"node_keys %zu\n"
			"cache_tuples %zu\n"
			"block_bfactor %zu\n"
			"block_height %zu\n"
			"block_leaves_off %zu\n"
			"%% used bytes %zu\n"
			"%% required bytes %zu\n"
			"%% density %f\n",
			tree->index_fd,
			tree->block_size,
			tree->slot_size,
			tree->blocks_alloc,
			tree->blocks_used,
			tree->block_slots,
			tree->block_nodes,
			tree->block_leaves,
			tree->node_keys,
			tree->cache_tuples,
			tree->block_bfactor,
			tree->block_height,
			tree->block_leaves_off,
			best_used,
			best_req,
			usage_density);
	
}

void fb_destr_tree(fb_tree *tree)
{
	munmap(tree->root, tree->block_size);
	close(tree->index_fd);
}

static void _fb_init_block(fb_tree *tree, fb_block_h* block, char type, size_t parent)
{
	block->type = type;
	block->parent = parent;

	for (size_t s = 0; s < tree->block_slots; ++s)
	{
		fb_cache_h *cache = (fb_cache_h *)(block->body + s * tree->slot_size);
		cache->slot.type = CFB_SLOT_TYPE_CACHE;
		cache->tuplec = 0;
	}

	fb_leaf *leaf = (fb_leaf *)(block->body + tree->block_leaves_off);
	for (size_t l = 0; l < tree->block_leaves; ++l)
	{
		leaf->type = CFB_LEAF_TYPE_NULL;
		leaf->pos = 0;
		++leaf;
	}
}

/**
 * @param tree The tree to query
 * @param block The block in the tree to query
 * @param node_pos The position of the node to search in
 * @param key The key being searched
 * @param leaf_pos Which child of the node is the leaf
 * @param status Whether we found the exact item, the range containing it, or none
 */
static inline size_t _fb_search_node(
		fb_tree *tree,
		fb_block_h *block,
		size_t node_pos,
		fb_key key,
		size_t *leaf_pos,
		char *status)
{
	fb_node_h *node = (fb_node_h *)(block->body + node_pos * tree->slot_size);
	
	if (node->keyc == 0)
	{
		// this should never happen
		// if a slot is marked a node, it must have at least one key
		assert(false);
	}

	for (uint8_t i = 0; i < node->keyc; ++i)
	{
		if (key <= node->keyv[i])
		{
			*leaf_pos = i;
			*status = key == node->keyv[i] ? CFB_FOUND_EXACT : CFB_FOUND_RANGE;
			return tree->block_bfactor * node_pos + 1 + i;
		}
	}
	
	*status = CFB_FOUND_RANGE;
	return tree->block_bfactor * node_pos + 1 + node->keyc;
}

/**
 * @param tree The tree to search in
 * @param block The block in the tree to search in
 * @param key The key being searched
 * @param node_pos The node that owns the leaf
 * @param leaf The leaf found by the search
 * @param status Whether we found the exact item or the range that should contain it
 */
static inline void _fb_search_block(
		fb_tree *tree,
		fb_block_h *block,
		fb_key key,
		size_t *node_pos,
		fb_leaf *leaf,
		size_t *leaf_pos,
		char *status)
{
	size_t node_pos_curr = 0;
	size_t node_pos_old = 0;
	*status = CFB_FOUND_NONE;

	while (node_pos_curr < tree->block_nodes)
	{
		fb_slot_h *slot = (fb_slot_h *)(block->body + node_pos_curr * tree->slot_size);
		if (slot->type == CFB_SLOT_TYPE_NODE)
		{
			// choose which child node to access
			node_pos_old = node_pos_curr;
			node_pos_curr = _fb_search_node(tree, block, node_pos_curr, key, leaf_pos, status);
		}
		else
		{
			// empty path (cache or uninitialized)
			// just take the first virtual child
			// status remains as per the last comparison
			node_pos_curr = tree->block_bfactor * node_pos_curr + 1;
		}
	}
	*node_pos = node_pos_old;
	*leaf_pos = node_pos_curr - tree->block_nodes;
	*leaf = *((fb_leaf *)(block->body + tree->block_leaves_off) + *leaf_pos);
}

void _fb_get(
		fb_tree *tree,
		fb_key key,
		bool *found,
		size_t *tuple_pos,
		size_t *block_pos,
		size_t *node_pos,
		size_t *leaf_pos,
		char *status)
{
	fb_block_h *block = tree->root;
	*status = CFB_FOUND_NONE;
	fb_leaf leaf;

	long page_size = sysconf(_SC_PAGESIZE);
	*block_pos = 0;
	_fb_search_block(tree, block, key, node_pos, &leaf, leaf_pos, status);
	
	while (leaf.type == CFB_LEAF_TYPE_BLOCK)
	{
		*block_pos = leaf.pos;
		size_t offset = (*block_pos / page_size) * page_size;
		char *mptr = mmap(NULL,
				tree->block_size,
				PROT_READ,
				MAP_PRIVATE,
				tree->index_fd,
				offset);
		assert(mptr != MAP_FAILED);
		
		block = (fb_block_h *)(mptr + *block_pos - offset);
		_fb_search_block(tree, block, key, node_pos, &leaf, leaf_pos, status);

		munmap(mptr, tree->block_size);
	}

	*found = false;
	if (leaf.type == CFB_LEAF_TYPE_TUPLE && *status == CFB_FOUND_EXACT)
	{
		*found = true;
		*tuple_pos = leaf.pos;
	}
}

void fb_get(
		fb_tree *tree,
		fb_key key,
		bool *found,
		size_t *tuple_pos)
{
	size_t block_pos;
	size_t node_pos;
	size_t leaf_pos;
	char status;
	_fb_get(tree, key, found, tuple_pos, &block_pos, &node_pos, &leaf_pos, &status);
}

static inline void _fb_insert_node(
		fb_tree *tree,
		fb_block_h *block,
		size_t node_pos,
		fb_key key)
{
	fb_node_h *node = (fb_node_h *)(block->body + node_pos * tree->slot_size);
	if (node->keyc >= tree->block_nodes - 1)
	{
		// there is no room for an insert
		// or the insert should be a split
		// should never reach this
		assert(false);
	}

	// place the new key in order
	fb_key last = key;
	fb_key tmp;
	size_t pos = SIZE_MAX;
	for (size_t i = 0; i <= node->keyc; ++i)
	{
		if (node->keyv[i] > key)
		{
			pos = pos < i ? pos : i;
			tmp = node->keyv[i];
			node->keyv[i] = last;
			last = tmp;
		}
	}

	++node->keyc;
}

static inline void _fb_insert_leaf(
		fb_tree *tree,
		fb_leaf *target,
		size_t insert_pos,
		size_t value)
{
	fb_leaf buff;
	fb_leaf next;
	next.type = CFB_LEAF_TYPE_TUPLE;
	next.pos = tuple_pos;
	
	for (size_t i = 0; i < tree->block_bfactor - insert_pos - 1; ++i)
	{
		buff = target[i];
		target[i] = next;
		next = buff;
	}
}

static inline bool _fb_node_needs_split(fb_tree *tree, fb_block_h *block, size_t node_pos)
{
	fb_node_h *node = (fb_node_h *)(block->body + node_pos * tree->slot_size);
	return node->keyc >= tree->node_keys - 1 ? true : false;
}

void fb_insert(
		fb_tree *tree,
		fb_key key,
		size_t value)
{
	bool found;
	size_t old_tuple_pos;
	size_t old_block_pos;
	size_t old_node_pos;
	size_t old_leaf_pos;
	char status;

	long page_size = sysconf(_SC_PAGESIZE);
	char *mptr = NULL;
	
	_fb_get(tree, key, &found, &old_tuple_pos, &old_block_pos, &old_node_pos, &old_leaf_pos, &status);
	
	fb_block_h *block = tree->root;
	if (old_block_pos != 0) // open the block owning the leaf
	{
		size_t offset = (old_block_pos / page_size) * page_size;
		mptr = mmap(NULL,
			tree->block_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			tree->index_fd,
			offset);
		assert(mptr != MAP_FAILED);
		block = (fb_block_h *)(mptr + old_block_pos - offset);
	}

	fb_leaf *target = ((fb_leaf *)(block->body + tree->block_leaves_off)) + old_leaf_pos;
	if (found) // replace an existing leaf
	{
		target->pos = value;
		return;
	}
	else // insert an item that was not present
	{
		_fb_node_needs_split(tree, block, old_node_pos);
		_fb_insert_node(tree, block, old_node_pos, key);
		_fb_insert_leaf(tree, target, old_leaf_pos, value);
	}
}

/*void fb_remove(
		fb_tree *tree,
		fb_key key)
{
}*/
