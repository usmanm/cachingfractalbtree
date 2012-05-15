#include "fb_tree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

static void _fb_init_block(fb_tree *tree, fb_block_h* block, uint8_t type, fb_pos parent)
{
	block->type = type;
	block->parent = parent;

	for (size_t s = 0; s < tree->block_slots; ++s)
	{
		fb_slot_h *slot = (fb_slot_h *)(block->body + s * tree->slot_size);
		slot->type = CFB_SLOT_TYPE_CACHE;
		slot->cont = 0;
	}
}

void fb_init_tree(
		fb_tree *tree,
		const char *file,
		size_t block_size,
		size_t slot_size,
		size_t bfactor)
{
	tree->block_size = block_size;
	tree->slot_size = slot_size;

	tree->block_slots = (block_size - sizeof(fb_block_h)) / slot_size;

	size_t cache_avail_body = slot_size - sizeof(fb_slot_h);
	size_t cache_tuples = cache_avail_body / sizeof(fb_tuple);
	tree->cache_tuples = cache_tuples;
	
	size_t min_slot_content = sizeof(fb_val) + (bfactor-1)*(sizeof(fb_key) + sizeof(fb_val));
	if (min_slot_content > slot_size - sizeof(slot_size))
	{
		fprintf(stderr, "ERROR: node cannot store enough key/value pairs for bfactor\n");
		exit(EXIT_FAILURE);
	}

	if (cache_tuples < 1)
	{
		fprintf(stderr, "ERROR: cache must fit at least 1 tuple\n");
		exit(EXIT_FAILURE);
	}
	
	if (bfactor < 3)
	{
		fprintf(stderr, "ERROR: a B+tree must have a bfactor > 2\n");
		exit(EXIT_FAILURE);
	}

	tree->bfactor = bfactor;
	tree->kfactor = bfactor - 1;
	tree->content = 0;

	tree->block_nodes = 0;
	for (size_t h = 0; h < tree->block_slots; ++h)
	{
		tree->block_nodes += pow(bfactor, h);
		if (tree->block_nodes <= tree->block_slots)
		{
			tree->block_height = h;
		}
		else
		{
			tree->block_nodes -= pow(bfactor, h);
			break;
		}
	}

	tree->index_fd = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(tree->index_fd != -1);

	ftruncate(tree->index_fd, block_size);
	tree->root = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, tree->index_fd, 0);

	_fb_init_block(tree, tree->root, CFB_BLOCK_TYPE_ROOT | CFB_BLOCK_TYPE_LEAF, 0);
	float usage_density = tree->block_nodes / (float)tree->block_slots;
	printf("index_fd %i\n"
			"block_size %zu\n"
			"slot_size %zu\n"
			"block_slots %zu\n"
			"block_nodes %zu\n"
			"cache_tuples %zu\n"
			"block_bfactor %zu\n"
			"block_height %zu\n"
			"%% density %f\n",
			tree->index_fd,
			tree->block_size,
			tree->slot_size,
			tree->block_slots,
			tree->block_nodes,
			tree->cache_tuples,
			tree->bfactor,
			tree->block_height,
			usage_density);
	
}

void fb_destr_tree(fb_tree *tree)
{
	munmap(tree->root, tree->block_size);
	close(tree->index_fd);
}


/**
 * @param[in] tree The tree to query
 * @param[in] block The block in the tree to query
 * @param[in] node_pos The node to be scanned, must be an inner node
 * @param[in] key The key being searched
 * @param[out] result Child position in block
 */
static inline void _fb_search_node(
		fb_tree *tree,
		fb_block_h *block,
		fb_pos node_pos,
		fb_key key,
		bool *exact,
		fb_val *result)
{
	fb_slot_h *node = (fb_slot_h *)(block->body + node_pos * tree->slot_size);
	fb_key *keys = (fb_key *)node->body;
	fb_val *vals = (fb_val *)(node->body + tree->kfactor * sizeof(fb_key));
	
	assert (node->cont > 0);

	// bounds of range being considered
	for (uint8_t i = 0; i < node->cont; ++i)
	{
		if (key < keys[i])
		{
			*exact = ((i > 0) && (keys[i-1] == key)) ? true : false;
			*result = vals[i];
			return;
		}
	}
	
	// larger or eq than last key
	*exact = keys[node->cont - 1] == key ? true : false;
	*result = vals[node->cont];
}


/**
 * @param[in] tree The tree to search in
 * @param[in] block The block in the tree to search in
 * @param[in] key The key being searched
 * @param[out] node_pos The node that owns the leaf
 * @param[out] leaf The leaf found by the search
 * @param[out] leaf_pos The absolute position of the leaf in the block
 * @param[out] child The child of the lowest existing node to which the leaf corresponds
 * @param[out] status Whether we found the exact item or the range that should contain it
 */
static inline void _fb_search_block(
		fb_tree *tree,
		fb_block_h *block,
		fb_key key,
		bool *exact,
		fb_val *result,
		fb_pos *node_pos)
{
	*node_pos = 0;
	while (*node_pos < tree->block_nodes)
	{
		fb_slot_h *slot = (fb_slot_h *)(block->body + *node_pos * tree->slot_size);
		if (slot->type == CFB_SLOT_TYPE_NODE)
		{
			_fb_search_node(tree, block, *node_pos, key, exact, result);

			switch (result->type)
			{
				case CFB_VALUE_TYPE_NODE: *node_pos = result->node_pos; break;
				case CFB_VALUE_TYPE_NULL: return;
				case CFB_VALUE_TYPE_BLOCK: return;
				case CFB_VALUE_TYPE_VALUE: return;
				default:
					fprintf(stderr, "ERROR: found leaf with unknown value\n");
					assert(false);
			}
		}
		else
		{
			assert(false);
		}
	}
	assert(false);
}

void _fb_get(
		fb_tree *tree,
		fb_key key,
		bool *exact,
		fb_val *result,
		fb_pos *block_pos,
		fb_pos *node_pos)
{
	if (tree->content == 0)
	{
		result->type = CFB_VALUE_TYPE_NULL;
		*exact = false;
		*block_pos = 0;
		*node_pos = 0;
		return;
	}

	fb_block_h *block = tree->root;

	long page_size = sysconf(_SC_PAGESIZE);
	*block_pos = 0;

	_fb_search_block(tree, block, key, exact, result, node_pos);
	
	while (result->type == CFB_VALUE_TYPE_BLOCK)
	{
		*block_pos = result->block_pos;
		size_t offset = (*block_pos / page_size) * page_size;
		char *mptr = mmap(NULL,
				tree->block_size,
				PROT_READ,
				MAP_PRIVATE,
				tree->index_fd,
				offset);
		assert(mptr != MAP_FAILED);
		
		block = (fb_block_h *)(mptr + *block_pos - offset);
		
		_fb_search_block(tree, block, key, exact, result, node_pos);

		munmap(mptr, tree->block_size);
	}

	if (result->type == CFB_VALUE_TYPE_NULL)
	{
		*exact = false;
	}
}

void fb_get(
		fb_tree *tree,
		fb_key key,
		bool *exact,
		fb_val *result)
{
	fb_pos block_pos;
	fb_pos node_pos;
	_fb_get(tree, key, exact, result, &block_pos, &node_pos);
}

/*static inline void _fb_insert_node(
		fb_tree *tree,
		fb_block_h *block,
		size_t node_pos,
		fb_key key)
{
	fb_node_h *node = (fb_node_h *)(block->body + node_pos * tree->slot_size);
	if (node->keyc >= tree->node_keys)
	{
		// there is no room for an insert
		// should never reach this
		assert(false);
	}

	// place the new key in order
	fb_key last = key;
	fb_key tmp;
	size_t pos = SIZE_MAX;
	for (size_t i = 0; i < node->keyc; ++i)
	{
		if (node->keyv[i] > key)
		{
			pos = pos < i ? pos : i;
			tmp = node->keyv[i];
			node->keyv[i] = last;
			last = tmp;
		}
	}
	node->keyv[node->keyc] = last;

	++node->keyc;
}

static inline void _fb_insert_leaf(
		fb_tree *tree,
		fb_leaf *target,
		size_t child,
		size_t value)
{
	fb_leaf buff;
	fb_leaf next;
	next.type = CFB_LEAF_TYPE_VAL;
	next.pos = value;

	// interval after the one we would have looked at
	// when inserting b:
	// [a;c) -> [a;b)[b;c)
	++target;
	
	for (size_t i = 0; i < tree->block_bfactor - (child + 1); ++i)
	{
		buff = target[i];
		target[i] = next;
		next = buff;
	}
}

static inline bool _fb_node_needs_split(fb_tree *tree, fb_block_h *block, size_t node_pos)
{
	fb_node_h *node = (fb_node_h *)(block->body + node_pos * tree->slot_size);
	return node->keyc >= tree->node_keys ? true : false;
}

static inline void _fb_split_block(
		fb_tree *tree)
{
	// TODO
	// make blocks leaves when necessary
}

static inline void _fb_split_node(
		fb_tree *tree,
		fb_block_h *block,
		size_t node_pos)
{
	if (node_pos == 0)
	{
		// TODO we need to branch the block
		assert(false);
		return;
	}

	size_t parent_pos = (node_pos - 1) / tree->block_bfactor;
	fb_node_h *node = ((fb_node_h *)block->body) + node_pos;
	fb_node_h *next = node_pos + 1;
	next->slot.type = CFB_SLOT_TYPE_NODE;
	next->keyc = 0;
	for (size_t i = node->keyc / 2 + 1; i < node->keyc; ++i)
	{
		next->keyv[next->keyc] = node->keyv[i];
		++next->keyc;
	}
	node->keyc -= next->keyc;

	// TODO this is not enough, we must take care of the leaves here

	// insert the min of the new node into the parent
	// if the parent becomes full, recursively split it
	_fb_insert_node(tree, block, parent_pos, next->keyv[0]);
	if (_fb_node_needs_split(tree, block, parent_pos))
	{
		_fb_split_node(tree, block, parent_pos);
	}
}*/

/*void fb_insert(
		fb_tree *tree,
		fb_key key,
		size_t value)
{
	bool found;
	size_t old_tuple_pos;
	size_t old_block_pos;
	size_t old_node_pos;
	size_t old_leaf_pos;
	size_t child;
	char status;

	long page_size = sysconf(_SC_PAGESIZE);
	char *mptr = NULL;
	
	_fb_get(tree, key, &found, &old_tuple_pos, &old_block_pos, &old_node_pos, &old_leaf_pos, &child, &status);
	
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
		goto cleanup;
	}
	else // insert an item that was not present
	{
		_fb_insert_node(tree, block, old_node_pos, key);
		_fb_insert_leaf(tree, target, child, value);

		_fb_node_needs_split(tree, block, old_node_pos);
		// TODO
	}

cleanup:

	if (block != tree->root)
	{
		munmap(mptr, tree->block_size);
	}
}*/

/*void fb_remove(
		fb_tree *tree,
		fb_key key)
{
}*/

