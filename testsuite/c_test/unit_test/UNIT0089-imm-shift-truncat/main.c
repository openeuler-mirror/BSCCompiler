#include <stdio.h>

void test1(_Bool b[][6]) {
  for (; b[2][0] << 9223372036854775807;)
    ;
}

void test2(_Bool b[][6]) {
  for (; b[2][0] << -9223372036854775807;)
    ;
}

void test3(_Bool b[][6]) {
  for (; b[2][0] << 8090864131964928;)
    ;
}

void test4(_Bool b[][6]) {
  for (; b[2][0] >> 9223372036854775807;)
    ;
}

void test5(_Bool b[][6]) {
  for (; b[2][0] >> -9223372036854775807;)
    ;
}

void test6(_Bool b[][6]) {
  for (; b[2][0] >> 8090864131964928;)
    ;
}


int main() {
  return 0;
}
