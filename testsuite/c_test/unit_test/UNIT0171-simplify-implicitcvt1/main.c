unsigned short a;
int i;
int main() {
  signed char c[] = {7, 7, 7, 7, 183};
  for (i = 0; i <= 4; i++) {
    // do sign extension for c[i] here
    a ^= c[i];
  }
  if (a != 65463) {
    abort();
  }
  return 0;
}
