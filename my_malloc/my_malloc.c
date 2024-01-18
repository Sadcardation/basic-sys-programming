#include "my_malloc.h"

static mem_block *free_mem_blocks = NULL;
// static size_t free_length = 0;

unsigned long size_heap_mem = 0;
unsigned long size_free_mem = 0;

void list_update(mem_block *block, size_t size) {
  // check if the current block can split
  if (block->size - size > META_SIZE) {
    // split a new block and add it to the free list
    mem_block *new_block = (void *)block + size + META_SIZE;
    new_block->size = block->size - size - META_SIZE;
    new_block->next = block->next;
    new_block->prev = block;
    block->next = new_block;
    if (new_block->next != NULL) {
      new_block->next->prev = new_block;
    }
    // free_length += 1;
  }
  block->size = size;
  // delete the current block from free list
  // free_length -= 1;
  size_free_mem -= block->size + META_SIZE;
  if (block->prev != NULL) {
    block->prev->next = block->next;
  } else {
    free_mem_blocks = block->next;
  }
  if (block->next != NULL) {
    block->next->prev = block->prev;
  }
}

mem_block *find_free_block(size_t size, const char *policy) {
  mem_block *current = free_mem_blocks;
  while (current != NULL) {
    if (current->size >= size) {
      list_update(current, size);
      return (void *)current + META_SIZE;
    }
    current = current->next;
  }
  return NULL;
  // TODO: implement best fit
}

void *new_mem_block(size_t size) {
  mem_block *block = sbrk(size + META_SIZE);
  size_heap_mem += size + META_SIZE;
  if (block == (void *)-1) {
    return NULL;
  }
  block->size = size;
  block->next = NULL;
  block->prev = NULL;
  return (void *)block + META_SIZE;
}

void merge_free_blocks(mem_block *block) {
  mem_block *left = block->prev;
  mem_block *right = block->next;
  int former = ((left != NULL) &&
                ((void *)left + left->size + META_SIZE == (void *)block))
                   ? 1
                   : 0;
  int latter = ((right != NULL) &&
                ((void *)block + block->size + META_SIZE == (void *)right))
                   ? 1
                   : 0;
  if (former == 1) {
    left->size = left->size + META_SIZE + block->size;
    left->next = right;
    if (left->next != NULL) {left->next->prev = left;}
    // free_length -= 1;
    if (latter == 1) {
      left->size = left->size + META_SIZE + right->size;
      left->next = right->next;
      if (left->next != NULL) {left->next->prev = left;}
      // free_length -= 1;
    }
  } else if (latter == 1) {
    block->size = block->size + META_SIZE + right->size;
    block->next = right->next;
    if (block->next != NULL) {block->next->prev = block;}
    // free_length -= 1;
  }
  return;
}

void insert_block(mem_block *block) {
  // insert the block to the sorted list based on address
  size_free_mem += block->size + META_SIZE;
  if (free_mem_blocks == NULL) {
    free_mem_blocks = block;
    // free_length += 1;
    return;
  }
  mem_block temp_node = {0};
  temp_node.next = free_mem_blocks;
  mem_block *current = &temp_node;
  while (current->next != NULL) {
    current = current->next;
    if (current > block) {
      block->next = current;
      block->prev = current->prev;
      current->prev = block;
      if (block->prev != NULL) {
        block->prev->next = block;
      } else {
        free_mem_blocks = block;
      }
      // merge adjacent free blocks if possible
      merge_free_blocks(block);
      // free_length += 1;
      return;
    }
  }
  current->next = block;
  current->next->prev = current;
  merge_free_blocks(block);
  // free_length += 1;
  return;
}

// first fit
void *ff_malloc(size_t size) {
  // get_free_list_size();
  void *result = find_free_block(size, "FF");
  return (result != NULL) ? result : new_mem_block(size);
}

void ff_free(void *ptr) {
  // get_free_list_size();
  mem_block *block = (void *)ptr - META_SIZE;
  insert_block(block);
  // get_free_list_size();
}

// performance measurement
unsigned long get_data_segment_size() {
  return size_heap_mem;
}

unsigned long get_data_segment_free_space_size() {
  return size_free_mem;
}

// void get_free_list_size() {
//   size_t real_free_length = 0;
//   mem_block *current = free_mem_blocks;
//   while (current != NULL) {
//     real_free_length += 1;
//     current = current->next;
//   }
//   assert(real_free_length == free_length);
// }