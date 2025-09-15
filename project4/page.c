#include "vm/page.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/kernel/hash.h"
#include <string.h>
#include "threads/palloc.h"


unsigned page_hash(const struct hash_elem *e, void *aux UNUSED) {
    const struct spt_entry *entry = hash_entry(e, struct spt_entry, elem);
    return hash_bytes(&entry->vaddr, sizeof entry->vaddr);
}

bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
    const struct spt_entry *entry_a = hash_entry(a, struct spt_entry, elem);
    const struct spt_entry *entry_b = hash_entry(b, struct spt_entry, elem);
    return entry_a->vaddr < entry_b->vaddr;
}

void spt_init(struct hash *spt) {
    if (spt == NULL) return;
    hash_init(spt, page_hash, page_less, NULL);
}

bool spt_insert(struct spt_entry *entry, struct hash *spt) {
    if (spt == NULL || entry == NULL) return false;
    if (spt_find(spt, entry->vaddr) != NULL) return false; 
    struct hash_elem *result = hash_insert(spt, &entry->elem);
    return result == NULL;
}

bool spt_delete(struct spt_entry *entry, struct hash *spt) {
    if (spt == NULL || entry == NULL) return false;
    struct hash_elem *result = hash_delete(spt, &entry->elem);
    if (result != NULL) {
        free(entry);
        return true;
    }
    return false;
}

void spt_destroy(struct hash *spt) {
    if (spt == NULL) return;
    hash_destroy(spt, spt_entry_destroy_func);
}

void spt_entry_destroy_func(struct hash_elem *elem, void *aux UNUSED) {
    struct spt_entry *entry = hash_entry(elem, struct spt_entry, elem);
    void *kaddr;

    if (entry -> is_loaded) {
        kaddr = pagedir_get_page(thread_current()->pagedir, entry->vaddr);
        if (kaddr != NULL) {
            palloc_free_page(kaddr);
            pagedir_clear_page(thread_current()->pagedir, entry->vaddr);
        }
    }

    free(entry);
}

struct spt_entry *spt_find(struct hash *spt, void *vaddr) {
    if (spt == NULL || vaddr == NULL) return NULL;

    struct spt_entry temp_entry;
    temp_entry.vaddr = pg_round_down(vaddr); 
    struct hash_elem *elem = hash_find(spt, &temp_entry.elem);

    return elem != NULL ? hash_entry(elem, struct spt_entry, elem) : NULL;
}

bool load_page(void *kaddr, struct spt_entry *entry) {
    if (kaddr == NULL || entry == NULL) return false;

    switch (entry->type) {
        case BIN:
        case FILE: {
            int bytes_read = file_read_at(entry->file, kaddr, entry->read_bytes, entry->offset);
            if (bytes_read != (int)entry->read_bytes) return false;
            memset(kaddr + entry->read_bytes, 0, entry->zero_bytes);
            break; 
        }
        default:
            return false;
    }

    return true;
}
