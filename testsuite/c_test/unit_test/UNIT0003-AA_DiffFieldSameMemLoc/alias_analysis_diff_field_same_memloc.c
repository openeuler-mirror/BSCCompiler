#include <stdio.h>

struct A {
  int fld1;
  int fld2;
};

struct B {
  int fld1;
  int fld2;
  int fld3;
  int fld4;
};

// move struct A's base ptr, and access different field of struct A
// but get the same memory location actually
int main()
{
  struct A *ptr;
  struct B structB;
  //                   structB
  // struct A *ptr--->| fld1 |
  //               └->| fld2 | <- assign 2 to fld2
  //                  | fld3 |
  //                  | fld4 |
  ptr = (struct A*)&structB;
  ptr->fld2 = 2;
  //                   structB
  //                  | fld1 |
  // struct A *ptr--->| fld2 | -> get fld1 of ptr, and it is actually fld2 of structB
  //               └->| fld3 |    the result expected is 2
  //                  | fld4 |
  ptr = (struct A*)((int*)ptr + 1);
  printf("%d\n", ptr->fld1);
  return 0;
}
