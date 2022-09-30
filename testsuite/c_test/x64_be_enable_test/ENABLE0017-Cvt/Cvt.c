#include <stdio.h>

int main()
{
  int i0 = 25;
  unsigned int i1 = i0;
  printf("int->uint: i1 = %d\n", i1);
  int i3 = i1;
  printf("uint->int: i3 = %d\n", i3);
  unsigned long long l0 = i0;
  printf("int->ulonglong: l0 = %lld\n", l0);
  unsigned long long l1 = i1;
  printf("uint->ulonglong: l1 = %lld\n", l1);
  long long l2 = i0;
  printf("int->longlong: l2 = %lld\n", l2);
  long long l3 = i1;
  printf("uint->longlong: l3 = %lld\n", l3);

  int i4 = l1;
  printf("ulonglong->int: i4 = %d\n", i4);
  unsigned int i5 = l1;
  printf("ulonglong->uint: i5 = %d\n", i5);
  int i6 = l2;
  printf("longlong->int: i6 = %d\n", i6);
  unsigned int i7 = l2;
  printf("longlong->uint: i7 = %d\n", i7);
  return 0;
}
