
static int x;
int func2();

int foo (int a)
{
  int b = a + 10;
  return b;
}

int bar (int y)
{
  int z = y + 20;
  return z;
}

void func()
{
  x = x + 5;
  func2 ();
}

int func2 ()
{
  x = 6;
  return 0;
}

int func3 ()
{
  x = 4;
  return 0;
}

void marker1 ()
{
}

int
main ()
{
  int result;
  int b, c;
  c = 5;
  b = 3;    /* advance this location */

  func (); /* stop here after leaving current frame */
  marker1 (); /* stop here after leaving current frame */
  func3 (); /* break here */
  result = bar (b + foo (c));
  return 0; /* advance malformed */
}

