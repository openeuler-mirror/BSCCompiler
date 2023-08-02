
int a;
struct b {
  char c;
  int d;
  int e;
  union {
    char *f;
  } g;
  char h;
  long i;
} j() {
  struct b k;
  a = k.g.f;
}

int main() {
  return 0;
}
