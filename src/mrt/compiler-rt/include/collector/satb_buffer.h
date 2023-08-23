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
#ifndef MAPLE_RUNTIME_SATB_BUFFER_H
#define MAPLE_RUNTIME_SATB_BUFFER_H

#include "allocator/page_pool.h"
#include "collector/mrt_bitmap.h"

namespace maplert {
// snapshot at the beginning buffer
// mainly used to buffer modified field of mutator write
class SatbBuffer {
  static constexpr size_t kInitialPages = 64; // 64 pages of initial satb buffer
  static constexpr size_t kNodeSize = 512 - 8; // size of SatbBuffer node, which can store 63 entries
 public:
  static SatbBuffer &Instance();

  class Node {
    friend class SatbBuffer;
   public:
    Node() {
      top = reinterpret_cast<address_t*>(this + 1);
      next = nullptr;
    }
    ~Node() = default;
    bool IsEmpty() const {
      return reinterpret_cast<size_t>(top) == (reinterpret_cast<size_t>(this) + sizeof(Node));
    }
    bool IsFull() const {
      static_assert((sizeof(Node) % sizeof(address_t*)) == 0, "Satb node must be aligned");
      return reinterpret_cast<size_t>(top) == (reinterpret_cast<size_t>(this) + kNodeSize);
    }
    void Push(address_t obj) {
      *top = obj;
      top++;
    }
    template<typename T>
    void GetObjects(T &stack) const {
      address_t *start = reinterpret_cast<address_t*>(const_cast<Node*>(this + 1));
      __MRT_ASSERT(top <= reinterpret_cast<address_t*>(reinterpret_cast<uintptr_t>(this) + kNodeSize), "invalid node");
      (void)std::copy(start, top, std::back_inserter(stack));
    }
   private:
    address_t *top;
    Node *next;
  };

  struct Page {
    Page *next;
  };

  // there is no need to use LL/SC to avoid ABA problem, becasue Nodes are all unique.
  template<typename T>
  class LockFreeList {
    friend class SatbBuffer;
   public:
    LockFreeList() : head(nullptr) {}
    ~LockFreeList() = default;

    void Reset() {
      head = nullptr;
    }

    void Push(T *n) {
      T *old = head.load(std::memory_order_relaxed);
      do {
        n->next = old;
      } while (!head.compare_exchange_weak(old, n, std::memory_order_release, std::memory_order_relaxed));
    }

    T *Pop() {
      T *old = head.load(std::memory_order_relaxed);
      do {
        if (old == nullptr) {
          return nullptr;
        }
      } while (!head.compare_exchange_weak(old, old->next, std::memory_order_release, std::memory_order_relaxed));
      old->next = nullptr;
      return old;
    }

    T *PopAll() {
      T *old = head.load(std::memory_order_relaxed);
      while (!head.compare_exchange_weak(old, nullptr, std::memory_order_release, std::memory_order_relaxed)) {};
      return old;
    }
   private:
    std::atomic<T*> head;
  };

  void EnsureGoodNode(Node *&node) {
    if (node == nullptr) {
      node = freeNodes.Pop();
    } else if (node->IsFull()) {
      // means current node is full
      retiredNodes.Push(node);
      node = freeNodes.Pop();
    } else {
      // not null & have slots
      return;
    }
    if (node == nullptr) {
      // there is no free nodes in the freeNodes list
      Page *page = GetPages(maple::kPageSize);
      Node *list = ConstructFreeNodeList(page, maple::kPageSize);
      node = list;
      Node *cur = list->next;
      node->next = nullptr;
      while (cur != nullptr) {
        Node *next = cur->next;
        freeNodes.Push(cur);
        cur = next;
      }
    }
  }
  bool ShouldEnqueue(address_t obj) {
    if (markMap->IsObjectMarked(obj)) {
      return false;
    }
    return !enqueueMap.MarkObject(obj);
  }
  // must not have thread racing
  void Init(MrtBitmap *map) {
    size_t initalBytes = kInitialPages * maple::kPageSize;
    Page *page = GetPages(initalBytes);
    Node *list = ConstructFreeNodeList(page, initalBytes);
    freeNodes.head = list;
    retiredNodes.head = nullptr;
    enqueueMap.Initialize();
    enqueueMap.ResetCurEnd();
    markMap = map;
  }
  void RetireNode(Node *node) {
    retiredNodes.Push(node);
  }
  // must not have thread racing
  void Reset() {
    Page *list = arena.head;
    while (list->next != nullptr) {
      Page *next = list->next;
      PagePool::Instance().ReturnPage(reinterpret_cast<uint8_t*>(list), maple::kPageSize);
      list = next;
    }
    PagePool::Instance().ReturnPage(reinterpret_cast<uint8_t*>(list), kInitialPages * maple::kPageSize);
    arena.head = nullptr;
    freeNodes.head = nullptr;
    retiredNodes.head = nullptr;
    enqueueMap.ResetBitmap();
    markMap = nullptr;
  }
  template<typename T>
  void GetRetiredObjects(T &stack) {
    Node *head = retiredNodes.PopAll();
    while (head != nullptr) {
      head->GetObjects(stack);
      head = head->next;
    }
  }
 private:
  Page *GetPages(size_t bytes) {
    Page *page = reinterpret_cast<Page*>(PagePool::Instance().GetPage(bytes));
    page->next = nullptr;
    arena.Push(page);
    return page;
  }
  Node *ConstructFreeNodeList(const Page *page, size_t bytes) const {
    address_t start = reinterpret_cast<address_t>(page) + sizeof(Page);
    address_t end = reinterpret_cast<address_t>(page) + bytes;
    Node *cur = nullptr;
    Node *head = nullptr;
    while (start <= (end - kNodeSize)) {
      Node *node = new (reinterpret_cast<void*>(start)) Node();
      if (cur == nullptr) {
        cur = node;
        head = node;
      } else {
        cur->next = node;
        cur = node;
      }
      start += kNodeSize;
    }
    return head;
  }

  LockFreeList<Page> arena; // arena of allocatable area, first area is 64 * 4k = 256k, the rest is 4k
  LockFreeList<Node> freeNodes; // free nodes, mutator will acquire nodes from this list to record old value writes
  LockFreeList<Node> retiredNodes; // has been filled by mutator, ready for scan
  MrtBitmap enqueueMap; // enqueue bitmap to indicate whether the obj has been enqueued to SatbBuffer
  MrtBitmap *markMap; // mark bitmap to filter marked objects from being enqueued to SatbBuffer
};
} // namespace maplert

#endif // MAPLE_RUNTIME_SATB_BUFFER_H
