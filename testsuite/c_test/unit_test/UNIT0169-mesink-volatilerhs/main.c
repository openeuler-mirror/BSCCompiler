int g0 = 0;
int *g1 = &g0;
int **volatile g2 = &g1;

__attribute__((noinline))
int test(int cond) {
  int l0;
  int **l2;
  if (cond) {
    test(0);
    l2 = &g1;
    l0 = **g2;
    *l2 = 0;
  } else {
    l2 = &g1;
    l0 = **g2;
    *l2 = 0;
  }
  // do not sink `l0 = **g2` to here
  return l0;
}

int main() { 
  test(0);
  return 0;
}
