#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "filesys/file.h"

struct page* alloc_page(enum palloc_flags flags){
    struct page *page;
    lock_acquire(&lru_list_lock);
    void *kaddr = palloc_get_page(flags);
    while (kaddr == NULL)
    {
        try_to_free_pages(flags);
        kaddr = palloc_get_page(flags);
    }
    page = (struct page *)malloc(sizeof(struct page));
    page->kaddr = kaddr;
    page->thread = thread_current();
    lru_insert(page);
    lock_release(&lru_list_lock);
    return page;
}

void free_page(void *kaddr){
    struct list_elem *e;
    struct page *page = NULL;
    struct page *candi_page;
    lock_acquire(&lru_list_lock);
    for (e=list_begin(&lru_list); e!=list_end(&lru_list); e=list_next(e))
    {
        candi_page = list_entry(e, struct page, lru);
        if (candi_page->kaddr == kaddr)
        {
            page = candi_page;
            break;
        }
    }
    if (page != NULL) {
        lru_free_page(page);
    }
    lock_release(&lru_list_lock);
}

static struct list_elem* get_next_lru_clock(void){
    struct list_elem *retval;
    if (list_empty(&lru_list)){
        retval = NULL;
    }
    else if (lru_ptr == NULL){
        retval = list_begin(&lru_list);
    }
    else if (lru_ptr == list_end(&lru_list)){
        retval = list_begin(&lru_list);
    }else if ( list_next(lru_ptr) == list_end(&lru_list)){
        retval = list_begin(&lru_list); 
    }
    else{
        retval = list_next(lru_ptr);
    }
    return retval;
}

void lru_init(void){
    list_init(&lru_list);
    lock_init(&lru_list_lock);
    lru_ptr = NULL;
}

void lru_insert(struct page* page){
    list_push_back(&lru_list, &page->lru);
}

void lru_remove(struct page* page){
    if (lru_ptr == &page->lru){
        lru_ptr = list_next(lru_ptr);
    }
    list_remove (&page->lru);
}

void lru_free_page(struct page* page){
    lru_remove(page);
    pagedir_clear_page(page->thread->pagedir, pg_round_down(page->vme->vaddr));
    palloc_free_page(page->kaddr);
    free(page);
}

void try_to_free_pages(enum palloc_flags flags){
    struct page *page;
    struct page *victim;

    lru_ptr = get_next_lru_clock();
    page = list_entry(lru_ptr, struct page, lru);
    while(page->vme->pinned || pagedir_is_accessed(page->thread->pagedir, page->vme->vaddr)){
        pagedir_set_accessed(page->thread->pagedir, page->vme->vaddr, false);
        lru_ptr = get_next_lru_clock();
        page = list_entry(lru_ptr, struct page, lru);
    }
    victim = page;

    bool dirty_flag = pagedir_is_dirty(victim->thread->pagedir, victim->vme->vaddr);     
    switch(victim->vme->type){
        case VM_BIN:
            if (dirty_flag) {victim->vme->swap_slot = swap_out(victim->kaddr); victim->vme->type=VM_ANON;}
            break;
        case VM_FILE:
            if(dirty_flag) {file_write_at(victim->vme->file, victim->vme->vaddr, victim->vme->read_bytes, victim->vme->offset);}
            break;
        case VM_ANON:
            victim->vme->swap_slot = swap_out(victim->kaddr);
            break;
    }
    victim->vme->is_loaded = false;
    lru_free_page(victim);
}
