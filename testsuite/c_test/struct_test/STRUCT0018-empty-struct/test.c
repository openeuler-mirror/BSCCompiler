#include "stdio.h"

struct a {
  short a;
};

struct c {
} __attribute__((aligned));

struct d {
} __attribute__((aligned(8)));

struct {
  int d;
  struct c e;
  struct a b;
}j;

struct {
  int d;
  struct d e;
  struct a b;
}f;

void f_check_offset(void* ps, void* pf, int ofst)
{
    if ((((char*)ps) + ofst) != ((char*)pf)) {
                printf("error\n");
        } else {
                printf("ok\n");
    }
}

int main() {
    f_check_offset(&j, &j.e, 16);
    f_check_offset(&f, &f.e, 8);
    return 0;
}