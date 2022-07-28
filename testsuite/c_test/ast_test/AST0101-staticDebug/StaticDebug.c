static int filelocal = 2; /* In Data section */
static int filelocal_bss; /* In BSS section */

static const int filelocal_ro = 202;  /* In Read-Only Data section */

void foo ()
{

  void bar ();

  static int funclocal = 3; /* In Data section */
  static int funclocal_bss; /* In BSS section */
  static const int funclocal_ro = 203;  /* RO Data */
  static const int funclocal_ro_bss;  /* RO Data */

  funclocal_bss = 103;
  bar ();
}

void bar ()
{
  static int funclocal = 4; /* In data section */
  static int funclocal_bss; /* In BSS section */
  funclocal_bss = 104;
}


int main() {
  foo ();
  return 0;
}
