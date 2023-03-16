#include "mplpgo.h"
#include <pthread.h>

struct Mpl_Lite_Pgo_ProfileInfoRoot __mpl_pgo_info_root __attribute__ ((__visibility__ ("hidden"))) = {0, 0, 0};
extern uint32_t __mpl_pgo_sleep_time;
extern char __mpl_pgo_wait_forks;
extern char *__mpl_pgo_dump_filename;
pthread_rwlock_t rwlock;

static inline int CheckAllZero(const struct Mpl_Lite_Pgo_FuncInfo *funcPtr) {
  const uint64_t *cptr = funcPtr->counters;
  for (size_t i = 0; cptr && i < funcPtr->counterNum; ++i, cptr++) {
    if (*cptr != 0) {
      return 0;
    }
  }
  return 1;
}

static inline struct Mpl_Lite_Pgo_DumpInfo *CreateDumpNode(char *inStr) {
  struct Mpl_Lite_Pgo_DumpInfo *di = malloc(sizeof(struct Mpl_Lite_Pgo_DumpInfo));
  if (di == NULL) {
    return NULL;
  }
  memset(di, 0, sizeof(struct Mpl_Lite_Pgo_DumpInfo));
  di->pgoFormatStr = inStr;
  di->next = NULL;
}

static inline void EmitFunctionDesc(unsigned int modHash, const struct Mpl_Lite_Pgo_FuncInfo *funcPtr, int fd) {
  /* skip function without counters */
  if (funcPtr->counterNum == 0) {
    return;
  }
  const uint64_t *cptr = funcPtr->counters;
  char lineBuf[BUFFSIZE];
  size_t emitSz = sprintf(lineBuf,"func &%s funcid %u,counterSz %lu,cfghash %u,\n\0",
      funcPtr->funcName, modHash, funcPtr->counterNum, funcPtr->cfgHash);
  write(fd, lineBuf,  emitSz);
  if (!CheckAllZero(funcPtr)) {
    for (size_t i = 0; cptr && i < funcPtr->counterNum; ++i) {
      char counterBuf[BUFFSIZE];
      size_t sizeOfCounter = sprintf(counterBuf, "%lu\n\0", *cptr);
      write(fd, counterBuf,  sizeOfCounter);
      cptr++;
    }
  }
  return;
}

static inline void EmitFlavor(int fd) {
  char flavorBuf[BUFFSIZE];
  char timeBuf[TIMEBUFSIZE];
  current_time_to_buf(timeBuf);
  size_t emitSz = sprintf(flavorBuf,"flavor %s\n\0", timeBuf);
  write(fd, flavorBuf, emitSz);
}

static inline void WriteToFile(const struct Mpl_Lite_Pgo_ObjectFileInfo *fInfo) {
  size_t txtLen = 0;
  struct Mpl_Lite_Pgo_DumpInfo *head = NULL;
  int fd = open(__mpl_pgo_dump_filename, O_RDWR | O_APPEND | O_CREAT, 0640);
  if (fd == -1) {
    perror("Error opening mpl_pgo_dump file");
    return;
  }
  pthread_rwlock_wrlock(&rwlock);
  EmitFlavor(fd);
  while (fInfo) {
    for (unsigned int i = 0; i != fInfo->funcNum; ++i) {
      const struct Mpl_Lite_Pgo_FuncInfo *funcPtr = fInfo->funcInfos[i];
      EmitFunctionDesc (fInfo->modHash, funcPtr, fd);
    }
    fInfo = fInfo->next;
  }
  pthread_rwlock_unlock(&rwlock);
  close(fd);
}

void ChildDumpProcess() {
  struct timespec ts = {0ull, 0ull};
  struct timespec rem = {0ull, 0ull};
  uint64_t ellapsed = 0ull;
  uint64_t ppid;
  if (__mpl_pgo_wait_forks) {
    ppid = -__getpgid(0);
    __setpgid(0, 0);
  } else {
    ppid = __getppid();
    if (ppid == 1) {
      if (!__mpl_pgo_info_root.dumpOnce) {
        WriteToFile(__mpl_pgo_info_root.ofileInfoList);
        __mpl_pgo_info_root.dumpOnce++;
      }
      __exit(0);
    }
  }

  ts.tv_nsec = 0;
  ts.tv_sec = 1;
  while (1) {
    __nanosleep(&ts, &rem);
    if (__kill(ppid, 0) < 0) {
      if (!__mpl_pgo_info_root.dumpOnce) {
        printf("dump in child process \n");
        WriteToFile(__mpl_pgo_info_root.ofileInfoList);
        printf("dump in child process end 01 \n");
        __mpl_pgo_info_root.dumpOnce++;
      }
      break;
    }
    if (++ellapsed < __mpl_pgo_sleep_time) {
      continue;
    }
    ellapsed = 0;
    if (!__mpl_pgo_info_root.dumpOnce) {
      printf("dump in child process 02 \n");
      WriteToFile(__mpl_pgo_info_root.ofileInfoList);
      __mpl_pgo_info_root.dumpOnce++;
    }
  }
  __exit(0);
}

void ShareCountersInProcess() {
#if ENABLE_MMAP
  const uint64_t CountersStart = (uint64_t)(&__start_mpl_counter);
  const uint64_t CountersEnd = (uint64_t)(&__stop_mpl_counter);
  if (CountersEnd <= CountersStart) {
    perror("empty mpl counter section");
    return;
  }
  printf("start mapping shared memory start address : %x end address : %x \n", CountersStart, CountersEnd);
  printf("shared memory length : %lu\n", CountersEnd - CountersStart);
  void *p = __mmap(
      CountersStart, CountersEnd - CountersStart, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON | MAP_FIXED, -1, 0);
  printf("mmap start address : %p\n", p);
#endif
}

static inline void FlushCounters(struct Mpl_Lite_Pgo_ObjectFileInfo *fInfo) {
  pthread_rwlock_wrlock(&rwlock);
  while (fInfo) {
    for (unsigned int i = 0; i != fInfo->funcNum; ++i) {
      struct Mpl_Lite_Pgo_FuncInfo *funcPtr = fInfo->funcInfos[i];
      uint64_t *cptr = funcPtr->counters;
      for (size_t i = 0; cptr && i < funcPtr->counterNum; ++i) {
        *cptr = 0;
        cptr++;
      }
    }
    fInfo = fInfo->next;
  }

  pthread_rwlock_unlock(&rwlock);
}

// use to watch dump timer && set mutex
void __mpl_pgo_setup() {
  if (!__mpl_pgo_info_root.setUp) {
    __mpl_pgo_info_root.setUp++;
    if (__mpl_pgo_sleep_time != 0) {
      ShareCountersInProcess();
      if (__mpl_pgo_wait_forks) {
        __setpgid(0, 0);
      }
      if (__fork() == 0) {
        ChildDumpProcess();
        return;
      }

    }
  }
}

void __mpl_pgo_init(struct Mpl_Lite_Pgo_ObjectFileInfo *fileInfo) {
  if (fileInfo->funcNum) {
    fileInfo->next = __mpl_pgo_info_root.ofileInfoList;
    __mpl_pgo_info_root.ofileInfoList = fileInfo;
  }
}

void __mpl_pgo_exit() {
  if (__mpl_pgo_info_root.dumpOnce == 0 && __mpl_pgo_sleep_time == 0) {
    WriteToFile(__mpl_pgo_info_root.ofileInfoList);
    __mpl_pgo_info_root.dumpOnce++;
  }
}

void __mpl_pgo_dump_wrapper() {
  asm volatile ( SAVE_ALL :::);
  pthread_rwlock_init(&rwlock, NULL);
  if (((unsigned int)(__mpl_pgo_info_root.dumpOnce) % 10000ul) == 0) {
    WriteToFile(__mpl_pgo_info_root.ofileInfoList);
  }
  __mpl_pgo_info_root.dumpOnce++;
  pthread_rwlock_destroy(&rwlock);
  asm volatile ( RESTORE_ALL :::);
}

void __mpl_pgo_flush_counter() {
  asm volatile ( SAVE_ALL :::);
  pthread_rwlock_init(&rwlock, NULL);
  FlushCounters(__mpl_pgo_info_root.ofileInfoList);
  pthread_rwlock_destroy(&rwlock);
  asm volatile ( RESTORE_ALL :::);
}