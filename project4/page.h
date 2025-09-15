#ifndef VM_PAGE_H
#define VM_PAGE_H


#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "devices/block.h"
#include "threads/synch.h"

enum page_type {
    BIN, FILE, ANON
};

struct spt_entry {
    void *vaddr;                /* Virtual address */
    enum page_type type;        /* Type of page */
    bool writable;              /* Writable */
    bool is_loaded;

    /*Type 0, 1*/
    struct file *file;          /* Mapping file */
    size_t offset;              /* Offset in the file */
    size_t read_bytes;          /* Bytes to read from file */
    size_t zero_bytes;          /* Bytes to zero out */
    
    /*Type 2, ANON */
    size_t swap_index;          /* Swap index */

    struct hash_elem elem;      /* Hash table element */
};

struct page {
  //struct frame *frame;   /* Back reference for frame */
  void *kaddr;            /* Address in terms of kernel space */
  struct spt_entry *spte; /* Supplemental page table entry */
  struct thread *thread; /* Back reference for thread */
};


unsigned page_hash(const struct hash_elem *e, void *aux UNUSED);
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);
void spt_init(struct hash *spt);
bool spt_insert(struct spt_entry *entry, struct hash *spt);
bool spt_delete(struct spt_entry *entry, struct hash *spt);
void spt_destroy(struct hash *spt);
void spt_entry_destroy_func(struct hash_elem *elem, void *aux UNUSED);
struct spt_entry *spt_find(struct hash *spt, void *vaddr);
bool load_page(void *kaddr, struct spt_entry *entry);    


#endif