#define _XOPEN_SOURCE 500

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "db.h"
#include "fb_tree.h"

int dbfd;
fb_tree tree;
size_t content = 0;

#define DB_FILE "db"
#define INDEX_FILE "index"

void init(size_t block_size, size_t slot_size, size_t bfactor)
{
	dbfd = open(DB_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	assert(dbfd == 0);

	fb_init_tree(&tree, INDEX_FILE, block_size, slot_size, bfactor);

}

void destr()
{
	close(dbfd);

	fb_destr_tree(&tree);
}

int insert(key_t key, tuple_t *tuple)
{
	lseek(dbfd, content * sizeof(tuple_t), SEEK_SET);
	write(dbfd, tuple, sizeof(tuple_t));
	fb_insert(&tree, key, content * sizeof(tuple_t));
	++content;
	return 0;
}

int search(key_t key, tuple_t *tuple)
{
	bool exact;
	uint32_t result;
	fb_get(&tree, key, &exact, &result);
	if (!exact)
	{
		return -1;
	}
	else
	{
		lseek(dbfd, result, SEEK_SET);
		read(dbfd, tuple, sizeof(tuple_t));
		return 0;
	}
}
