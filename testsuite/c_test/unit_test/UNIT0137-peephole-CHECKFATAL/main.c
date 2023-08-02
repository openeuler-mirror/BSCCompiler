#define c(a, b)                                                                \
  ({                                                                           \
    __typeof__(a) d = a;                                                       \
    __typeof__(b) e = b;                                                       \
    e ? d : e;                                                                 \
  })
void h(char f, _Bool g) {
  if (g == c(f, 10494309543783454886)) {
    unsigned short var_624 = 0;
  }
}
void main() {}