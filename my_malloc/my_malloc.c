#include "my_malloc.h"
#include <stdio.h>

static mem_block *free_mem_blocks = NULL;

unsigned long size_heap_mem = 0;

void list_update(mem_block *block, size_t size) {
  // check if the current block can split
  if (block->size > size + META_SIZE) {
    // split a new block and add it to the free list
    mem_block *new_block = (void *)block + size + META_SIZE;
    new_block->size = block->size - size - META_SIZE;
    new_block->next = NULL;
    new_block->prev = NULL;
    insert_block(new_block);
    block->size = size;
  }
  // delete the current block from free list
  delete_block(block);
}

mem_block *find_free_block_FF(size_t size) {
  mem_block *current = free_mem_blocks;
  while (current != NULL) {
    if (current->size >= size) {
      list_update(current, size);
      return (void *)current + META_SIZE;
    }
    current = current->next;
  }
  return NULL;
}

mem_block *find_free_block_BF(size_t size) {
  mem_block *current = free_mem_blocks;
  mem_block *best_block = NULL;
  while (current != NULL) {
    if (current->size >= size) {
      if (current->size == size) {
        list_update(current, size);
        return (void *)current + META_SIZE;
      }
      if (best_block == NULL) {
        best_block = current;
      } else if (current->size < best_block->size) {
        best_block = current;
      }
    }
    current = current->next;
  }
  if (best_block != NULL) {
    list_update(best_block, size);
    return (void *)best_block + META_SIZE;
  }
  return NULL;
}

void *new_mem_block(size_t size) {
  mem_block *block = (void *) sbrk(size + META_SIZE);
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
  if (latter == 1) {
    block->size = block->size + META_SIZE + right->size;
    delete_block(right);
  }
  if (former == 1) {
    left->size = left->size + META_SIZE + block->size;
    delete_block(block);
  }
  return;
}

void insert_block(mem_block *block) {
  // insert the block to the sorted list based on address
  if (free_mem_blocks == NULL) {
    free_mem_blocks = block;
    return;
  }

  mem_block *current = free_mem_blocks;
  mem_block *previous = NULL;
  while (current != NULL) {
    if (current > block) {
      break;
    }
    previous = current;
    current = current->next;
  }

  block->next = current;
  block->prev = previous;
  if (previous == NULL) {
    current->prev = block;
    free_mem_blocks = block;
  } else {
    previous->next = block;
    if (current != NULL) {
      current->prev = block;
    }
  }

  merge_free_blocks(block);
  return;
}

void delete_block(mem_block *block) {
  if (block->prev != NULL) {
    block->prev->next = block->next;
  } else {
    free_mem_blocks = block->next;
  }
  if (block->next != NULL) {
    block->next->prev = block->prev;
  }
  block->next = NULL;
  block->prev = NULL;
  return;
}

// first fit
void *ff_malloc(size_t size) {
  void *result = find_free_block_FF(size);
  return (result != NULL) ? result : new_mem_block(size);
}

void ff_free(void *ptr) {
  mem_block *block = (void *)ptr - META_SIZE;
  insert_block(block);
}

// best fit
void *bf_malloc(size_t size) {
  void *result = find_free_block_BF(size);
  return (result != NULL) ? result : new_mem_block(size);
}

void bf_free(void *ptr) {
  mem_block *block = (void *)ptr - META_SIZE;
  insert_block(block);
}

// performance measurement
unsigned long get_data_segment_size() {
  return size_heap_mem;
}

unsigned long get_data_segment_free_space_size() {
  mem_block *current = free_mem_blocks;
  unsigned long size = 0;
  while (current != NULL) {
    size += current->size + META_SIZE;
    current = current->next;
  }
  return size;
}