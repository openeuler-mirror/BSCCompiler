unsigned long long g = 0;

__attribute__ ((noinline))
void test() {
  unsigned int a = 4294967290UL; // -8
  for (long long i = 0; i <= 500; i += 1) {
    // do not change `a > -9` to `a - a_init > -9 - a_init`
    g |= (a > -9);
    a++;
  }
}

int main() {
  test();
  if (g != 1) {
    abort();
  }
  return 0;
}
