#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "lib/kernel/list.h"
#include "lib/kernel/hash.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "vm/page.h"


struct frame {
    void *kaddr;                /* Address in terms of kernel space */
    struct page *page;          /* page */
    struct list_elem elem;      /* List element */
};


void frame_table_init(void);
struct frame *frame_allocate(struct page *page);
void frame_free(void *kva);
struct frame *frame_evict(void);

#endif 