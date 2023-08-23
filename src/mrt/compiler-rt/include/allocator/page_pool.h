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
#ifndef MAPLE_RUNTIME_ALLOCATOR_PAGE_POOL_H
#define MAPLE_RUNTIME_ALLOCATOR_PAGE_POOL_H

#include <sys/mman.h>
#include "globals.h"
#include "mrt_api_common.h"
#include "syscall.h"

namespace maplert {
// a page pool maintain a pool of free pages, serve page allocation and free
class PagePool {
  static constexpr int kNumTotalPages = 8192; // maximum of 8192 * 4K ~ 32M
  static constexpr int kLargeMemAllocateStart = kNumTotalPages / 4;
  static constexpr double kCachedRatio = 1.05; // 1.05 * used_page of pages should be cached

  class Bitmap {
    friend class PagePool;
   public:
    void UnmarkBits(int index, int num) {
      int row = GetRow(index);
      int line = GetLine(index);
      uint32_t cur = data[line].load(std::memory_order_acquire);
      uint32_t mask = GetMask(num) << static_cast<unsigned int>(row);
      while (!data[line].compare_exchange_weak(cur, cur & (~mask), std::memory_order_release,
                                               std::memory_order_acquire)) {
      }
    }

    void Unmark(int index, int num) {
      if (num <= kBitsInBitmapWord) {
        UnmarkBits(index, num);
      } else {
        UnmarkLongBits(index, num);
      }
    }

    // get the first free index
    int MarkFirstNBits(int num) {
      // the bitmap only serve maximum of 32 * 4k = 128k bytes allocation
      if (num > kBitsInBitmapWord) {
        return MarkLongBits(num);
      }

      // calculate the mask
      uint32_t mask = GetMask(num);

      // get the first index
      for (int i = 0; i < kLen; ++i) {
        uint32_t cur = data[i].load(std::memory_order_acquire);
        if ((~cur) == 0u) {
          continue;
        }
        uint32_t bit = mask;
        for (int j = 0; j < kBitsInBitmapWord - num + 1; ++j) {
          if ((cur & bit) == 0u) {
            if (data[i].compare_exchange_weak(cur, cur | bit, std::memory_order_release, std::memory_order_acquire)) {
              return i * kBitsInBitmapWord + j;
            }
          }
          bit = bit << 1;
        }
      }
      return -1;
    }

   protected:
    static inline bool InSmallArena(int num) {
      return num <= kBitsInBitmapWord;
    }

    // mark the whole word at the index
    inline bool TryMarkWord(int index) {
      int line = GetLine(index);
      uint32_t cur = data[line].load(std::memory_order_acquire);
      if (cur != 0u) {
        return false;
      }
      uint32_t mask = 0u;
      mask = ~mask;

      if (data[line].compare_exchange_weak(cur, mask,
          std::memory_order_release, std::memory_order_acquire)) {
        return true;
      }
      return false;
    }

    // mark maximum sucessive bits at index in a word
    int TryMarkBits(int index) {
      int row = GetRow(index);
      int line = GetLine(index);
      uint32_t cur = data[line].load(std::memory_order_acquire);
      uint32_t mask = 1u;
      mask = mask << static_cast<unsigned int>(row);
      if ((mask & cur) != 0) {
        return 0;
      }
      uint32_t bit = mask;
      int avail = 1;
      for (int i = 1; i < (kBitsInBitmapWord - row); ++i) {
        bit = bit << 1;
        if ((cur & bit) != 0) {
          break;
        }
        mask = mask | bit;
        ++avail;
      }

      if (data[line].compare_exchange_weak(cur, cur | mask, std::memory_order_release, std::memory_order_acquire)) {
        return avail;
      }
      return 0;
    }

   private:
    inline int GetRow(int index) const {
      return index % kBitsInBitmapWord;
    }

    inline int GetLine(int index) const {
      return index / kBitsInBitmapWord;
    }

    inline int GetTotalWords(int num) const {
      return (num + kBitsInBitmapWord - 1) / kBitsInBitmapWord;
    }

    inline uint32_t GetMask(int num) const {
      uint32_t mask = 1u;
      for (int i = 1; i < num; ++i) {
        mask = mask << 1;
        mask = mask | 1;
      }
      return mask;
    }

    bool MarkSuccesiveWords(int start, int len) {
      if (start > kNumTotalPages - kBitsInBitmapWord) {
        // exceed the last word
        return false;
      }
      if (len == 0) {
        return true;
      }
      if (TryMarkWord(start)) {
        if (MarkSuccesiveWords(start + kBitsInBitmapWord, len - 1)) {
          return true;
        } else {
          // undo current mark
          UnmarkBits(start, kBitsInBitmapWord);
        }
      }
      return false;
    }

    int MarkLongBits(int num) {
      int index = -1;
      int len = GetTotalWords(num);
      for (int i = kLargeMemAllocateStart; i < kNumTotalPages - num; i += kBitsInBitmapWord) {
        if (MarkSuccesiveWords(i, len)) {
          index = i;
          break;
        }
      }
      return index;
    }

    void UnmarkLongBits(int index, int num) {
      int len = GetTotalWords(num);
      int start = index;
      for (int i = 0; i < len; ++i) {
        UnmarkBits(start, kBitsInBitmapWord);
        start += kBitsInBitmapWord;
      }
    }

    static constexpr int kBitsInBitmapWord = 32;
    static constexpr int kLen = kNumTotalPages / kBitsInBitmapWord;
    std::atomic<uint32_t> data[kLen] = {};
  };

 public:
  PagePool() {
    size_t size = kNumTotalPages * maple::kPageSize;
    uint8_t *result = MapMemory(size);
    base = reinterpret_cast<uint8_t*>(result);
    end = base + size;
  }

  ~PagePool() {
    (void)munmap(base, kNumTotalPages * maple::kPageSize);
  }

  uint8_t *GetPage(size_t bytes = maple::kPageSize) {
    int num = static_cast<int>((bytes + maple::kPageSize - 1) / maple::kPageSize);
    int index = bitmap.MarkFirstNBits(num);
    if (index == -1) {
      return MapMemory(num * maple::kPageSize);
    }

    if (Bitmap::InSmallArena(num)) {
      smallPageUsed += num;
    }
    uint8_t *ret = base + index * maple::kPageSize;
    return ret;
  }

  void ReturnPage(uint8_t *page, size_t bytes = maple::kPageSize) {
    int num = static_cast<int>((bytes + maple::kPageSize - 1) / maple::kPageSize);
    if (page >= base && page < end) {
      int index = static_cast<int>((page - base) / maple::kPageSize);
      bitmap.Unmark(index, num);
      if (Bitmap::InSmallArena(num)) {
        smallPageUsed -= num;
      }
    } else {
      (void)munmap(page, num * maple::kPageSize);
    }
  }

  // return unused pages to os
  void Trim() {
    int start = static_cast<int>(smallPageUsed.load() * kCachedRatio);
    for (int i = start; i < kNumTotalPages;) {
      int num = bitmap.TryMarkBits(i);
      if (num == 0) {
        ++i;
        continue;
      }
      uint8_t *addr = base + i * maple::kPageSize;
      (void)madvise(addr, num * maple::kPageSize, MADV_DONTNEED);
      bitmap.UnmarkBits(i, num);
      i = i + num;
    }
  }

  MRT_EXPORT static PagePool& Instance();

 private:
  uint8_t *MapMemory(size_t size) {
    void *result = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
      LOG(FATAL) << "allocate create page failed! Out of Memory!" << maple::endl;
    }
    MRT_PRCTL(result, size, "PagePool");
    return reinterpret_cast<uint8_t*>(result);
  }

  uint8_t *base;   // start address of the mapped pages
  uint8_t *end;    // end address of the mapped pages
  std::atomic<int> smallPageUsed = { 0 };
  Bitmap bitmap; // record the state of the mapped memory: mapped or unmapped
};
} // namespace maplert
#endif  // MAPLE_RUNTIME_ALLOCATOR_PAGE_POOL_H
