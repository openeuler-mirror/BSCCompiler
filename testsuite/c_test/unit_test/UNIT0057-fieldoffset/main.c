#pragma pack(1)
struct S1 {
  volatile signed f0;
  volatile signed f2;
  unsigned f4;
  volatile signed f5;
};
#pragma pack()
struct S2 {
  const unsigned f7 : 3;
  struct S1 f8;
};
static volatile struct S2 a = {0, 3, 1, {4, 7, 9 - 7, 1 - 9}, 1};
int main() {
  if (a.f8.f0 != 3) {
    abort();
  }
}
