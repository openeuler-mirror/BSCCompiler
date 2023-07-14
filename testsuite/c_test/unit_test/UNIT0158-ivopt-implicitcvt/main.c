long long a[8];

__attribute__ ((noinline))
void test(int step, int cond) {
  // we will generate add with implicit conversion for "i",
  // this may cause unexpected optimization behavior.
  for (int i = ((unsigned long long)cond >= 0); i < 6; i += step + 1) {
    a[i] = 1;
  }
}

int main() {
  test(1, 1);
  if (a[7] != 0) {
    abort();
  }
  return 0;
}
