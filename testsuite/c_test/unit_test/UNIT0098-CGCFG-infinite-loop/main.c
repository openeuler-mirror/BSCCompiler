// To test updating commonExit info after cg-cfgo in infinite-loop case,
// which may cause pdom analysis errors.
int a;
int
main ()
{
  int b = 0;
  while (a < 0 || b)
    {
      b = 0;
      for (; b < 9; b++)
        ;
    }
  exit (0);
}
