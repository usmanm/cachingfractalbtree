#include "cb_tree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

void cb_init_tree(
		cb_tree *tree,
		const char *file,
		size_t block_size,
		size_t slot_size)
{
	assert(slot_size >= sizeof(cb_node_h));
	assert(slot_size >= sizeof(cb_cache_h));
	assert(block_size >= sizeof(cb_block_h));

	tree->block_size = block_size;
	tree->slot_size = slot_size;

	size_t node_avail_body = slot_size - sizeof(cb_node_h);
	size_t node_keys = node_avail_body / sizeof(cb_key);
	size_t max_branching_factor = node_keys + 1 < UINT8_MAX ? node_keys + 1 : UINT8_MAX;

	size_t cache_avail_body = slot_size - sizeof(cb_node_h);
	size_t cache_keys = cache_avail_body / sizeof(cb_tuple);

	tree->cache_tuples = cache_keys;
	assert(cache_keys > 0);

	// empirically find the branching factor leading to least waste
	// space is \sum_{i=0}^h b^i \cdot (slot_size) + b^{h+1} \cdot (leaf_size)
	size_t best_b = 2;
	size_t best_h = 1;
	size_t best_req = 0;
	size_t best_used = 0;
	size_t best_slots = 0;
	size_t best_leaves = 0;
	// set a brancing factor
	for (size_t b = 2; b <= max_branching_factor; ++b)
	{
		// set a height
		size_t req_bytes = 0;
		size_t used_bytes = 0;
		size_t num_slots = 0;
		size_t num_leaves = 0;
		for (size_t h = 1; used_bytes <= block_size; ++h)
		{
			// add space for the slots
			req_bytes = sizeof(cb_block_h);
			used_bytes = sizeof(cb_block_h);
			for (size_t i = 0; i <= h; ++i)
			{
				req_bytes += pow(b, i) * slot_size;
				used_bytes += pow(b, i) * (sizeof(cb_node_h) + sizeof(cb_key)*(b - 1));
				num_slots += pow(b, i);
			}
			// add space for the leaves
			req_bytes += pow(b, h + 1) * sizeof(cb_leaf);
			used_bytes += pow(b, h + 1) * sizeof(cb_leaf);
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
					best_slots = num_slots;
					best_leaves = num_leaves;
				}
			}
		}
	}
	float usage_density = best_used / (float)block_size;
	printf("chosen branch_factor, height, slots, leaves, req, used, density: %zu %zu %zu %zu %zu %zu %f\n",
			best_b, best_h, best_slots, best_leaves, best_req, best_used, usage_density);

	tree->block_bfactor = best_b;
	tree->block_height = best_h;
	tree->block_slots = best_slots;

	tree->index_fd = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(tree->index_fd != -1);

	ftruncate(tree->index_fd, block_size);
	tree->root = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, tree->index_fd, 0);

	tree->root->parent = NULL;
	
}

void cb_destr_tree(cb_tree *tree)
{
	munmap(tree->root, tree->block_size);
	close(tree->index_fd);
}

/**
 * @param tree The tree to search in
 * @param block The block in the tree to search in
 * @param node_pos The position of the node to search in
 * @param key The key being searched
 * @param status Whether we found the exact item, the range containing it, or none
 */
static inline size_t _cb_search_node(
		cb_tree *tree,
		cb_block_h *block,
		size_t node_pos,
		cb_key key,
		char *status)
{
	cb_node_h *node = (cb_node_h *)(block->body + node_pos * tree->slot_size);
	for (uint8_t i = 0; i < node->keyc; ++i)
	{
		if (key <= node->keyv[i])
		{
			*status = key == node->keyv[i] ? CB_FOUND_EXACT : CB_FOUND_RANGE;
			return tree->block_bfactor * node_pos + 1 + i;
		}
	}
	
	if (node->keyc == 0)
	{
		*status = CB_FOUND_NONE;
		return 0;
	}
	else
	{
		*status = CB_FOUND_RANGE;
		return tree->block_bfactor * node_pos + 1 + node->keyc;
	}
}

/**
 * @param tree The tree to search in
 * @param block The block in the tree to search in
 * @param key The key being searched
 * @param res Will store a pointer to the result here
 * @param status Whether we found the exact item, the range containing it, or none
 */
static inline void _cb_search_block(
		cb_tree *tree,
		cb_block_h *block,
		cb_key key,
		cb_leaf **res,
		char *status)
{
	size_t node_pos = 0;
	*status = CB_FOUND_NONE;
	while (node_pos < tree->block_nodes)
	{
		cb_slot_h *slot = (cb_slot_h *)(block->body + node_pos * tree->slot_size);
		if (slot->type == CB_SLOT_TYPE_NODE)
		{
			// choose which child to access
			node_pos = _cb_search_node(tree, block, node_pos, key, status);
		}
		else
		{
			// just take the first virtual child
			// exact remains as per the last real comparison
			node_pos = tree->block_bfactor * node_pos + 1;
		}
	}
	size_t leaf = node_pos - tree->block_nodes;
	*res = (cb_leaf *)(block->body + tree->block_leaves_off) + leaf;
}

size_t cb_get(
		cb_tree *tree,
		cb_key key,
		char *status)
{
	cb_block_h *block = tree->root;
	cb_leaf *leaf;
	*status = CB_FOUND_NONE;
	
	long page_size = sysconf(_SC_PAGESIZE);

	_cb_search_block(tree, block, key, &leaf, status);
	
	while (leaf->type != CB_LEAF_TYPE_TUPLE)
	{
		if (*status == CB_FOUND_NONE)
		{
			return 0;
		}
		if (leaf->type == CB_LEAF_TYPE_NULL)
		{
			return 0;
		}
		
		size_t offset = (leaf->pos / page_size) * page_size;
		char *mptr = mmap(NULL,
				tree->block_size,
				PROT_READ,
				MAP_PRIVATE,
				tree->index_fd,
				offset);
		assert(mptr != MAP_FAILED);
		block = (cb_block_h *)(mptr + leaf->pos - offset);

		_cb_search_block(tree, block, key, &leaf, status);

		munmap(mptr, tree->block_size);
	}

	return leaf->pos;
}

/*static void _cb_index_insert(
		cb_tree *tree,
		cb_key key,
		size_t loc)
{
}

void cb_insert(
		cb_tree *tree,
		cb_key key,
		cb_tuple tuple)
{

}

void cb_remove(
		cb_tree *tree,
		cb_key key)
{
}*/
