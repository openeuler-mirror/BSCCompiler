#include <stdio.h>

typedef struct empty {} empty;

struct  BFu17i_Sf_BFu15ll  {
  unsigned int v1:17;
  struct empty v2;
  unsigned long long v3:15;
};

union U {
    unsigned long long v0;
    struct  BFu17i_Sf_BFu15ll v;
};

int main()
{
    union U u;
    u.v0 = 0x0;
    printf("sizeof(union) = %lu\n", sizeof(union U));
    u.v.v1 = 0x0;//v1值都是0.
    u.v.v3 = ~(0x0);//v3值都是1
    unsigned char* s;
    s = (unsigned char*)&u;
    printf("byte 1 of u = %d\n", s[0]);
    printf("byte 2 of u = %d\n", s[1]);
    printf("byte 3 of u = %d\n", s[2]);
    printf("byte 4 of u = %d\n", s[3]);
    printf("byte 5 of u = %d\n", s[4]);
    printf("byte 6 of u = %d\n", s[5]);
    printf("byte 7 of u = %d\n", s[6]);
    printf("byte 8 of u = %d\n", s[7]);
    return 0;
}
