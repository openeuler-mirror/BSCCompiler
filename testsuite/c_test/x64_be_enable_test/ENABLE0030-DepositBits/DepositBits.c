struct __attribute__((packed)) S {
  int s : 12;
  int t : 16;
};

int main() {
  struct S i = {-0x111, 0x2222}, j = {0x00, 0x00};
  j.s = 0x333;
  j.t = 0x4444;
  printf("%x,%x,%x,%x\n", i.s, i.t, j.s, j.t);
  return 0;
}