extern void abort();
int g = 0;

// argument a is nouse
// FI:Unknown  RI:  PI:nouse
__attribute__((__noinline__)) void bar(int *a) {
  g = 20;
}

__attribute__((__noinline__)) int foo(int *k, int cond) { // FI unknown
  int *j = k;
  if (cond == 1) {
    j = 0;
  }
  *j = 10;
  bar(j); // *j is def here, should be inserted into mayDef list
  return *j;
}


int main() {
  int res = foo(&g, 0);
  if (res != 20) {
    abort();
  }
  return 0;
}

