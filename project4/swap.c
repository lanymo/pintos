#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <bitmap.h>

static struct block *swap_block;
static struct bitmap *swap_map;
static struct lock swap_lock;


void swap_init(void) {
    swap_block = block_get_role(BLOCK_SWAP);
    if (!swap_block) {
        return;
    }

    size_t swap_size = block_size(swap_block) / SECTORS_PER_PAGE;
    swap_map = bitmap_create(swap_size);
    if (!swap_map) {
        return;
    }

    bitmap_set_all(swap_map, false); 
    lock_init(&swap_lock);
}


void swap_in(size_t index, void *kaddr) {
    if(kaddr == NULL) {
        return;
    }

    lock_acquire(&swap_lock);

    if (!bitmap_test(swap_map, index)) {
        return;
    }

    for (size_t i = 0; i < SECTORS_PER_PAGE; i++) {
        block_read(swap_block, index * SECTORS_PER_PAGE + i,
                   (uint8_t *)kaddr + i * BLOCK_SECTOR_SIZE);
    }

    bitmap_flip(swap_map, index);
    lock_release(&swap_lock);
}


size_t swap_out(void *kaddr) {
    if(kaddr == NULL) {
        return BITMAP_ERROR;
    }

    lock_acquire(&swap_lock);

    size_t index = bitmap_scan_and_flip(swap_map, 0, 1, false);
    if (index == BITMAP_ERROR) {
        return BITMAP_ERROR;
    }

    for (size_t i = 0; i < SECTORS_PER_PAGE; i++) {
        block_write(swap_block, index * SECTORS_PER_PAGE + i,
                    (uint8_t *)kaddr + i * BLOCK_SECTOR_SIZE);
    }

    lock_release(&swap_lock);
    return index;
}
