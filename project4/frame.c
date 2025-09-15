#include "vm/frame.h"
#include "vm/page.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/kernel/list.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"

static struct list lru_list;         
static struct list_elem *lru_clock; 
static struct lock frame_lock;      

extern struct lock file_lock;


void frame_table_init(void) {
    list_init(&lru_list);
    lru_clock = NULL;
    lock_init(&frame_lock);
}


struct frame *frame_allocate(struct page *page) {
    if(page == NULL) {
        return NULL;
    }

    lock_acquire(&frame_lock);

    void *kaddr = palloc_get_page(PAL_USER);
    if (kaddr == NULL) {
    
        struct frame *victim = frame_evict();
        if (victim == NULL) {
            lock_release(&frame_lock);
            return NULL;
        }
        kaddr = victim->kaddr;
        victim->page->kaddr = kaddr;
        victim->page = page;
    }

    
    struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
    if (frame == NULL) {
        palloc_free_page(kaddr); 
        lock_release(&frame_lock);
        return NULL;
    }

    frame->kaddr = kaddr;
    frame->page = page;
    list_push_back(&lru_list, &frame->elem);

    lock_release(&frame_lock);
    return frame;
}


void frame_free(void *kva) {
    if(kva == NULL) {
        return;
    }

    lock_acquire(&frame_lock);

    struct list_elem *e;
    for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e)) {
        struct frame *frame = list_entry(e, struct frame, elem);
        if (frame->kaddr == kva) {
            list_remove(&frame->elem);
            palloc_free_page(kva);
            free(frame);
            break;
        }
    }

    lock_release(&frame_lock);
}


struct frame *frame_evict(void) {
    if (list_empty(&lru_list)) {
        return NULL;
    }

    lock_acquire(&frame_lock);

    struct frame *victim = NULL;
    while (true) {
        if (lru_clock == NULL || lru_clock == list_end(&lru_list)) {
            lru_clock = list_begin(&lru_list);
        }
        struct frame *frame = list_entry(lru_clock, struct frame, elem);

        if (!pagedir_is_accessed(thread_current()->pagedir, frame->page->spte->vaddr)) {
            victim = frame;
            break;
        }

        pagedir_set_accessed(thread_current()->pagedir, frame->page->spte->vaddr, false);
        lru_clock = list_next(lru_clock);
    }

    if (pagedir_is_dirty(thread_current()->pagedir, victim->page->spte->vaddr) || victim->page->spte->type == ANON) {
        if (victim->page->spte->type == FILE) {
            lock_acquire(&file_lock);
            file_write_at(victim->page->spte->file, victim->kaddr, victim->page->spte->read_bytes, victim->page->spte->offset);
            lock_release(&file_lock);
        } else {
            victim->page->spte->swap_index = swap_out(victim->kaddr);
        }
    }

    pagedir_clear_page(thread_current()->pagedir, victim->page->spte->vaddr);
    victim->page->spte->is_loaded = false;

    lock_release(&frame_lock);
    return victim;
}

