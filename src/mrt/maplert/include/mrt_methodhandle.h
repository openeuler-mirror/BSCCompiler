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
#ifndef MAPLE_RUNTIME_METHODHANDLE
#define MAPLE_RUNTIME_METHODHANDLE
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
#include "mrt_handlecommon.h"
#include "mstring_inline.h"
namespace maplert {
enum Kind : uint32_t {
  kVirtualCall = 0,
  kSuperCall,
  kDirectCall,
  kStaticCall,
  kInterfaceCall,
  kTransformCall,
  kCallSiteTransformCall,
  kInstanceGet = 9,
  kInstancePut,
  kStaticGet,
  kStaticPut,
};
struct SizeInfo {
  SizeInfo(uint32_t paramNum) : refNum(0), byteArrSize(0) {
    arraySize = paramNum + 1;
  }
  uint32_t arraySize;
  uint32_t refNum;
  uint32_t byteArrSize;
};
struct CallParam {
  uint32_t paramNum;
  bool isExactMatched;
  MClass **paramTypes;
  char *typesMark;
  bool isStaticMethod;
  uint8_t beginIndex;
  uint32_t arraySize;
  uint32_t dregSize;
};

class EmStackFrame {
 public:
  EmStackFrame(MObject *emStackFrameObj) : emStackFrameJavaObj(emStackFrameObj) {}
  ~EmStackFrame() {}
  EmStackFrame(const EmStackFrame&) = default;
  ALWAYS_INLINE uint32_t GetStackFrameNativeBytesCount() const noexcept {
    MArray *stackFrameByteArray = reinterpret_cast<MArray*>(LoadRefField(
        reinterpret_cast<address_t>(emStackFrameJavaObj), WellKnown::GetMFieldEmStackFrameStackFrameOffset()));
    return stackFrameByteArray->GetLength();
  }
  ALWAYS_INLINE int8_t *GetStackFrameNativeBytes() const noexcept {
    MArray *stackFrameByteArray = reinterpret_cast<MArray*>(LoadRefField(
        reinterpret_cast<address_t>(emStackFrameJavaObj), WellKnown::GetMFieldEmStackFrameStackFrameOffset()));
    int8_t *cArray = reinterpret_cast<int8_t*>(stackFrameByteArray->ConvertToCArray());
    return cArray;
  }
  ALWAYS_INLINE MObject *GetReferencesReturnObj() const noexcept {
    MArray *refArray = reinterpret_cast<MArray*>(LoadRefField(
        reinterpret_cast<address_t>(emStackFrameJavaObj), WellKnown::GetMFieldEmStackFrameReferencesOffset()));
    uint32_t idx = refArray->GetLength() - 1;
    MObject *elem = refArray->GetObjectElementNoRc(idx);
    return elem;
  }
  ALWAYS_INLINE MObject *GetReferencesObj(uint32_t idx) const noexcept {
    MArray *refArray = reinterpret_cast<MArray*>(LoadRefField(
        reinterpret_cast<address_t>(emStackFrameJavaObj), WellKnown::GetMFieldEmStackFrameReferencesOffset()));
    if (idx >= refArray->GetLength()) {
      return nullptr;
    }
    MObject *elem = refArray->GetObjectElementNoRc(idx);
    return elem;
  }
  ALWAYS_INLINE void PutReferencesObj(const MObject *obj, uint32_t idx) const noexcept {
    MArray *refArray = reinterpret_cast<MArray*>(LoadRefField(
        reinterpret_cast<address_t>(emStackFrameJavaObj), WellKnown::GetMFieldEmStackFrameReferencesOffset()));
    refArray->SetObjectElement(idx, obj);
  }

 private:
  MObject *emStackFrameJavaObj;
};

class MethodHandle {
 public:
  Arg &args;
  MethodHandle(MObject *methodHandleObj, Arg &argsP) : args(argsP), methodHandleJavaObj(methodHandleObj) {
    validMethodTypeMplObjCache = nullptr;
    handleKind = static_cast<Kind>(MRT_LOAD_JINT(methodHandleObj, WellKnown::GetMFieldMethodHandleHandleKindOffset()));
    MObject *methodTypeObj = GetMethodTypeJavaObj();
    mplFieldOrMethod = reinterpret_cast<MObject*>(
        MRT_LOAD_JLONG(methodHandleObj, WellKnown::GetMFieldMethodHandleArtFieldOrMethodOffset()));
    MObject *nominalType = GetNominalTypeJavaObj();
    if (nominalType != nullptr) {
      validMethodTypeMplObjCache = new MethodType(nominalType);
      methodTypeMplObjCache = new MethodType(methodTypeObj);
    } else {
      validMethodTypeMplObjCache = new MethodType(methodTypeObj);
      methodTypeMplObjCache = validMethodTypeMplObjCache;
    }
    handleClassType = methodHandleObj->GetClass();
  }
  ~MethodHandle() {
    if (GetNominalTypeJavaObj() != nullptr) {
      delete validMethodTypeMplObjCache;
      delete methodTypeMplObjCache;
    } else {
      delete validMethodTypeMplObjCache;
    }
  }
  MethodHandle(const MethodHandle&) = default;
  bool FieldSGet(MClass **parameterTypes, bool doConvert, uint32_t arraySize, jvalue &result);
  bool FieldAccess(MClass **parameterTypes, bool doConvert, uint32_t arraySize, jvalue &result);
  void DirectCall(CallParam paramStruct, jvalue &result);
  bool FillInvokeArgs(
      const ArgsWrapper &argsWrapper, CallParam &paramStruct, BaseArgValue &paramArray, ScopedHandles&) const;
  bool NoParamFastCall(CallParam &paramStruct, jvalue &result);
  bool InvokeWithEmStackFrame(MObject *emStFrameObj, jvalue &result);
  jvalue InvokeMethodNoParameter(MObject *obj, MethodMeta &mthd);
  ALWAYS_INLINE MObject *GetMethodTypeJavaObj() const noexcept {
    return reinterpret_cast<MObject*>(
        LoadRefField(reinterpret_cast<address_t>(methodHandleJavaObj), WellKnown::GetMFieldMethodHandleTypeOffset()));
  }
  ALWAYS_INLINE const MethodType *GetMethodTypeMplObj() const noexcept {
    return methodTypeMplObjCache;
  }
  ALWAYS_INLINE const MethodType *GetValidMethodTypeMplObj() const noexcept {
    return validMethodTypeMplObjCache;
  }
  ALWAYS_INLINE Kind GetHandleKind() const noexcept {
    return handleKind;
  }
  ALWAYS_INLINE MethodMeta *GetMethodMeta() const noexcept {
    return reinterpret_cast<MethodMeta*>(mplFieldOrMethod);
  }
  ALWAYS_INLINE FieldMeta *GetFieldMeta() const noexcept {
    return reinterpret_cast<FieldMeta*>(mplFieldOrMethod);
  }
  ALWAYS_INLINE MObject *GetNominalTypeJavaObj() const noexcept {
    return reinterpret_cast<MObject*>(LoadRefField(
        reinterpret_cast<address_t>(methodHandleJavaObj), WellKnown::GetMFieldMethodHandleNominalTypeOffset()));
  }
  ALWAYS_INLINE const MClass *GetHandleJClassType() const noexcept {
    return handleClassType;
  }
  ALWAYS_INLINE const MObject *GetHandleJavaObj() const noexcept {
    return methodHandleJavaObj;
  }
  ALWAYS_INLINE MethodMeta *GetRealMethod(MObject *obj) const noexcept;
  ALWAYS_INLINE bool ExactInvokeCheck(const MString *calleeName, MClass **parameterTypes, uint32_t) const;
  ALWAYS_INLINE bool InvokeTransform(const SizeInfo&, const char *mark, MClass **paramTypes, jvalue &retVal);

  string ValidTypeToString() const noexcept;
  static MObject *GetMemberInternal(const MObject *methodHandle);
  ALWAYS_INLINE bool ParamsConvert(const MClass *from, const MClass *to, MClass**,
                                   uint32_t arraySize, jvalue &internVal) const;

 private:
  bool FieldGet(MClass **parameterTypes, bool doConvert, uint32_t arraySize, jvalue &result);
  bool FieldPut(MClass **parameterTypes, bool doConvert, uint32_t arraySize);
  bool FieldSPut(MClass **parameterTypes, bool doConvert, uint32_t arraySize);
  ALWAYS_INLINE void GetValFromVargs(jvalue &value, char shortFieldTypee);
  ALWAYS_INLINE char GetReturnTypeMark(char markInProto) const noexcept;
  ALWAYS_INLINE bool IsExactMatch() const noexcept;
  ALWAYS_INLINE bool IsExactMatch(MClass **types, uint32_t arraySize, bool isNominal = false) const noexcept;
  void InvokeTransformVirtualMethod(MObject *emStFrameObj, jvalue &retVal) const;
  MObject *CreateEmStackFrame(const MArray *bArray, const MObject *refArray, MClass **types, uint32_t arraySize) const;
  bool FillEmStackFrameArray(const char *mark, MClass **paramTypes, int8_t *cArray, const MArray*);

  static constexpr char kInvokeMtd[] =
      "Ljava_2Flang_2Finvoke_2FMethodHandle_3B_7Cinvoke_7C_28ALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B";
  static constexpr char kInvokeExactMtd[] =
      "Ljava_2Flang_2Finvoke_2FMethodHandle_3B_7CinvokeExact_7C_28ALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B";

  MObject *methodHandleJavaObj;
  Kind handleKind;
  MObject *mplFieldOrMethod;
  MethodType *validMethodTypeMplObjCache;  // return nominal if nominal exist, or methodType
  MethodType *methodTypeMplObjCache;
  MClass *handleClassType;
  uint32_t emByteIdx = 0;
  uint32_t realParamNum = 0;
};

class EmStackFrameInvoker {
 public:
  EmStackFrameInvoker(MObject *frameObj, MethodHandle &handle) : emStFrameObj(frameObj), methodHandleMplObj(handle) {
    emStackFrameMplObj = new EmStackFrame(emStFrameObj);
    MObject *calleeMethodType = reinterpret_cast<MObject*>(
        emStFrameObj->LoadObjectNoRc(WellKnown::GetMFieldEmStackFrameCallsiteOffset()));
    calleeMethodTypeMplObj = new MethodType(calleeMethodType);
    typesArr = calleeMethodTypeMplObj->GetTypesArray();
    paramLength = calleeMethodTypeMplObj->GetTypesArraySize();

    mplFieldOrMethodMeta = methodHandleMplObj.GetMethodMeta();
    callerType = methodHandleMplObj.GetMethodTypeJavaObj();
    callerTypeMplObj = new MethodType(callerType);
    ptypesOfHandle = &callerTypeMplObj->GetParamsType();
  }
  ~EmStackFrameInvoker() {
    delete emStackFrameMplObj;
    delete calleeMethodTypeMplObj;
    delete callerTypeMplObj;
  }
  bool Invoke(jvalue &result);

 private:
  bool InvokeInterpMethod(jvalue &result, MethodMeta &realMethod);
  bool InvokeStaticCmpileMethod(jvalue &result, const MethodMeta &realMethod);
  ALWAYS_INLINE bool FillParamsForEmStackFrame(BaseArgValue &paramArray);

  MObject *emStFrameObj;
  MethodHandle &methodHandleMplObj;
  EmStackFrame *emStackFrameMplObj;

  // typesinfo from callee
  MethodType *calleeMethodTypeMplObj;
  MClass **typesArr;
  uint32_t paramLength;

  // info of mpl field or method
  const MethodMeta *mplFieldOrMethodMeta;

  // info of caller
  MObject *callerType;
  MethodType *callerTypeMplObj;
  const vector<MClass*> *ptypesOfHandle;

  uint32_t byteIdx = 0;
  uint32_t refIdx = 0;
};

static std::unordered_map<string, MethodMeta*> StringToStringFactoryMap = {};
static std::set<string> stringInitMethodSet = { "newEmptyString",
                                                "newStringFromBytes",
                                                "newStringFromChars",
                                                "newStringFromString",
                                                "newStringFromStringBuffer",
                                                "newStringFromCodePoints",
                                                "newStringFromStringBuilder" };
static std::mutex mtx;
MethodMeta *RefineTargetMethod(Kind handleKind, MethodMeta *method, const MClass *referClass);

class EMSFWriter {
 public:
  static void WriteByMark(char mark, int8_t *cArray, uint32_t &byteIdx, const jvalue &val, bool isDec = false);
 private:
  static inline void WriteToEmStackFrame(int8_t *array, uint32_t &idx, int32_t value) {
    *reinterpret_cast<int*>(array + idx) = value;
    idx += sizeof(int);
  }
  static inline void WriteToEmStackFrame(int8_t *array, uint32_t &idx, int64_t value) {
    *reinterpret_cast<jlong*>(array + idx) = value;
    idx += sizeof(jlong);
  }
  static inline void WriteToEmStackFrame(int8_t *array, uint32_t &idx, float value) {
    *reinterpret_cast<float*>(array + idx) = value;
    idx += sizeof(float);
  }
  static inline void WriteToEmStackFrame(int8_t *array, uint32_t &idx, double value) {
    *reinterpret_cast<double*>(array + idx) = value;
    idx += sizeof(double);
  }
};

class EMSFReader {
 public:
  static jvalue GetRetValFromEmStackFrame(const MArray *refArray, uint32_t refNum, uint32_t, const int8_t*, char);
  static inline int32_t GetIntFromEmStackFrame(const int8_t *array, uint32_t &idx) {
    const uint32_t val = *reinterpret_cast<const uint32_t*>(array + idx);
    idx += sizeof(int);
    return val;
  }
  static inline int64_t GetLongFromEmStackFrame(const int8_t *array, uint32_t &idx) {
    const int64_t val = *reinterpret_cast<const int64_t*>(array + idx);
    idx += sizeof(jlong);
    return val;
  }
};

static inline bool IsCallerTransformerCStr(uint32_t paramNum, const std::string protoString) {
  if (paramNum == 1 && strstr(protoString.c_str(), "(Ldalvik/system/EmulatedStackFrame;)") != nullptr) {
    return true;
  }
  return false;
}

static inline bool IsCallerTransformerJStr(uint32_t paramNum, const MString *protoString) {
  std::string protoStr(reinterpret_cast<char*>(protoString->GetContentsPtr()));
  return IsCallerTransformerCStr(paramNum, protoStr);
}

inline bool IsFieldAccess(Kind handleKind) {
  return (handleKind >= Kind::kInstanceGet && handleKind <= Kind::kStaticPut);
}

void DoCalculate(char name, uint32_t &byteArrSize, uint32_t &refNum);

static inline void CalcFrameSize(const MClass *rType, vector<MClass*> &ptypes, uint32_t &byteSz, uint32_t &refNum) {
  size_t size = ptypes.size();
  for (size_t i = 0; i < size; ++i) {
    MClass *type = ptypes[i];
    const char *name = type->GetName();
    DoCalculate(*name, byteSz, refNum);
  }
  DoCalculate(*rType->GetName(), byteSz, refNum);
}

#define STATEMENTCHECK(statement) \
  if (statement) {                \
    return false;                 \
  }
}
#endif
