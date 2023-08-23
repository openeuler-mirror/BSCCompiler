int t;
int check;

__attribute__ ((noinline))
void foo(long long x) {
  int a = 0;
  if (0) {}
  else {
    // check here
    check = -((long long)((int)x == 0) == (long long)(a / t));
  }
}

int main() {
  foo(3);
  if (check != -1) {
    abort();
  }
  return 0;
}
