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
#include "allocator.h"

#include <cstdint>
#include <cinttypes>
#include <dlfcn.h>

#include "mm_config.h"
#include "address.h"
#include "sizes.h"
#include "imported.h"
#include "lock_word.h"
#include "deps.h"
#include "object_base.h"
#include "allocator/page_allocator.h"

namespace maplert {
using namespace std;

void Allocator::ReleaseResource(address_t obj) {
  if (maple::ObjectBase::ReleaseResource(obj)) {
    return;
  }
  MClass *classInfo = MObject::Cast<MObject>(obj)->GetClass();
  // failed to release resource means fatal error.
  if (classInfo == nullptr) {
    LOG(FATAL) << "classInfo is nullptr in ReleaseResource, obj = " << obj << maple::endl;
  }
  LOG(FATAL) << "obj = " << obj << ", class name = " << classInfo->GetName() << maple::endl;
}

Allocator::Allocator()
#if ALLOC_ENABLE_LOCK_CONTENTION_STATS
    : oome(nullptr), oomeCreated(false), globalLock("allocator lock", maple::kAllocatorLock) {}
#else
    : oome(nullptr), oomeCreated(false) {}
#endif

void Allocator::NewOOMException() {
  bool oldOOMECreated = false;
  if (oome == nullptr && oomeCreated.compare_exchange_strong(oldOOMECreated, true)) {
    ScopedObjectAccess soa;
    MClass *exClass = MClass::JniCast(MRT_ReflectClassForCharName("java/lang/OutOfMemoryError", true, nullptr));
    if (exClass == nullptr) {
      LOG(FATAL) << "New OOM exception object failed, because exClass is nullptr" << maple::endl;
      return;
    }
    size_t size = exClass->GetObjectSize();
    // leave cause and stack null is fine, it will get default empty stack and null cause
    oome = MObject::Cast<MObject>(NewObj(size));
    if (oome == nullptr) {
      LOG(FATAL) << "New OOM exception object failed, because oome is nullptr" << maple::endl;
      return;
    }
    MRT_SetJavaClass(oome->AsUintptr(), exClass->AsUintptr());
  }
}

// PageAlllocator
// the input parameter cat should be guaranteed in the range of value of enum type AllocationTag by
// external invoker, in order to avoid exceed the border of matrix
AggregateAllocator &AggregateAllocator::Instance(AllocationTag tag) {
  static ImmortalWrapper<AggregateAllocator> instance[kMaxAllocationTag];
  return *(instance[tag]);
}

// PagePool
PagePool &PagePool::Instance() {
  static ImmortalWrapper<PagePool> instance;
  return *instance;
}
}
