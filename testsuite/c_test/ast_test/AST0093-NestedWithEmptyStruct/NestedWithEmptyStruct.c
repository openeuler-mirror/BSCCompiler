#include <stdio.h>
struct empty { };
struct something {
  int spacer;
  struct empty foo;
  int bar;
};
struct something X = {
  foo: { },
  bar: 1,
};
int main() {
  printf("%d\n", sizeof X);
  return 0;
}
