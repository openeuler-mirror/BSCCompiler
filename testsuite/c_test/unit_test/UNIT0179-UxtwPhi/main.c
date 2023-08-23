int a = 1;
int b = 0;
long long c = 0xFFFF00000000;
int d = 0;

int main() {
  if (a ? c : b) {
    d = 1;
  }
  printf("%d\n", d);
  return 0;
}
