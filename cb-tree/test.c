#include <stdio.h>
#include <stdlib.h>

#include "cb_tree.h"

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "\tUsage: %s index_file block_size slot_size\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	long block, slot;
	block = strtol(argv[2], NULL, 10);
	slot = strtol(argv[3], NULL, 10);

	cb_tree tree;
	cb_init_tree(&tree, argv[1], block, slot);
	cb_destr_tree(&tree);
	return EXIT_SUCCESS;
}

