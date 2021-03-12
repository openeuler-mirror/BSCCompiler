extern int bior32I(int a);
extern int bior32I_2(int a);
extern int bior32RR(int a, int b);

extern long long bior64I(long long a);
extern long long bior64I_2(long long a);
extern long long bior64RR(long long a, long long b);

int main() {

  if((7 | 0x12)== bior32I(7))
    printf("bior32I pass.\n");
  else {
    printf("bior32I false.\n");
    return 1;
  }
  if((7 | 0x112)== bior32I_2(7))
    printf("bior32I_2 pass.\n");
  else {
    printf("bior32I_2 false.\n");
    return 1;
  }
  if((7 | 5)== bior32RR(7, 5))
    printf("bior32RR pass.\n");
  else {
    printf("bior32RR false.\n");
    return 1;
  }
  if((0x1200000007 | 0x1200000015)== bior64I(0x1200000007))
    printf("bior64I pass.\n");
  else {
    printf("bior64I false.\n");
    return 1;
  }
  if((0x1200000115 | 0x11200000127)== bior64I_2(0x11200000127))
    printf("bior64I_2 pass.\n");
  else {
    printf("bior64I_2 false.\n");
    return 1;
  }
  if((0x1200000007 | 0x11200000127)== bior64RR(0x1200000007, 0x11200000127))
    printf("bior64RR pass.\n");
  else {
    printf("bior64RR false.\n");
    return 1;
  }
  return 0;
}
