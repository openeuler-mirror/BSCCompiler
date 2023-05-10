#include <stdio.h>

void f_check_field_offset(void* ps, void* pf, int ofst)
{
    if ((((char*)ps) + ofst) != ((char*)pf)) {
                printf("error\n");
        } else {
                printf("ok\n");
    }
}

struct  empty  {
};

struct  BFu15i_Sf_BFu15i  {
  unsigned int v1:15;
  struct empty v2;
  unsigned int v3:15;
};

unsigned long long hide_ull(unsigned long long p) { return p; }

int main()
{
    struct BFu15i_Sf_BFu15i a;
    printf("sizeof(struct) = %lu\n", sizeof(struct BFu15i_Sf_BFu15i));
    f_check_field_offset(&a, &a.v2, 2);//预期v2的地址与a的地址偏移两字节
    return 0;
}