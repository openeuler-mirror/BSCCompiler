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
#ifndef MAPLE_RUNTIME_HANDLECOMMON
#define MAPLE_RUNTIME_HANDLECOMMON

#include <jni.h>
#include <iostream>
#include <algorithm>
#include <mutex>
#include "cpphelper.h"
#include "chelper.h"
#include "mrt_libs_api.h"
#include "exception/mrt_exception.h"
#include "mrt_well_known.h"
#include "mrt_primitive_util.h"
namespace maplert{
MClass *GetDcClasingFromFrame();
void ParseSignatrueType(char *descriptor, const char *&methodSig);

string GeneIllegalArgumentExceptionString(const MClass *from, const MClass *to);

string GeneClassCastExceptionString(const MClass *from, const MClass *to) ;

bool GetPrimShortType(const MClass *klass, char &type);

bool CheckPrimitiveCanBoxed(char shortType);

bool GetPrimShortTypeAndValue(const MObject *o, char &type, jvalue &value, const MClass *fromType);

using DoubleLongConvert = union {
  jlong j;
  jdouble d;
};
using FloatIntConvert = union {
  jint i;
  jfloat f;
};
using FloatDoubleConvert = union {
  jdouble d;
  jfloat f;
};


class Arg {
 public:
  virtual jint GetJint() = 0;
  virtual jfloat GetJfloat() = 0;
  virtual jdouble GetJdouble() = 0;
  virtual jlong GetJlong() = 0;
  virtual MObject *GetObject() = 0;
  virtual ~Arg() = default;
};

class VArg : public Arg {
 public:
  VArg(va_list ag) : args(ag) {}
  jint GetJint() noexcept {
    return va_arg(args, int);
  }
  jfloat GetJfloat() noexcept {
    FloatDoubleConvert u;
    u.d = va_arg(args, double);
    return u.f;
  }
  jdouble GetJdouble() noexcept {
    return va_arg(args, double);
  }
  jlong GetJlong() noexcept {
    return va_arg(args, jlong);
  }
  MObject *GetObject() noexcept {
    return reinterpret_cast<MObject*>(va_arg(args, MObject*));
  }
  virtual ~VArg() = default;
 private:
  va_list args;
};

class JValueArg : public Arg {
 public:
  JValueArg(jvalue *ag) : args(ag) {}
  jint GetJint() noexcept {
    return args[idx++].i;
  }
  jfloat GetJfloat() noexcept {
    return args[idx++].f;
  }
  jdouble GetJdouble() noexcept {
    return args[idx++].d;
  }
  jlong GetJlong() noexcept {
    return args[idx++].j;
  }
  MObject *GetObject() noexcept {
    return reinterpret_cast<MObject*>(args[idx++].l);
  }
  virtual ~JValueArg() {
    args = nullptr;
  }
 private:
  uint32_t idx = 0;
  jvalue *args;
};

class Arm32Arg : public Arg {
 public:
  Arm32Arg(int32_t *params) : args(params) {}
  jint GetJint() noexcept {
    if (gRegIdx <= gRegMaxIdx) {
      return *static_cast<jint*>(args + gRegIdx++);
    }
    return *static_cast<jint*>(args + stackIdx++);
  }
  jfloat GetJfloat() noexcept {
    if (dRegIdx <= dRegMaxIdx) {
      return *reinterpret_cast<jfloat*>(args + dRegIdx++);
    }
    return *reinterpret_cast<jfloat*>(args + stackIdx++);
  }
  jdouble GetJdouble() noexcept {
    if ((dRegIdx <= dRegMaxIdx) && (dRegIdx & 1)) {
      ++dRegIdx;
    }
    if (dRegIdx <= dRegMaxIdx) {
      jdouble val = *reinterpret_cast<jdouble*>(args + dRegIdx);
      dRegIdx = dRegIdx + kEightBytes;
      return val;
    }

    if (stackIdx & 1) {
      ++stackIdx;
    }
    jdouble val = *reinterpret_cast<jdouble*>(args + stackIdx);
    stackIdx += kEightBytes;
    return val;
  }

  jlong GetJlong() noexcept {
    if (stackIdx & 1) {
      ++stackIdx;
    }
    jlong val = *reinterpret_cast<jlong*>(args + stackIdx);
    stackIdx += kEightBytes;
    return val;
  }

  MObject *GetObject() noexcept {
    return reinterpret_cast<MObject*>(GetJint());
  }

  virtual ~Arm32Arg() {
    args = nullptr;
  }

 private:
  constexpr static uint8_t kEightBytes = 2;
  uint16_t gRegIdx = 0;
  constexpr static uint8_t gRegMaxIdx = 3;
  constexpr static uint8_t dRegIdxStart = 4;
  constexpr static uint8_t dRegMaxIdx = 18;
  uint16_t dRegIdx = dRegIdxStart;
  constexpr static uint8_t stackStartIdx = 20;
  uint16_t stackIdx = stackStartIdx;
  int32_t *args;
};

class Arm32SoftFPArg : public Arg {
 public:
  Arm32SoftFPArg(int32_t *params) : args(params) {}
  jint GetJint() noexcept {
    if (gRegIdx <= gRegMaxIdx) {
      return *static_cast<jint*>(args + gRegIdx++);
    }
    return *static_cast<jint*>(args + stackIdx++);
  }
  jfloat GetJfloat() noexcept {
    if (gRegIdx <= gRegMaxIdx) {
      return *reinterpret_cast<jfloat*>(args + gRegIdx++);
    }
    return *reinterpret_cast<jfloat*>(args + stackIdx++);
  }
  jdouble GetJdouble() noexcept {
    if ((gRegIdx <= gRegMaxIdx) && (gRegIdx & 1)) {
      ++gRegIdx;
    }
    if (gRegIdx == gRegMaxIdx) {
      ++gRegIdx;
    }
    if (gRegIdx <= gRegMaxIdx) {
      jdouble val = *reinterpret_cast<jdouble*>(args + gRegIdx);
      gRegIdx = gRegIdx + kEightBytes;
      return val;
    }

    if (stackIdx & 1) {
      ++stackIdx;
    }
    jdouble val = *reinterpret_cast<jdouble*>(args + stackIdx);
    stackIdx += kEightBytes;
    return val;
  }

  jlong GetJlong() noexcept {
    if (stackIdx & 1) {
      ++stackIdx;
    }
    jlong val = *reinterpret_cast<jlong*>(args + stackIdx);
    stackIdx += kEightBytes;
    return val;
  }

  MObject *GetObject() noexcept {
    return reinterpret_cast<MObject*>(GetJint());
  }

  virtual ~Arm32SoftFPArg() {
    args = nullptr;
  }

 private:
  constexpr static uint8_t kEightBytes = 2;
  uint16_t gRegIdx = 0;
  constexpr static uint8_t gRegMaxIdx = 3;
  constexpr static uint8_t stackStartIdx = 4;
  uint16_t stackIdx = stackStartIdx;
  int32_t *args;
};

class ArgsWrapper {
 public:
  ArgsWrapper(Arg &ag) : arg(ag) {}
  ~ArgsWrapper() = default;
  jint GetJint() const noexcept {
    return arg.GetJint();
  }
  jfloat GetJfloat() const noexcept {
    return arg.GetJfloat();
  }
  jdouble GetJdouble() const noexcept {
    return arg.GetJdouble();
  }
  jlong GetJlong() const noexcept {
    return arg.GetJlong();
  }
  MObject *GetObject() const noexcept {
    return arg.GetObject();
  }
 private:
  Arg &arg;
};

class MethodType {
 public:
  MethodType(const MObject *methodTypeObj) {
    MArray *ptypesVal =
        reinterpret_cast<MArray*>(methodTypeObj->LoadObject(WellKnown::GetMFieldMethodHandlePTypesOffset()));
    returnType =
        reinterpret_cast<MClass*>(methodTypeObj->LoadObjectNoRc(WellKnown::GetMFieldMethodHandlepRTypeOffset()));
    uint32_t num = ptypesVal->GetLength();
    for (uint32_t i = 0; i < num; ++i) {
      paramsType.push_back(static_cast<MClass*>(ptypesVal->GetObjectElementNoRc(i)));
    }
    RC_LOCAL_DEC_REF(ptypesVal);
    typesArrSize = static_cast<uint32_t>(paramsType.size());
    typesArr = nullptr;
  }
  MethodType(const MethodType&) = default;
  MethodType& operator=(const MethodType&) = default;
  std::string ToString() const noexcept;
  ALWAYS_INLINE const MClass *GetReTType() const {
    return returnType;
  }
  ALWAYS_INLINE const vector<MClass*> &GetParamsType() const noexcept {
    return paramsType;
  }
  ALWAYS_INLINE uint32_t GetTypesArraySize() const noexcept {
    return typesArrSize;
  }
  ALWAYS_INLINE MClass **GetTypesArray() {
    if (typesArr == nullptr) {
      typesArr = static_cast<MClass**>(malloc((paramsType.size() + 1) * sizeof(MClass*)));
      if (typesArr == nullptr) {
        LOG(ERROR) << "GetTypesArray malloc error" << maple::endl;
      }
    }
    int idx = 0;
    for (auto it = paramsType.begin(); it != paramsType.end(); ++it) {
      typesArr[idx++] = *it;
    }
    typesArr[idx] = returnType;
    return typesArr;
  }
  ~MethodType() {
    if (typesArr != nullptr) {
      free(typesArr);
    }
    returnType = nullptr;
    typesArr = nullptr;
  }

 private:
  MClass *returnType;
  vector<MClass*> paramsType;
  MClass **typesArr;
  uint32_t typesArrSize;
};

void FillArgsInfoNoCheck(const ArgsWrapper&, const char *typesMark, uint32_t arrSize,
                         BaseArgValue &paramArray, uint32_t begin = 0);
void DoInvoke(const MethodMeta &method, jvalue &result, ArgValue &paramArray);
}
#endif
