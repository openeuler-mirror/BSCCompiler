#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

void  __attribute__ ((noinline)) print_backtrace() {
  void *buffer[64];
  char **symbols;

  int num = backtrace(buffer, 64);
  printf("backtrace() returned %d addresses\n", num);

  symbols = backtrace_symbols(buffer, num);
  if (symbols == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }

  for (int j = 0; j < num; ++j) {
    printf("%s\n", symbols[j]);
  }

  free(symbols);
}

void  __attribute__ ((noinline)) myfunc3() {
  print_backtrace();
}

void  __attribute__ ((noinline)) myfunc2() {
  myfunc3();
}

void  __attribute__ ((noinline)) myfunc1() {
  myfunc2();
}

int main() {
  myfunc1();
}
