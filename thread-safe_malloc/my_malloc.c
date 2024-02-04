#include "my_malloc.h"
#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <pthread.h>

static mem_block *global_free_mem_block_list = NULL;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread mem_block *ts_local_free_mem_blocks = NULL;

mem_block **(*get_mem_block_list)(void);

mem_block **get_global_mem_block_list(void) {
    return &global_free_mem_block_list;
}

mem_block **get_thread_local_mem_block_list(void) {
    return &ts_local_free_mem_blocks;
}

void set_mem_block_list(int use_thread_local) {
    get_mem_block_list = use_thread_local ? get_thread_local_mem_block_list : get_global_mem_block_list;
}


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

mem_block *find_free_block(size_t size) {
  mem_block **free_mem_blocks = get_mem_block_list();
  mem_block *current = *free_mem_blocks;
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

mem_block *ts_sbrk(size_t size) {
  pthread_mutex_lock(&lock);
  mem_block *block = (void *) sbrk(size + META_SIZE);
  pthread_mutex_unlock(&lock);
  return block;
}

void *new_mem_block(size_t size) {
  mem_block *block = ts_sbrk(size);
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
  mem_block **free_mem_blocks = get_mem_block_list();
  // insert the block to the sorted list based on address
  if (*free_mem_blocks == NULL) {
    *free_mem_blocks = block;
    return;
  }

  mem_block *current = *free_mem_blocks;
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
    *free_mem_blocks = block;
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
  mem_block **free_mem_blocks = get_mem_block_list();
  if (block->prev != NULL) {
    block->prev->next = block->next;
  } else {
    *free_mem_blocks = block->next;
  }
  if (block->next != NULL) {
    block->next->prev = block->prev;
  }
  block->next = NULL;
  block->prev = NULL;
  return;
}

// locking version
void *ts_malloc_lock(size_t size) {
  set_mem_block_list(0);
  pthread_mutex_lock(&lock);
  void *result = find_free_block(size);
  pthread_mutex_unlock(&lock);
  return (result != NULL) ? result : new_mem_block(size);
}

void ts_free_lock(void *ptr) {
  set_mem_block_list(0);
  mem_block *block = (void *)ptr - META_SIZE;
  pthread_mutex_lock(&lock);
  insert_block(block);
  pthread_mutex_unlock(&lock);
}

// non-locking version
void *ts_malloc_nolock(size_t size) {
  set_mem_block_list(1);
  void *result = find_free_block(size);
  return (result != NULL) ? result : new_mem_block(size);
}

void ts_free_nolock(void *ptr) {
  set_mem_block_list(1);
  mem_block *block = (void *)ptr - META_SIZE;
  insert_block(block);
}