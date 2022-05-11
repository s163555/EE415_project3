#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "vm/vpage.h"
#include "threads/palloc.h"

struct list lru_list;
struct lock lru_list_lock;
struct list_elem *lru_ptr;

void lru_init(void);
void lru_insert(struct page* page);
void lru_remove(struct page* page);
struct page* alloc_page(enum palloc_flags flags);
void free_page(void *kaddr);
void lru_free_page(struct page *); 
void try_to_free_pages(enum palloc_flags flags);

static struct list_elem* get_next_lru_clock(void);

#endif 