long a;
short b = 0;
unsigned long c;

int main() {
  (a ?: (b || (int)(-9223372036854775807LL - 1 & c)) % 0 || c);
  return 0;
}