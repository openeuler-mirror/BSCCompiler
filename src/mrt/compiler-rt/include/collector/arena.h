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
#ifndef MAPLE_RUNTIME_ARENA_H
#define MAPLE_RUNTIME_ARENA_H

#include "address.h"
#include "base/logging.h"
#include "jni.h"
#include "panic.h"
#include "allocator/page_pool.h"

namespace maplert {
static constexpr uint32_t kLocalVarHeaderSlotCount = 1;
static constexpr uint32_t kLocalVarBankSlotCount = maple::kPageSize / sizeof(address_t) - kLocalVarHeaderSlotCount;

// Use dynamic allocated MemoryBank to save address_t content. Basic operation is save snapshot
// pop to snapshot/iterate Object Handle Arena or iterate to snapshot.
// HandleArena use bump-the-pointer allocation to serve object handle allocation. It should have
// stack-style allocation and deallocation, which a set of successive allocated handles are bulk
// freed at a specific code point.
class HandleArena {
  struct MemoryBank {
    MemoryBank *prev = nullptr;
    address_t *BeginSlotAddr() {
      return (reinterpret_cast<address_t*>(this)) + kLocalVarHeaderSlotCount;
    }
    address_t *EndSlotAddr() {
      return (reinterpret_cast<address_t*>(this)) + kLocalVarHeaderSlotCount + kLocalVarBankSlotCount;
    }
  };

 public:
  HandleArena() : tail(nullptr), top(nullptr) {}
  ~HandleArena() {
    if (tail != nullptr) {
      PopBanks(nullptr, nullptr);
    }
  }
  HandleArena &operator=(const HandleArena &other) {
    this->tail = other.tail;
    this->top = other.top;
    return *this;
  }
  void VisitGCRoots(const RefVisitor &visitor);
  void VisitTopSnapShot(const RefVisitor &visitor, const HandleArena &snapShot) noexcept {
    VisitTopSnapShot(visitor, snapShot.tail, snapShot.top);
  }

  // bump-the-pointer allocation
  address_t *AllocateSlot() {
    if (tail == nullptr || top == tail->EndSlotAddr()) {
      GetNewBank();
    }
    address_t *result = top;
    ++top;
    return result;
  }

  // utility function for bulk free
  void PopBanks(const HandleArena &snapshot) noexcept {
    PopBanks(snapshot.tail, snapshot.top);
  }

  inline void Clear() noexcept {
    tail = nullptr;
    top = nullptr;
  }
 private:
  MemoryBank *tail;    // current allocatable bank
  address_t *top;      // current allocatable pointer
  inline void GetNewBank() {
    static_assert(sizeof(MemoryBank*) == sizeof(address_t), "not equal size");
    MemoryBank *mb = reinterpret_cast<MemoryBank*>(PagePool::Instance().GetPage());
    if (UNLIKELY(mb == nullptr)) {
      LOG(FATAL) << "Failed to allocate a new bank" << maple::endl;
    }
    mb->prev = tail;
    tail = mb;
    top = tail->BeginSlotAddr();
  }

  void PopBanks(MemoryBank *toTail, address_t *toTop) noexcept {
    while (tail != toTail && tail != nullptr) {
      MemoryBank *prev = tail->prev;
      PagePool::Instance().ReturnPage(reinterpret_cast<uint8_t*>(tail));
      tail = prev;
    }
    top = toTop;
  }

  void VisitTopSnapShot(const RefVisitor &visitor, MemoryBank *toBank, address_t *toTop) {
    MemoryBank *curBank = tail;
    address_t *curTop = top;
    if (curBank == nullptr) {
      return;
    }
    while (curBank != toBank) {
      address_t *start = curBank->BeginSlotAddr();
      address_t *end = curTop;
      for (address_t *iter = start; iter < end; ++iter) {
        visitor(*iter);
      }
      curBank = curBank->prev;
      if (curBank != nullptr) {
        curTop = curBank->EndSlotAddr();
      } else {
        __MRT_ASSERT(toBank == nullptr && toTop == nullptr, "unexpected");
        curTop = nullptr;
      }
    }
    // curBank is endBank, visit from curTop/end to endTop
    if (curTop != toTop) {
      for (address_t *iter = toTop; iter < curTop; ++iter) {
        visitor(*iter);
      }
    }
  }
};

class MObject;
class MArray;
class MString;
// Indirect holder for Java heap object in runtime, points to a slot in HandleArena
// It can be passed to callee, but cannot returned directly.
// Hierarchy:
//   HandleBase
//     ObjHandle<MObject>
//        ObjHandle<MString>
//        ObjHandle<MArray>
class HandleBase {
 public:
  virtual ~HandleBase() {
    handle = nullptr;
  }
  inline address_t AsRaw() const {
    return *handle;
  }
  inline jobject AsJObj() const {
    return reinterpret_cast<jobject>(AsRaw());
  }
  inline jobjectArray AsJObjArray() const {
    return reinterpret_cast<jobjectArray>(AsRaw());
  }
  inline MObject *AsObject() const {
    return reinterpret_cast<MObject*>(AsRaw());
  }
  inline MArray *AsArray() const {
    return reinterpret_cast<MArray*>(AsRaw());
  }
  inline bool operator==(const HandleBase &ref) const {
    return AsRaw() == ref.AsRaw();
  }
  inline bool operator==(const address_t otherObj) const {
    return AsRaw() == otherObj;
  }
  inline address_t Return() {
    address_t old = AsRaw();
    *handle = 0;
    return old;
  }
  inline jobject ReturnJObj() {
    return reinterpret_cast<jobject>(Return());
  }
  inline MObject *ReturnObj() {
    return reinterpret_cast<MObject*>(Return());
  }
 protected:
  inline void Release() noexcept {
    if (handle != nullptr) {
      *handle = 0;
    }
  }
  void Push(address_t ref);
  address_t *handle = nullptr; // indirect pointer(reference) of raw heap object pointer
};

// RC collector is supported in maple, rc count should be maintained in the Handle.
// Some Handle do produce rc count, for example return value from another function,
// it should use the following pattern:
//   ObjectHandle<MObject, true> newObj(NewObject(...))
//
// But some handle do not produce rc count, for example the parameter arguments, it
// should use the following pattern:
//   ObjectHandle<MObject, false> argment(arg0)
template<class T, bool needRC = true>
class ObjHandle : public HandleBase {
  using pointer = typename std::add_pointer<T>::type;
  using lref = typename std::add_lvalue_reference<T>::type;
 public:
  explicit ObjHandle(address_t ref) {
    Push(ref);
  }
  explicit ObjHandle(jobject ref) {
    Push(reinterpret_cast<address_t>(ref));
  }
  explicit ObjHandle(MObject *object) {
    Push(reinterpret_cast<address_t>(object));
  }
  explicit ObjHandle(MString *str) {
    Push(reinterpret_cast<address_t>(str));
  }
  explicit ObjHandle(MArray *array) {
    Push(reinterpret_cast<address_t>(array));
  }
  ~ObjHandle() {
    if (!needRC) {
      Release();
    }
  }
  inline pointer operator->() {
    return reinterpret_cast<pointer>(AsRaw());
  }
  inline pointer operator()() {
    return reinterpret_cast<pointer>(AsRaw());
  }
  inline lref operator*() {
    return reinterpret_cast<lref>(AsRaw());
  }
};

// Used to snapshot the arena usage.
// It should be used before Handle allocation (means ObjectHandle declaration),
// the current top of the arena is recorded, and the memory above the recorded
// top is returned upon the destruction of ScopedHandles.
class ScopedHandles {
 public:
  ScopedHandles();
  ~ScopedHandles();

  ScopedHandles(const ScopedHandles&) = delete;
  ScopedHandles(ScopedHandles&&) = delete;
  ScopedHandles &operator=(const ScopedHandles&) = delete;
  ScopedHandles &operator=(ScopedHandles&&) = delete;
 private:
  HandleArena snapshot;
};
}  // namespace maplert

#endif
