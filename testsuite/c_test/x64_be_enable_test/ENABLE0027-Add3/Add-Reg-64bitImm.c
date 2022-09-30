typedef unsigned long long T;
#define N (__SIZEOF_LONG_LONG__ * __CHAR_BIT__ / 2)

//test add reg(opnd0) 64bit imm(opnd1)
int
main ()
{
  T a = ((T) 1) << (N + 1);
  a += ((T) 1) << (N + 1);
  printf("%lld\n", a);
  return 0;
}
