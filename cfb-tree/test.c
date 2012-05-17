#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fb_tree.c"

void test_fb_search_node(fb_tree *tree, fb_block_h *block, fb_key key, fb_pos node_pos)
{
	bool exact;
	fb_val result;
	_fb_search_node(tree, block, node_pos, key, &exact, &result);
	printf("fb_search_node: seek %i | exact %i | found_type %i | found_val %i | node %u\n",
			key, (int)exact, result.type, result.value, node_pos);
}

void test_fb_search_block(fb_tree *tree, fb_block_h *block, fb_key key)
{
	bool exact;
	fb_val result;
	fb_pos node_pos;
	_fb_search_block(tree, block, key, &exact, &result, &node_pos);
	printf("fb_search_block: seek %i | exact %i | found_type %i | found_val %i | node %u\n",
			key, (int)exact, result.type, result.value, node_pos);
}

void test_fb_get(fb_tree *tree, int key)
{
	bool exact;
	fb_val result;
	fb_pos block_pos, node_pos = 0;
	_fb_get(tree, key, &exact, &result, &block_pos, &node_pos);
	//printf("fb_get: seek %i | exact %i | found_type %i | found_val %i | block %u | node %u\n",
	//		key, (int)exact, result.type, result.value, block_pos, node_pos);
}

void test_fb_insert(fb_tree *tree, fb_key key, uint32_t val)
{
	fb_insert(tree, key, val);
	//printf("fb_insert: key %i | val: %u\n", key, val);
}

int main(int argc, char *argv[])
{
	if (argc != 5)
	{
		fprintf(stderr, "\tUsage: %s index_file block_size slot_size bfactor\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	long block, slot, bfactor;
	block = strtol(argv[2], NULL, 10);
	slot = strtol(argv[3], NULL, 10);
	bfactor = strtol(argv[4], NULL, 10);
	//slot = sizeof(fb_slot_h) + 3 * sizeof(fb_key);
	//block = sizeof(fb_block_h) + slot * 5;

	fb_tree tree;
	fb_init_tree(&tree, argv[1], block, slot, bfactor);
	
	int items = 20000;
	for (int i = items-1; i >= 0; --i)
	{
		if (i % 3 != 0)
		{
			test_fb_insert(&tree, i, i+1);
		}
		//fb_print_tree(&tree);
	}
	for (int i = 0; i < items; ++i)
	{
		if (i % 3 == 0)
		{
			test_fb_insert(&tree, i, 33);
		}
	}

	//fb_print_tree(&tree);
	for (int i = 0; i < items; ++i)
	{
		test_fb_get(&tree, i);
	}
	//test_fb_insert(&tree, 0x44, 0x44);
	//test_fb_insert(&tree, 0x77, 0x77);
	//test_fb_insert(&tree, 0x66, 0x66);
	//test_fb_search_node(&tree, tree.root, 0xAA, 0);
	//test_fb_search_block(&tree, tree.root, 0xAA);

	/*char m[] = {
		CFB_SLOT_TYPE_NODE, 2,
		3, 0, 0, 0,
		6, 0, 0, 0,
		0, 0, 0, 0,
		CFB_SLOT_TYPE_NODE, 2,
		1, 0, 0, 0,
		2, 0, 0, 0,
		0, 0, 0, 0,
		CFB_SLOT_TYPE_NODE, 2,
		3, 0, 0, 0,
		5, 0, 0, 0,
		0, 0, 0, 0,
		CFB_SLOT_TYPE_NODE, 2,
		6, 0, 0, 0,
		7, 0, 0, 0,
		0, 0, 0, 0,
		CFB_SLOT_TYPE_CACHE, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_VAL, 1, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_VAL, 2, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_VAL, 3, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_VAL, 5, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_NULL, 0, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_VAL, 6, 0, 0, 0, 0, 0, 0, 0,
		CFB_LEAF_TYPE_VAL, 7, 0, 0, 0, 0, 0, 0, 0,
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
	test_fb_insert(&tree, 4, 0xCB);
	test_fb_get(&tree, 4);
	test_fb_insert(&tree, 8, 0xCC);
	test_fb_get(&tree, 8);*/

	fb_destr_tree(&tree);
	return EXIT_SUCCESS;
}

