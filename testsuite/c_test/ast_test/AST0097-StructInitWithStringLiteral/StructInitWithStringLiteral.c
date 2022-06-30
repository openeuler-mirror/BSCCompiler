struct subFoo {
  int d;
  char c[3][8];
  char c2[13];
};

struct A {
  int a;
  char c[11];
  int b;
  char *p;
  struct subFoo f;
};

int main() {
  char *p1 = "j\0\0\0\0\0\0\0\0\0";
  char *p2_1 = "apple\0\0";
  char *p2_2 = "orange\0";
  char *p2_3 = "bananas";
  char *p3 = "k";
  char *p4 = "hello\0\0\0\0\0\0\0";
  struct A a = {1,"j",2, "k",{3,{"apple","orange","bananas"},"hello"}};
  printf("a.a=%d\n", a.a);
  if (__builtin_memcmp (a.c, p1, 11) == 0) {
    printf("a.c ok\n");
  }
  printf("a.b=%d\n", a.b);
  if (__builtin_memcmp (a.p, p3, 2) == 0) {
    printf("a.p ok\n");
  }
  printf("a.f.d=%d\n", a.f.d);
  if (__builtin_memcmp (a.f.c[0], p2_1, 8) == 0 &&
      __builtin_memcmp (a.f.c[1], p2_2, 8) == 0 &&
      __builtin_memcmp (a.f.c[2], p2_3, 8) == 0) {
    printf("a.f.c ok\n");
  }
  if (__builtin_memcmp (a.f.c2, p4, 13) == 0) {
    printf("a.f.c2 ok\n");
  }
  return 0;
}
