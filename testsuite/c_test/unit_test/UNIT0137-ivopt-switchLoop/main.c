struct A {
  short a;
};

struct A *x;
int b;

__attribute__ ((noinline))
void func() {
  while(1) {
    x->a++;
    switch(b) {
      case 7:
      case 9:
        return;
      default:
        continue;
    }
  }
}

int main() {
  func();
  return 0;
}
