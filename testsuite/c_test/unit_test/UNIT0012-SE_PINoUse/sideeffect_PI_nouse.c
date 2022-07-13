extern void abort();
int *g = 0;
// argument a is nouse
__attribute__((__noinline__)) void bar(int *a) {
  *g = 20;
}

__attribute__((__noinline__)) int foo(int cond) {
  int i = 10;
  int k = 11;
  g = &i;
  int *j = 0;
  // use a branch to disable propagation of j
  if (cond) {
    j = &i;
  } else {
    j = &k;
  }
  *j = 10;
  bar(j); // j is no use in bar, but *j will be modified through g
          // we should add *j to MAYD list, otherwise return will be
          // propagated as 10
  return *j;
}

int main() {
  int res = foo(1);
  if (res != 20) {
    abort();
  }
  return 0;
}
