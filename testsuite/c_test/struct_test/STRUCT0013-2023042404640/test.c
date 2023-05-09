
#include "stdio.h"
#pragma pack(1)
struct S0 {
  volatile long long f2;
  signed : 0;
  short f5;
};

struct S0 a[2] = {
  {2, 1},
  {2, 1}
};

void f_check_struct_size(void* ps, void* pf, int ofst)
{
    if ((((char*)ps) + ofst) != ((char*)pf)) {
                printf("error\n");
        } else {
                printf("ok\n");
    }
}


int main() {
  f_check_struct_size(&a[0], &a[1], 12);
}
