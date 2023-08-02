#include <stdint.h>
#include <limits.h>
int64_t var = 0x4dfbd6c3;
uint8_t shift(uint8_t x, int size) {
  return x >> size;
}
__attribute__((noinline))
uint8_t func(uint8_t x) {
  return x;
}

int main() {
  // CHECK:bl{{.*}}func
  // CHECK:uxtb
  // CHECK:adrp
  uint8_t a = shift(func(var), 6);
  printf("%d\n", a);
  return 0;
}
