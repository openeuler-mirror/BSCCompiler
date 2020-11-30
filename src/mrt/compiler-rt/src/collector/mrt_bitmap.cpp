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
#include "collector/mrt_bitmap.h"

#include "chosen.h"
#include "syscall.h"

namespace maplert {
// round value up to alignValue which must be a power of 2.
ALWAYS_INLINE uint32_t MrtBitmap::AlignRight(uint32_t value, uint32_t alignValue) const noexcept {
  return (value + alignValue - 1) & ~(alignValue - 1);
}

void MrtBitmap::ResetCurEnd() {
  curEnd = spaceStart + (*theAllocator).GetCurrentSpaceCapacity();
  LOG2FILE(kLogtypeGc) << "resetCurEnd Bitmap " << std::hex << spaceStart << " " << curEnd << std::dec << '\n';
}

ALWAYS_INLINE size_t MrtBitmap::GetBitmapSizeByHeap(size_t heapBytes) {
  size_t nBytes;
  size_t nBits;

  if ((heapBytes & ((static_cast<uint32_t>(1) << kLogObjAlignment) - 1)) == 0) {
    nBits = heapBytes >> kLogObjAlignment;
  } else {
    nBits = (heapBytes >> kLogObjAlignment) + 1;
  }

  nBytes = AlignRight(static_cast<uint32_t>(nBits), kBitsPerWord) >> kLogBitsPerByte;
  return nBytes;
}

void MrtBitmap::Initialize() {
  if (isInitialized) {
    return;
  }

  size_t maxHeapBytes = (*theAllocator).GetMaxCapacity();
  spaceStart = (*theAllocator).HeapLowerBound();
  spaceEnd = spaceStart + maxHeapBytes;
  curEnd = spaceStart + (*theAllocator).GetCurrentSpaceCapacity();

  bitmapSize = GetBitmapSizeByHeap(maxHeapBytes);
  size_t roundUpPage = AlignRight(static_cast<uint32_t>(bitmapSize), maple::kPageSize);
  void *ret = mmap(nullptr, roundUpPage, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ret == MAP_FAILED) {
    LOG(FATAL) << "maple failed to initialize MrtBitmap";
  } else {
    MRT_PRCTL(ret, roundUpPage, "MrtBitmap_Initialize");
  }

  bitmapBegin = reinterpret_cast<atomic<uint32_t>*>(ret);
  isInitialized = true;
  LOG2FILE(kLogtypeGc) << "Initialize Bitmap " << std::hex << spaceStart << " " << curEnd << std::dec << '\n';
  size_t curBitmapSize = GetBitmapSizeByHeap(curEnd - spaceStart);
  if (memset_s(reinterpret_cast<void*>(bitmapBegin), curBitmapSize, 0, curBitmapSize) != EOK) {
    LOG(FATAL) << "MrtBitmap init memset_s failed." << maple::endl;
  }
}

MrtBitmap::~MrtBitmap() {
  if (munmap(bitmapBegin, AlignRight(static_cast<uint32_t>(bitmapSize), maple::kPageSize)) != EOK) {
    LOG(ERROR) << "munmap error in MrtBitmap destruction!" << maple::endl;
  }
  bitmapBegin = nullptr;
}

#if MRT_DEBUG_BITMAP
void MrtBitmap::CopyBitmap(const MrtBitmap &bitmap) {
  if (bitmapBegin != nullptr) {
    if (munmap(bitmapBegin, AlignRight(bitmapSize, maple::kPageSize)) != 0) {
      LOG(FATAL) << "munmap error in copy bitmap";
    }
    bitmapBegin = nullptr;
  }
  isInitialized = bitmap.Initialized();
  bitmapSize = bitmap.Size();
  if (isInitialized) {
    void *result = mmap(nullptr, AlignRight(bitmapSize, maple::kPageSize),
                        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
      LOG(FATAL) << "mmap(" << bitmapSize << ") failed in copy bitmap";
    }
    bitmapBegin = reinterpret_cast<atomic<uint32_t>*> (result);
    if (memcpy_s(bitmapBegin, bitmapSize, bitmap.bitmapBegin, bitmapSize) != EOK) {
      LOG(ERROR) << "memcpy_s error in copy bitmap";
    }
  }
}
#endif
}
