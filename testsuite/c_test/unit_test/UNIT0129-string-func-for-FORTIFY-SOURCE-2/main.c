#include <string.h>
int main() {
  __builtin_strcspn("aaabbbccc", "a");
  // CHECK: bl  strcspn
  __builtin_strspn("aaabbbccc", "a");
  // CHECK: bl  strspn
  __builtin_strpbrk("Hello world!", "world");
  // CHECK: bl strpbrk
  return 0;
}
