#include <stdio.h>
#include <stdint.h>

#pragma pack(push)
#pragma pack(1)
struct subFoo {
  int16_t f0;
  int8_t f1;
  volatile uint8_t f2;
};
#pragma pack(pop)

struct Foo1 {
  volatile uint64_t f0;
  struct subFoo f1;
  uint32_t f2;
  signed f3 : 22;
  const uint32_t f4;
  const struct subFoo f5;
  int64_t f6;
  volatile int16_t f7;
} __attribute__ ((packed));

#pragma pack(push)
#pragma pack(2)
struct Foo2 {
  volatile uint64_t f0;
  struct subFoo f1;
  signed f2 : 15  __attribute__ ((packed)); 
  signed f3 : 18  __attribute__ ((packed)); 
  const uint32_t f4;
  const struct subFoo f5;
};
#pragma pack(pop)

struct Foo3 {
  volatile uint64_t f0;
  struct subFoo f1;
  signed f2 : 18;
  char f3 : 3;
  const uint32_t f4;
} __attribute__ ((packed));


struct Foo1 s1[1][2] = {{{0x55D2D43A6B74DC4BLL, {0L, 0xF8L, 0xF9L}, 6UL, 544, 0xD114BA5AL, {0xD87EL, 0x2AL, 8UL}, 0x2BD476EEB3D548DBLL, 0xC3D8L},
                         {0x55D2D43A6B74DC4BLL, {0L, 0xF8L, 0xF9L}, 6UL, 544, 0xD114BA5AL, {0xD87EL, 0x2AL, 8UL}, 0x2BD476EEB3D548DBLL, 0xC3D8L}}};
struct Foo2 s2[1][2] = {{{0x55D2D43A6B74DC4BLL, {0L, 0xF8L, 0xF9L}, 0x7fff, 0x3ffff, 0xD114BA5AL, {0xD87EL, 0x2AL, 8UL}},
                         {0x55D2D43A6B74DC4BLL, {0L, 0xF8L, 0xF9L}, 0x7fff, 0x3ffff, 0xD114BA5AL, {0xD87EL, 0x2AL, 8UL}}}};
struct Foo3 s3[1][2] = {{{0x55D2D43A6B74DC4BLL, {0L, 0xF8L, 0xF9L}, 0x3ffff, -3, 0xD114BA5AL},
                         {0x55D2D43A6B74DC4BLL, {0L, 0xF8L, 0xF9L}, 0x3ffff, -3, 0xD114BA5AL}}};
int64_t *a = &s1[0][1].f6;
int8_t *b = &s2[0][1].f5.f1;
uint32_t *c = &s3[0][1].f4;

int main() {
  if (*a != s1[0][1].f6) {
    printf("%lx\n", *a);
    printf("%lx\n", s1[0][1].f6);	  
    return 1;
  }
  if (*b != s2[0][1].f5.f1) {
    printf("%x\n", *b);
    printf("%x\n", s2[0][1].f5.f1);
    return 1;
  }
  if (*c != s3[0][1].f4) {
    printf("%ux\n", *c);
    printf("%ux\n", s3[0][1].f4);
    return 1;
  }
  return 0;
}
