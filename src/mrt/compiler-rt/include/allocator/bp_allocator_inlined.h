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
#ifndef MAPLE_RUNTIME_BP_ALLOCATOR_INLINED_H
#define MAPLE_RUNTIME_BP_ALLOCATOR_INLINED_H

#include <algorithm>
#include <sstream>

#include "chosen.h"
#include "bp_allocator.h"

namespace maplert {
inline address_t BumpPointerAlloc::AllocThrowExp(size_t size) {
  size_t internalSize = AllocUtilRndUp<address_t>(size, kBPAllocObjAlignment);
  address_t allocAddr = AllocInternal<true>(internalSize);
  if (UNLIKELY(allocAddr == 0)) {
    (*theAllocator).OutOfMemory();
    return 0;
  }
  return allocAddr;
}

// tries to allocate an object given the size
template <bool throwExp>
inline address_t BumpPointerAlloc::AllocInternal(const size_t &allocSize) {
  lock_guard<mutex> guard(globalLock);
  if (UNLIKELY((allocSize + currentAddr) > endAddr)) {
    size_t requiredSize = ALLOCUTIL_PAGE_RND_UP((allocSize - (endAddr - currentAddr)));
    size_t remainingSize = reinterpret_cast<address_t>(memMap->GetMappedEndAddr()) - endAddr;
    size_t extendSize = std::max(requiredSize, std::min(remainingSize, kExtendedSize));
    if (UNLIKELY(!memMap->Extend(extendSize))) {
      if (throwExp) {
        LOG(ERROR) << showmapName << " space out of memory, alloc size " << allocSize << ", space left " <<
            remainingSize << maple::endl;
        return 0;
      }
      // allocator failure is fatal (no OOME thrown)
      LOG(FATAL) << showmapName << " space out of memory, alloc size " << allocSize << ", space left " <<
          remainingSize << maple::endl;
      __builtin_unreachable();
      return 0;
    }
    endAddr = reinterpret_cast<address_t>(memMap->GetCurrEnd());
  }
  address_t allocAddr = currentAddr;
  currentAddr += allocSize;
  return allocAddr;
}
} // namespace maplert

#endif // MAPLE_RUNTIME_BPALLOCATOR_INLINED_H
