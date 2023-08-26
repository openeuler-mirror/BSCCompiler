struct a {
  short b;
  int c;
  unsigned d;
  int e;
  short f;
} __attribute__((aligned));
#pragma pack(1)
struct g {
  unsigned b : 1;
  struct a c;
};
#pragma pack()
struct {
  struct g d;
} h = {0, 5, 1048, 8, 5, 2};
struct a i;
int main() {
  struct a j = h.d.c;
  i = j;
  printf("%d\n", i.f);
  return 0;
}
