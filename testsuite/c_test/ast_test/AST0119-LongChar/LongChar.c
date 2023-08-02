void
marker1 ()
{

}

int main() {
   struct {
    char c[100];
  } cbig;

  struct {
    long l[900];
  } lbig;

  cbig.c[0] = '\0';
  cbig.c[99] = 'A';

  lbig.l[333] = 999999999;
  marker1 ();

  return 0;
}

