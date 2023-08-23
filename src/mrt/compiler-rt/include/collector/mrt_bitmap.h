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
#ifndef MAPLE_RUNTIME_MRT_BITMAP_H
#define MAPLE_RUNTIME_MRT_BITMAP_H

#include <atomic>
#include <sys/mman.h>
#include "mm_utils.h" // address_t

// set 1 to enable bitmap debug functions
#define MRT_DEBUG_BITMAP 0
namespace maplert {
// kLogObjAlignment == 4 indicates 2^4(16) bytes per bit in bitmap
static constexpr size_t kLogObjAlignment = 4;
static constexpr size_t kLogBitsPerByte = 3; // 8 bit per byte
static constexpr size_t kLogBytesPerWord = 2; // 4 byte per word
static constexpr size_t kLogBitsPerWord = kLogBitsPerByte + kLogBytesPerWord;

static constexpr size_t kBitsPerWord = sizeof(uint32_t) * 8; // 8 bits per byte.


class MrtBitmap {
 public:
  MrtBitmap() : spaceStart(0), spaceEnd(0), curEnd(0), isInitialized(false), bitmapBegin(nullptr), bitmapSize(0) {}
  ~MrtBitmap();
  // for initialization
  bool Initialized() const {
    return isInitialized;
  }

  void Initialize();
  void ResetCurEnd();
  ALWAYS_INLINE void ResetBitmap() {
    if (bitmapBegin != nullptr) {
#ifdef __ANDROID__
      int result = madvise(bitmapBegin, bitmapSize, MADV_DONTNEED);
      if (result != 0) {
        LOG(WARNING) << "ResetBitmap madvise failed";
        size_t curBitmapSize = GetBitmapSizeByHeap(curEnd - spaceStart);
        (void)memset_s(reinterpret_cast<void*>(bitmapBegin), curBitmapSize, 0, curBitmapSize);
      }
#else
      // use memset on qemu
      size_t curBitmapSize = GetBitmapSizeByHeap(curEnd - spaceStart);
      (void)memset_s(reinterpret_cast<void*>(bitmapBegin), curBitmapSize, 0, curBitmapSize);
#endif
    }
  }

  // return true if object already marked, false if unmarked
  ALWAYS_INLINE bool MarkObject(address_t addr) {
    uint32_t lineIndex;
    uint32_t bitMask;
    GetLocation(addr, lineIndex, bitMask);
    std::atomic<uint32_t> *line = bitmapBegin + lineIndex;

    uint32_t old = line->fetch_or(bitMask, std::memory_order_relaxed);
    return ((old & bitMask) != 0);
  }

  ALWAYS_INLINE bool IsObjectMarked(address_t addr) const {
    uint32_t lineIndex;
    uint32_t bitMask;
    GetLocation(addr, lineIndex, bitMask);
    std::atomic<uint32_t> *line = bitmapBegin + lineIndex;
    uint32_t word = line->load(std::memory_order_relaxed);
    return (word & bitMask) != 0;
  }

#if MRT_DEBUG_BITMAP
  // copy from another bitmap, for debug purpose.
  void CopyBitmap(const BitMap &bitmap);

  // size in bytes, just for debug purpose.
  inline size_t Size() const {
    return bitmapSize;
  }

  // data as an uint32_t array, for debug purpose.
  inline const uint32_t *Data() const {
    return reinterpret_cast<uint32_t*>(bitmapBegin);
  }
#endif

 private:
  ALWAYS_INLINE bool AddrInRange(address_t addr) const {
    return (addr >= spaceStart) && (addr <= spaceEnd);
  }

  ALWAYS_INLINE void CheckAddr(address_t addr) const {
    if (UNLIKELY(!AddrInRange(addr))) {
      LOG(FATAL) << "invalid object address." << " addr:" << std::hex << addr << " spaceStart:" << spaceStart <<
          " curEnd:" << curEnd << " spaceEnd:" << spaceEnd << std::dec << maple::endl;
    }
  }

  // round value up to alignValue
  uint32_t AlignRight(uint32_t value, uint32_t alignValue) const noexcept;
  size_t GetBitmapSizeByHeap(size_t heapBytes);

  // get index in the bitmap array and bit mask in the word.
  ALWAYS_INLINE void GetLocation(address_t addr, uint32_t &lineIndex, uint32_t &bitMask) const {
#if MRT_DEBUG_BITMAP
    CheckAddr(addr);
#endif
    uint32_t offset = static_cast<uint32_t>(addr - spaceStart);

    // lower bitmap word in the bitmapWords array encodes lower address
    lineIndex = (offset >> kLogBitsPerWord) >> kLogObjAlignment;

    // higher bits in each word encodes lower address
    unsigned int bitIndex = kBitsPerWord - 1 - ((offset >> kLogObjAlignment) & (kBitsPerWord - 1));
    bitMask = static_cast<uint32_t>(1) << bitIndex;
  }

  // the start and end of memory space covered by the bitmap.
  address_t spaceStart;
  address_t spaceEnd;
  address_t curEnd;
  bool isInitialized;
  std::atomic<uint32_t> *bitmapBegin;
  size_t bitmapSize;
};
}  // namespace maplert

#endif // MAPLE_RUNTIME_MRT_BITMAP_H
