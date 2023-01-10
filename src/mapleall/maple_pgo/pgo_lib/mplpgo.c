#include "mplpgo.h"

struct Mpl_Lite_Pgo_ProfileInfoRoot __mpl_pgo_info_root;
extern uint32_t __mpl_pgo_sleep_time;
extern char __mpl_pgo_wait_forks;
extern char *__mpl_pgo_dump_filename;

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

static inline void DumpNodeTailInsert(struct Mpl_Lite_Pgo_DumpInfo *header, struct Mpl_Lite_Pgo_DumpInfo *new){
  struct Mpl_Lite_Pgo_DumpInfo *tmp = header;
  while (tmp->next != NULL){
    tmp = tmp->next;
  }
  tmp->next = new;
}

static size_t AppendOneString(
    unsigned int modHash, const char *funcName, struct Mpl_Lite_Pgo_DumpInfo **head, const uint64_t *value) {
  size_t sizeOfALine = 8 + sizeof(modHash) + strlen(funcName) + sizeof(value);
  char *tempStr = malloc(sizeOfALine);

  sprintf(tempStr, "1 %u$$%s %lu\n\0", modHash, funcName, *value);
  struct Mpl_Lite_Pgo_DumpInfo *dumpNode = CreateDumpNode(tempStr);
  if (*head == NULL) {
    *head = dumpNode;
  } else {
    DumpNodeTailInsert(*head, dumpNode);
  }
  return sizeOfALine;
}

static inline void WriteToFile(const struct Mpl_Lite_Pgo_ObjectFileInfo *fInfo) {
  size_t txtLen = 0;
  struct Mpl_Lite_Pgo_DumpInfo *head = NULL;
  while (fInfo) {
    for (unsigned int i = 0; i != fInfo->funcNum; ++i) {
      const struct Mpl_Lite_Pgo_FuncInfo *funcPtr = fInfo->funcInfos[i];
      if (!CheckAllZero(funcPtr)) {
        const uint64_t *cptr = funcPtr->counters;
        txtLen += AppendOneString(fInfo->modHash, funcPtr->funcName, &head, &funcPtr->counterNum);
        for (size_t i = 0; cptr && i < funcPtr->counterNum; ++i) {
          txtLen += AppendOneString(fInfo->modHash, funcPtr->funcName, &head, cptr);
          cptr++;
        }
      }
    }
    fInfo = fInfo->next;
  }
  if (txtLen == 0) {
    return;
  }
  char *inputBuf = (char *)malloc(txtLen);
  memset(inputBuf, 0, strlen(inputBuf));
  const char *stop = "\0";
  strncpy(inputBuf, stop, 1);
  while(head) {
    strcat(inputBuf, head->pgoFormatStr);
    struct Mpl_Lite_Pgo_DumpInfo *prev = head;
    head = head->next;
    free(prev->pgoFormatStr);
    free(prev);
  }

  // highest file in ct device cannot over 640
  int fd = open(__mpl_pgo_dump_filename, O_RDWR | O_APPEND | O_CREAT, 0640);
  if (fd == -1) {
    perror("Error opening mpl_pgo_dump file");
    return;
  }
  int result = lseek(fd, txtLen - 1, SEEK_SET);
  if (result == -1)
  {
    close(fd);
    perror("Error calling lseek() to 'stretch' the file");
    return;
  }
  result = write(fd, "", 1);
  if (result == -1)
  {
    close(fd);
    perror("Error writing last byte of the file");
    return;
  }
#if ENABLE_MMAP // stop using mmap until hpf ready
  char *buffer = (char *)__mmap(NULL, txtLen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  memcpy(buffer, inputBuf, strlen(inputBuf));
  msync(buffer, txtLen, MS_SYNC);
  munmap(buffer, txtLen);
#else
  write(fd, inputBuf,  strlen(inputBuf));
  close(fd);
#endif
  free(inputBuf);
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
    printf("dump in main process\n");
    WriteToFile(__mpl_pgo_info_root.ofileInfoList);
    __mpl_pgo_info_root.dumpOnce++;
  }
}

/* provide internal call for user */
void __mpl_pgo_dump_wrapper() {
  asm volatile ( SAVE_ALL :::);
  if (((unsigned int)(__mpl_pgo_info_root.dumpOnce) & 31ul) == 0) {
    WriteToFile(__mpl_pgo_info_root.ofileInfoList);
  }
  __mpl_pgo_info_root.dumpOnce++;
  asm volatile ( RESTORE_ALL :::);
}
