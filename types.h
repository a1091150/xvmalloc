#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */

#if BITS_PER_LONG == 32
#define _BITOPS_LONG_SHIFT 5
#elif BITS_PER_LONG == 64
#define _BITOPS_LONG_SHIFT 6
#else
#error "Unexpected BITS_PER_LONG"
#endif

#ifndef _TYPES_H
#define _TYPES_H
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint16_t u16;
typedef uint8_t u8;
typedef uint64_t u64;
typedef uint32_t u32;
typedef unsigned long ulong;

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely_notrace(x) likely(x)
#define unlikely_notrace(x) unlikely(x)

#define DIV_ROUND_UP(n, d) (((n) + (d) -1) / (d))
#define roundup(x, y)                    \
    ({                                   \
        typeof(y) __y = y;               \
        (((x) + (__y - 1)) / __y) * __y; \
    })

#define INLINE static inline __attribute__((always_inline))

INLINE u32 ALIGN(u32 size, u32 align)
{
    const u32 mask = align - 1;
    return (size + mask) & ~mask;
}

INLINE u32 bitmap_ffs(u32 x)
{
    u32 i = (u32) __builtin_ffs((u32) x);
    return i - 1U;
}

#define __ffs(x) (bitmap_ffs(x))

#define PAGE_SIZE (4096UL)
#define PAGE_SHIFT (12)
#define BIT(nr) (1UL << (nr))
#endif