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
#include "mrt_handleutil.h"
namespace maplert {
class Converter{
 public:
  Converter(ConvertJStruct &params, const MethodHandle &mh, const MClass *fromCls, const MClass *toCls, bool decFlag)
      : from(fromCls), to(toCls), param(params), handle(mh), needDec(decFlag) {
    toShortType = to ? *to->GetName() : 'V';
    fromShortType = *from->GetName();
    fromIsPrim = (from != nullptr) ? from->IsPrimitiveClass() : true;
    toIsPrim = (to != nullptr) ? to->IsPrimitiveClass() : true;
  }
  ~Converter() = default;

  bool Convert() noexcept {
    if (!toIsPrim && (from == WellKnown::GetMClassVoid() || from == WellKnown::GetMClassV())) {
      return true;
    }
    jvalue kSrcValue(*(param.value));
    (*(param.value)).j = 0;
    if (fromIsPrim && toIsPrim) {
      if (UNLIKELY(!primitiveutil::ConvertNarrowToWide(fromShortType, toShortType, kSrcValue, *(param.value)))) {
        ThrowWMT();
        return false;
      }
      return true;
    } else if (!fromIsPrim && !toIsPrim) {
      if (kSrcValue.l) {
        from = reinterpret_cast<MObject*>(kSrcValue.l)->GetClass();
      }
      param.value->l = kSrcValue.l;
      if (UNLIKELY(!MRT_ReflectClassIsAssignableFrom(*to, *from))) {
        MRT_ThrowNewException("java/lang/ClassCastException", GeneClassCastExceptionString(from, to).c_str());
        return false;
      }
      return true;
    } else if (!toIsPrim) {
      return ConvertPrimToObj(kSrcValue);
    } else {
      return ConvertObjToPrim(kSrcValue);
    }
  }
 private:
  bool ConvertObjToPrim(const jvalue &kSrcValue) noexcept {
    char unboxedType = '0';
    jvalue unboxedVal;
    unboxedVal.j = 0;
    if (UNLIKELY(!GetPrimShortTypeAndValue(reinterpret_cast<MObject*>(kSrcValue.l), unboxedType, unboxedVal, from))) {
      ThrowWMT();
      return false;
    }

    if (UNLIKELY(kSrcValue.l == nullptr)) {
      MRT_ThrowNewException("java/lang/NullPointerException", "");
      return false;
    }
    if (needDec) {
      RC_LOCAL_DEC_REF(kSrcValue.l);
    }

    if (UNLIKELY(!primitiveutil::ConvertNarrowToWide(unboxedType, toShortType, unboxedVal, *(param.value)))) {
      if (from == WellKnown::GetMClassNumber()) { // boxed to type must be assignablefrom number
        MRT_ThrowNewException("java/lang/ClassCastException", GeneClassCastExceptionString(from, to).c_str());
      } else {
        ThrowWMT();
      }
      return false;
    }
    return true;
  }

  bool ConvertPrimToObj(const jvalue &kSrcValue) noexcept {
    char type;
    if (!GetPrimShortType(to, type)) {
      // to_type unboxed failed, so if from_type can be boxed(is prim type),
      // it could be converted if to_type is Number or Object
      if (CheckPrimitiveCanBoxed(fromShortType) && (to == WellKnown::GetMClassNumber() ||
          to == WellKnown::GetMClassObject())) {
        type = fromShortType;
      } else {
        ThrowWMT();
        return false;
      }
    } else if (UNLIKELY(fromShortType != type)) {
      ThrowWMT();
      return false;
    }

    if (UNLIKELY(!primitiveutil::ConvertNarrowToWide(fromShortType, type, kSrcValue, *(param.value)))) {
      ThrowWMT();
      return false;
    }
    jobject boxed = reinterpret_cast<jobject>(primitiveutil::BoxPrimitive(type, kSrcValue));
    param.value->l = boxed;
    return true;
  }

  void ThrowWMT() const noexcept {
    string exceptionStr = GeneExceptionString(handle, param.parameterTypes, param.arraySize);
    MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
  }

  const MClass *from;
  const MClass *to;
  ConvertJStruct &param;
  const MethodHandle &handle;
  bool needDec;
  char toShortType;
  char fromShortType;
  bool fromIsPrim;
  bool toIsPrim;
};

bool ConvertJvalue(ConvertJStruct &param, const MethodHandle &handle,
                   const MClass *from, const MClass *to, bool needDec) {
  Converter converter(param, handle, from, to, needDec);
  return converter.Convert();
}

bool IllegalArgumentCheck(const ArgsWrapper &args, MClass *from, MClass *to, jvalue &value) {
  jobject obj = *args.GetObject();
  value.l = obj;
  if (from != to) {
    if (obj != nullptr) {
      from = reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(obj))->GetClass();
    }
    if (UNLIKELY(!MRT_ReflectClassIsAssignableFrom(*to, *from))) {
      MRT_ThrowNewException("java/lang/IllegalArgumentException", GeneIllegalArgumentExceptionString(from, to).c_str());
      return false;
    }
  }
  return true;
}

bool DirectCallParamsConvert(jvalue &val, CallParam &param, const MethodHandle &mh,
                             const MClass *from, const MClass *to) {
  ConvertJStruct convertJStruct = { param.paramTypes, param.arraySize, &val };
  if (to != from && !ConvertJvalue(convertJStruct, mh, from, to)) {
    return false;
  }
  return true;
}

#define VALUECONVERT(statement)               \
  STATEMENTCHECK(statement)                   \
  if (to == WellKnown::GetMClassD()) {        \
    paramArray.AddDouble(curValue.d);         \
  } else if (to == WellKnown::GetMClassF()) { \
    paramArray.AddFloat(curValue.f);          \
  } else if (to == WellKnown::GetMClassJ()) { \
    paramArray.AddInt64(curValue.j);          \
  } else if (to == WellKnown::GetMClassI() || \
             to == WellKnown::GetMClassZ() || \
             to == WellKnown::GetMClassB() || \
             to == WellKnown::GetMClassS() || \
             to == WellKnown::GetMClassC()) { \
    paramArray.AddInt32(curValue.i);          \
  } else {                                    \
    paramArray.AddReference(reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(curValue.l)));\
  }                                           \
  break;

// we put converted jobject to SLR, and will be destructed in caller
bool ConvertParams(CallParam &param, const MethodHandle &mh,
                   const ArgsWrapper &args, BaseArgValue &paramArray, ScopedHandles&) {
  MethodMeta *method = mh.GetMethodMeta();
  vector<MClass*> ptypesVal = mh.GetMethodTypeMplObj()->GetParamsType();
  uint32_t size = param.arraySize - 1;
  for (uint32_t i = param.beginIndex; i < size; ++i) {
    MClass *to = ptypesVal[i];
    MClass *from = param.paramTypes[i];
    jvalue curValue;
    curValue.j = 0;
    switch (param.typesMark[i]) {
      case 'B':
        curValue.b = static_cast<jbyte>(args.GetJint());
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      case 'C':
        curValue.c = static_cast<jchar>(args.GetJint());
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      case 'S':
        curValue.s = static_cast<jshort>(args.GetJint());
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      case 'Z':
        curValue.z = static_cast<jboolean>(args.GetJint());
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      case 'I':
        curValue.i = args.GetJint();
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      case 'D':
        curValue.d = args.GetJdouble();
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      case 'F':
        curValue.f = args.GetJfloat();
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      case 'J':
        curValue.j = args.GetJlong();
        VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
      default: {
        // maple throw IllegalArgumentException when this obj convert fail
        if (i == 0 && !method->IsStatic() && from != to) {
          VALUECONVERT(!IllegalArgumentCheck(args, from, to, curValue))
        } else {
          curValue.l = *args.GetObject();
          VALUECONVERT(!DirectCallParamsConvert(curValue, param, mh, from, to))
        }
      }
    }
    if (from->IsPrimitiveClass() && !to->IsPrimitiveClass()) {
      ObjHandle<MObject> keepAlive(curValue.l);
    }
  }
  return true;
}

string GeneExceptionString(const MethodHandle &mh, MClass **parameterTypes, uint32_t arraySize) {
  string str = "Expected ";
  string className;
  str += mh.ValidTypeToString();
  str += " but was (";
  uint32_t paraNum = arraySize - 1; // Remove the return value.
  for (uint32_t i = 0; i < paraNum; ++i) {
    if (parameterTypes[i] != nullptr) {
      className.clear();
      parameterTypes[i]->GetTypeName(className);
      str += className;
    } else {
      str += "void";
    }
    if (i != paraNum - 1) {
      str += ", ";
    }
  }
  str += ")";
  if (parameterTypes[arraySize - 1] != nullptr) {
    className.clear();
    parameterTypes[arraySize - 1]->GetTypeName(className);
    str += className;
  } else {
    str += "void";
  }
  return str;
}

bool ConvertReturnValue(const MethodHandle &mh, MClass **parameterTypes, uint32_t arraySize, jvalue &value) {
  const MClass *from = mh.GetMethodTypeMplObj()->GetReTType();
  const MClass *to = parameterTypes[arraySize - 1];
  if (from == to) {
    return true;
  }
  if (to == WellKnown::GetMClassV()) {
    if (!from->IsPrimitiveClass()) {
      RC_LOCAL_DEC_REF(value.l);
    }
    return true;
  }
  if (from == WellKnown::GetMClassVoid() || from == WellKnown::GetMClassV()) {
    value.j = 0UL;
    return true;
  }
  ConvertJStruct convertJStruct = { parameterTypes, arraySize, &value };
  bool convertResult = ConvertJvalue(convertJStruct, mh, from, to, true);
  if (!convertResult) {
    RC_LOCAL_DEC_REF(value.l);
  }
  return convertResult;
}

void IsConvertibleOrThrow(uint32_t arraySize, const MethodHandle &mh, MClass **parameterTypes, uint32_t paramNum) {
  if (!IsConvertible(mh, arraySize)) {
    if (paramNum == 1 && mh.GetHandleKind() != kStaticCall) {
      MClass *from = parameterTypes[0];
      const vector<MClass*> &paramVec = mh.GetMethodTypeMplObj()->GetParamsType();
      MClass *to = paramVec.size() > 0 ? paramVec[0] : nullptr;
      string msg = GeneIllegalArgumentExceptionString(to, from);
      MRT_ThrowNewExceptionUnw("java/lang/IllegalArgumentException", msg.c_str());
    } else {
      string exceptionStr = GeneExceptionString(mh, parameterTypes, arraySize);
      MRT_ThrowNewExceptionUnw("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
    }
  }
}

bool ClinitCheck(const MClass *decCls) {
  if (UNLIKELY(!MRT_ReflectIsInit(*decCls))) {
    if (UNLIKELY(MRT_TryInitClass(*decCls) == ClassInitState::kClassInitFailed)) {
      LOG(ERROR) << "MRT_TryInitClass return fail" << maple::endl;
    }
    if (UNLIKELY(MRT_HasPendingException())) {
      return false;
    }
  }
  return true;
}

static MClass *GetClassFromDescriptorAndCalSize(const char *descriptor, uint32_t &refNum, uint32_t &byteArrSize,
                                                char *typesMark, const MClass *callerCls) {
  DCHECK(typesMark != nullptr) << "typesMark is nullptr" << maple::endl;
  DCHECK(descriptor != nullptr) << "GetClassFromDescriptorAndCalSize::descriptor is nullptr" << maple::endl;
  switch (*descriptor) {
    case 'I':
      byteArrSize += sizeof(int);
      *typesMark = 'I';
      return WellKnown::GetMClassI();
    case 'B':
      byteArrSize += sizeof(int);
      *typesMark = 'B';
      return WellKnown::GetMClassB();
    case 'F':
      byteArrSize += sizeof(int);
      *typesMark = 'F';
      return WellKnown::GetMClassF();
    case 'C':
      byteArrSize += sizeof(int);
      *typesMark = 'C';
      return WellKnown::GetMClassC();
    case 'S':
      byteArrSize += sizeof(int);
      *typesMark = 'S';
      return WellKnown::GetMClassS();
    case 'J':
      byteArrSize += sizeof(double);
      *typesMark = 'J';
      return WellKnown::GetMClassJ();
    case 'D':
      byteArrSize += sizeof(double);
      *typesMark = 'D';
      return WellKnown::GetMClassD();
    case 'Z':
      byteArrSize += sizeof(int);
      *typesMark = 'Z';
      return WellKnown::GetMClassZ();
    case 'V':
      byteArrSize += sizeof(int);
      *typesMark = 'V';
      return WellKnown::GetMClassV();
    default:
      *typesMark = 'L';
      ++refNum;
      return MClass::JniCast(MRT_GetClassByContextClass(*callerCls, descriptor));
  }
}

// the last element is return type
void GetParameterAndRtType(MClass **types, char *typesMark, const MString *protoStr,
                           SizeInfo &sz, const MClass *declareClass) {
  DCHECK(protoStr != nullptr);
  const char *methodSig = reinterpret_cast<const char*>(protoStr->GetContentsPtr());
  uint32_t count = protoStr->GetLength();

  int idx = 0;
  DCHECK(methodSig != nullptr) << "GetParameterAndRtTypeA::methodSig is nullptr" << maple::endl;
  ++methodSig;
  size_t len = count + 1;
  char *descriptor = reinterpret_cast<char*>(calloc(len, 1));
  if (UNLIKELY(descriptor == nullptr)) {
    return;
  }
  DCHECK(types != nullptr) << "rtTypeParam.types is nullptr" << maple::endl;
  while (*methodSig != ')') {
    if (memset_s(descriptor, len, 0, len) != EOK) {
      LOG(FATAL) << "memset_s fail." << maple::endl;
    }
    ParseSignatrueType(descriptor, methodSig);
    MClass *ptype = GetClassFromDescriptorAndCalSize(descriptor, sz.refNum, sz.byteArrSize,
                                                     typesMark + idx, declareClass);
    *(types + idx) = ptype;
    ++idx;
    ++methodSig;
  }
  ++methodSig;
  if (memset_s(descriptor, len, 0, len) != EOK) {
    LOG(FATAL) << "memset_s fail." << maple::endl;
  }
  ParseSignatrueType(descriptor, methodSig);
  MClass *rtType = GetClassFromDescriptorAndCalSize(descriptor, sz.refNum, sz.byteArrSize,
                                                    typesMark + idx, declareClass);
  *(types + idx) = rtType;
  free(descriptor);
}

static bool TransformEnter(MethodHandle &mh, SizeInfo &sz, const TypeInfo &typeInfo, jvalue &result) {
  vector<MClass*> handleTypes = mh.GetMethodTypeMplObj()->GetParamsType();
  Kind handleKind = mh.GetHandleKind();
  // invoke transform , marshal EmStackFrame and invoke transform method
  if (handleKind == kTransformCall || handleKind == kCallSiteTransformCall) {
    if (handleKind != kCallSiteTransformCall) {
      sz.byteArrSize = 0;
      sz.refNum = 0;
      CalcFrameSize(mh.GetMethodTypeMplObj()->GetReTType(), handleTypes, sz.byteArrSize, sz.refNum);
    }
    if (!mh.InvokeTransform(sz, typeInfo.typesMark, typeInfo.types, result)) {
      MRT_CheckThrowPendingExceptionUnw();
    }
    return true;
  }
  return false;
}

MClass *GetContextCls(Arg &arg, MString *&calleeName) {
  MClass *callerClass = nullptr;
  constexpr int kNewABIFlag = 1;
  if (reinterpret_cast<intptr_t>(calleeName) & kNewABIFlag) {
    calleeName = reinterpret_cast<MString*>(reinterpret_cast<intptr_t>(calleeName) - kNewABIFlag);
    MClass *callerCls = reinterpret_cast<MClass*>(arg.GetObject());
    if (callerCls != nullptr) {
      callerClass = callerCls->GetClass();
    }
    if (callerClass == WellKnown::GetMClassClass()) {
      callerClass = callerCls;
    }
  }
  return callerClass;
}

#if defined(__arm__)
int64_t PolymorphicCallEnter32(int32_t *args) {
#if defined(__ARM_PCS_VFP)
  Arm32Arg argsHandle(args);
#elif defined(__ARM_PCS)
  Arm32SoftFPArg argsHandle(args);
#endif
  MString *calleeStr = reinterpret_cast<MString*>(argsHandle.GetObject());
  MString *protoString = reinterpret_cast<MString*>(argsHandle.GetObject());
  uint32_t paramNum = static_cast<uint32_t>(argsHandle.GetJint());
  MObject *mhObj = argsHandle.GetObject();
  MClass *callerClass = GetContextCls(argsHandle, calleeStr);
  jvalue result = PolymorphicCallEnter(calleeStr, protoString, paramNum, mhObj, argsHandle, callerClass);
  return result.j;
}
#endif

jvalue PolymorphicCallEnter(const MString *calleeStr, const MString *protoString,
    uint32_t paramNum, MObject *mhObj, Arg &args, const MClass *declareClass) {
  if (mhObj == nullptr) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "");
  }
  SizeInfo sz(paramNum);
  char typesMark[sz.arraySize];
  MClass *paramTypes[sz.arraySize];

  GetParameterAndRtType(paramTypes, typesMark, protoString, sz, declareClass);
  jvalue result;
  result.j = 0;
  MethodHandle mh = MethodHandle(mhObj, args);
  MethodMeta &mplFieldOrMethodMeta = *mh.GetMethodMeta();
  bool isStaticMethod = mplFieldOrMethodMeta.IsStatic();
  bool isExactMatched = mh.ExactInvokeCheck(calleeStr, paramTypes, sz.arraySize);
  if (IsCallerTransformerJStr(paramNum, protoString)) { // handle EmStackFrame invoke
    if (!mh.InvokeWithEmStackFrame(args.GetObject(), result)) {
      MRT_CheckThrowPendingExceptionUnw();
    }
    return result;
  }
  if (IsFieldAccess(mh.GetHandleKind())) {
    if (!mh.FieldAccess(paramTypes, !isExactMatched, sz.arraySize, result)) {
      MRT_CheckThrowPendingExceptionUnw();
    }
    return result;
  }
  TypeInfo typeInfo = { paramTypes, typesMark };
  if (TransformEnter(mh, sz, typeInfo, result) || !ClinitCheck(mplFieldOrMethodMeta.GetDeclaringClass())) {
    return result;
  }
  if (!isExactMatched) { // isExactmatch check, if not we continue check IsConvertible
    IsConvertibleOrThrow(sz.arraySize, mh, paramTypes, paramNum);
  }
  CallParam dCP = { paramNum,  isExactMatched, paramTypes, typesMark, isStaticMethod, 0, sz.arraySize, 0 };
  if (!mh.NoParamFastCall(dCP, result)) {
    mh.DirectCall(dCP, result);
  }
  if (UNLIKELY(MRT_HasPendingException() || !ConvertReturnValue(mh, paramTypes, sz.arraySize, result))) {
    MRT_CheckThrowPendingExceptionUnw();
  }
  return result;
}
}  // namespace maple
