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

#ifndef MAPLE_GC_ROOTS_H_
#define MAPLE_GC_ROOTS_H_
#include <functional>
#include <iostream>
#include <sstream>
#include "jni.h"
#include "base/macros.h"
#include "base/types.h"
#include "gc_callback.h"

namespace maple {

typedef std::function<void(address_t)> rootObjectFunc;
typedef std::function<void(uint32_t, address_t)> irtVisitFunc;
typedef std::function<void(address_t*)> rootObjectAddrFunc;

class GCRootsVisitor {
 public:
  // Acquire the threadListLock.  This must be called before triggering
  // yieldpoint to prevent a race condition.  Alternative solutions exist, which
  // should be considered in a long run.
  static void AcquireThreadListLock();

  static bool TryAcquireThreadListLock();

  // Release the threadListLock. This should be called after
  // GCRootsVisotor::Visit(rootObjectFunc) is called.
  static void ReleaseThreadListLock();

  static bool IsThreadListLockLockBySelf();

  // visit all roots in android mrt, visite its value
  static void Visit(maplert::RefVisitor &f);

  static void VisitLocalRef(maplert::RefVisitor &f);
  static void VisitGlobalRef(maplert::RefVisitor &f);
  static void VisitThreadException(rootObjectFunc &f);

  static void FixLockedObjects(maplert::RefVisitor &f);
  // visit all roots in android mrt, visite its address
  static void Visit(rootObjectAddrFunc &f ATTR_UNUSED) {
  }

  static void ClearWeakGRTReference(uint32_t index, jobject obj);

  // visit weak GRT
  // VisitWeakGRT is used in STW
  // VisitWeakGRTConcurrent is used in none-STW
  static void VisitWeakGRT(irtVisitFunc &f);
  static void VisitWeakGRT(maplert::RefVisitor &f);
  static void VisitWeakGRTConcurrent(irtVisitFunc &f);
};

enum RootType {
  kRootUnknown = 0,
  kRootJNIGlobal,
  kRootJNILocal,
  kRootJavaFrame,
  kRootNativeStack,
  kRootStickyClass,
  kRootThreadBlock,
  kRootMonitorUsed,
  kRootThreadObject,
  kRootInternedString,
  kRootFinalizing,
  kRootDebugger,
  kRootReferenceCleanup,
  kRootVMInternal,
  kRootJNIMonitor,
};
inline std::ostream &operator<<(std::ostream &os, const RootType &root_type) {
  os << (uint32_t)root_type;
  return os;
}
class RootInfo {
 public:
  explicit RootInfo(RootType type, uint32_t thread_id = 0);
  RootInfo(const RootInfo&) = default;
  virtual ~RootInfo();
  RootType GetType() const;
  uint32_t GetThreadId() const;
  virtual void Describe(std::ostream &os) const;
  inline std::string ToString() const;

 private:
  const RootType type_;
  const uint32_t thread_id_;
};

std::ostream &operator<<(std::ostream &os, const RootInfo &root_info);

enum VisitRootFlags : uint8_t {
  kVisitRootFlagAllRoots = 0x1,
  kVisitRootFlagNewRoots = 0x2,
  kVisitRootFlagStartLoggingNewRoots = 0x4,
  kVisitRootFlagStopLoggingNewRoots = 0x8,
  kVisitRootFlagClearRootLog = 0x10,
  kVisitRootFlagClassLoader = 0x20,
  kVisitRootFlagPrecise = 0x80,
};

class RootVisitor {
 public:
  virtual ~RootVisitor();

  virtual ALWAYS_INLINE void VisitRoot(jobject root, const RootInfo &info);

  ALWAYS_INLINE void VisitRootIfNonNull(jobject root, const RootInfo &info);

  virtual void VisitRoots(jobject *roots, size_t count, const RootInfo &info) = 0;
};
class SingleRootVisitor : public RootVisitor {
 private:
  void VisitRoots(jobject *roots, size_t count, const RootInfo &info) override;

  virtual void VisitRoot(jobject root, const RootInfo &info) override = 0;
};
}  // namespace maple
#endif  // MAPLE_GC_ROOTS_H_
