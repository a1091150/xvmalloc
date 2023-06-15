#ifndef _TEST_PAGE_H
#define _TEST_PAGE_H
#include <stdbool.h>
#include <stddef.h>
struct page {
    void *ptr;
};

void __clear_bit(long nr, unsigned long *addr);
bool test_bit(long nr, unsigned long *addr);
void __set_bit(long nr, unsigned long *addr);
void *kmap_local_page(struct page *page);
void kunmap_atomic(void *ptr);
void *alloc_page();
void __free_page(void *ptr);
void *kalloc(size_t size);
void kfree(void *ptr);
#endif