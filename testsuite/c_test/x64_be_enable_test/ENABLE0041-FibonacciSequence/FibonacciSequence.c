long long fibonacci(long long num) {
  if (num < 2) {
    return num;
  }
  return fibonacci(num - 1) + fibonacci(num - 2);
}

int main() {
  printf("%lld\n", fibonacci(10));
  return 0;
}