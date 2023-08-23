struct a {
  int x;
  int x1 : 31;
  int x2 : 18;
  int x3 : 14;
} g[4];
int *g1 = &g[0].x;
int main() {
  *(++g1) = 1;
  g[0].x3 = 4;
  return *g1 != 1;
}
