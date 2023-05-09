struct X {
  int a;
};

struct X funcA(int i) {
  struct X x = {.a = i};
  return x;
}
int i = 10;
int main() {
  funcA(i).a;
  return 0;
}