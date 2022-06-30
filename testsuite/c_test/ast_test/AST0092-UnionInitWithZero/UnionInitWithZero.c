#include <stdio.h>
#include <limits.h>

struct subFoo {
  int a;
  short b;
  double c;
};

union uTestA {
  char a;
  long b;
  float c;
  char str[20];
};

union uTestB {
  int a;
};

union uTestC {
  int a;
  struct subFoo f;
  union uTestA uA;
  union uTestB uB;
};

int main() {
  union uTestA ua = {.b=0};
  union uTestC uc1 = {.a=0};
  union uTestC uc2 = {.a=INT_MAX};
  union uTestC uc3 = {.f={INT_MAX, 0, 3.14}};
  union uTestC uc4 = {.uB={.a=INT_MAX}};
  union uTestC uc5 = {.uA={.str={'h','i'}}};
  union uTestC uc6 = {.f={0}};
  union uTestC uc7={.f={INT_MAX, 0, 0}};

  printf("ua={%ld}\n", ua.b);
  printf("uc1={%d}\n", uc1.a);
  printf("uc2={%d}\n", uc2.a);
  printf("uc3={{%d,%hd,%f}}\n", uc3.f.a, uc3.f.b,uc3.f.c);
  printf("uc4={{%d}}\n", uc4.uB.a);
  printf("uc5={{%c,%c,%c,%c,...}}\n", uc5.uA.str[0], uc5.uA.str[1],uc5.uA.str[2], uc5.uA.str[3]);
  printf("uc6={{%d,%hd,%f}}\n", uc6.f.a, uc6.f.b,uc6.f.c);
  printf("uc7={{%d,%hd,%f}}\n", uc7.f.a, uc7.f.b,uc7.f.c);
  return 0;
}
