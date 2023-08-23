extern int shared_add(int a, int b);
extern int shared_sub(int a, int b);

int main() {
  // we say shared_add computes `param1 + param2`
  //        shared_sub computes `param1 - param2`
  if (shared_add(5, 2) != 7 || shared_sub(5, 2) != 3) {
    abort();
  }
  return 0;
}
