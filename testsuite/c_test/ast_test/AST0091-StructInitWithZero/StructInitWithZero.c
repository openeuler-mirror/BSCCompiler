#include <stdio.h>
#include <limits.h>

struct subFoo {
  int a;
  short b;
  double c;
};

union uTest {
  char a;
  long b;
  float c;
};

struct FooA {
  int a;
  long long b;
  char c[4];
  struct subFoo f;
};

struct FooB {
  int a;
  char *p;
  struct subFoo f;
  union uTest u;
};

struct FooC {
  int a:31;
  int b:15;
  int c;
};

int main() {
  char str[] = {'h', 'i'};
  struct subFoo subfoo = {INT_MAX,0,-3.141592};
  union uTest utest = {0};

  struct FooA a1 = {1,0,{'a', 'b', 'c','d'},.f=subfoo};
  struct FooA a2 = {0,0,{'a', 'b', 'c'},{0}};
  struct FooA a3 = {1,0,{0},.f=subfoo};
  struct FooA a4 = {1,1};
  struct FooA a5 = {1,0};

  struct FooB b1 = {1,str,{0},{.b=-1}};
  struct FooB b2 = {1,NULL,{0},{.a='a'}};
  struct FooB b2_2 = {INT_MAX,str,subfoo,{.a='a'}};
  struct FooB b3 = {1,NULL,.f=subfoo,{0}};
  struct FooB b4 = {1,str,{0},{0}};
  struct FooB b5 = {1};

  struct FooC c1 = {0};
  struct FooC c2 = {0,1,1};
  struct FooC c3 = {1,0,1};

  printf("subfoo={%d,%hd,%f}\n", subfoo.a, subfoo.b, subfoo.c);
  printf("utest={%ld}\n", utest.b);

  printf("a1={%d,%lld,{%c,%c,%c,%c},{%d,%hd,%f}\n",a1.a, a1.b, a1.c[0], a1.c[1], a1.c[2], a1.c[3],a1.f.a,a1.f.b,a1.f.c);
  printf("a2={%d,%lld,{%c,%c,%c,%c},{%d,%hd,%f}\n",a2.a, a2.b, a2.c[0], a2.c[1], a2.c[2], a2.c[3],a2.f.a,a2.f.b,a2.f.c);
  printf("a3={%d,%lld,{%c,%c,%c,%c},{%d,%hd,%f}\n",a3.a, a3.b, a3.c[0], a3.c[1], a3.c[2], a3.c[3],a3.f.a,a3.f.b,a3.f.c);
  printf("a4={%d,%lld,{%c,%c,%c,%c},{%d,%hd,%f}\n",a4.a, a4.b, a4.c[0], a4.c[1], a4.c[2], a4.c[3],a4.f.a,a4.f.b,a4.f.c);

  printf("b1={%d,{%c,%c},{%d,%hd,%f},{%ld}}\n", b1.a, b1.p[0], b1.p[1], b1.f.a, b1.f.b, b1.f.c, b1.u.b);
  printf("b2={%d,%d,{%d,%hd,%f},{%c}}\n", b2.a, b2.p, b2.f.a, b2.f.b, b2.f.c, b2.u.a);
  printf("b2_2={%d,{%c,%c},{%d,%hd,%f},{%c}}\n", b2_2.a, b2_2.p[0], b1.p[1], b2_2.f.a, b2_2.f.b, b2_2.f.c, b2_2.u.a);
  printf("b3={%d,%d,{%d,%hd,%f},{%ld}}\n", b3.a, b3.p, b3.f.a, b3.f.b, b3.f.c, b3.u.b);
  printf("b4={%d,{%c,%c},{%d,%hd,%f},{%ld}}\n", b4.a, b4.p[0], b4.p[1], b4.f.a, b4.f.b, b4.f.c, b4.u.b);
  printf("b5={%d,%d,{%d,%hd,%f},{%ld}}\n", b5.a, b5.p, b5.f.a, b5.f.b, b5.f.c, b5.u.b);

  printf("c1={%d,%d,%d}\n",c1.a, c1.b, c1.c);
  printf("c2={%d,%d,%d}\n",c2.a, c2.b, c2.c);
  printf("c3={%d,%d,%d}\n",c3.a, c3.b, c3.c);
  return 0;
}
