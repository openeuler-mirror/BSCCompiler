#define min(a, b)                                                              \
  {                                                                            \
    __typeof__(a) _a = a;                                                      \
    __typeof__(b) _b = b;                                                      \
    _a < _b ? _a : _b;                                                         \
  }

__attribute__ ((noinline))
void test(long long *arr, long long *a, int cond, int start) {
  // if you change the `(min(-2147483647 - 1, 0)) + 2147483647 + 2` compute order,
  // you will see signed integer overflow which is undefined, and that may cause
  // following optimizations get wrong results.
  for (int i = start; i < cond; i += (min(-2147483647 - 1, 0)) + 2147483647 + 2) {
    (*a) += arr[i];
  }
}

int main() {
  long long arr[50];
  long long res;
  test(arr, &res, 50, 10);
  return 0;
}
