extern int shared_add(int a, int b);

int main() {
  // we say shared_add computes `param1 + param2`
  if (shared_add(5, 2) != 7) {
    abort();
  }
  return 0;
}
