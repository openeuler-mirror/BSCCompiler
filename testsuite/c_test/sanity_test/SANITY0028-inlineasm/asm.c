#include <stdio.h>
#include <stdlib.h>

#define TEST01
#define TEST02
#define TEST03
#define TEST04
#define TEST05
#define TEST06 // "m"
#undef TEST07  // fail // goto label
#define TEST08
#define TEST09 // "m"
#define TEST10
#undef TEST11  // fail // unsupported op
#define TEST12
#define TEST13
#undef TEST14  // works on arm server not qemu
#define TEST15
#define TEST16
#define TEST17
#define TEST18

#ifdef TEST01
int test01a(int a, int b) {
  int c;
//  asm volatile("\tadd %0, %1, %2\n"
  asm volatile("\tadd %w0, %w1, %w2\n"
	       	: "=r"(c)
	       	: "r"(a), "r"(b));
  return c;
}
void test01() {
  long long l1 = 0x80000000LL;
  long long l2 = 0x100000000LL;
  if (test01a(l1, l2) != 0x80000000) {
    printf("test01 failed\n");
    abort();
  }
}
#endif
#ifdef TEST02
int test02a(int a, int b) {
  int c;
  asm volatile("add %w0, %w1, %w2" : "=r"(c) : "r"(a), "r"(b) : "x5", "w4", "s2", "d3", "v9");
  return c;
}
void test02() {
  long long l1 = 0x80000000LL;
  long long l2 = 0x100000000LL;
  if (test02a(l1, l2) != 0x80000000) {
    printf("test02 failed\n");
    abort();
  }
}
#endif
#ifdef TEST03
int test03a(int a, int b) {
  int c, d;
  asm volatile("add %w0, %w2, %w3\n\t"
               "add %w1, %w2, %w3\n"
      : "=&r"(c), "=r"(d) : "r"(a), "r"(b) : "x5", "w4", "s2", "d3", "v9");
  return (c + d);
}
void test03() {
  long long l1 = 0x00000800LL;
  long long l2 = 0x00090000LL;
  if (test03a(l1, l2) != 0x00121000) {
    printf("test03 failed\n");
    abort();
  }
}
#endif
#ifdef TEST04
int test04a(int a, int b) {
  int *e = &a;
  asm ("mov %w[b], %w[e]" : [b] "=r" (b) : [e] "r" (*e));
  return a;
}
void test04() {
  long long l1 = 0x00000800LL;
  long long l2 = 0x00090000LL;
  if (test04a(l1, l2) != 0x00000800) {
    printf("test04 failed\n");
    abort();
  }
}
#endif
#ifdef TEST05
int test05a(int a, int b) {
  int c;
  asm ("add %w[c], %w1, %w2" : [c] "=r" (c) : "r" (a), "r" (b));
  return c;
}
void test05() {
  long long l1 = 0x00000800LL;
  long long l2 = 0x00090000LL;
  if (test05a(l1, l2) != 0x00090800) {
    printf("test05 failed\n");
    abort();
  }
}
#endif
#ifdef TEST06
int test06a(int a, int b) {
  int c;
  asm ("ldr %x[c], %x1" : [c] "=r" (c) : "m" (a));
  return c;
}
void test06() {
  long long l1 = 0x00000800LL;
  long long l2 = 0x00090000LL;
  if (test06a(l1, l2) != 0x00000800) {
    printf("test06 failed\n");
    abort();
  }
}
#endif
#ifdef TEST07
int test07a(int a, int b) {
  asm goto ("b %l0\n"
   : /* no output */
   : /* no input */
   : /* no clobber */
   : gohere);
  return 0;
gohere:
  return 1;
}
void test07() {
  long long l1 = 0x00000800LL;
  long long l2 = 0x00090000LL;
  if (test07a(l1, l2) != 1) {
    printf("test07 failed\n");
    abort();
  }
}
#endif
#ifdef TEST08
int test08a(int a, int b) {
  /* %w[c] should not be in the same reg as %w1 */
  int c;
  asm (
    "add %w[c], %w1, %w2\n\t"
    "mov %w[c], %w1"
    : [c] "+r" (c)
    : "r" (a), "r" (b));
  return c;
}
void test08() {
  long long l1 = 0x00000800LL;
  long long l2 = 0x00090000LL;
  if (test08a(l1, l2) != 0x00000800) {
    printf("test08 failed\n");
    abort();
  }
}
#endif

#ifdef TEST09
long g;
int test09a(long x) {
  asm volatile("\tldr %0, %1\n"
	       	: "=r"(x)
	       	: "m"(g));
  return x;
};
void test09() {
  g = 0x400;
  long l1 = 0x00000800LL;
  if (test09a(l1) != 0x0400) {
    printf("test09 failed\n");
    abort();
  }
}
#endif

#ifdef TEST10
int h1[2];
int *h = h1;
int test10a() {
  long long y;
  int z;
  asm volatile("add %x0, %x2, #4\n"
  "\tldr %w1, [%x0]\n"
  : "=r"(y), "=r"(z)
  : "r"(h), "m"(h)
  : "x1");
  return z;
}
void test10() {
  h1[0] = 0x400;
  h1[1] = 0x800;
  if (test10a() != 0x0800) {
    printf("test10 failed\n");
    abort();
  }
}
#endif

#ifdef TEST11
unsigned int
crc32c_arm64_u8(unsigned char data, unsigned int init_val)
{
        __asm__ volatile(
                        "crc32cb %w[crc], %w[crc], %w[value]"
                        : [crc] "+r" (init_val)
                        : [value] "r" (data));
        return init_val;
}
unsigned int test11() {
  unsigned char data = 10;
  unsigned int init_val = 5;
  init_val = crc32c_arm64_u8(data, init_val);
  printf("init_val %x\n", init_val);
}
#endif

#ifdef TEST12
void
prfm_arm64_u8(void *ptr)
{
        asm volatile(
		"prfm pstl1keep, %a0\n"
	       	:
	       	: "p" (ptr));
}
unsigned int test12() {
  long long ptr;
  prfm_arm64_u8(&ptr);
}
#endif

#ifdef TEST13
#define octeontx_load_pair(val0, val1, addr) ({         \
                        asm volatile(                   \
                        "ldp %x[x0], %x[x1], [%x[p1]]"  \
                        :[x0]"=r"(val0), [x1]"=r"(val1) \
                        :[p1]"r"(addr)                  \
                        ); })

#define octeontx_store_pair(val0, val1, addr) ({                \
                        asm volatile(                   \
                        "stp %x[x0], %x[x1], [%x[p1]]"  \
                        ::[x0]"r"(val0), [x1]"r"(val1), [p1]"r"(addr) \
                        :"memory"); })
void test13()
{
  long long addr[2] = {0x111, 0x222};
  long long val0, val1;
  long long *ptr = addr;
  octeontx_load_pair(val0, val1, ptr);
  if (val0 != 0x111) {
    printf("test13 val0 failed\n");
    abort();
  }
  if (val1 != 0x222) {
    printf("test13 val1 failed\n");
    abort();
  }
  addr[0] = 0x333;
  addr[1] = 0x444;
  octeontx_store_pair(val0, val1, ptr);
  if (addr[0] != 0x111) {
    printf("test13 addr[0] failed\n");
    abort();
  }
  if (addr[1] != 0x222) {
    printf("test13 addr[1] failed\n");
    abort();
  }
}
#endif

#ifdef TEST14
unsigned long long
test14a(void *addr, long long off)
{
        unsigned long long old_val;

        __asm__ volatile(
                " .cpu   generic+lse+sve\n\t"
                " ldadd %1, %0, [%2]\n"
                : "=r" (old_val) : "r" (off), "r" (addr) : "memory");

        return old_val;
}
void test14()
{
  long long val = 5;
  long long off = 1;
  if (test14a(&val, off) != 5LL && val != 6) {
    printf("test14 failed\n");
    abort();
  }
}
#endif

#ifdef TEST15
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>

	uint16x4_t byte_cnt;
unsigned char cp = 5;
	unsigned char *p = &cp;
	uint8x16_t rxdf;
	uint64x2_t rearm;
uint8_t e0p;
	uint8_t *e0 = &e0p;
uint8_t e1p;
	uint8_t *e1 = &e1p;
uint8_t e2p;
	uint8_t *e2 = &e2p;
uint8_t e3p;
	uint8_t *e3 = &e3p;
	uint8x16_t mcqe_shuf_m1;
	uint8x16_t mcqe_shuf_m2;
	uint16x8_t crc_adj;
	uint8x8_t len_shuf_m;

uint16x4_t
test15()
{
		__asm__ volatile (
		/* A.1 load mCQEs into a 128bit register. */
		"ld1 {v16.16b - v17.16b}, [%[mcq]] \n\t"
		/* B.1 store rearm data to mbuf. */
	"st1 {%[rearm].2d}, [%[e0]] \n\t"
		"add %[e0], %[e0], #16 \n\t"
		"st1 {%[rearm].2d}, [%[e1]] \n\t"
		"add %[e1], %[e1], #16 \n\t"
		/* C.1 combine data from mCQEs with rx_descriptor_fields1. */
		"tbl v18.16b, {v16.16b}, %[mcqe_shuf_m1].16b \n\t"
		"tbl v19.16b, {v16.16b}, %[mcqe_shuf_m2].16b \n\t"
		"sub v18.8h, v18.8h, %[crc_adj].8h \n\t"
		"sub v19.8h, v19.8h, %[crc_adj].8h \n\t"
		"orr v18.16b, v18.16b, %[rxdf].16b \n\t"
		"orr v19.16b, v19.16b, %[rxdf].16b \n\t"
		/* D.1 store rx_descriptor_fields1. */
		"st1 {v18.2d}, [%[e0]] \n\t"
		"st1 {v19.2d}, [%[e1]] \n\t"
		/* B.1 store rearm data to mbuf. */
		"st1 {%[rearm].2d}, [%[e2]] \n\t"
		"add %[e2], %[e2], #16 \n\t"
		"st1 {%[rearm].2d}, [%[e3]] \n\t"
		"add %[e3], %[e3], #16 \n\t"
		/* C.1 combine data from mCQEs with rx_descriptor_fields1. */
		"tbl v18.16b, {v17.16b}, %[mcqe_shuf_m1].16b \n\t"
		"tbl v19.16b, {v17.16b}, %[mcqe_shuf_m2].16b \n\t"
		"sub v18.8h, v18.8h, %[crc_adj].8h \n\t"
		"sub v19.8h, v19.8h, %[crc_adj].8h \n\t"
		"orr v18.16b, v18.16b, %[rxdf].16b \n\t"
		"orr v19.16b, v19.16b, %[rxdf].16b \n\t"
		/* D.1 store rx_descriptor_fields1. */
		"st1 {v18.2d}, [%[e2]] \n\t"
		"st1 {v19.2d}, [%[e3]] \n\t"
		:[byte_cnt]"=&w"(byte_cnt)
		:[mcq]"r"(p),
		 [rxdf]"w"(rxdf),
		 [rearm]"w"(rearm),
		 [e3]"r"(e3), [e2]"r"(e2), [e1]"r"(e1), [e0]"r"(e0),
		 [mcqe_shuf_m1]"w"(mcqe_shuf_m1),
		 [mcqe_shuf_m2]"w"(mcqe_shuf_m2),
		 [crc_adj]"w"(crc_adj),
		 [len_shuf_m]"w"(len_shuf_m)
		:"memory", "v16", "v17", "v18", "v19");
    return byte_cnt;
}
#endif

#ifdef TEST16
void test16(unsigned int a) {
  __asm__ volatile("mrs %x0, CNTV_CTL_EL0\n\t" : "=r" (a) : : "memory");
}
#endif

#ifdef TEST17
int test17a(int a) {
  int c;
  asm volatile("\tadd %w0, %w1, %2\n"
	       	: "=r"(c)
	       	: "r"(a), "i"(16));
  return c;
}
void test17() {
  long long l1 = 0x80000000LL;
  if (test17a(l1) != 0x80000010) {
    printf("test01 failed\n");
    abort();
  }
}
#endif

#ifdef TEST18
#if 0
#define _ATOMIC_CMPSET(bar, a, l)		\
_ATOMIC_CMPSET_IMPL(8, w, b, bar, a, l)
//_ATOMIC_CMPSET_IMPL(16, w, h, bar, a, l)
//_ATOMIC_CMPSET_IMPL(32, w, , bar, a, l)
//_ATOMIC_CMPSET_IMPL(64, , , bar, a, l)

#define _ATOMIC_CMPSET_IMPL(t, w, s, bar, a, l)		\
_ATOMIC_CMPSET_PROTO(t, bar, _llsc)		\
{		\
uint##t##_t tmp;		\
int res;		\
		\
__asm __volatile(		\
"1: mov %w1, #1\n"		\
" ld"#a"xr"#s" %"#w"0, [%2]\n"		\
" cmp %"#w"0, %"#w"3\n"		\
" b.ne 2f\n"		\
" st"#l"xr"#s" %w1, %"#w"4, [%2]\n"		\
" cbnz %w1, 1b\n"		\
"2:"		\
: "=&r"(tmp), "=&r"(res)		\
: "r" (p), "r" (cmpval), "r" (newval)		\
: "cc", "memory"		\
);	\
return (!res); \

#endif

void *p1;
long long cmpval;
int newval;
void test18a() {
  unsigned long long tmp;
  int res;
    __asm __volatile(
	"1: mov %w1, #1\n"
	" ldr %x0, [%2]\n"
	" cmp %x0, %x3\n"
	" b.ne 2f\n"
	" stp %w1, %w4, [%2]\n"
	" cbnz %w1, 1b\n"
	"2:"
		: "=&r"(tmp), "=&r"(res)
		: "r" (p1), "r" (cmpval), "r" (newval)
		: "cc", "memory"
	);
}

//void test18() {
//  test18a();
//}
#endif

int main()
{
#ifdef TEST01
  test01();
#endif
#ifdef TEST02
  test02();
#endif
#ifdef TEST03
  test03();
#endif
#ifdef TEST04
  test04();
#endif
#ifdef TEST05
  test05();
#endif
#ifdef TEST06
  test06();
#endif
#ifdef TEST07
  test07();
#endif
#ifdef TEST08
  test08();
#endif
#ifdef TEST09
  test09();
#endif
#ifdef TEST10
  test10();
#endif
#ifdef TEST11
  test11();
#endif
#ifdef TEST12
  test12();
#endif
#ifdef TEST13
  test13();
#endif
#ifdef TEST14
  test14();
#endif
#ifdef TEST15
  test15();
#endif
#ifdef TEST17
  test17();
#endif
}
