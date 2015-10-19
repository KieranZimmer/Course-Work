#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "smalloc.h"



void *mem;
struct block *freelist;
struct block *allocated_list;


void *smalloc(unsigned int nbytes) {
    struct block *free_block = freelist, *alloc_block, *free_parent_block = NULL;
    void *ret_addr;

    while (free_block && free_block->size < nbytes) {
        free_parent_block = free_block;
        free_block = free_block->next;  //if the block is too small, move to the next
    }
    if (!free_block) return NULL; //If no blocks big enough exist, return
    ret_addr = free_block->addr;

    alloc_block = malloc(sizeof(struct block)); //error check
    alloc_block->addr = ret_addr;
    alloc_block->size = nbytes;
    alloc_block->next = allocated_list;
    allocated_list = alloc_block; //added allocated block to allocated_list

    if (free_block->size == nbytes) { //if whole block is used
        if (free_parent_block) {
            free_parent_block->next = free_block->next;
            free(free_block);
        }
        else { //if the block being used is first in freelist
            freelist = free_block->next;
            free(free_block);
        }

    }
    else if (free_block->size > nbytes) {
        free_block->addr = ret_addr + nbytes; //modify free block to reflect new size
        free_block->size -= nbytes;
    }
    return ret_addr;
}


int sfree(void *addr) {
    struct block *to_free = allocated_list, *to_free_parent = NULL;

    while (to_free && to_free->addr != addr) {
        to_free_parent = to_free;
        to_free = to_free->next;
    }
    if (!to_free) return -1;

    if (to_free_parent) { //if the block to be freed isn't first on the allocated_list
        to_free_parent->next = to_free->next;
    }
    else {
        allocated_list = to_free->next;
    }

    struct block *prev_free = NULL, *next_free = NULL, *iter = freelist;
    int prev_adj = 0, next_adj = 0; //are the previous and following free blocks directly adjacent to block being freed?
    while (iter) {
        if (iter->addr < addr) prev_free = iter;
        else if (iter->addr > addr) {
            next_free = iter;
            break;
        }
        else return -1; //duplicate address, something went wrong
        iter = iter->next;
    }
    if (prev_free) prev_adj = prev_free->addr + prev_free->size == addr; //if the previous free block intersects the current block
    if (next_free) next_adj = addr + to_free->size == next_free->addr;  //if the following free block intersects with the current block

    if (!prev_free) { //if the block being freed comes first
        if (next_adj) { //assimilate adjacent blocks
            next_free->addr = addr;
            next_free->size += to_free->size;
        }
        else { //link new block to next
            struct block *new_block = malloc(sizeof(struct block));
            new_block->addr = addr;
            new_block->size = to_free->size;
            new_block->next = freelist;
            freelist = new_block;
        }
    }
    else {
        if (prev_adj && next_adj) { //assimilate all adjacent blocks
            prev_free->size += to_free->size + next_free->size;
            prev_free->next = next_free->next;
            free(next_free);  //This block is assimilated
        }
        else if (prev_adj && !next_adj) {
            prev_free->size += to_free->size;
        }
        else if (!prev_adj && next_adj) {
            next_free->addr = addr;
            next_free->size += to_free->size;
        }
        else if (!prev_adj && !next_adj) {
            struct block *new_block = malloc(sizeof(struct block));
            new_block->addr = addr;
            new_block->size = to_free->size;
            new_block->next = next_free;
            prev_free->next = new_block;
        }
    }

    free(to_free);
    return 0;
}


/* Initialize the memory space used by smalloc,
 * freelist, and allocated_list
 * Note:  mmap is a system call that has a wide variety of uses.  In our
 * case we are using it to allocate a large region of memory.
 * - mmap returns a pointer to the allocated memory
 * Arguments:
 * - NULL: a suggestion for where to place the memory. We will let the
 *         system decide where to place the memory.
 * - PROT_READ | PROT_WRITE: we will use the memory for both reading
 *         and writing.
 * - MAP_PRIVATE | MAP_ANON: the memory is just for this process, and
 *         is not associated with a file.
 * - -1: because this memory is not associated with a file, the file
 *         descriptor argument is set to -1
 * - 0: only used if the address space is associated with a file.
 */
void mem_init(int size) {
    mem = mmap(NULL, size,  PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(mem == MAP_FAILED) {
         perror("mmap");
         exit(1);
    }

    freelist = malloc(sizeof(struct block)); //add error check
    freelist->addr = mem;
    freelist->next = NULL;
    freelist->size = size;
    allocated_list = NULL;
}

void mem_clean() {
    struct block *to_free;
    while (freelist) {
        to_free = freelist;
        freelist = freelist->next;
        free(to_free);
    }
    while (allocated_list) {
        to_free = allocated_list;
        allocated_list = allocated_list->next;
        free(to_free);
    }
}
