#ifndef IO_H
#define IO_H

#include <stdint.h>

static inline uint8_t inb(uint16_t addr) {
  uint8_t out;
  __asm__ __volatile__("inb %1, %0" : "=a"(out) : "d"(addr));
  return out;
}

static inline void outb(uint16_t addr, uint8_t val) {
  __asm__ __volatile__("outb %1, %0" : : "d"(addr), "a"(val));
}

static inline uint16_t inw(uint16_t addr) {
  uint16_t out;
  __asm__ __volatile__("inw %1, %0" : "=a"(out) : "d"(addr));
  return out;
}

static inline void outw(uint16_t addr, uint16_t val) {
  __asm__ __volatile__("outw %1, %0" : : "d"(addr), "a"(val));
}

static inline uint32_t inl(uint16_t addr) {
  uint32_t out;
  __asm__ __volatile__("inl %1, %0" : "=a"(out) : "d"(addr));
  return out;
}

static inline void outl(uint16_t addr, uint32_t val) {
  __asm__ __volatile__("outl %1, %0" : : "d"(addr), "a"(val));
}

static inline void io_wait(void) { outb(0x80, 0); }

#endif /* IO_H */
