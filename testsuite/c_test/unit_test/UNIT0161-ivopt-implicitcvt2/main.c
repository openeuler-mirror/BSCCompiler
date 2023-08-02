#define max(a, b)                                                              \
  ({                                                                           \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    _a > _b ? _a : _b;                                                         \
  })

short out[25][22];

__attribute__((noinline)) 
void test(int start, int cond, long in1[25][22], long in2[25][22]) {
  // when promote the type of `step` and `i` to long, be careful with the cvt
  int step = ((int)max(!(_Bool)(cond ? 5 : 3), 4294967289U) + 8);
  for (int i = start; i < 24; i += step) {
    for (unsigned long j = 0; j < 20; j += 1) {
      out[i][j] = in2[i][j] <= max(255 | in1[i][j], in1[i][j]);
    }
  }
}

long arr1[25][22];
long arr2[25][22];

int main() {
  test(8, 8, arr1, arr2);
  return 0;
}
