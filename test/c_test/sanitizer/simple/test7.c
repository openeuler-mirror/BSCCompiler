// ./run_ubsan.sh test7.c

#include <string.h>

int main() {
  char src[] = "hello";
  char buf[3];
  strcpy(buf, src);
}
