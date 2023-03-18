typedef struct {
  double s1;
  double s2;
  double s3;
  double s4;
} st_t;

__attribute__((noinline))  double func(st_t a0, st_t a1, st_t a2) {
  st_t *a00 = &a0;
  double *s1 = &a1.s1;
  double s2 = a2.s3;
  st_t *a01 = &a2;

  return a00->s4 + *s1 + s2 + a01->s2;
}

int main() {
  st_t a0 = {1.0, 2.0, 3.0, 4.0};
  double res = func(a0, a0, a0);

  printf("%.2f\n", res);

  return 0;
}