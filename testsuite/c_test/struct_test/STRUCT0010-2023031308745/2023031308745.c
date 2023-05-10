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

struct  BFu15i_BFu15i_Sf  {
  unsigned int v1:15;
  unsigned int v2:15;
  struct empty v3;
};

int main()
{
    struct BFu15i_BFu15i_Sf a;
    printf("sizeof(struct) = %lu\n", sizeof(struct BFu15i_BFu15i_Sf));
    f_check_field_offset(&a, &a.v3, 4);//预期v2的地址与a的地址偏移4字节
    // printf("address of a=%p, address of a.v3=%p\n", &a, &a.v3);
    return 0;
}
