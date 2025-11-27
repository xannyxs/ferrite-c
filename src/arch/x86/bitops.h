#ifndef __BITOPS_H__
#define __BITOPS_H__

#ifdef CONFIG_SMP
#    define LOCK_PREFIX "lock ; "
#    define SMPVOL volatile
#else
#    define LOCK_PREFIX ""
#    define SMPVOL
#endif

__attribute__((always_inline)) static inline int
atomic_set_bit(int nr, SMPVOL void* addr)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX "btsl %2,%1\n \
                         sbbl %0,%0"
                         : "=r"(oldbit), "=m"(*(char(*)[])addr)
                         : "ir"(nr));
    return oldbit;
}

__attribute__((always_inline)) static inline int
atomic_clear_bit(int nr, SMPVOL void* addr)
{
    int oldbit;

    __asm__ __volatile__(LOCK_PREFIX "btrl %2,%1\n\tsbbl %0,%0"
                         : "=r"(oldbit), "=m"(*(char(*)[])addr)
                         : "ir"(nr));
    return oldbit;
}

#endif
