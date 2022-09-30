unsigned short func(unsigned short x) {
  return (unsigned short int)(((x >> 8) & 0xff) | ((x & 0xff) << 8));
}

int main() {
  unsigned short x = 0xabcd;
  printf("%x\n", (unsigned int)func(x));
  return 0;
}