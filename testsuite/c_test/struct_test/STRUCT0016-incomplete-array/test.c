// #include "stdio.h"

void f_check_struct_size(void* ps, void* pf, int ofst)
{
    if ((((char*)ps) + ofst) != ((char*)pf)) {
                printf("error\n");
        } else {
                printf("ok\n");
    }
}

struct S1 {
  int length;
  double contents[];
};

struct S2 {
  int length;
  char contents[];
};

#pragma pack(1)
struct S3 {
  int length;
  double contents[];
};

#pragma pack(1)
struct __attribute__ ((aligned(8))) S4 {
  int length;
  double contents[];
};


int main() {
   struct S1 a[2];
   struct S2 b[2];
   struct S3 c[2];
   struct S4 d[2];
   
   f_check_struct_size(&a[0], &a[1], 8);
   f_check_struct_size(&b[0], &b[1], 4);
   f_check_struct_size(&c[0], &c[1], 4);
   f_check_struct_size(&d[0], &d[1], 8);
}