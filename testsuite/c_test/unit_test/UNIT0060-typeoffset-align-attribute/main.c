struct A {
  int a;
  __attribute__((aligned(8))) int b;
  int c;
  __attribute__((aligned(8))) int d;
};

int main() {
  struct A a;
  if ((unsigned long)&a.b - (unsigned long)&a != 8) {
    abort();
  }
  if ((unsigned long)&a.c - (unsigned long)&a != 12) {
    abort();
  }
  if ((unsigned long)&a.d - (unsigned long)&a != 16) {
    abort();
  }
}

