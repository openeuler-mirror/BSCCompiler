int volatile a = 7;

unsigned char **b;
//unsigned char b;
unsigned char ***c = &b;
unsigned char ****volatile d = &c;
unsigned char f = 6;
signed char fn1();
void fn2() {
  signed char e;
  e = fn1();
}
signed char fn1(int p1, long long p2, unsigned long long p3, int p4) {
  unsigned char *g = &f;
  unsigned char **h = &g;
  *d = &h; // *d 代表c的值，相当于c指针指向了 h的地址，三层解引用后指向的是f
  (***c)++;
  for (0;; 0)
    if (a)
      break;
  for (0; 0; 0)
    ;
  return p1;
}

int main() { fn2(); }


