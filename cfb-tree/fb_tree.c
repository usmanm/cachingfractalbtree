#include "fb_tree.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct _fb_node_data fb_node_data;
struct _fb_node_data
{
	fb_slot_h *slot;
	fb_key *keys;
	fb_val *vals;
};

static inline fb_node_data _fb_node_content(
		fb_tree *tree,
		fb_block_h *block,
		fb_pos node_pos)
{
	fb_node_data data;
	data.slot = (fb_slot_h *)(block->body + node_pos * tree->slot_size);
	data.keys = (fb_key *)data.slot->body;
	data.vals = (fb_val *)(data.slot->body + tree->kfactor * sizeof(fb_key));
	return data;
}

void fb_print_block(fb_tree *tree, fb_block_h *block)
{
	printf(" --- block ---\n");
	printf("type %i | cont %i | parent %i | root %i | height %i\n",
			block->type, block->cont, block->parent, block->root, block->height);
	for (size_t i = 0; i < tree->block_slots; ++i)
	{
		fb_node_data slot = _fb_node_content(tree, block, i);
		if (slot.slot->type == CFB_SLOT_TYPE_CACHE)
		{
			//printf("> slot %zu is cache\n", i);
		}
		else
		{
			printf("> slot %zu: type %i | cont %i | parent % i\n",
					i, slot.slot->type, slot.slot->cont, slot.slot->parent);
		}
	}
	printf(" -------------\n");
}

static void _fb_init_node(
		fb_tree *tree,
		fb_block_h *block,
		fb_pos node_pos)
{
	fb_node_data node = _fb_node_content(tree, block, node_pos);
	
	for (fb_pos i = 0; i < tree->bfactor; ++i)
	{
		node.vals[i].type = CFB_VALUE_TYPE_NULL;
	}

	node.slot->type = CFB_SLOT_TYPE_NODE;
	node.slot->cont = 0;
	++block->cont;
}


static void _fb_init_block(
		fb_tree *tree,
		fb_block_h* block,
		uint8_t type, fb_pos parent)
{
	block->type = type;
	block->parent = parent;
	block->root = 0;
	block->cont = 0;
	block->height = 0;

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
	tree->root_mptr = (char *)tree->root;

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
	munmap(tree->root_mptr, tree->block_size);
	close(tree->index_fd);
}

static inline fb_pos _fb_get_fresh_node(
		fb_tree *tree,
		fb_block_h *block)
{
	fb_pos node_pos;
	for (node_pos = 0; node_pos < tree->block_slots; ++node_pos)
	{
		fb_node_data node = _fb_node_content(tree, block, node_pos);
		if (node.slot->type == CFB_SLOT_TYPE_CACHE)
		{
			_fb_init_node(tree, block, node_pos);
			return node_pos;
		}
	}
	fprintf(stderr, "ERROR: could not find room for a node\n");
	exit(EXIT_FAILURE);
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
	fb_node_data node = _fb_node_content(tree, block, node_pos);
	assert (node.slot->cont > 0);

	// bounds of range being considered
	for (uint8_t i = 0; i < node.slot->cont; ++i)
	{
		if (key < node.keys[i])
		{
			*exact = ((i > 0) && (node.keys[i-1] == key)) ? true : false;
			*result = node.vals[i];
			return;
		}
	}
	
	// larger or eq than last key
	*exact = node.keys[node.slot->cont - 1] == key ? true : false;
	*result = node.vals[node.slot->cont];
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
	*node_pos = block->root;
	while (true)
	{
		fb_slot_h *slot = (fb_slot_h *)(block->body + *node_pos * tree->slot_size);
		//printf("~ search_block node-type %i\n", slot->type);
		if (slot->type == CFB_SLOT_TYPE_NODE)
		{
			_fb_search_node(tree, block, *node_pos, key, exact, result);

			//printf("~ search_block leaf-type %i\n", result->type);
			switch (result->type)
			{
				case CFB_VALUE_TYPE_NODE: *node_pos = result->node_pos; break;
				case CFB_VALUE_TYPE_NULL: return;
				case CFB_VALUE_TYPE_BLOCK: return;
				case CFB_VALUE_TYPE_CNTNT: return;
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

	if (result->type != CFB_VALUE_TYPE_CNTNT)
	{
		*exact = false;
	}
}

void fb_get(
		fb_tree *tree,
		fb_key key,
		bool *exact,
		uint32_t *result)
{
	fb_pos block_pos;
	fb_pos node_pos;
	fb_val res;
	_fb_get(tree, key, exact, &res, &block_pos, &node_pos);
	*result = res.value;
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

void _fb_insert_node(
		fb_tree *tree,
		fb_block_h *block,
		fb_pos node_pos,
		fb_key key,
		fb_val val);

void _fb_split_node(
		fb_tree *tree,
		fb_block_h *block,
		fb_pos node_pos,
		fb_pos *next_pos)
{
	fb_node_data node = _fb_node_content(tree, block, node_pos);
	*next_pos = _fb_get_fresh_node(tree, block);
	fb_node_data next = _fb_node_content(tree, block, *next_pos);
	next.slot->parent = node.slot->parent;

	size_t target_size = tree->bfactor / 2;
	for (size_t i = target_size; i < node.slot->cont; ++i)
	{
		next.keys[i-target_size] = node.keys[i];
		next.vals[i-target_size+1] = node.vals[i+1];
		++next.slot->cont;
	}
	node.slot->cont -= next.slot->cont;

	fb_val val;
	val.type = CFB_VALUE_TYPE_NODE;
	val.node_pos = *next_pos;
	_fb_insert_node(tree, block, node.slot->parent, next.keys[0], val);
}

static inline bool _fb_node_needs_split(fb_tree *tree, fb_block_h *block, size_t node_pos)
{
	fb_node_data node = _fb_node_content(tree, block, node_pos);
	return node.slot->cont == tree->kfactor ? true : false;
}

void _fb_insert_node(
		fb_tree *tree,
		fb_block_h *block,
		fb_pos node_pos,
		fb_key key,
		fb_val val)
{
	fb_node_data node = _fb_node_content(tree, block, node_pos);
	
	fb_key key_tmp;
	fb_key key_old = key;
	fb_val val_tmp;
	fb_val val_old = val;
	for (size_t i = 0; i < node.slot->cont; ++i)
	{
		if (node.keys[i] > key_old)
		{
			key_tmp = node.keys[i];
			val_tmp = node.vals[i+1];
			node.keys[i] = key_old;
			node.vals[i+1] = val_old;
			key_old = key_tmp;
			val_old = val_tmp;
		}
	}
	node.keys[node.slot->cont] = key_old;
	node.vals[node.slot->cont+1] = val_old;

	++node.slot->cont;
	++tree->content;

	fb_pos next_pos;
	if (_fb_node_needs_split(tree, block, node_pos))
	{
		if (node_pos != block->root) // guaranteed to have one free node
		{
			_fb_split_node(tree, block, node_pos, &next_pos);
		}
		else
		{
			if (block->height < tree->block_height)
			{
				// add a parent to the current root
				fb_pos parent_pos =_fb_get_fresh_node(tree, block);
				fb_node_data parent = _fb_node_content(tree, block, parent_pos);
				parent.vals[0].type = CFB_VALUE_TYPE_NODE;
				parent.vals[0].node_pos = node_pos;
				block->root = parent_pos;
				node.slot->parent = parent_pos;

				++block->height;
				
				// split the former root
				_fb_split_node(tree, block, node_pos, &next_pos);
			}
			else
			{
				// split block
				assert(false);
			}
		}
	}
}

void _fb_replace_value(
		fb_tree *tree,
		fb_block_h *block,
		fb_pos node_pos,
		fb_key key,
		fb_val val)
{
	fb_node_data node = _fb_node_content(tree, block, node_pos);
	
	for (size_t i = 0; i < node.slot->cont; ++i)
	{
		if (node.keys[i] == key)
		{
			node.vals[i+1] = val;
		}
	}
}

void fb_insert(
		fb_tree *tree,
		fb_key key,
		uint32_t value)
{
	bool exact;
	fb_pos block_pos;
	fb_pos node_pos;
	
	fb_val val;
	val.type = CFB_VALUE_TYPE_CNTNT;
	val.value = value;
	
	long page_size = sysconf(_SC_PAGESIZE);
	char *mptr = NULL;

	if (tree->content == 0) // insertion on empty tree
	{
		_fb_init_node(tree, tree->root, 0);
		_fb_insert_node(tree, tree->root, 0, key, val);
		return;
	}

	// find the leaf where the result should be
	fb_val result;
	_fb_get(tree, key, &exact, &result, &block_pos, &node_pos);
	
	fb_block_h *block = NULL;
	if (block_pos != 0) // open the block owning the leaf
	{
		size_t offset = (block_pos / page_size) * page_size;
		mptr = mmap(NULL,
			tree->block_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			tree->index_fd,
			offset);
		assert(mptr != MAP_FAILED);
		block = (fb_block_h *)(mptr + block_pos - offset);
	}
	else // the leaf is owned by the root, which is always ready
	{
		block = tree->root;
	}

	if (exact) // exact match, replace value
	{
		_fb_replace_value(tree, block, node_pos, key, val);
		return;
	}
	else // true insertion
	{
		_fb_insert_node(tree, block, node_pos, key, val);
	}

	if (mptr != NULL)
	{
		munmap(mptr, tree->block_size);
	}
}

/*void fb_remove(
		fb_tree *tree,
		fb_key key)
{
}*/

