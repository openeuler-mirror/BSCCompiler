struct A {
  int f1;
  long f2;
  long f3[100];
  long f4;
  long f5;
};

int main() {
  static struct A a = {1234567};
  if (a.f1 != 1234567) {
    abort();
  }
  return 0;
}
