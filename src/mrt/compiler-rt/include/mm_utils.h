/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_RUNTIME_MM_UTILS_H
#define MAPLE_RUNTIME_MM_UTILS_H

#include <cinttypes>
#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <fstream>

#include "gc_log.h"

using address_t = uintptr_t;

namespace maplert {
namespace util {
void PrintPCSymbol(const void *pc);

void PrintPCSymbolToLog(const void *pc, uint32_t logtype = kLogtypeRcTrace, bool printBt = true);

void PrintPCSymbolToLog(const void *pc, std::ostream &ofs, bool printBt);

void PrintBacktrace();

void PrintBacktrace(int32_t logFile);

void PrintBacktrace(size_t limit, int32_t logFile);

extern "C" void DumpObject(address_t obj, std::ostream &ofs);

void WaitUntilAllThreadsStopped();

std::string GetLogDir();
} // namespace util
} // namespace maplert

#endif
