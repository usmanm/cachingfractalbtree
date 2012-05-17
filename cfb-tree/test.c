#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfb_tree.c"
#include "db.c"

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

	fb_destr_tree(&tree);

	init(block, slot, bfactor);
	key_t key = 50000;
	tuple_t tuple;
	tuple.id = key;
	memcpy(tuple.name, "Edsger Dijkstra    \0", 20);
	tuple.items[0] = 19300511;
	tuple.items[1] = 20020806;
	insert(key, &tuple);
	tuple_t res1, res2;
	search(key, &res1);
	search(key, &res1);
	search(key, &res1);
	search(key, &res1);
	search(key, &res1);
	search(key, &res2);
	printf("<%i, %s, %i, %i>\n", res2.id, res2.name, res2.items[0], res2.items[1]);
	destr();

	return EXIT_SUCCESS;
}

