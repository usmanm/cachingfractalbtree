#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "cfb_tree.h"


/*void test_fb_search_node(fb_tree *tree, fb_block_h *block, fb_key key, fb_pos node_pos)
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
}*/

int main(int argc, char *argv[])
{
	if (argc != 5)
	{
		fprintf(stderr, "\tUsage: %s index_file block_size slot_size bfactor\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	long block_size, slot_size, bfactor;
	block_size = strtol(argv[2], NULL, 10);
	slot_size = strtol(argv[3], NULL, 10);
	bfactor = strtol(argv[4], NULL, 10);

	init(block_size, slot_size, bfactor);
	
	
	fb_key key;
	fb_tuple tuple;

	int items = 20000;
	for (int i = items-1; i >= 0; --i)
	{
		if (i % 3 != 0)
		{
			key = i;
			tuple.id = i;
			memcpy(tuple.name, "abcdefghijklmnopqrs\0", 20);
			tuple.items[0] = i+1;
			tuple.items[1] = i-1;
			insert_uncached(key, &tuple);
		}
	}
	for (int i = 0; i < items; ++i)
	{
		if (i % 3 == 0)
		{
			key = i;
			tuple.id = i;
			memcpy(tuple.name, "srqponmlkjihgfedcba\0", 20);
			tuple.items[0] = i+1;
			tuple.items[1] = i-1;
			insert_uncached(key, &tuple);
		}
	}

	fb_tuple res;
	for (int i = 0; i < items; ++i)
	{
		key = i;
		int ret_val = search_uncached(key, &res);
		//ret_val = search_cached(key, &res);
		if (ret_val)
		{
			printf("MISSED\n");
		}
		//printf("<%i, %s, %i, %i>\n", res.id, res.name, res.items[0], res.items[1]);
	}

	destr();

	return EXIT_SUCCESS;
}

