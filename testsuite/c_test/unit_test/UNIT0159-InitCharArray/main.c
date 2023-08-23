#include  <stddef.h>
typedef __WCHAR_TYPE__ wchar_t;
extern int memcmp (const void *, const void *, size_t);
extern void abort (void);
extern void exit (int);

#define N 20
int main (void)
{
  struct M {
    char P[N];
    int L;
  } z[] = { { { "foo" }, 1 }, [0].P[0] = 'b', [0].P[1] = 'c', [0].L = 3 };
  for (int i = 0; i < N; i++) {
    printf("%d ", z[0].P[i]);
  }
  printf("\n");
  exit (0);
}
