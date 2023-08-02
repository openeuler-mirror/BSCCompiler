#include "stdio.h"



struct S1 {
   volatile char f2 : 1;
   __attribute__ ((aligned(1))) unsigned int f5 : 24;
   volatile char f : 2;
   __attribute__ ((aligned(1))) unsigned int f6 : 25;
};

// #pragma pack(1)
struct S2 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile char f;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};

#pragma pack(2)
struct S3 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile char f;
   __attribute__ ((aligned(2))) unsigned int f6 : 17;
};

#pragma pack(2)
struct S4 {
   volatile char f2 : 1;
   __attribute__ ((aligned(8))) unsigned int f5 : 16;
   volatile char f;
   __attribute__ ((aligned(8))) unsigned int f6 : 17;
};

#pragma pack(4)
struct S5 {
   volatile char f2 : 1;
   __attribute__ ((aligned(2))) unsigned int f5 : 16;
   volatile char f;
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

struct S6 {
  signed f0 : 3;
  volatile signed f1 : 21;
  signed f2 : 26;
  unsigned f3 : 1;
  signed f4 : 26;
  unsigned f5 : 29;
  volatile unsigned f6 : 21;
  unsigned f7 : 14;
  const signed f8 : 29;
  signed f9 : 23;
};


typedef struct empty {} empty;

struct  S7  {
  unsigned int v1:17;
  struct empty v2;
  unsigned long long v3:15;
};


struct S8{
	int a : 4;
	int b : 1;
	int c : 4;
	int d : 1;
	int e : 4;
};

int main() {
   struct S1 a[2];
   struct S2 b[2];
   struct S3 c[2];
   struct S4 d[2];
   struct S5 e[2];
   struct S6 f[2];
   struct S7 g[2];
   struct S8 h[2];
   
   f_check_struct_size(&a[0], &a[1], 12);
   f_check_struct_size(&b[0], &b[1], 12);
   f_check_struct_size(&c[0], &c[1], 10);
   f_check_struct_size(&d[0], &d[1], 10);
   f_check_struct_size(&e[0], &e[1], 12);
   f_check_struct_size(&f[0], &f[1], 32);
   f_check_struct_size(&g[0], &g[1], 8);
   f_check_struct_size(&h[0], &h[1], 4);
}