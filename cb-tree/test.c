#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cb_tree.c"

bool test_cb_search_node(cb_tree *tree, cb_block_h *block, int key)
{
	char status;
	size_t pos = _cb_search_node(tree, block, 0, key, &status);
	printf("_cb_search_node: seek %i status %i pos %zu\n", key, (int)status, pos);
	return true;
}

bool test_cb_search_block(cb_tree *tree, cb_block_h *block, int key)
{
	char status;
	cb_leaf *leaf;
	_cb_search_block(tree, block, key, &leaf, &status);
	printf("_cb_search_block: seek %i status %i cont %zu\n", key, (int)status, leaf->pos);
	return true;
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
	cb_block_h *block_h = malloc(block);
	block_h->type = CB_BLOCK_TYPE_ROOT;
	block_h->parent = 33;

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
		7, 0, 0, 0,
		8, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 2, 0, 0, 0, 0, 0, 0, 0,
		1, 3, 0, 0, 0, 0, 0, 0, 0,
		1, 4, 0, 0, 0, 0, 0, 0, 0,
		1, 5, 0, 0, 0, 0, 0, 0, 0,
		1, 6, 0, 0, 0, 0, 0, 0, 0,
		1, 7, 0, 0, 0, 0, 0, 0, 0,
		1, 8, 0, 0, 0, 0, 0, 0, 0
	};
	memcpy(block_h->body, (char *)m, sizeof(m));

	test_cb_search_node(&tree, block_h, 1);
	test_cb_search_node(&tree, block_h, 2);
	test_cb_search_node(&tree, block_h, 5);
	test_cb_search_node(&tree, block_h, 7);
	
	test_cb_search_block(&tree, block_h, 0);
	test_cb_search_block(&tree, block_h, 1);
	test_cb_search_block(&tree, block_h, 3);
	
	free(block_h);

	cb_destr_tree(&tree);
	return EXIT_SUCCESS;
}

