typedef struct {
  int : 22;
} S0;

typedef struct {
  S0 a;
  int b : 22;
  int c : 22;
  int d : 22;
  int e : 14;
} S1;

S1 s1 = {{}, -560076, -1305926, -317877, 1067};

int main() {
  printf("%d\n", s1.b);
  printf("%d\n", s1.c);
  printf("%d\n", s1.d);
  printf("%d\n", s1.e);
  return 0;
}
