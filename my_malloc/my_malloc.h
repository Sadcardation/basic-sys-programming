

#include <stdlib.h>
#include <unistd.h>

#define META_SIZE sizeof(mem_block)

struct mem_block_t {
  size_t size;
  struct mem_block_t *next;
  struct mem_block_t *prev;
};
typedef struct mem_block_t mem_block;

// first fit
void *ff_malloc(size_t size);
void ff_free(void *ptr);

// best fit
void *bf_malloc(size_t size);
void bf_free(size_t size);

// performance measurement
unsigned long get_data_segment_size(); // in bytes
unsigned long get_data_segment_free_space_size(); // in bytes
