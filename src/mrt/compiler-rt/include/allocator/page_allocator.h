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
#ifndef MAPLE_RUNTIME_PAGEALLOCATOR_H
#define MAPLE_RUNTIME_PAGEALLOCATOR_H

#include <pthread.h>
#include <sys/mman.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>

#include "mm_utils.h"
#include "globals.h"
#include "panic.h"
#include "deps.h"
#include "page_pool.h"
#include "mrt_api_common.h"

#define PA_VERBOSE_LVL DEBUGY

namespace maplert {
// when there is a need to use PageAllocator to manage
// the memory for a specific data structure, please add
// a new type
enum AllocationTag : uint32_t {
  kMinAllocationTag = 0u,
  // allocation type for customized data struction
  kCartesianTree = kMinAllocationTag, // manage memory for CartesianTree Node

  // alllocation type for std container
  kReferenceProcessor,   // manage the std container in ReferenceProcessor
  kROSAllocator,         // for ROS
  kMutatorList,          // for mutator list
  kClassNameStringPool,  // for className string pool
  kZterpAllocator,       // for Zterp
  kGCWorkStack,          // for gc mark and write barrier
  kGCWriteBarrier,       // for write barrier
  kClassLoaderAllocator, // for classloader
  kGCTaskQueue,          // for gc task queue
  // more to come
  kMaxAllocationTag
};

// constants and utility function
class AllocatorUtils {
 public:
  AllocatorUtils() = delete;
  AllocatorUtils(const AllocatorUtils&) = delete;
  AllocatorUtils(AllocatorUtils&&) = delete;
  AllocatorUtils &operator=(const AllocatorUtils&) = delete;
  AllocatorUtils &operator=(AllocatorUtils&&) = delete;
  ~AllocatorUtils() = delete;
  static constexpr uint32_t kLogAllocPageSize = 12;
  static constexpr uint32_t kAllocPageSize = maple::kPageSize;
  static constexpr uint32_t kLogAllocAlignment = 3;
  static constexpr uint32_t kAllocAlignment = 1 << kLogAllocAlignment;
  static constexpr uint32_t kMaxSlotSize = kAllocPageSize / 2;
};

// Allocator manages page allocation and deallocation
class PageAllocator {
  static constexpr uint32_t kMaxCached = 0;

  // slots in a page are managed as a linked list
  struct Slot {
    Slot *next = nullptr;
  };

  // pages are linked to each other as double-linked list.
  // the free slot list and other infomation are also in
  // the page header
  class Page {
    friend class PageAllocator;

   public:
    // get a slot from the free slot list
    inline void *Allocate() {
      if (header != nullptr) {
        void *result = reinterpret_cast<void*>(header);
        header = header->next;
        --free;
        return result;
      }
      return nullptr;
    }

    // return a slot to the free slot list
    inline void Deallocate(void *slot) {
      Slot *cur = reinterpret_cast<Slot*>(slot);
      cur->next = header;
      header = cur;
      ++free;
    }

    inline bool Available() const {
      return free != 0;
    }

    inline bool Empty() const {
      return free == total;
    }

   private:
    Page *prev = nullptr;
    Page *next = nullptr;
    Slot *header = nullptr;
    uint16_t free = 0;
    uint16_t total = 0;
  };

 public:
  PageAllocator() : nonFull(nullptr), cached(nullptr), totalPages(0), totalCached(0), slotSize(0), slotAlignment(0) {}

  explicit PageAllocator(uint32_t size)
      : nonFull(nullptr), cached(nullptr), totalPages(0), totalCached(0), slotSize(size) {
    slotAlignment = maple::AlignUp<uint32_t>(size, AllocatorUtils::kAllocAlignment);
  }

  ~PageAllocator() = default;

  void Destroy() {
    DestroyList(nonFull);
    DestroyList(cached);
  }

  void Init(uint32_t size) {
    slotSize = size;
    slotAlignment = maple::AlignUp<uint32_t>(size, AllocatorUtils::kAllocAlignment);
  }

  // allocation entrypoint
  void *Allocate() {
    void *result = nullptr;
    {
      std::lock_guard<std::mutex> guard(allocLock);

      if (nonFull == nullptr) {
        if (cached != nullptr) {
          Page *cur = cached;
          RemoveFromList(cached, *cur);
          AddToList(nonFull, *cur);
          --totalCached;
        } else {
          Page *cur = CreatePage();
          InitPage(*cur);
          nonFull = cur;
          ++totalPages;
        }
        LOG(PA_VERBOSE_LVL) << "\ttotal pages mapped:  " << totalPages <<
            ", total cached page: " << totalCached << ", slot_size: " << slotSize << maple::endl;
      }

      result = nonFull->Allocate();

      if (!(nonFull->Available())) {
        // move from nonFull to full
        Page *cur = nonFull;
        RemoveFromList(nonFull, *cur);
      }
    }
    if (result != nullptr) {
      if (memset_s(result, slotSize, 0, slotSize) != EOK) {
        LOG(FATAL) << "memset_s fail" << maple::endl;
      }
    }
    return result;
  }

  // deallocation entrypoint
  void Deallocate(void *slot) {
    Page *cur = reinterpret_cast<Page*>(maple::AlignDown<uintptr_t>(reinterpret_cast<uintptr_t>(slot),
                                                                    AllocatorUtils::kAllocPageSize));

    std::lock_guard<std::mutex> guard(allocLock);
    if (!(cur->Available())) {
      // move from full to nonFull
      AddToList(nonFull, *cur);
    }
    cur->Deallocate(slot);
    if (cur->Empty()) {
      RemoveFromList(nonFull, *cur);
      if (totalCached < kMaxCached) {
        AddToList(cached, *cur);
        ++totalCached;
      } else {
        DestroyPage(*cur);
        --totalPages;
      }
      LOG(PA_VERBOSE_LVL) << "\ttotal pages mapped:  " << totalPages <<
          ", total cached page: " << totalCached << ", slot_size: " << slotSize << maple::endl;
    }
  }

 private:
  // get a page from os
  static inline Page *CreatePage() {
    return reinterpret_cast<Page*>(PagePool::Instance().GetPage());
  }

  // return the page to os
  static inline void DestroyPage(Page &page) {
    if (page.free != page.total) {
      LOG(FATAL) << "\t destroy page in use: total = " << page.total << ", free = " << page.free << maple::endl;
    } else {
      LOG(PA_VERBOSE_LVL) << "\t destroy page " << std::hex << &page << std::dec <<
          " total = " << page.total << ", free = " << page.free << maple::endl;
    }
    PagePool::Instance().ReturnPage(reinterpret_cast<uint8_t*>(&page));
  }

  // construct the data structure of a new allocated page
  void InitPage(Page &page) {
    page.prev = nullptr;
    page.next = nullptr;
    constexpr uint32_t offset = maple::AlignUp<uint32_t>(sizeof(Page), AllocatorUtils::kAllocAlignment);
    page.free = (AllocatorUtils::kAllocPageSize - offset) / slotAlignment;
    page.total = page.free;
    if (UNLIKELY(page.free < 1)) {
      LOG(FATAL) << "use the wrong allocator! slot size = " << slotAlignment << maple::endl;
    }

    char *start = reinterpret_cast<char*>(&page);
    char *end = start + AllocatorUtils::kAllocPageSize - 1;
    char *slot = start + offset;
    page.header = reinterpret_cast<Slot*>(slot);
    Slot *prevSlot = page.header;
    while (true) {
      slot += slotAlignment;
      char *slotEnd = slot + slotAlignment - 1;
      if (slotEnd > end) {
        break;
      }

      Slot *cur = reinterpret_cast<Slot*>(slot);
      prevSlot->next = cur;
      prevSlot = cur;
    }

    LOG(PA_VERBOSE_LVL) << "new page start = " << std::hex << start << ", end = " << end <<
        ", slot header = " << page.header << std::dec <<
        ", total slots = " << page.total << ", slot size = " << slotAlignment <<
        ", sizeof(Page) = " << sizeof(Page) << maple::endl;
  }

  // linked-list management
  inline void AddToList(Page *&list, Page &page) {
    if (list != nullptr) {
      list->prev = &page;
    }
    page.next = list;
    list = &page;
  }

  inline void RemoveFromList(Page *&list, Page &page) {
    Page *prev = page.prev;
    Page *next = page.next;
    if (&page == list) {
      list = next;
      if (next != nullptr) {
        next->prev = nullptr;
      }
    } else {
      prev->next = next;
      if (next != nullptr) {
        next->prev = prev;
      }
    }
    page.next = nullptr;
    page.prev = nullptr;
  }

  inline void DestroyList(Page *&list) {
    Page *cur = nullptr;
    while (list != nullptr) {
      cur = list;
      list = list->next;
      DestroyPage(*cur);
    }
  }

  Page *nonFull;
  Page *cached;
  std::mutex allocLock;
  uint32_t totalPages;
  uint32_t totalCached;
  uint32_t slotSize;
  uint32_t slotAlignment;
};

// Utility class used for StdContainerAllocator
// It has lots of PageAllocators, each for different slot size,
// so all allocation sizes can be handled by this bridge class.
class AggregateAllocator {
 public:
  static constexpr uint32_t kMaxAllocators =
      AllocatorUtils::kMaxSlotSize >> AllocatorUtils::kLogAllocAlignment;

  MRT_EXPORT static AggregateAllocator &Instance(AllocationTag tag);

  AggregateAllocator() {
    uint32_t slotSize = AllocatorUtils::kAllocAlignment;
    for (uint32_t i = 0; i < kMaxAllocators; ++i) {
      allocator[i].Init(slotSize);
      slotSize += AllocatorUtils::kAllocAlignment;
    }
  }
  ~AggregateAllocator() = default;

  // choose appropriate allocation to allocate
  void *Allocate(size_t size) {
    uint32_t alignedSize = maple::AlignUp(static_cast<uint32_t>(size), AllocatorUtils::kAllocAlignment);
    if (alignedSize <= AllocatorUtils::kMaxSlotSize) {
      uint32_t index = alignedSize >> AllocatorUtils::kLogAllocAlignment;
      return allocator[index - 1].Allocate();
    } else {
      return PagePool::Instance().GetPage(size);
    }
  }

  void Deallocate(void *p, size_t size) {
    uint32_t alignedSize = maple::AlignUp(static_cast<uint32_t>(size), AllocatorUtils::kAllocAlignment);
    if (alignedSize <= AllocatorUtils::kMaxSlotSize) {
      uint32_t index = alignedSize >> AllocatorUtils::kLogAllocAlignment;
      allocator[index - 1].Deallocate(p);
    } else {
      PagePool::Instance().ReturnPage(reinterpret_cast<uint8_t*>(p), size);
    }
  }

 private:
  PageAllocator allocator[kMaxAllocators];
};

// Allocator used to take control of memory allocation for std containers.
// It uses AggregateAllocator to dispatch the memory operation to appropriate PageAllocator.
template<class T, AllocationTag cat>
class StdContainerAllocator {
 public:
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using value_type = T;

  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;

  template<class Up>
  struct rebind {
    using other = StdContainerAllocator<Up, cat>;
  };

  StdContainerAllocator() = default;
  ~StdContainerAllocator() = default;

  template<class U>
  StdContainerAllocator(const StdContainerAllocator<U, cat>&) {}

  StdContainerAllocator(const StdContainerAllocator<T, cat>&) {}

  StdContainerAllocator(StdContainerAllocator<T, cat>&&) {}

  StdContainerAllocator<T, cat> &operator = (const StdContainerAllocator<T, cat>&) {
    return *this;
  }

  StdContainerAllocator<T, cat> &operator = (StdContainerAllocator<T, cat>&&) {
    return *this;
  }

  pointer address(reference x) const {
    return std::addressof(x);
  }

  const_pointer address(const_reference x) const {
    return std::addressof(x);
  }

  pointer allocate(size_type n, const void *hint __attribute__((unused)) = 0) {
    pointer result = static_cast<pointer>(AggregateAllocator::Instance(cat).Allocate(sizeof(T) * n));
    return result;
  }

  void deallocate(pointer p, size_type n) {
    AggregateAllocator::Instance(cat).Deallocate(p, sizeof(T) * n);
  }

  size_type max_size() const {
    return static_cast<size_type>(~0) / sizeof(value_type);
  }

  void construct(pointer p, const_reference val) {
    ::new (reinterpret_cast<void*>(p)) value_type(val);
  }

  template<class Up, class... Args>
  void construct(Up *p, Args&&... args) {
    ::new (reinterpret_cast<void*>(p)) Up(std::forward<Args>(args)...);
  }

  void destroy(pointer p) {
    p->~value_type();
  }
};
}  // end of namespace maplert

#endif
