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

#ifndef LOW_MEM_SET_H
#define LOW_MEM_SET_H

#include <memory>
#include "base/logging.h"
#include "base/macros.h"

namespace maple {
template <class T, class HashEqualFn>
class LowMemSet {
 public:
  static constexpr double kGuardPercent = 0.9;
  static constexpr size_t kSpaceExpandRate = 2;
  static constexpr size_t kMinCapacity = 1000;

  LowMemSet() : size(0u), capacity(0u), guardSize(0u), buf(nullptr) {}

  LowMemSet(const LowMemSet &other) = delete;

  LowMemSet(LowMemSet &&other) = delete;

  LowMemSet& operator=(LowMemSet &&other) = delete;

  LowMemSet& operator=(const LowMemSet &other) = delete;

  ~LowMemSet() {
    DeleteBuf(buf);
  }

  void Insert(T &&data) {
    EnsureSize();
    buf[FindEmptyIndex(data)] = std::move(data);
    size++;
  }

  void Insert(const T &data) {
    EnsureSize();
    buf[FindEmptyIndex(data)] = data;
    size++;
  }

  // ======== condition ======== 0 ================================= (capacity - 1)
  //               |             |                                         |
  // nextHashIndex <= nextIndex: |*****|nextHashIndex|--D--|nextIndex|*****|
  // nextHashIndex >  nextIndex: |--D--|nextIndex|*****|nextHashIndex|--D--|
  // 1.nextIndex should be large it's hashIndex. In other words: nextIndex is behind nextHashIndex.
  //   so, if nextIndex < nextHashIndex, nextIndex must be folded, then unfold it.
  // 2.if some index between in [nextHashIndex,nextIndex) was delte, nextIndex should be moved.
  void Erase(const size_t index) {
    CHECK(index < capacity) << "LowMemSet Erase index is invalid!!";
    if (UNLIKELY(fn.IsEmpty(buf[index]))) {
      return;
    }

    size_t eraseIndex = index;
    size_t nextIndex = GetNextIndex(eraseIndex);
    do {
      T &data = buf[nextIndex];
      if (fn.IsEmpty(data)) {
        fn.Clear(buf[eraseIndex]);
        break;
      }
      size_t nextHashIndex = fn(data) % capacity;
      size_t nextNoCircleIndex =
          nextIndex < nextHashIndex ? nextIndex + capacity : nextIndex;
      size_t eraseNoCircleIndex =
          eraseIndex < nextHashIndex ? eraseIndex + capacity : eraseIndex;
      if (nextHashIndex <= eraseNoCircleIndex && eraseNoCircleIndex < nextNoCircleIndex) {
        buf[eraseIndex] = std::move(buf[nextIndex]);
        eraseIndex = nextIndex;
      }
      nextIndex = GetNextIndex(nextIndex);
    } while (nextIndex != index);

    size--;

    return;
  }

  // Warning: call this funtion before insert
  void Reserve(size_t num) {
    if (capacity != 0) {
      return;
    }
    capacity = static_cast<size_t>(num / kGuardPercent + 1);
    guardSize = num;
    buf = NewBuf(capacity);
  }

  // For set: K is same to T.
  // For map: K is Key, T is std::pair<K, V>
  // return index of element.
  template <class K>
  size_t Find(const K data) {
    size_t index = (capacity != 0) ? (fn(data) % capacity) : 0;
    while (true) {
      T &entry = buf[index];
      if (fn.IsEmpty(entry)) {
        return capacity;
      }
      if (fn(data, buf[index])) {
        return index;
      }
      index = GetNextIndex(index);
    }
  }

  size_t End() const {
    return capacity;
  }

  size_t Size() const {
    return size;
  }

  // caller must ensure index is valid
  T &Element(size_t index) {
    CHECK(index < capacity) << "LowMemSet index is invalid!!";
    return buf[index];
  }

  template<typename Func>
  void ForEach(Func &&func) {
    size_t count = 0;
    for (size_t i = 0; i < capacity; i++) {
      if (fn.IsEmpty(buf[i])) {
        continue;
      }
      func(buf[i]);
      count++;
    }
    CHECK(size == count) << "LowMemSet count & size mismatch!!";
  }

 private:
  void EnsureSize() {
    if (guardSize > size) {
      return;
    }

    T *oldBuf = buf;
    size_t oldCapacity = capacity;

    if (capacity == 0) {
      capacity = kMinCapacity;
    } else {
      capacity = capacity * kSpaceExpandRate;
    }
    guardSize = static_cast<size_t>(capacity * kGuardPercent);

    buf = NewBuf(capacity);
    RestoreBuf(oldBuf, oldCapacity);
    DeleteBuf(oldBuf);
  }

  T *NewBuf(size_t newSize) {
    T *newBuf = new T[newSize];
    for (size_t i = 0; i < newSize; i++) {
      fn.Clear(newBuf[i]);
    }
    return newBuf;
  }

  void DeleteBuf(T *data) {
    if (UNLIKELY(data == nullptr)) {
      return;
    }
    delete []data;
  }

  void RestoreBuf(T *oldBuf, size_t oldCapacity) {
    for (size_t i = 0; i < oldCapacity; i++) {
      T &data = oldBuf[i];
      if (fn.IsEmpty(data)) {
        continue;
      }
      size_t index = FindEmptyIndex(data);
      buf[index] = std::move(data);
    }
  }

  size_t FindEmptyIndex(const T data) {
    size_t hash = fn(data);
    const size_t index = (capacity != 0) ? (hash % capacity) : 0;
    size_t nextIndex = index;
    bool isFind = false;
    do {
      T &tmp = buf[nextIndex];
      if (fn.IsEmpty(tmp)) {
        isFind = true;
        break;
      }
      nextIndex = GetNextIndex(nextIndex);
    } while (nextIndex != index);
    CHECK(isFind) << "LowMemSet can not find empty index!";
    return nextIndex;
  }

  inline size_t GetNextIndex(size_t index) {
    return (++index == capacity) ? 0 : index;
  }

  size_t size;
  size_t capacity;
  size_t guardSize;
  T *buf;
  HashEqualFn fn;
};
} // namespace maple

#endif // LOW_MEM_SET_H