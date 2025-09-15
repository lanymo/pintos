#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "devices/block.h"

/* Sectors per page for the swap partition */
#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

void swap_init(void);
void swap_in(size_t index, void *kaddr);
size_t swap_out(void *kaddr);

#endif /* VM_SWAP_H */
