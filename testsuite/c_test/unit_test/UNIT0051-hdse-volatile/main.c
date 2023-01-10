union d {
  int e;
} p;
int *g;
int **volatile j = &g;
void k() {
  int *al = &p.e;
  int **am = &g;
  *am = al;
  *am = *j;
}
int main() {
  int old = 0;
  g = &old;
  p.e = 3;
  k();
  if (*g != p.e) {
    abort();
  }
  return 0;
}
