#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef MAP_ANON
#define MAP_ANON 0
#endif
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

typedef struct {
  unsigned char dmac[6];
  unsigned char smac[6];
  unsigned short ethType;
} HpfEthHdr;

int test_1() {
  char *p = mmap((void *)0, 131072, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) return 0;
  char *endp = p + 65536;
  if (munmap(endp, 65536) < 0) return 0;

  HpfEthHdr *s1 = (HpfEthHdr *)endp - 1;
  HpfEthHdr s2;
  HpfEthHdr *s3 = &s2;
  *s3 = *s1;

  return 0;
}

int test_2() {
  char *p = mmap((void *)0, 131072, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) return 0;
  char *endp = p + 65536;
  if (munmap(endp, 65536) < 0) return 0;

  HpfEthHdr *s1 = (HpfEthHdr *)endp;
  HpfEthHdr s2;
  HpfEthHdr *s3 = &s2;
  *s3 = *s1;

  return 0;
}

int main() {
  return 0;
}
