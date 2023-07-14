#include <stdint.h>

#pragma pack(push)
#pragma pack(1)
struct S0 {
  signed f0 : 31;
  signed f1 : 24;
  unsigned f2 : 26;
  uint32_t f3;
  unsigned f4 : 31;
  unsigned f5 : 15;
} __attribute__((aligned(128))) __attribute__((aligned(2)));
#pragma pack(pop)

union U1 {
  uint8_t f0;
  int32_t f1;
  int64_t f2;
  const uint32_t f3;
} __attribute__((aligned(32)));

struct S0 g_154[10][4];
int64_t func_29();
__attribute__((noinline)) union U1 func_12(union U1 e) {
  union U1 f = {};
  func_29(e, g_154[8][3]);
  return f;
}

__attribute__((noinline)) int64_t func_29() {
  uint64_t g = 4073709551615;
  return g;
}

int main() {
  union U1 e;
  func_12(e);
  return 0;
}