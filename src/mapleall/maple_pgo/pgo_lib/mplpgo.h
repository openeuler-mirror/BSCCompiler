#ifndef MPLPGO_C_LIBRARY_H
#define MPLPGO_C_LIBRARY_H
#include "common_util.h"

#define BUFFSIZE 1024

struct Mpl_Lite_Pgo_DumpInfo {
  char *pgoFormatStr;
  struct Mpl_Lite_Pgo_DumpInfo *next;
};
/* information about all counters/icall/value in a function */
struct Mpl_Lite_Pgo_FuncInfo {
  const char *funcName;
  uint32_t cfgHash;
  uint32_t counterNum;
  const uint64_t * const counters;
};

/* information about all function profile instrumentation in an object file */
struct Mpl_Lite_Pgo_ObjectFileInfo {
  unsigned int modHash;
  struct Mpl_Lite_Pgo_ObjectFileInfo *next; /* build list for all object file info */
  uint64_t funcNum;
  const struct Mpl_Lite_Pgo_FuncInfo *const *funcInfos;
};

struct Mpl_Lite_Pgo_ProfileInfoRoot {
  struct Mpl_Lite_Pgo_ObjectFileInfo *ofileInfoList;
  int dumpOnce;
  int setUp;
};

extern struct Mpl_Lite_Pgo_ProfileInfoRoot __mpl_pgo_info_root  __attribute__ ((__visibility__ ("hidden"))) =
    {0, 0, 0};

void __mpl_pgo_setup();
void __mpl_pgo_init(struct Mpl_Lite_Pgo_ObjectFileInfo *fileInfo);
void __mpl_pgo_exit();
/* provide internal call for user */
void __mpl_pgo_dump_wrapper();
/* restart counting at specific point in time */
void __mpl_pgo_flush_counter();
#endif