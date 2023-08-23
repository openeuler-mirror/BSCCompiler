int var_0 = 78205889;
short var_1 = (short)-15260;
unsigned long long int var_2 = 8608153893901660375ULL;
long long int var_3 = -3995671790595460071LL;
_Bool var_4 = (_Bool)1;
short var_5 = (short)-26830;
short var_6 = (short)1423;
unsigned long long int var_7 = 10121301245951723816ULL;
unsigned int var_16 = 2382740467U;

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
  *seed ^= v + 0x9e3779b9 + ((*seed) << 6) + ((*seed) >> 2 );
}


void checksum() {
  hash(&seed, var_0);
  hash(&seed, var_1);
  hash(&seed, var_2);
  hash(&seed, var_3);
  hash(&seed, var_4);
  hash(&seed, var_5);
  hash(&seed, var_6);
  hash(&seed, var_7);
}

int main() {
  test(var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
  checksum();
  printf("var_16 = %d\n", var_16);
  printf("%llu\n", seed);
}