int add(int x, int y) {
  int res = 4;
  if (y > 4) {
    res +=10;
  }
  if (y > 7)
    return res - y + x;
  else
    return res - y - x;
}

int zbh(int x) {
  int res = 0;
  for (int i = 0; i < x; i++) {
    res += add(x, 13);
    if (res > 100) {
      res += add(x, 14);
    }
  }
  return res;
}

int main(int argc , char *argv[]) {
  int x = 10 + add(11, 3);
  return add(argc + x, 5) + sub(1) + add(x, 6) + sub(1) + foo(0);
}

int sub(int x) {
  return 10 - add(x, 2);
}


int testHello(int x) {
  int res;
  while (x > 0) {
    if (x < 10)
      res += sub(1);
  }
  return res;
}

int foo(int *p) {
  if (p > 0) {
    if (p < 100) {
      return *p + sub(1);
    }
  }
  return 0;
}
