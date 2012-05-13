#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cb_tree.c"

void test_cb_search_node(cb_tree *tree, cb_block_h *block, int key)
{
	size_t node_pos = 0;
	char status;
	size_t key_pos = 0;
	size_t pos = _cb_search_node(tree, block, node_pos, key, &key_pos, &status);
	printf("_cb_search_node: seek %i | status %i | pos %zu | key_pos %zu\n", 
			key, (int)status, pos, key_pos);
}

void test_cb_search_block(cb_tree *tree, cb_block_h *block, int key)
{
	char status;
	cb_leaf leaf;
	size_t leaf_pos, node_pos;
	_cb_search_block(tree, block, key, &node_pos, &leaf, &leaf_pos, &status);
	printf("_cb_search_block: seek %i | status %i | content %zu | node_pos %zu | leaf_pos %zu\n",
			key, (int)status, leaf.pos, node_pos, leaf_pos);
}

void test_cb_get(cb_tree *tree, int key)
{
	bool found;
	size_t tuple_pos, block_pos, node_pos, leaf_pos;
	char status;
	cb_get(tree, key, &found, &tuple_pos, &block_pos, &node_pos, &leaf_pos, &status);
	printf("_cb_get: seek %i | found %i | status %i | content %zu | block_pos %zu | node_pos %zu | leaf_pos %zu\n",
			key, (int)found, (int)status, tuple_pos, block_pos, node_pos, leaf_pos);
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
	slot = 2 + 2*4;
	block = 9 + slot * 4 + 9 * 9;

	cb_tree tree;
	cb_init_tree(&tree, argv[1], block, slot);
	cb_block_h *block_h = tree.root;

	char m[121] = {
		1, 2,
		2, 0, 0, 0,
		4, 0, 0, 0,
		1, 2,
		1, 0, 0, 0,
		2, 0, 0, 0,
		1, 2,
		3, 0, 0, 0,
		4, 0, 0, 0,
		1, 2,
		6, 0, 0, 0,
		7, 0, 0, 0,
		2, 1, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 3, 0, 0, 0, 0, 0, 0, 0,
		2, 4, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 6, 0, 0, 0, 0, 0, 0, 0,
		2, 7, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	memcpy(block_h->body, (char *)m, sizeof(m));

	test_cb_search_node(&tree, block_h, 0);
	test_cb_search_node(&tree, block_h, 1);
	test_cb_search_node(&tree, block_h, 2);
	test_cb_search_node(&tree, block_h, 3);
	test_cb_search_node(&tree, block_h, 4);
	test_cb_search_node(&tree, block_h, 5);
	test_cb_search_node(&tree, block_h, 6);
	test_cb_search_node(&tree, block_h, 7);
	test_cb_search_node(&tree, block_h, 8);
	
	test_cb_search_block(&tree, block_h, 0);
	test_cb_search_block(&tree, block_h, 1);
	test_cb_search_block(&tree, block_h, 2);
	test_cb_search_block(&tree, block_h, 3);
	test_cb_search_block(&tree, block_h, 4);
	test_cb_search_block(&tree, block_h, 5);
	test_cb_search_block(&tree, block_h, 6);
	test_cb_search_block(&tree, block_h, 7);
	test_cb_search_block(&tree, block_h, 8);

	test_cb_get(&tree, 0);
	test_cb_get(&tree, 1);
	test_cb_get(&tree, 2);
	test_cb_get(&tree, 3);
	test_cb_get(&tree, 4);
	test_cb_get(&tree, 5);
	test_cb_get(&tree, 6);
	test_cb_get(&tree, 7);
	test_cb_get(&tree, 8);

	cb_destr_tree(&tree);
	return EXIT_SUCCESS;
}

