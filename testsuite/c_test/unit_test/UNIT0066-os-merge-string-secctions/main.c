/* Test for C99 designated initializers */
/* Origin: Jakub Jelinek <jakub@redhat.com> */
/* { dg-do run } */
/* { dg-options "-std=iso9899:1999 -pedantic-errors" } */

typedef __SIZE_TYPE__ size_t;
typedef __WCHAR_TYPE__ wchar_t;
extern void abort (void);
extern void exit (int);
extern int memcmp (const void *, const void *, size_t);

struct H {
  char I[6];
  int J;
} k[] = { { { "too" }, 1 }, [0].I[0] = 'b' };

//should put boo\0\0 in rodata section not "aMS" rodata section
int main (void)
{
  printf("%d\n", memcmp(k[0].I, "boo\0\0", 6));
  exit(0);
}
