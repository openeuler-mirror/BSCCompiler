extern int band32I(int a);
extern int band32I_2(int a);
extern int band32RR(int a, int b);

extern long long band64I(long long a);
extern long long band64I_2(long long a);
extern long long band64RR(long long a, long long b);

int main() {

  if((7 & 0x12)== band32I(7))
    printf("band32I pass.\n");
  else {
    printf("band32I false.\n");
    return 1;
  }
  if((7 & 0x112)== band32I_2(7))
    printf("band32I_2 pass.\n");
  else {
    printf("band32I_2 false.\n");
    return 1;
  }
  if((7 & 5)== band32RR(7, 5))
    printf("band32RR pass.\n");
  else {
    printf("band32RR false.\n");
    return 1;
  }
  if((0x1200000007 & 0x1200000015)== band64I(0x1200000007))
    printf("band64I pass.\n");
  else {
    printf("band64I false.\n");
    return 1;
  }
  if((0x1200000115 & 0x11200000127)== band64I_2(0x11200000127))
    printf("band64I_2 pass.\n");
  else {
    printf("band64I_2 false.\n");
    return 1;
  }
  if((0x1200000007 & 0x11200000127)== band64RR(0x1200000007, 0x11200000127))
    printf("band64RR pass.\n");
  else {
    printf("band64RR false.\n");
    return 1;
  }
  return 0;
}
