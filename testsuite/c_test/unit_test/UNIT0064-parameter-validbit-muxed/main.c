#include <stdint.h>

extern void func_2(uint32_t b);
extern void func_1(uint8_t a);

__attribute__((noinline))
uint64_t foo(uint64_t b) {
  return b + 7731735620;
}

int main() {
  func_1(0xFFFF);
  func_2(foo(1));
  return 0;
}
