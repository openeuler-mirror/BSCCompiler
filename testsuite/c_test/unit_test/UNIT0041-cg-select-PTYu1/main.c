#include <stdio.h>

unsigned long long int seed = 0;
unsigned long long int seed1 = 0;
unsigned int var_22 = 8;
unsigned int var_23 = 8;
_Bool var = 0;

struct S1 {
  _Bool var_11;
};

struct S1 s1 = {0};

void hash(unsigned long long int *p1, unsigned long long int const p2) {
  *p1 = p2 + 9 + 0 + 0;
}

void test(long long int p2) {
  var_22 = var ? 0 : 0 ^ p2;
  var_23 = s1.var_11 ? 0 : 0 ^ p2;
}

int main() {
  test(3);
  hash(&seed, var_22);
  hash(&seed1, var_23);
  printf("%llu\n%llu\n", seed, seed1);
}
