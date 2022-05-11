#ifndef VM_VPAGE_H
#define VM_VPAGE_H

#include <lib/kernel/hash.h>
#include <filesys/file.h>
#include "filesys/off_t.h"
#include <list.h>
#include <debug.h>
#include <stdint.h>

enum vme_type
  {
    VM_BIN,     
    VM_ANON,
    VM_FILE
  };

struct vm_entry{
    void *vaddr;
    bool writable;
    bool pinned;

    enum vme_type type; // type of vme
    bool is_loaded; // is loaded
    struct file* file; // file struct

    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;

    size_t swap_slot;
    struct list_elem mmap_elem;
    struct hash_elem elem;
};

struct mmap_file{
    int mapid;
    struct file* file;
    struct list_elem elem;
    struct list vme_list;

};

struct page {
    void *kaddr; // physcal addr of page
    struct vm_entry *vme; // ref. to the vpage object which physical page is mapped
    struct thread *thread; // ref. to belonging thread
    struct list_elem lru;// LRU list linking field
};

/* SWAP */
void vpage_init(struct hash *);
void vpage_action(struct hash_elem *, void*);
bool insert_vme(struct hash *, struct vm_entry *);
bool delete_vme(struct hash *, struct vm_entry *);
struct vm_entry *find_vme(void *);
void vm_destroy(struct hash *);
void do_munmap(struct mmap_file *,struct list_elem *);
void all_munmap(void);
static bool vpage_less(const struct hash_elem *a, const struct hash_elem *b, void* aux UNUSED);

#endif
