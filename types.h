#ifndef _ASM_GENERIC_ERRNO_BASE_H
#define _ASM_GENERIC_ERRNO_BASE_H

#define EPERM 1    /* Operation not permitted */
#define ENOENT 2   /* No such file or directory */
#define ESRCH 3    /* No such process */
#define EINTR 4    /* Interrupted system call */
#define EIO 5      /* I/O error */
#define ENXIO 6    /* No such device or address */
#define E2BIG 7    /* Argument list too long */
#define ENOEXEC 8  /* Exec format error */
#define EBADF 9    /* Bad file number */
#define ECHILD 10  /* No child processes */
#define EAGAIN 11  /* Try again */
#define ENOMEM 12  /* Out of memory */
#define EACCES 13  /* Permission denied */
#define EFAULT 14  /* Bad address */
#define ENOTBLK 15 /* Block device required */
#define EBUSY 16   /* Device or resource busy */
#define EEXIST 17  /* File exists */
#define EXDEV 18   /* Cross-device link */
#define ENODEV 19  /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21  /* Is a directory */
#define EINVAL 22  /* Invalid argument */
#define ENFILE 23  /* File table overflow */
#define EMFILE 24  /* Too many open files */
#define ENOTTY 25  /* Not a typewriter */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG 27   /* File too large */
#define ENOSPC 28  /* No space left on device */
#define ESPIPE 29  /* Illegal seek */
#define EROFS 30   /* Read-only file system */
#define EMLINK 31  /* Too many links */
#define EPIPE 32   /* Broken pipe */
#define EDOM 33    /* Math argument out of domain of func */
#define ERANGE 34  /* Math result not representable */

#endif


#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */

#if BITS_PER_LONG == 32
# define _BITOPS_LONG_SHIFT 5
#elif BITS_PER_LONG == 64
# define _BITOPS_LONG_SHIFT 6
#else
# error "Unexpected BITS_PER_LONG"
#endif

#ifndef _TYPES_H
#define _TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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