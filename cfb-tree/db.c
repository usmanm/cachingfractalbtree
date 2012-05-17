
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "db.h"
#include "cfb_tree.h"

int dbfd;
fb_tree tree;
size_t content = 0;

#define DB_FILE "db"
#define INDEX_FILE "index"

void init(size_t block_size, size_t slot_size, size_t bfactor)
{
	dbfd = open(DB_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(dbfd != -1);

	fb_init_tree(&tree, INDEX_FILE, block_size, slot_size, bfactor);

}

void destr()
{
	close(dbfd);

	fb_destr_tree(&tree);
}

int insert_uncached(fb_key key, fb_tuple *tuple)
{
	lseek(dbfd, content * sizeof(fb_tuple), SEEK_SET);
	write(dbfd, tuple, sizeof(fb_tuple));
	fb_val val;
	val.type = CFB_VALUE_TYPE_CNTNT;
	val.value = content * sizeof(fb_tuple);
	fb_insert(&tree, key, val);
	++content;
	return 0;
}

int insert_cached(fb_key key, fb_tuple *tuple)
{
	bool exact;
	fb_val result;
	fb_pos block_pos, node_pos;
	if (tree.content > 0)
	{
		_fb_retrieve(&tree, key, &exact, &result, &block_pos, &node_pos);
	}
	
	lseek(dbfd, content * sizeof(fb_tuple), SEEK_SET);
	write(dbfd, tuple, sizeof(fb_tuple));
	fb_val value;
	value.type = CFB_VALUE_TYPE_CNTNT;
	value.value = content * sizeof(fb_tuple);
	_fb_insert(&tree, key, value, exact, block_pos, node_pos);
	fb_cache_replace(&tree, block_pos, key, tuple);
	++content;
	return 0;
}

int search_uncached(fb_key key, fb_tuple *tuple)
{
	bool exact;
	fb_val result;
	fb_pos block_pos, node_pos;
	_fb_retrieve(&tree, key, &exact, &result, &block_pos, &node_pos);
	if (!exact)
	{
		return -1;
	}
	else
	{
		lseek(dbfd, result.value, SEEK_SET);
		read(dbfd, tuple, sizeof(fb_tuple));
		return 0;
	}
}

int search_cached(fb_key key, fb_tuple *tuple)
{
	bool exact;
	fb_val result;
	fb_pos block_pos, node_pos;
	_fb_retrieve(&tree, key, &exact, &result, &block_pos, &node_pos);
	if (!exact)
	{
		return -1;
	}
	else
	{
		if (fb_cache_probe(&tree, block_pos, key, tuple))
		{
			//printf("cached!\n");
			return 0;
		}
		else
		{
			lseek(dbfd, result.value, SEEK_SET);
			read(dbfd, tuple, sizeof(fb_tuple));
			fb_cache_add(&tree, block_pos, key, tuple);
			return 0;
		}
	}
}

