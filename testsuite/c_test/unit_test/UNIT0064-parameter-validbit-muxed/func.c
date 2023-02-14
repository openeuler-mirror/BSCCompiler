#include <stdio.h>
#include <stdint.h>

uint64_t c = 0xFF00000000;
int64_t d = 255;

__attribute__((noinline))
void func_1(uint8_t a) {
  if (a != d) {
    printf("%d\n", 1);
  } else {
    printf("%d\n", 0);
  }
}

__attribute__((noinline))
void func_2(uint32_t b) {
  if ((b & c) > 0) {
    printf("%d\n", 1);
  } else {
    printf("%d\n", 0);
  } 
}
