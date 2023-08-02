#include <stdio.h>

struct FieldStruct1 {
  int fld1 : 20; // (0, 0)
  int      : 10; // (0, 20)
  int fld3 : 4;  // 20 + 10 + 4 > 32, over sizeof(int), start from next int unit
                 // (4, 0)
  int      : 0;  // zero field, force next field start from new int unit
                 // (4, 4)
  int fld5 : 13; // (8, 0)
};

int main() {
  struct FieldStruct1 s1 = {0};
  char *ptr = (char *)&s1;
  s1.fld1 = 33;
  s1.fld3 = 2;
  s1.fld5 = 5;
  // (gdb)x/12xb ptr
  // 0x7fffffffce8c:	0x21	0x00	0x00	0x00	0x02	0x00	0x00	0x00
  // 0x7fffffffce94:	0x05	0x00	0x00	0x00

  int *intptr = (int*)(ptr + 8); // point to field5;
  *intptr = 6;

  printf("%d\n", s1.fld5);
  printf("%ld\n", sizeof(s1));
  return 0;
}
