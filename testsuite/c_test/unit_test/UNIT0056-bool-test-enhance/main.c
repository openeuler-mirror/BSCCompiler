#include <stdio.h>

unsigned long long int seed;
unsigned long long int seed1;
unsigned int var_22;
unsigned int var_23;
_Bool var;

struct S1 {
  _Bool var_11;  // bool in aggType
  int a;
};

union U1 {
  int a;
  _Bool var0;
};

union U1 u1 = {.var0 = 255};
struct S1 s1 = {3, 1};
struct S1 s2 = {12, 2};

void hash(unsigned long long int *p1, const unsigned long int p2) {
  *p1 = p2 + u1.var0 + 0 + 0;  // type convert
}

void test(long long int p2) {  // callee save
  var = 3;
  s1.var_11 = 3;
  var_22 = var ? 0 ^ p2 : 0;
  var_23 = s1.var_11 ? 0 ^ s2.var_11 : 0;
}

int main() {
  var = 252; // bool = 252:0x11111100
  if (var > s1.var_11 || s2.var_11 > s1.var_11 || var < seed) {
    printf("%llu\n%llu\n", var, s1.var_11);
  }
  test(s1.var_11);  // bool -> long long
  hash(&seed, var_22);
  hash(&seed1, var_23);
  printf("%llu\n%llu\n", seed, seed1);
}
