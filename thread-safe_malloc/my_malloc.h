#ifndef MY_MALLOC
#define MY_MALLOC

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define META_SIZE sizeof(mem_block)

struct mem_block_t {
  size_t size;
  struct mem_block_t *next;
  struct mem_block_t *prev;
};
typedef struct mem_block_t mem_block;

// multi-threaded related helper functions
mem_block **get_global_mem_block_list(void);
mem_block **get_thread_local_mem_block_list(void);
void set_mem_block_list(int use_thread_local);

// helper functions
void list_update(mem_block *block, size_t size);
mem_block *find_free_block(size_t size);
mem_block *ts_sbrk(size_t size);
void *new_mem_block(size_t size);
void merge_free_blocks(mem_block *block);
void insert_block(mem_block *block);
void delete_block(mem_block *block);

// Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

// Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

#endif