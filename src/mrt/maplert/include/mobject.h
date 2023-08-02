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
#ifndef MRT_MAPLERT_INCLUDE_MOBJECT_H_
#define MRT_MAPLERT_INCLUDE_MOBJECT_H_
#include <vector>
#include <string>
#include <mutex>
#include <atomic>

#include "jni.h"
#include "metadata_layout.h"
#include "base/logging.h"
namespace maplert {
class MClass;
class MArray;
class MString;
class MMethod;
class MField;
class MethodMeta;
#define PACKED(n) __attribute__ ((__aligned__(n), __packed__))

class PACKED(4) MObject {
 public:
  inline MClass *GetClass() const;
  inline uint32_t GetSize() const;
  inline bool IsInstanceOf(const MClass &mClass) const;
  inline bool IsArray() const;
  inline bool IsString() const;
  inline bool IsClass() const;
  inline bool IsObjectArray() const;
  inline bool IsPrimitiveArray() const;
  inline bool IsOffHeap() const;
  inline void ResetMonitor();
  template<typename T>
  inline T Load(size_t offset, bool isVolatile = false) const;
  template<typename T>
  inline void Store(size_t offset, T value, bool isVolatile = false);
  inline MObject *LoadObject(size_t offset, bool isVolatile = false) const;
  inline void StoreObject(size_t offset, const MObject *value, bool isVolatile = false) const;
  inline MObject *LoadObjectNoRc(size_t offset) const;
  inline void StoreObjectNoRc(size_t offset, const MObject *value) const;
  inline MObject *LoadObjectOffHeap(size_t offset) const;
  inline void StoreObjectOffHeap(size_t offset, const MObject *value) const;
  static MObject *NewObject(const MClass &klass, size_t objectSize, bool isJNI = false);
  static MObject *NewObject(const MClass &klass, bool isJNI = false);
  static MObject *NewObject(const MClass &klass, const MethodMeta *constructor, ...);
  static MObject *NewObject(const MClass &klass, const MethodMeta &constructor, const jvalue &args, bool isJNI = false);
  static uint32_t GetReffieldSize();
  operator jobject () {
    return reinterpret_cast<jobject>(this);
  }

  operator jobject () const {
    return reinterpret_cast<jobject>(const_cast<MObject*>(this));
  }

  inline uintptr_t AsUintptr() const;
  static inline MObject *JniCast(jobject o);
  static inline MObject *JniCastNonNull(jobject o);
  template<typename T0, typename T1>
  static inline T0 *Cast(T1 o);
  template<typename T0, typename T1>
  static inline T0 *CastNonNull(T1 o);
  inline MClass *AsMClass() const;
  inline MArray *AsMArray() const;
  inline MString *AsMString() const;
  inline MMethod *AsMMethod() const;
  inline MField *AsMField() const;
  inline jobject AsJobject() const;

 protected:
  MetaRef shadow;
  int32_t monitor;
  MObject() = delete;
  ~MObject() = delete;
  MObject(MObject&&) = delete;
  MObject &operator=(MObject&&) = delete;
  MObject *SetClass(const MClass &mClass);

  static inline MObject *NewObjectInternal(const MClass &klass, size_t objectSize, bool isJNI);
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MOBJECT_H_
