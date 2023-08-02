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
#include "fast_alloc_inline.h"
#include "sizes.h"
namespace maplert {
MObject *MObject::SetClass(const MClass &mClass) {
  if (mClass.HasFinalizer()) {
    ScopedHandles sHandles;
    ObjHandle<MObject> newObj(this);
    MRT_SetJavaClass(newObj.AsRaw(), mClass.AsUintptr());
    return newObj.ReturnObj();
  } else {
    MRT_SetJavaClass(AsUintptr(), mClass.AsUintptr());
    return this;
  }
}

inline MObject *MObject::NewObjectInternal(const MClass &klass, size_t objectSize, bool isJNI) {
  address_t addr = (*theAllocator).NewObj(objectSize);
  if (UNLIKELY(addr == 0)) {
    (*theAllocator).OutOfMemory(isJNI);
  }
  MObject *newObj = MObject::Cast<MObject>(addr);
  if (UNLIKELY(newObj == nullptr)) {
    CHECK(MRT_HasPendingException()) << "has no OOM exception when new obj is null" << maple::endl;
    return nullptr;
  }
  newObj = newObj->SetClass(klass);
  return newObj;
}

MObject *MObject::NewObject(const MClass &klass, size_t objectSize, bool isJNI) {
  MObject *objAddr = MObject::Cast<MObject>(MRT_TryNewObject(klass.AsUintptr(), objectSize));
  if (LIKELY(objAddr != nullptr)) {
    return objAddr;
  }
  return NewObjectInternal(klass, objectSize, isJNI);
}

MObject *MObject::NewObject(const MClass &klass, bool isJNI) {
  MObject *objAddr = MObject::Cast<MObject>(MRT_TryNewObject(klass.AsUintptr()));
  if (LIKELY(objAddr != nullptr)) {
    return objAddr;
  }
  size_t objectSize = klass.GetObjectSize();
  CHECK(klass.IsArrayClass() == false) << "must not Array class." << maple::endl;
  return NewObjectInternal(klass, objectSize, isJNI);
}

MObject *MObject::NewObject(const MClass &klass, const MethodMeta *constructor, ...) {
  va_list args;
  va_start(args, constructor);
  if (UNLIKELY(!klass.InitClassIfNeeded())) {
    va_end(args);
    return nullptr;
  }
  ScopedHandles sHandles;
  ObjHandle<MObject> obj(NewObject(klass));
  DCHECK(obj.AsJObj() != nullptr) << "new object fail." << maple::endl;
  DCHECK(constructor != nullptr) << "MObject::NewObject::constructor is nullptr" << maple::endl;
  (void)constructor->Invoke<int, calljavastubconst::kVaArg>(obj.AsObject(), &args);
  va_end(args);
  if (UNLIKELY(MRT_HasPendingException())) {
    return nullptr;
  }
  return obj.ReturnObj();
}

MObject *MObject::NewObject(const MClass &klass, const MethodMeta &constructor, const jvalue &args, bool isJNI) {
  ScopedHandles sHandles;
  ObjHandle<MObject> obj(NewObject(klass, isJNI));
  if (UNLIKELY(obj() == 0)) {
    CHECK(MRT_HasPendingException()) << "has no OOM exception when new obj is null" << maple::endl;
    return nullptr;
  }
  (void)constructor.Invoke<int, calljavastubconst::kJvalue>(obj.AsObject(), &args);
  if (UNLIKELY(MRT_HasPendingException())) {
    return nullptr;
  }
  return obj.ReturnObj();
}
} // namespace maplert
