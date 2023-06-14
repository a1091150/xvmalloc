#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "tlsf.h"
#include "xvmalloc.h"
#include "xvmalloc_int.h"

void set_page_unused(int page_index);
int64_t find_page_unused();
void set_page_used(int page_index);

static size_t MAX_PAGES;
void *start_addr = NULL;

static size_t pages_used_size = 0;
static u64 *pages_used = NULL;

void __clear_bit(long nr, unsigned long *addr)
{
    addr[nr >> _BITOPS_LONG_SHIFT] &= ~(1UL << nr);
}

bool test_bit(long nr, unsigned long *addr)
{
    return (addr[nr >> _BITOPS_LONG_SHIFT] & (1UL << nr)) != 0;
}

void __set_bit(long nr, unsigned long *addr)
{
    addr[nr >> _BITOPS_LONG_SHIFT] |= 1UL << (nr & (BITS_PER_LONG - 1));
}

void *kmap_local_page(struct page *page)
{
    return page;
}

void kunmap_atomic(void *ptr)
{
    /* Do nothing */
}

void *alloc_page()
{
    int64_t page_index = find_page_unused();
    if (page_index == -1)
        return NULL;

    set_page_used(page_index);
    char *addr = start_addr;
    addr = addr + page_index * PAGE_SIZE;
    return addr;
}

void __free_page(void *ptr)
{
    char *p = ptr;
    char *base = start_addr;
    int page_index = (p - base) / PAGE_SIZE;
    set_page_unused(page_index);
}


void *kalloc(size_t size)
{
    void *p = malloc(size);
    if (!p)
        return NULL;
    memset(p, 0, size);
    return p;
}

void kfree(void *ptr)
{
    free(ptr);
}

int64_t find_page_unused()
{
    for (int i = 0; i < pages_used_size; i++) {
        u64 not_used = ~pages_used[i];
        if (not_used) {
            return __builtin_clzll(not_used) + (i << 6);
        }
    }
    return -1;
}

void set_page_used(int page_index)
{
    u64 div = page_index >> 6;
    u64 rem = page_index & ((1 << 6) - 1);
    u64 set_bit = (1UL << 63) >> rem;
    pages_used[div] |= set_bit;
}

void set_page_unused(int page_index)
{
    u64 div = page_index >> 6;
    u64 rem = page_index & ((1 << 6) - 1);
    u64 set_bit = rem ? ((1UL << 63) >> (rem - 1)) : (1UL << 63);
    pages_used[div] &= ~set_bit;
}

void test_alloc()
{
    struct xv_pool *pool = xv_create_pool();
    struct page *page;
    u32 offset;
    const u32 size = XV_MAX_ALLOC_SIZE - 4;
    int x = xv_malloc(pool, size, &page, &offset);
    if (page) {
        xv_free(pool, page, offset);
    }
}


int main()
{
    MAX_PAGES = 20 * TLSF_MAX_SIZE / PAGE_SIZE;
    start_addr = mmap(0, MAX_PAGES * PAGE_SIZE, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);

    u64 div = (MAX_PAGES >> 6);
    u64 rem = MAX_PAGES & ((1 << 6) - 1);
    pages_used_size = div + !!rem;
    pages_used = malloc(sizeof(u64) * pages_used_size);
    memset(pages_used, 0, sizeof(u64) * pages_used_size);
    if (rem) {
        pages_used[pages_used_size - 1] = ~((1L << 63) >> (rem - 1));
    }

    if (!start_addr || !pages_used)
        return 0;

    test_alloc();

    return 0;
}