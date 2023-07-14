#pragma pack(push)
#pragma pack(1)
struct S0 {
  signed f0 : 16;
  volatile unsigned f1 : 5;
  signed f2 : 19;
  unsigned f3 : 19;
  signed f4 : 22;
  volatile signed f5 : 21;
  unsigned f6 : 31;
  signed f7 : 24;
  signed f8 : 28;
} __attribute__((aligned(16), warn_if_not_aligned(8), unused)) __attribute__((deprecated));
#pragma pack(pop)

struct S0 g_569 = {4, 2, 122, 200, -1797, 502, 21441, -2866, -6088};

#pragma pack(1)
struct {
  signed a;
  unsigned : 19;
  signed : 22;
  signed : 21;
  unsigned : 31;
  signed : 24;
  signed b : 28;
} e;

#pragma pack(1)
struct {
  unsigned : 29;
  signed a : 30
} b;

int main(int argc, char *argv[]) {
  g_569.f8 &= 0x5431BED5L;
  printf("g_569.f8 = %x\n", g_569.f8);

  e.b = 70368983;
  printf("e.b = %x\n", e.b);

  b.a = 256611831;
  printf("b.a = %x\n", b.a);
  return 0;
}