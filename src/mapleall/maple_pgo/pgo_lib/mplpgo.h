/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
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