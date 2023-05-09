int i = 0;

// test iread
void __attribute__ ((noinline)) iread(int *a, int *b, int cond) {
  if (cond) {
    i = *a;
  } else {
    i = *b;
  }
  if (cond && i != *a) {
    abort();
  }
}

// test conststr
void __attribute__ ((noinline)) conststr(int cond) {
  char *s;
  if (cond) {
    s = "hello";
  } else {
    s = "wello";
  }
  if (cond && s[0] != 'h') {
    abort();
  }
}

// test addrof func
void __attribute__ ((noinline)) addroffunc(int cond) {
  void *func = 0;
  if (cond) {
    func = &iread;
  } else {
    func = &conststr;
  }
  if (cond && func != &iread) {
    abort();
  }
}

// test addrof label
void __attribute__ ((noinline)) addroflabel(int cond) {
  void *label = 0;
  int t = 0;
  l1:
    t = 1;
  l2:
    t = 2;
  if (cond) {
    label = &&l1;
  } else {
    label = &&l2;
  }
  i = *(int *)label;
  if (cond && label != &&l1) {
    abort();
  }
}

int main() {
  int a = 1;
  int b = 2;
  iread(&a,&b,1);
  conststr(1);
  addroffunc(1);
  addroflabel(1);
}
