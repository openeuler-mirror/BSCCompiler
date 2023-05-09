typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} __attribute__((packed)) S0;

typedef struct {
  unsigned char r;
  S0 s0;
  unsigned char b;
} __attribute__((packed)) S1;

S1 s1 = {'a', {'b', 'c', 'd'}, 'e'};

int main() {
  S0 *s0 = &s1.s0;
  S0 tmp = {'r', 'f', 's'};
  *s0 = tmp;
  printf("%c,%c,%c,%c,%c\n", s1.b, s1.r, s1.s0.b, s1.s0.g, s1.s0.r);

  return 0;
}