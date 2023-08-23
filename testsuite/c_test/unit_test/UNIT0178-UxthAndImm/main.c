signed char a = 69;
unsigned short b = 61034;

int main() {
  int c = -16 & (unsigned short)(~a | ~b);
  printf("%d\n", c);
  return 0;
}
