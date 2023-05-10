int a = 0;
int b = 0;

// test if available tripcount of refused loop equals to 0
__attribute__ ((noinline))
void refuse1() {
  unsigned int i = -2;
  for (;i <= 5; i++) {
    a += 1;
  }
}

// test if guarded tripcount guards refused loop
__attribute__ ((noinline))
unsigned int refuse2(unsigned int i) {
  for (;i <= 5; i++) {
    b += 1;
  }
  return i;
}

int main() {
  refuse1();
  if (a != 0) {
    abort();
  }

  unsigned int i = refuse2(-2);
  if (i != -2 || b != 0) {
    abort();
  }
}
