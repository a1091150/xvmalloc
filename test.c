#include <assert.h>
#include <errno.h>
#include <sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

/* tlsf.h from jserv/tlsf-bsd */
#include "tlsf.h"
#include "xvmalloc.h"
#include "xvmalloc_int.h"

/* Memory for page array */
static size_t MAX_PAGES;
void *start_addr = NULL;

/* A set bit represents page whether in-use or free.*/
void set_page_unused(int page_index);
int64_t find_page_unused();
void set_page_used(int page_index);
static size_t pages_used_size = 0;
static u64 *pages_used = NULL;

/* Copy form linux/bitops.h */
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

/* Get memory address from page */
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
    size_t addr = (size_t) start_addr;
    addr = addr + page_index * PAGE_SIZE;
    return (void *) addr;
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
    u64 set_bit = (1UL << 63) >> rem;
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

/* benchmark from jserv/tlsf-bsd */
static void usage(const char *name)
{
    printf(
        "run a malloc benchmark.\n"
        "usage: %s [-s blk-size|blk-min:blk-max] [-l loop-count] "
        "[-n num-blocks] [-c]\n",
        name);
    exit(-1);
}

/* Parse a size argument, which is either an integer or two integers separated
 * by a colon, denoting a range.
 */
static void parse_size_arg(const char *arg,
                           const char *exe_name,
                           size_t *blk_min,
                           size_t *blk_max)
{
    char *endptr;
    *blk_min = (size_t) strtol(arg, &endptr, 0);

    if (errno)
        usage(exe_name);

    if (endptr && *endptr == ':') {
        *blk_max = (size_t) strtol(endptr + 1, NULL, 0);
        if (errno)
            usage(exe_name);
    }

    if (*blk_min > *blk_max)
        usage(exe_name);
}

/* Parse an integer argument. */
static size_t parse_int_arg(const char *arg, const char *exe_name)
{
    long ret = strtol(arg, NULL, 0);
    if (errno)
        usage(exe_name);

    return (size_t) ret;
}

/* Get a random block size between blk_min and blk_max. */
static size_t get_random_block_size(size_t blk_min, size_t blk_max)
{
    if (blk_max > blk_min)
        return blk_min + ((size_t) rand() % (blk_max - blk_min));
    return blk_min;
}

static void run_alloc_benchmark(size_t loops,
                                size_t blk_min,
                                size_t blk_max,
                                void **blk_array,
                                u32 *blk_offset_array,
                                size_t num_blks,
                                bool clear)
{
    struct xv_pool *pool = xv_create_pool();
    assert(pool);
    while (loops--) {
        size_t next_idx = (size_t) rand() % num_blks;
        size_t blk_size = get_random_block_size(blk_min, blk_max);
        if (blk_array[next_idx]) {
            struct page *page = blk_array[next_idx];
            u32 offset = blk_offset_array[next_idx];
            xv_free(pool, page, offset);
            blk_array[next_idx] = NULL;
            blk_offset_array[next_idx] = 0;
        } else {
            struct page *page;
            u32 offset;
            int r = xv_malloc(pool, blk_size, &page, &offset);
            if (!r) {
                blk_array[next_idx] = page;
                blk_offset_array[next_idx] = offset;
            }
        }

        if (clear && blk_array[next_idx]) {
            char *addr = blk_array[next_idx];
            u32 offset = blk_offset_array[next_idx];
            memset(addr + offset, 0, blk_size);
        }
    }

    for (size_t i = 0; i < num_blks; i++) {
        if (blk_array[i]) {
            struct page *page = blk_array[i];
            u32 offset = blk_offset_array[i];
            xv_free(pool, page, offset);
        }
    }
}

int bench_main(int argc, char **argv)
{
    size_t blk_min = 512, blk_max = 512, num_blks = 10000;
    size_t loops = 10000000;
    bool clear = false;
    int opt;

    while ((opt = getopt(argc, argv, "s:l:r:t:n:b:ch")) > 0) {
        switch (opt) {
        case 's':
            parse_size_arg(optarg, argv[0], &blk_min, &blk_max);
            break;
        case 'l':
            loops = parse_int_arg(optarg, argv[0]);
            break;
        case 'n':
            num_blks = parse_int_arg(optarg, argv[0]);
            break;
        case 'c':
            clear = true;
            break;
        case 'h':
            usage(argv[0]);
            break;
        default:
            usage(argv[0]);
            break;
        }
    }

    blk_max = blk_max > 4088 ? 4088 : blk_max;

    void **blk_array = (void **) calloc(num_blks, sizeof(void *));
    void *blk_offset_array = calloc(num_blks, sizeof(u32));
    assert(blk_array);
    assert(blk_offset_array);

    struct timespec start, end;

    int err = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    assert(err == 0);

    printf("blk_min=%zu to blk_max=%zu\n", blk_min, blk_max);

    run_alloc_benchmark(loops, blk_min, blk_max, blk_array, blk_offset_array,
                        num_blks, clear);

    err = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    assert(err == 0);
    free(blk_array);

    double elapsed = (double) (end.tv_sec - start.tv_sec) +
                     (double) (end.tv_nsec - start.tv_nsec) * 1e-9;

    struct rusage usage;
    err = getrusage(RUSAGE_SELF, &usage);
    assert(err == 0);

    /* Dump both machine and human readable versions */
    printf(
        "%zu:%zu:%zu:%u:%lu:%.6f: took %.6f s for %zu malloc/free\nbenchmark "
        "loops of %zu-%zu "
        "bytes.  ~%.3f us per loop\n",
        blk_min, blk_max, loops, clear, usage.ru_maxrss, elapsed, elapsed,
        loops, blk_min, blk_max, elapsed / (double) loops * 1e6);

    return 0;
}


int main(int argc, char **argv)
{
    MAX_PAGES = 20 * TLSF_MAX_SIZE / PAGE_SIZE;
    // MAX_PAGES = 64;
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
    bench_main(argc, argv);
    return 0;
}