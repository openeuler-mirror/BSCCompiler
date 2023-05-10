#include <stdint.h>
uint32_t ui_0 = 0;
uint64_t u3 = 3;
uint64_t u4 = 5;
uint8_t u5 = 5;
uint32_t s8 = 6;
int64_t l9 = 1;
int64_t foo() {
  uint64_t *p_10 = &u4;
  for (*p_10 = 9; *p_10 <= 0; *p_10++) {
    uint8_t uc_15 = 1;
    uint32_t *p_17 = &s8;
    for (uc_15 = 3; uc_15 <= 3; uc_15 = 3) {
      for (*p_17 = 8; *p_17 <= 0; *p_17 = 4)
        ;
      uint32_t ui_20 = 0;
      for (u3 = 7; u3 <= 4; u3 = 4)
        for (ui_20 = 3; ui_20 <= 0; ui_20 = 3)
          ;
    labelA:;
    }
  }
  for (u3 = 1; u3 <= 8; u3 = 3) {
    int16_t s_12 = 0;
    int8_t c_13 = 3;
    for (c_13 = 5; l9 <= 6; l9 = 5) {
      uint16_t us_16 = 2;
      for (*p_10 = 1; *p_10 <= 70; u5 = 2) {
        uint32_t *p_18 = &ui_0;
        uint32_t **p_20 = &p_18;
        for (**p_20 = 2; **p_20 <= 5; **p_20 = 5)
          goto labelA;
      }
      for (us_16 = 7; us_16 <= 1; us_16 = 4)
        for (s_12 = 1; s_12 <= 9; s_12++)
          ;
    }
  }
}

struct a {
  signed b;
};
int16_t baz(signed x, int16_t y);
void bar(struct a d) {
  int16_t e[5];
  int f;
  for (f = 0; f < 5; f++)
    e[f] = baz(d.b, e[1]);
  for (;;)
    ;
}

int main() {}

