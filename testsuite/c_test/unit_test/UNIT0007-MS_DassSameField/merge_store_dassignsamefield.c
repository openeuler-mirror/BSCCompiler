typedef struct A {
  int i;
  int j;
  int k;
  int x;
  int y;
  int z;
} StructA;

int main ()
{
  StructA a;
  // insert many dassign pair here
  // dassign pair : first is to set a value, second is to override the first value
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
  for (int i = 0; i < sizeof a; i++)
    if (((char *)&a)[i] != 0x12)
      __builtin_abort ();
  return 0;
}
