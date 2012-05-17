#ifndef DB_H
#define DB_H

typedef uint32_t key_t;

typedef struct _tuple_t tuple_t;
struct _tuple_t 
{
	uint32_t id;
	char name[20];
	uint32_t items[2];
}
__attribute__((packed));

void init(size_t block_size, size_t slot_size, size_t bfactor);

void destr();

int insert(key_t key, tuple_t *tuple);

int search(key_t key, tuple_t *t);

#endif

