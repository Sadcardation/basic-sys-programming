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

// helper functions
void list_update(mem_block *block, size_t size);
mem_block *find_free_block(size_t size, const char *policy);
void *new_mem_block(size_t size);
void merge_free_blocks(mem_block *block);
void insert_block(mem_block *block);
void delete_block(mem_block *block);

// first fit
void *ff_malloc(size_t size);
void ff_free(void *ptr);

// best fit
void *bf_malloc(size_t size);
void bf_free(size_t size);

// performance measurement
unsigned long get_data_segment_size(); // in bytes
unsigned long get_data_segment_free_space_size(); // in bytes

#endif