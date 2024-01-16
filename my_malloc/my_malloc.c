#include "my_malloc.h"

mem_block * free_list = NULL;

void list_update(mem_block * block, size_t size) {
    block->size = size;
    // check if the current block can split
    if (block->size - size > META_SIZE) {
        // split a new block and add it to the free list
        mem_block * new_block = (mem_block *) block + size;
        new_block->size = block->size - size - META_SIZE;
        new_block->next = block->next;
        new_block->prev = block;
        block->next = new_block;
        block->size = size;
        if (new_block->next != NULL) {
            new_block->next->prev = new_block;
        }
    }
    // delete the current block from free list
    if (block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        free_list = block->next;
    }
    if (block->next != NULL) {
        block->next->prev = block->prev;
    }
}

mem_block * find_free_block(size_t size, const char * policy) {
    mem_block * current = free_list;
    while (current != NULL) {
        if (current->size >= size) {
            list_update(current, size);
            return (void *) current + META_SIZE;
        }
        current = current->next;
    }
    return NULL;
    // TODO: implement best fit
}

void * new_mem_block(size_t size) {
    mem_block * block = sbrk(size + META_SIZE);
    if (block == (void *) -1) {
        return NULL;
    }
    block->size = size;
    block->next = NULL;
    block->prev = NULL;
    return (void *) block + META_SIZE;
}

void insert_block(mem_block * block) {
    // insert the block to the sorted list based on address
    // TODO: merge the adjacent free blocks
    if (free_list == NULL) {
        free_list = block;
        return;
    }
    mem_block * current = free_list;
    while (current != NULL) {
        if (current > block) {
            block->next = current;
            block->prev = current->prev;
            current->prev = block;
            if (block->prev != NULL) {
                block->prev->next = block;
            } else {
                free_list = block;
            }
            return;
        }
        current = current->next;
    }
}

//first fit
void * ff_malloc(size_t size) {
    void * result = find_free_block(size, "FF");
    return (result != NULL) ? result : new_mem_block(size);
}

void ff_free(void * ptr) {
    mem_block * block = (mem_block *) ptr - META_SIZE;
    insert_block(block);
}