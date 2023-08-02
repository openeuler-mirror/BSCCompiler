#include "stdio.h"

struct S1 {
   volatile char f2 : 1;
   __attribute__ ((aligned(1))) unsigned int f5 : 24;
   volatile char : 2;
   __attribute__ ((aligned(1))) unsigned int f6 : 25;
};

// #pragma pack(1)
struct S2 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile char : 2;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};

#pragma pack(2)
struct S3 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile char : 2;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};

#pragma pack(2)
struct S4 {
   volatile char f2 : 1;
   __attribute__ ((aligned(8))) unsigned int f5 : 16;
   volatile char : 2;
   __attribute__ ((aligned(8))) unsigned int f6 : 17;
};

#pragma pack(4)
struct S5 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile char : 2;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};
#pragma pack(0)


struct SS1 {
   volatile char f2 : 1;
   __attribute__ ((aligned(1))) unsigned int f5 : 24;
   volatile char : 0;
   __attribute__ ((aligned(1))) unsigned int f6 : 25;
};

// #pragma pack(1)
struct SS2 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile int : 0;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};

#pragma pack(2)
struct SS3 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile short : 0;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};

#pragma pack(2)
struct SS4 {
   volatile char f2 : 1;
   __attribute__ ((aligned(8))) unsigned int f5 : 16;
   volatile long : 0;
   __attribute__ ((aligned(8))) unsigned int f6 : 17;
};

#pragma pack(4)
struct SS5 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile long long : 0;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};
#pragma pack(0)





void f_check_struct_size(void* ps, void* pf, int ofst)
{
    if ((((char*)ps) + ofst) != ((char*)pf)) {
                printf("error\n");
        } else {
                printf("ok\n");
    }
}

int main() {
    struct S1 a[2];
    struct S2 b[2];
    struct S3 c[2];
    struct S4 d[2];
    struct S5 e[2];
    struct SS1 a1[2];
    struct SS2 b1[2];
    struct SS3 c1[2];
    struct SS4 d1[2];
    struct SS5 e1[2];
    f_check_struct_size(&a[0], &a[1], 12);
    f_check_struct_size(&b[0], &b[1], 12);
    f_check_struct_size(&c[0], &c[1], 10);
    f_check_struct_size(&d[0], &d[1], 10);
    f_check_struct_size(&e[0], &e[1], 12);

    f_check_struct_size(&a1[0], &a1[1], 8);
    f_check_struct_size(&b1[0], &b1[1], 8);
    f_check_struct_size(&c1[0], &c1[1], 8);
    f_check_struct_size(&d1[0], &d1[1], 16);
    f_check_struct_size(&e1[0], &e1[1], 16);
}