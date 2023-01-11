#ifndef MPLPGO_C_COMMON_UTIL_H
#define MPLPGO_C_COMMON_UTIL_H

#define NULL ((void *)0)
typedef long unsigned int size_t;
typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int		uint32_t;
# define __uint32_t_defined
#endif
typedef unsigned long long int	uint64_t;

typedef long int		int64_t;

#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02
#ifndef O_CREAT
#define O_CREAT	     0100	/* Not fcntl.  */
#endif
#ifndef O_APPEND
# define O_APPEND	  02000
#endif

#define MAP_SHARED	0x01		/* Share changes.  */
#define MAP_PRIVATE	0x02		/* Changes are private.  */
#define MAP_FIXED       0x10            /* Interpret addr exactly */
#define MAP_ANONYMOUS	0x20		/* Don't use a file.  */
#define MAP_ANON	MAP_ANONYMOUS

# define SEEK_SET	0	/* Seek from beginning of file.  */
# define SEEK_CUR	1	/* Seek from current position.  */
# define SEEK_END	2	/* Seek from end of file.  */
#define PROT_READ	0x1		/* Page can be read.  */
#define PROT_WRITE	0x2		/* Page can be written.  */
#define PROT_EXEC	0x4		/* Page can be executed.  */
#define PROT_NONE	0x0		/* Page can not be accessed.  */
/* Flags to `msync'.  */
#define MS_ASYNC	1		/* Sync memory asynchronously.  */
#define MS_SYNC		4		/* Synchronous memory sync.  */
#define MS_INVALIDATE	2		/* Invalidate the caches.  */

/* implement in arm v8 */
uint64_t __nanosleep(const struct timespec *req,  struct timespec *rem) {
  uint64_t ret;
  register const struct timespec *x0 __asm__("x0") = req;
  register struct timespec *x1 __asm__("x1") = rem;
  register uint32_t w8 __asm__("w8") = 101;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret), "+r"(x0), "+r"(x1)
  : "r"(w8)
  : "cc", "memory");
  return ret;
}

int64_t __fork() {
  uint64_t ret;
  // clone instead of fork with flags
  // "CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD"
  register uint64_t x0 __asm__("x0") = 0x1200011;
  register uint64_t x1 __asm__("x1") = 0;
  register uint64_t x2 __asm__("x2") = 0;
  register uint64_t x3 __asm__("x3") = 0;
  register uint64_t x4 __asm__("x4") = 0;
  register uint32_t w8 __asm__("w8") = 220;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret), "+r"(x0), "+r"(x1)
  : "r"(x2), "r"(x3), "r"(x4), "r"(w8)
  : "cc", "memory");
  return ret;
}

uint64_t __getpid() {
  uint64_t ret;
  register uint32_t w8 __asm__("w8") = 172;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret)
  : "r"(w8)
  : "cc", "memory", "x0", "x1");
  return ret;
}

uint64_t __getppid() {
  uint64_t ret;
  register uint32_t w8 __asm__("w8") = 173;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret)
  : "r"(w8)
  : "cc", "memory", "x0", "x1");
  return ret;
}

uint64_t __getpgid(uint64_t pid) {
  uint64_t ret;
  register uint64_t x0 __asm__("x0") = pid;
  register uint32_t w8 __asm__("w8") = 155;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret), "+r"(x0)
  : "r"(w8)
  : "cc", "memory", "x1");
  return ret;
}

int __setpgid(uint64_t pid, uint64_t pgid) {
  int ret;
  register uint64_t x0 __asm__("x0") = pid;
  register uint64_t x1 __asm__("x1") = pgid;
  register uint32_t w8 __asm__("w8") = 154;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret), "+r"(x0), "+r"(x1)
  : "r"(w8)
  : "cc", "memory");
  return ret;
}

int __kill(uint64_t pid, int sig) {
  int ret;
  register uint64_t x0 __asm__("x0") = pid;
  register int x1 __asm__("x1") = sig;
  register uint32_t w8 __asm__("w8") = 129;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret), "+r"(x0), "+r"(x1)
  : "r"(w8)
  : "cc", "memory");
  return ret;
}

uint64_t __exit(uint64_t code) {
  uint64_t ret;
  register uint64_t x0 __asm__("x0") = code;
  register uint32_t w8 __asm__("w8") = 94;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret), "+r"(x0)
  : "r"(w8)
  : "cc", "memory", "x1");
  return ret;
}

void *__mmap(uint64_t addr, uint64_t size, uint64_t prot, uint64_t flags,
             uint64_t fd, uint64_t offset) {
  void *ret;
  register uint64_t x0 __asm__("x0") = addr;
  register uint64_t x1 __asm__("x1") = size;
  register uint64_t x2 __asm__("x2") = prot;
  register uint64_t x3 __asm__("x3") = flags;
  register uint64_t x4 __asm__("x4") = fd;
  register uint64_t x5 __asm__("x5") = offset;
  register uint32_t w8 __asm__("w8") = 222;
  __asm__ __volatile__("svc #0\n"
                       "mov %0, x0"
  : "=r"(ret), "+r"(x0), "+r"(x1)
  : "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(w8)
  : "cc", "memory");
  return ret;
}

#define SAVE_ALL                                                               \
  "stp x0, x1, [sp, #-16]!\n"                                                  \
  "stp x2, x3, [sp, #-16]!\n"                                                  \
  "stp x4, x5, [sp, #-16]!\n"                                                  \
  "stp x6, x7, [sp, #-16]!\n"                                                  \
  "stp x8, x9, [sp, #-16]!\n"                                                  \
  "stp x10, x11, [sp, #-16]!\n"                                                \
  "stp x12, x13, [sp, #-16]!\n"                                                \
  "stp x14, x15, [sp, #-16]!\n"                                                \
  "stp x16, x17, [sp, #-16]!\n"                                                \
  "stp x18, x19, [sp, #-16]!\n"                                                \
  "stp x20, x21, [sp, #-16]!\n"                                                \
  "stp x22, x23, [sp, #-16]!\n"                                                \
  "stp x24, x25, [sp, #-16]!\n"                                                \
  "stp x26, x27, [sp, #-16]!\n"                                                \
  "stp x28, x29, [sp, #-16]!\n"                                                \
  "str x30, [sp,#-16]!\n"

#define RESTORE_ALL                                                            \
  "ldr x30, [sp], #16\n"                                                       \
  "ldp x28, x29, [sp], #16\n"                                                  \
  "ldp x26, x27, [sp], #16\n"                                                  \
  "ldp x24, x25, [sp], #16\n"                                                  \
  "ldp x22, x23, [sp], #16\n"                                                  \
  "ldp x20, x21, [sp], #16\n"                                                  \
  "ldp x18, x19, [sp], #16\n"                                                  \
  "ldp x16, x17, [sp], #16\n"                                                  \
  "ldp x14, x15, [sp], #16\n"                                                  \
  "ldp x12, x13, [sp], #16\n"                                                  \
  "ldp x10, x11, [sp], #16\n"                                                  \
  "ldp x8, x9, [sp], #16\n"                                                    \
  "ldp x6, x7, [sp], #16\n"                                                    \
  "ldp x4, x5, [sp], #16\n"                                                    \
  "ldp x2, x3, [sp], #16\n"                                                    \
  "ldp x0, x1, [sp], #16\n"

#endif //MPLPGO_C_COMMON_UTIL_H
