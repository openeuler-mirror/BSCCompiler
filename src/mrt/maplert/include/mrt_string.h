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
#ifndef JAVA_MRT_STRING_H_
#define JAVA_MRT_STRING_H_

#include <cstdio>
#include <iostream>
#include <cstring>
#include <functional>
#include <jni.h>
#include <limits.h>
#include "cinterface.h"
#include "address.h"
#include "mrt_string_api.h"
#include "collector/mpl_thread_pool.h"
#include "mstring.h"
#include "gc_callback.h"
#include "panic.h"

namespace maplert {
// for GC root scanning.  This is a C++ function, not extern "C"
// roots in the constant-string pool.
void VisitStringPool(const RefVisitor &visitor);
void CreateAppStringPool();

// Return the size (in bytes) of a given String object.
// It is a specialized function for GetObjectSize.
size_t ConstStringPoolSize(bool literal);
size_t ConstStringPoolNum(bool literal);
size_t ConstStringAppPoolNum(bool literal);
void DumpConstStringPool(std::ostream &os, bool literal);

MString *NewStringUtfFromPoolForClassName(const MClass &classObj);
MArray *MStringToMutf8(const MString &stringObj, int32_t offset, int32_t length, int compress);
MArray *MStringToBytes(const MString &stringObj, int32_t offset, int32_t length, uint16_t maxValidChar, int compress);

MString *StringNewStringFromString(const MString &stringObj);
MString *StringNewStringFromCharArray(int32_t offset, uint32_t charCount, const MArray &arrayObj);
MString *StringNewStringFromByteArray(const MArray &arrayObj, int32_t highByteT, int32_t offset, uint32_t byteLength);
MString *StringNewStringFromSubstring(const MString &stringObj, int32_t start, uint32_t length);
MString *JStringDoReplace(const MString &stringObj, uint16_t oldChar, uint16_t newChar);
MString *StringConcat(MString &stringObj1, MString &stringObj2);
int32_t StringFastIndexOf(const MString &stringObj, int32_t ch, int32_t start);
MString *NewStringFromUTF16(const std::string &str);

#ifdef __OPENJDK__
// new native functions for openjdk
int32_t StringNativeIndexOfP3(MString &subStrObj, MString &srcStrObj, int32_t fromIndex);
int32_t StringNativeIndexOfP5(MString &subStrObj, MArray &srcArray,
                              int32_t srcOffset, int32_t srcCount, int32_t fromIndex);
int32_t StringNativeLastIndexOfP3(MString &subStrObj, MString &srcStrObj, int32_t fromIndex);
int32_t StringNativeLastIndexOfP5(MString &subStrObj, MArray &srcArray,
                                  int32_t srcOffset, int32_t srcCount, int32_t fromIndex);
MString *StringNewStringFromCodePoints(MArray &mArray, int32_t offset, int32_t count);
int32_t StringNativeCodePointAt(MString &strObj, int32_t index);
int32_t StringNativeCodePointBefore(MString &strObj, int32_t index);
int32_t StringNativeCodePointCount(MString &strObj, int32_t beginIndex, int32_t endIndex);
int32_t StringNativeOffsetByCodePoint(MString &strObj, int32_t index, int32_t codePointOffset);
#endif // __OPENJDK__

// if c-string is in 16bit coding, then use CStrToJStr, otherwise use NewStringUTF
MString *NewStringUTF(const char *kCStr, size_t cStrLen);

// for normal MString internization, use GetOrInsertStringPool
MString *GetOrInsertStringPool(MString &stringObj);
// get/insert a MFile literal into const pool.
// used by mpl-linker to insert literal (also stored in static-fields)
MString *GetOrInsertLiteral(MString &literalObj);

// only for RC
void RemoveStringFromPool(MString &stringObj);
size_t RemoveDeadStringFromPool();

// for gc
void StringPrepareConcurrentSweeping();
size_t ConcurrentSweepDeadStrings(MplThreadPool *threadPool);

#ifdef __cplusplus
extern "C" {
#endif

class MStringRef {
 public:
#ifdef USE_32BIT_REF
  using RawType = uint32_t;
#else
  using RawType = address_t;
#endif

  MStringRef() : ref(0) {} // default initialized to 0 as empty
  RawType GetRef() const {
    return ref;
  }
  RawType &GetRawRef() {
    return ref;
  }
  void SetRef(const address_t addr) {
    __MRT_ASSERT(addr <= std::numeric_limits<RawType>::max(), "String address overflow for MStringRef!");
    ref = static_cast<RawType>(addr);
  }
  void SetRef(const MString *str) {
    SetRef(reinterpret_cast<address_t>(str));
  }
  void Clear() {
    ref = 0;
  }
  bool IsEmpty() const {
    return ref == 0;
  }
 private:
  RawType ref;
};

jstring CStrToJStr(const char *ca, jint len);
// empty, hash and equal
struct MStringHashEqual {
  // compare mstring with MFile literal
  size_t operator()(const MString *a) const {
    return a->GetHash();
  }

  size_t operator()(const MStringRef a) const {
    return reinterpret_cast<MString*>(a.GetRef())->GetHash();
  }

  bool operator()(const MString *a, const MStringRef b) const {
    return a->Equals(*(reinterpret_cast<MString*>(b.GetRef())));
  }

  bool operator()(const MStringRef a, const MStringRef b) const {
    return reinterpret_cast<MString*>(a.GetRef())->Equals(*(reinterpret_cast<MString*>(b.GetRef())));
  }

  void Clear(MStringRef &ref) const {
    ref.Clear();
  }

  bool IsEmpty(const MStringRef ref) const {
    return ref.IsEmpty();
  }
};

struct MStringMapNotEqual {
  // compare mstring with MFile literal
  size_t operator()(const MString *a) const {
    return a->GetHash();
  }

  bool operator()(const MString *a, const MString *b) const {
    return !a->Equals(*b);
  }
};

#ifdef __cplusplus
}
#endif // __cplusplus
} // namespace maplert
#endif // JAVA_MRT_STRING_H_
