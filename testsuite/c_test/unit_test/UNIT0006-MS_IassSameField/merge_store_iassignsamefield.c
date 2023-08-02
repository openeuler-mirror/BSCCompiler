struct A {
  int i;
  int j;
  int k;
  int x;
  int y;
  int z;
} a;

int main ()
{
  int i;
  // insert many iassign here
  a.i = 0x12991212;
  a.i = 0x12121212;
  a.j = 0x12991212;
  a.j = 0x12121212;
  a.x = 0x12991212;
  a.x = 0x12121212;
  a.y = 0x12991212;
  a.y = 0x12121212;
  a.z = 0x12991212;
  a.z = 0x12121212;
  a.k = 0x12991212;
  a.k = 0x12121212;
  for (i = 0; i < sizeof a; i++)
    if (((char *)&a)[i] != 0x12)
      __builtin_abort ();
  return 0;
}
