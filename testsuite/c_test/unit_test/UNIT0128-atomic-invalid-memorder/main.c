#include <stdint.h>

void a() {
  uint32_t *b;
  __atomic_sub_fetch(b, 0, 21); // warning: "invalid memory model argument"
}

struct s {};
void b() {
  struct s a, b;
  __atomic_load(&a, &b, 45); // warning: "invalid memory model argument"
}

void c() {
  uint32_t *b;
   __atomic_add_fetch(b, 5, -1); // warning: "invalid memory model argument"
}

int main() {
  return 0;
}
