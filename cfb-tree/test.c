#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fb_tree.c"

void test_fb_search_node(fb_tree *tree, fb_block_h *block, int key)
{
	size_t node_pos = 0;
	char status;
	size_t child = 0;
	size_t abs_child_pos = _fb_search_node(tree, block, node_pos, key, &child, &status);
	printf("_fb_search_node: seek %i | status %i | abs_child_pos %zu | child %zu\n", 
			key, (int)status, abs_child_pos, child);
}

void test_fb_search_block(fb_tree *tree, fb_block_h *block, int key)
{
	char status;
	fb_leaf leaf;
	size_t leaf_pos = 0;
	size_t node_pos = 0;
	size_t child = 0;
	_fb_search_block(tree, block, key, &node_pos, &leaf, &leaf_pos, &child, &status);
	printf("_fb_search_block: seek %i | status %i | content %zu | node_pos %zu | leaf_pos %zu | child %zu\n",
			key, (int)status, leaf.pos, node_pos, leaf_pos, child);
}

void test_fb_get(fb_tree *tree, int key)
{
	bool found;
	size_t tuple_pos, block_pos, node_pos, leaf_pos, child;
	char status;
	_fb_get(tree, key, &found, &tuple_pos, &block_pos, &node_pos, &leaf_pos, &child, &status);
	printf("_fb_get: seek %i | found %i | status %i | content %zu | block_pos %zu | node_pos %zu | leaf_pos %zu | child %zu \n",
			key, (int)found, (int)status, tuple_pos, block_pos, node_pos, leaf_pos, child);
}

void test_fb_insert(fb_tree *tree, int key, size_t val)
{
	fb_insert(tree, key, val);
	printf("fb_insert: key %i | val: %zu\n", key, val);
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "\tUsage: %s index_file block_size slot_size\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	long block, slot;
	//block = strtol(argv[2], NULL, 10);
	//slot = strtol(argv[3], NULL, 10);
	slot = 2 + 3*4;
	block = 9 + slot * 5 + 16 * 9;

	fb_tree tree;
	fb_init_tree(&tree, argv[1], block, slot);
	fb_block_h *block_h = tree.root;

	char m[] = {
		CFB_SLOT_TYPE_NODE, 2,
		2, 0, 0, 0,
		4, 0, 0, 0,
		8, 0, 0, 0,
		CFB_SLOT_TYPE_NODE, 2,
		1, 0, 0, 0,
		2, 0, 0, 0,
		0, 0, 0, 0,
		CFB_SLOT_TYPE_NODE, 2,
		3, 0, 0, 0,
		4, 0, 0, 0,
		0, 0, 0, 0,
		CFB_SLOT_TYPE_NODE, 2,
		6, 0, 0, 0,
		7, 0, 0, 0,
		0, 0, 0, 0,
		CFB_SLOT_TYPE_CACHE, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		CFB_LEAF_TYPE_TUPLE, 1, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_TUPLE, 2, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_TUPLE, 3, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_TUPLE, 4, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_TUPLE, 6, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_TUPLE, 7, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0
	};
	memcpy(block_h->body, (char *)m, sizeof(m));

	test_fb_search_node(&tree, block_h, 0);
	test_fb_search_node(&tree, block_h, 1);
	test_fb_search_node(&tree, block_h, 2);
	test_fb_search_node(&tree, block_h, 3);
	test_fb_search_node(&tree, block_h, 4);
	test_fb_search_node(&tree, block_h, 5);
	test_fb_search_node(&tree, block_h, 6);
	test_fb_search_node(&tree, block_h, 7);
	test_fb_search_node(&tree, block_h, 8);
	printf("\n");
	
	test_fb_search_block(&tree, block_h, 0);
	test_fb_search_block(&tree, block_h, 1);
	test_fb_search_block(&tree, block_h, 2);
	test_fb_search_block(&tree, block_h, 3);
	test_fb_search_block(&tree, block_h, 4);
	test_fb_search_block(&tree, block_h, 5);
	test_fb_search_block(&tree, block_h, 6);
	test_fb_search_block(&tree, block_h, 7);
	test_fb_search_block(&tree, block_h, 8);
	printf("\n");

	test_fb_get(&tree, 0);
	test_fb_get(&tree, 1);
	test_fb_get(&tree, 2);
	test_fb_get(&tree, 3);
	test_fb_get(&tree, 4);
	test_fb_get(&tree, 5);
	test_fb_get(&tree, 6);
	test_fb_get(&tree, 7);
	test_fb_get(&tree, 8);
	printf("\n");

	test_fb_insert(&tree, 3, 0xCA);
	test_fb_get(&tree, 3);
	test_fb_insert(&tree, 5, 0xCB);
	test_fb_get(&tree, 5);

	fb_destr_tree(&tree);
	return EXIT_SUCCESS;
}

