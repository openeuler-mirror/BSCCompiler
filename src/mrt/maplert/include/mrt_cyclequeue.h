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
#ifndef MRT_CYCLEQUEUE_H_
#define MRT_CYCLEQUEUE_H_
#include <queue>

namespace maplert {
template<typename T>
class CycleQueue {
 public:
  CycleQueue() : frontIndex(0), rearIndex(0) {}

  ~CycleQueue() {
    if (UNLIKELY(queueBuffer != nullptr)) {
      delete queueBuffer;
    }
  }

  inline bool Empty() const {
    return (queueBuffer == nullptr) ? EmptyArray() : EmptyQueue();
  }

  inline void Push(T ele) {
    if (LIKELY(queueBuffer == nullptr)) {
      if (PushArray(ele)) {
        return;
      } else {
        // cycle array is full, move to std::queue
        CopyElementToQueue();
      }
    }
    PushQueue(ele);
  }

  inline void Pop() {
    if (LIKELY(queueBuffer == nullptr)) {
      PopArray();
    } else {
      PopQueue();
    }
  }

  inline T Front() {
    return (queueBuffer == nullptr) ? FrontArray() : FrontQueue();
  }

  static constexpr uint16_t kMaxSize = 30;
  uint16_t frontIndex;
  uint16_t rearIndex;
  T arrayBuffer[kMaxSize];
  std::queue<T> *queueBuffer = nullptr;

 private:
  inline bool EmptyArray() const {
    return frontIndex == rearIndex;
  }

  inline bool PushArray(T ele) {
    uint16_t tmpRear = (rearIndex + 1) % kMaxSize;
    if (UNLIKELY(frontIndex == tmpRear)) {
      return false;
    }
    arrayBuffer[rearIndex] = ele;
    rearIndex = tmpRear;
    return true;
  }

  // just adjust index, no pop value
  inline void PopArray() {
    frontIndex = (frontIndex + 1) % kMaxSize;
  }

  inline T FrontArray() {
    return arrayBuffer[frontIndex];
  }

  inline bool EmptyQueue() const {
    return queueBuffer->empty();
  }

  inline void PushQueue(T ele) {
    queueBuffer->push(ele);
  }

  inline void PopQueue() {
    queueBuffer->pop();
  }

  inline T FrontQueue() {
    return queueBuffer->front();
  }

  inline void CopyElementToQueue() {
    queueBuffer = new (std::nothrow) std::queue<T>;
    CHECK(queueBuffer != nullptr) << "queueBuffer is nullptr" << maple::endl;
    while (!EmptyArray()) {
      PushQueue(FrontArray());
      PopArray();
    }
  }
};
}

#endif // MRT_CYCLEQUEUE_H_
