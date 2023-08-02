#include <stdlib.h>
#include <string.h> // 注意：此头文件必须包含

struct C {
  char *buf;
};

int main() {
  struct C c;
  if (rand()) {
    c.buf = malloc(10 * sizeof(char));
  } else {
    c.buf = malloc(8 * sizeof(char));
  }
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 12
  memcpy(c.buf, "ddddddddddddddddddddd", 12); // warning 10/8
  return 0;
}
