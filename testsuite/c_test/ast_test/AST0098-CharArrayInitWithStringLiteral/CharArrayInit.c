#include <stdio.h>

int main() {
  char c1[10] = {"apple"};
  char c2[10] = "apple";
  char c3[3][10] = {{"apple"},{"banana"},{"redorange"}};
  char c4[3][10] = {"apple", "banana", "redorange"};
  char *p1 = "apple\0\0\0\0";
  char *p2 = "banana\0\0\0";
  char *p3 = "redorange";
  if (__builtin_memcmp (c1, p1, 10) == 0 &&
      __builtin_memcmp (c2, p1, 10) == 0 &&
      __builtin_memcmp (c3[0], p1, 10) == 0 &&
      __builtin_memcmp (c3[1], p2, 10) == 0 &&
      __builtin_memcmp (c3[2], p3, 10) == 0 &&
      __builtin_memcmp (c4[0], p1, 10) == 0 &&
      __builtin_memcmp (c4[1], p2, 10) == 0 &&
      __builtin_memcmp (c4[2], p3, 10) == 0) {
    printf("test ok\n");
  }
  return 0;
}
