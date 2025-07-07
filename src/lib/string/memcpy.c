#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n) {
  uint8_t *d = dest;
  const uint8_t *s = src;

  if (src || dest) {
    while (n) {
      n--;
      *d = *s;
      d++;
      s++;
    }
  }
  return (dest);
}
