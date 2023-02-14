#include <stdio.h>
#include <stdint.h>

uint64_t c = 0xFF00000000;
int64_t d = 255;

void func_2();

__attribute__((noinline))
uint64_t foo(uint64_t b) {
  return b + 7731735620;
}

int main() {
  func_2(foo(1));
  return 0;
}

__attribute__((noinline))
void func_2(uint32_t b) {
  if ((b & c) > 0) {
    printf("%d\n", 1);
  } else {
    printf("%d\n", 0);
  } 
}
