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
#include "mrt_methodhandle_mpl.h"
namespace maplert {
static MClass *GetClassFromDescriptorAndCalSize(const char *desc, vector<char> &typesMark, const MClass *callerCls) {
  DCHECK(desc != nullptr) << "GetClassFromDescriptorAndCalSize::descriptor is nullptr" << maple::endl;
  switch (*desc) {
    case 'I':
      typesMark.push_back('I');
      return WellKnown::GetMClassI();
    case 'B':
      typesMark.push_back('B');
      return WellKnown::GetMClassB();
    case 'F':
      typesMark.push_back('F');
      return WellKnown::GetMClassF();
    case 'C':
      typesMark.push_back('C');
      return WellKnown::GetMClassC();
    case 'S':
      typesMark.push_back('S');
      return WellKnown::GetMClassS();
    case 'J':
      typesMark.push_back('J');
      return WellKnown::GetMClassJ();
    case 'D':
      typesMark.push_back('D');
      return WellKnown::GetMClassD();
    case 'Z':
      typesMark.push_back('Z');
      return WellKnown::GetMClassZ();
    case 'V':
      typesMark.push_back('V');
      return WellKnown::GetMClassV();
    default:
      typesMark.push_back('L');
      string str(desc);
      return reinterpret_cast<MClass*>(MRT_GetClassByContextClass(*callerCls, str));
  }
}

// the last element is return type
void GetParameterAndRtType(vector<MClass*> &types, vector<char> &typesMark,
                           MObject *protoStr, const MClass *declareClass) {
  const char *methodSig = MRT_GetStringContentsPtrRaw(reinterpret_cast<jstring>(protoStr));
  int count = MRT_StringGetStringLength(reinterpret_cast<jstring>(protoStr));

  DCHECK(methodSig != nullptr) << "GetParameterAndRtType::methodSig is nullptr" << maple::endl;
  ++methodSig;
  size_t len = static_cast<size_t>(count) + 1;
  char descriptor[len];

  while (*methodSig != ')') {
    if (memset_s(descriptor, len, 0, len) != EOK) {
      LOG(FATAL) << "memset_s fail." << maple::endl;
    }
    ParseSignatrueType(descriptor, methodSig);
    MClass *ptype = GetClassFromDescriptorAndCalSize(descriptor, typesMark, declareClass);
    types.push_back(ptype);
    ++methodSig;
  }
  ++methodSig;
  if (memset_s(descriptor, len, 0, len) != EOK) {
    LOG(FATAL) << "memset_s fail." << maple::endl;
  }
  ParseSignatrueType(descriptor, methodSig);
  MClass *rtType = GetClassFromDescriptorAndCalSize(descriptor, typesMark, declareClass);
  types.push_back(rtType);
}

void VargToJvalueArray(vector<jvalue> &paramArray, VArg &varg, uint32_t paramNum, vector<char> &typesMark) {
  for (uint32_t i = 0; i < paramNum; ++i) {
    jvalue val;
    switch (typesMark[i]) {
      case 'C':
        val.c = static_cast<jchar>(varg.GetJint());
        break;
      case 'B':
        val.b = static_cast<jbyte>(varg.GetJint());
        break;
      case 'S':
        val.s = static_cast<jshort>(varg.GetJint());
        break;
      case 'Z':
        val.z = static_cast<jboolean>(varg.GetJint());
        break;
      case 'I':
        val.i = varg.GetJint();
        break;
      case 'D':
        val.d = varg.GetJdouble();
        break;
      case 'F':
        val.f = varg.GetJfloat();
        break;
      case 'J':
        val.j = varg.GetJlong();
        break;
      default:
        val.l = *varg.GetObject();
        break;
    }
    paramArray.push_back(val);
  }
}

jvalue MethodHandleCallEnter(MString *calleeName, MString *protoString, uint32_t paramNum,
                             MObject *methodHandle, VArg &args, const MClass *declareClass) {
  if (methodHandle == nullptr) {
    maplert::MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "");
  }
  vector<char> typesMark;
  vector<MClass*> paramTypes;
  vector<jvalue> paramValueArray;

  GetParameterAndRtType(paramTypes, typesMark, protoString, declareClass);
  VargToJvalueArray(paramValueArray, args, paramNum, typesMark);
  MethodHandleMpl methodHandleMplObj(methodHandle, MethodHandleMpl::IsExactInvoke(calleeName));
  methodHandleMplObj.CheckReturnType();
  jvalue val = methodHandleMplObj.invoke(paramValueArray, paramNum, typesMark, paramTypes);
  MRT_CheckThrowPendingExceptionUnw();
  return val;
}

void DropArguments(vector<jvalue> &paramArray, uint32_t &paramNum, const MObject *data,
                   vector<char> &typesMark, vector<MClass*> &cSTypes) {
  size_t numDroppedOffset = WellKnown::GetMFieldDropArgumentsDataNumDroppedOffset();
  size_t startPosOffset = WellKnown::GetMFieldDropArgumentsDataStartPosOffset();
  int32_t numDropped = MRT_LOAD_JINT(data, numDroppedOffset);
  int32_t startPos = MRT_LOAD_JINT(data, startPosOffset);
  uint32_t newSize = paramNum - static_cast<uint32_t>(numDropped);
  typesMark.erase(typesMark.begin() + startPos, typesMark.begin() + numDropped + startPos);
  cSTypes.erase(cSTypes.begin() + startPos, cSTypes.begin() + numDropped + startPos);
  paramArray.erase(paramArray.begin() + startPos, paramArray.begin() + numDropped + startPos);
  paramNum = newSize;
}

void BindTo(vector<jvalue> &paramArray, const MObject *data, uint32_t &paramNum, vector<char> &typesMark,
            vector<MClass*> &cSTypes) {
  size_t receiverOffset = WellKnown::GetMFieldBindToDataReceiverOffset();
  MObject *receiver = reinterpret_cast<MObject*>(data->LoadObjectNoRc(receiverOffset));
  if (receiver == nullptr) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "");
    return;
  }
  MClass *receiverCls = receiver->GetClass();
  jvalue val;
  val.l = reinterpret_cast<jobject>(receiver);
  paramArray.insert(paramArray.begin(), val);
  typesMark.insert(typesMark.begin(), 'L');
  cSTypes.insert(cSTypes.begin(), receiverCls);
  ++paramNum;
}

jvalue MethodHandleMpl::FilterReturnValue(vector<jvalue> &paramArray, const uint32_t paramNum,
    const MObject *data, vector<char> &typesMark, vector<MClass*> &cSTypes) {
  size_t targetOffset = WellKnown::GetMFieldFilterReturnValueDataTargetOffset();
  size_t filterOffset = WellKnown::GetMFieldFilterReturnValueDataFilterOffset();
  MObject *target = reinterpret_cast<MObject*>(data->LoadObjectNoRc(targetOffset));
  MObject *filter = reinterpret_cast<MObject*>(data->LoadObjectNoRc(filterOffset));
  MethodHandleMpl targetHandle(target, isInvokeExact);
  MethodHandleMpl filterHandle(filter, isInvokeExact);

  jvalue val = targetHandle.invoke(paramArray, paramNum, typesMark, cSTypes, false);
  vector<jvalue> filterParams;
  filterParams.push_back(val); // no needed if returnytpe=V
  MObject *finalType = targetHandle.typeArray->GetObjectElementNoRc(0);
  MethodType mt(finalType);
  MClass *returnType = const_cast<MClass*>(mt.GetReTType());
  vector<char> retTypeMark;
  retTypeMark.push_back(returnType->GetName()[0]);
  if (returnType != WellKnown::GetMClassV() && returnType != WellKnown::GetMClassVoid()) {
    MClass *csRetType = cSTypes.back();
    cSTypes.clear();
    cSTypes.push_back(returnType);
    cSTypes.push_back(csRetType);
    jvalue ret = filterHandle.invoke(filterParams, 1, retTypeMark, cSTypes);
    if (!returnType->IsPrimitiveClass()) {
      RC_LOCAL_DEC_REF(val.l);
    }
    return ret;
    // filterReturnValue Transform just have one param
  } else {
    MClass *csRetType = cSTypes.back();
    cSTypes.clear();
    cSTypes.push_back(csRetType);
    return filterHandle.invoke(filterParams, 0, retTypeMark, cSTypes);
  }
}

void PermuteArguments(vector<jvalue> &paramArray, const uint32_t paramNum, const MObject *data, vector<char> &typesMark,
                      vector<MClass*> &cSTypes) {
  size_t targetOffset = WellKnown::GetMFieldPermuteArgumentsDataTargetOffset();
  size_t reorderOffset = WellKnown::GetMFieldPermuteArgumentsDataReorderOffset();
  jobject target = reinterpret_cast<jobject>(data->LoadObjectNoRc(targetOffset));
  if (target == nullptr) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "");
    return;
  }
  int *reorder = reinterpret_cast<int*>(
      reinterpret_cast<MArray*>(data->LoadObjectNoRc(reorderOffset))->ConvertToCArray());
  vector<jvalue> paramArrayS = paramArray;
  vector<char> typesMarkS = typesMark;
  vector<MClass*> cSTypeS = cSTypes;
  for (uint32_t i = 0; i < paramNum; ++i) {
    uint32_t idx = static_cast<uint32_t>(reorder[i]);
    paramArray[i] = paramArrayS[idx];
    typesMark[i] = typesMarkS[idx];
    cSTypes[i] = cSTypeS[idx];
  }
}

static MethodMeta *RefineTargetMethodMpl(MethodMeta *method, const MObject *ref) {
  MClass *decClass = method->GetDeclaringClass();
  MClass *referClass = ref->GetClass();
  if (decClass != referClass && referClass != nullptr) {
    MethodMeta *realMthd = referClass->GetMethod(method->GetName(), method->GetSignature());
    if (realMthd == nullptr) {
      string msg = GeneIllegalArgumentExceptionString(referClass, decClass);
      MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.c_str());
      return method;
    }
    return realMthd;
  }
  return method;
}

jvalue FinalNode(uint32_t paramNum, vector<jvalue> &paramPtr, vector<char> &typesMarkPtr, MObject *meta) {
  ArgValue paramArray(0);
  JValueArg valueArg(paramPtr.data());
  ArgsWrapper argsWrapper(valueArg);
  FillArgsInfoNoCheck(argsWrapper, typesMarkPtr.data(), paramNum + 1, paramArray);
  jvalue result;
  result.l = 0UL;
  MethodMeta *method = reinterpret_cast<MethodMeta*>(meta);
  if (!method->IsStatic()) {
    method = RefineTargetMethodMpl(reinterpret_cast<MethodMeta*>(meta), paramArray.GetReceiver());
  }
  DoInvoke(*method, result, paramArray);
  return result;
}

string GeneExpectedNotMatchExceptionString(const MethodType &mt, vector<MClass*> cSTypes) {
  string str = "expected (";
  string className;
  vector<MClass*> types = mt.GetParamsType();
  size_t typeSize = types.size();
  for (size_t i = 0; i < typeSize; ++i) {
    className.clear();
    types[i]->GetTypeName(className);
    size_t idx = className.rfind(".");
    str += className.substr(idx + 1);
    if (i != typeSize - 1) {
      str += ",";
    }
  }
  str += ")";
  className.clear();
  const MClass *retType = mt.GetReTType(); // return type
  retType->GetTypeName(className);
  size_t index = className.rfind(".");
  str += className.substr(index + 1);

  str += " but found (";
  size_t paramNum = cSTypes.size() - 1; // Remove the return value.
  for (size_t i = 0; i < paramNum; ++i) {
    className.clear();
    cSTypes[i]->GetTypeName(className);
    size_t idx = className.rfind(".");
    str += className.substr(idx + 1);
    if (i != paramNum - 1) {
      str += ",";
    }
  }
  str += ")";
  className.clear();
  cSTypes[paramNum]->GetTypeName(className); // return value
  str += className;
  return str;
}

class ConverterMPL {
 public:
  ConverterMPL(const MethodType mt, const vector<MClass*> types, jvalue *val, const MClass *from, const MClass *to)
      : from(from), to(to), value(val), methodType(mt), cSTypes(types) {
    toShortType = to ? *to->GetName() : 'V';
    fromShortType = *from->GetName();
  }

  ~ConverterMPL() {
    from = nullptr;
    to = nullptr;
    value = nullptr;
  }

  bool Convert() noexcept {
    if (from == to) {
      return true;
    }
    bool toIsPrim = (to != nullptr) ? to->IsPrimitiveClass() : true;
    if (!toIsPrim && (from == WellKnown::GetMClassVoid() || from == WellKnown::GetMClassV())) {
      return true;
    }
    jvalue kSrcValue(*(value));
    (*(value)).j = 0;
    bool fromIsPrim = (from != nullptr) ? from->IsPrimitiveClass() : true;
    if (fromIsPrim && toIsPrim) {
      if (UNLIKELY(!primitiveutil::ConvertNarrowToWide(fromShortType, toShortType, kSrcValue, *(value)))) {
        ThrowWMT();
        return false;
      }
      return true;
    } else if (!fromIsPrim && !toIsPrim) {
      if (kSrcValue.l) {
        from = reinterpret_cast<MObject*>(kSrcValue.l)->GetClass();
      }
      value->l = kSrcValue.l;
      if (UNLIKELY(!MRT_ReflectClassIsAssignableFrom(*to, *from))) {
        MRT_ThrowNewException("java/lang/ClassCastException", GeneClassCastExceptionString(from, to).c_str());
        return false;
      }
      return true;
    } else if (!toIsPrim) {
      return ConvertPrimToObj(kSrcValue);
    } else {
      bool ret = ConvertObjToPrim(kSrcValue);
      if (ret == false) {
        value->j = kSrcValue.j;
      }
      return ret;
    }
  }

  void SetDec() {
    needDec = true;
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

    if (UNLIKELY(!primitiveutil::ConvertNarrowToWide(unboxedType, toShortType, unboxedVal, *(value)))) {
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

    if (UNLIKELY(!primitiveutil::ConvertNarrowToWide(fromShortType, type, kSrcValue, *(value)))) {
      ThrowWMT();
      return false;
    }
    jobject boxed = reinterpret_cast<jobject>(primitiveutil::BoxPrimitive(type, kSrcValue));
    value->l = boxed;
    return true;
  }

  string GeneConvertFailExceptionString(const MethodType mt, vector<MClass*> callsiteTypes) const {
    string str = "cannot convert MethodHandle(";
    string className;
    vector<MClass*> types = mt.GetParamsType();
    size_t paramNum = callsiteTypes.size() - 1; // Remove the return value.
    uint32_t typesIdx = 0;
    for (size_t i = 0; i < paramNum; ++i) {
      className.clear();
      if (types[typesIdx]->IsArrayClass()) {
        types[typesIdx]->GetTypeName(className);
        size_t idxL = className.rfind(".");
        size_t idxR = className.rfind("[");
        str += className.substr(idxL + 1, idxR - idxL - 1);
      } else {
        types[i]->GetTypeName(className);
        size_t idx = className.rfind(".");
        str += className.substr(idx + 1);
        ++typesIdx;
      }
      if (i != paramNum - 1) {
        str += ",";
      }
    }
    str += ")";
    className.clear();
    const MClass *retType = mt.GetReTType(); // return type
    retType->GetTypeName(className);
    size_t index = className.rfind(".");
    str += className.substr(index + 1);
    str += " to (";
    for (size_t i = 0; i < paramNum; ++i) {
      className.clear();
      callsiteTypes[i]->GetTypeName(className);
      size_t idx = className.rfind(".");
      str += className.substr(idx + 1);
      if (i != paramNum - 1) {
        str += ",";
      }
    }
    str += ")";
    className.clear();
    callsiteTypes[paramNum]->GetTypeName(className); // return value
    index = className.rfind(".");
    str += className.substr(index + 1);
    return str;
  }

  void ThrowWMT() const noexcept {
    string exceptionStr = GeneConvertFailExceptionString(methodType, cSTypes);
    MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
  }

  const MClass *from;
  const MClass *to;
  jvalue *value;
  bool needDec = false;
  char toShortType;
  char fromShortType;
  MethodType methodType;
  vector<MClass*> cSTypes;
};

bool ConvertJvalueMPL(const MethodType mt, const vector<MClass*> cSTypes,
                      jvalue *val, const MClass *from, const MClass *to) {
  ConverterMPL converter(mt, cSTypes, val, from, to);
  return converter.Convert();
}

bool ConvertJvalueWithDecMPL(const MethodType mt, const vector<MClass*> cSTypes, jvalue *val,
                             const MClass *from, const MClass *to) {
  ConverterMPL converter(mt, cSTypes, val, from, to);
  converter.SetDec();
  return converter.Convert();
}

bool MethodHandleMpl::CheckParamsType(vector<MClass*> &cSTypes, uint32_t csTypesNum, vector<jvalue> &paramPtr,
                                      uint32_t idx, bool checkRet) {
  MObject *type = typeArray->GetObjectElementNoRc(idx);
  MethodType mt(type);
  const vector<MClass*> &paramsVec = mt.GetParamsType();
  if (paramsVec.size() != csTypesNum) {
    string exceptionStr = GeneExpectedNotMatchExceptionString(mt, cSTypes);
    MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
    return false;
  }
  if (isInvokeExact) { // todo, consider nominal type
    if (std::equal(paramsVec.begin(), paramsVec.end(), cSTypes.begin()) &&
        (!checkRet || mt.GetReTType() == cSTypes[csTypesNum])) {
      return true;
    }
    string exceptionStr = GeneExpectedNotMatchExceptionString(mt, cSTypes);
    MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
    return false;
  }
  for (uint32_t i = 0; i < csTypesNum; ++i) {
    if (!ConvertJvalueMPL(mt, cSTypes, &paramPtr[i], cSTypes[i], paramsVec[i])) {
      string exceptionStr = GeneExpectedNotMatchExceptionString(mt, cSTypes);
      MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
      return false;
    }
    bool fromIsPrim = (cSTypes[i] != nullptr) ? cSTypes[i]->IsPrimitiveClass() : true;
    bool toIsPrim = (paramsVec[i] != nullptr) ? paramsVec[i]->IsPrimitiveClass() : true;
    if (fromIsPrim && !toIsPrim) {
      ObjHandle<MObject> convertedVal(reinterpret_cast<MObject*>(paramPtr[i].j));
    }
  }
  if (checkRet && mt.GetReTType() != cSTypes[csTypesNum]) {
    string exceptionStr = GeneExpectedNotMatchExceptionString(mt, cSTypes);
    MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
    return false;
  }
  return true;
}

bool ConvertReturnValueMPL(const MethodType mt, const vector<MClass*> cSTypes,
                           const MClass *from, const MClass *to, jvalue *val) {
  if (from == to) {
    return true;
  }
  if (to == WellKnown::GetMClassV()) {
    if (!from->IsPrimitiveClass()) {
      RC_LOCAL_DEC_REF(val->l);
    }
    return true;
  }
  if (from == WellKnown::GetMClassVoid() || from == WellKnown::GetMClassV()) {
    val->l = 0L;
    return true;
  }
  bool convertResult = ConvertJvalueWithDecMPL(mt, cSTypes, val, from, to);
  if (!convertResult) {
    RC_LOCAL_DEC_REF(val->l);
  }
  return convertResult;
}
static MArray *GeneratePrimMArray(const MethodType mt, uint32_t arrayLength, vector<MClass*> &cSTypes,
                                  vector<jvalue> &paramPtr, const MClass *elemType) {
  ObjHandle<MArray> arrayHandle(MArray::NewPrimitiveArrayComponentClass(arrayLength, *elemType));
  for (uint32_t i = 0; i < arrayLength; ++i) {
    uint32_t idx = static_cast<uint32_t>(cSTypes.size()) - arrayLength + i - 1;
    jvalue val = paramPtr[idx];
    if (!ConvertJvalueMPL(mt, cSTypes, &val, cSTypes[idx], elemType)) {
      return nullptr;
    }
    switch (*elemType->GetName()) {
      case 'I':
        arrayHandle()->SetPrimitiveElement(i, val.i);
        break;
      case 'B':
        arrayHandle()->SetPrimitiveElement(i, val.b);
        break;
      case 'S':
        arrayHandle()->SetPrimitiveElement(i, val.s);
        break;
      case 'C':
        arrayHandle()->SetPrimitiveElement(i, val.c);
        break;
      case 'Z':
        arrayHandle()->SetPrimitiveElement(i, val.z);
        break;
      case 'D':
        arrayHandle()->SetPrimitiveElement(i, val.d);
        break;
      case 'F':
        arrayHandle()->SetPrimitiveElement(i, val.f);
        break;
      case 'J':
        arrayHandle()->SetPrimitiveElement(i, val.j);
        break;
      default:
        arrayHandle()->SetPrimitiveElement(i, val.l);
        break;
    }
  }
  return reinterpret_cast<MArray*>(arrayHandle.Return());
}

static MArray *GenerateObjMArray(const MethodType mt, uint32_t arrayLength, vector<MClass*> &cSTypes,
                                 vector<jvalue> &paramPtr, const MClass *elemType) {
  ObjHandle<MArray> arrayHandle(MArray::NewObjectArrayComponentClass(arrayLength, *WellKnown::GetMClassObject()));
  for (uint32_t i = 0; i < arrayLength; ++i) {
    uint32_t idx = static_cast<uint32_t>(cSTypes.size()) - arrayLength + i - 1;
    jvalue val = paramPtr[idx];
    if (!ConvertJvalueMPL(mt, cSTypes, &val, cSTypes[idx], elemType)) {
      return nullptr;
    }
    if (cSTypes[idx]->IsPrimitiveClass()) {
      ObjHandle<MObject> convertedVal(reinterpret_cast<MObject*>(val.j));
    }
    arrayHandle()->SetObjectElement(i, reinterpret_cast<MObject*>(val.l));
  }
  return reinterpret_cast<MArray*>(arrayHandle.Return());
}

static MArray *GenerateMArray(const MethodType mt, uint32_t arrayLength, vector<MClass*> &cSTypes,
                              vector<jvalue> &paramPtr, const MClass *elemType) {
  if (elemType->IsPrimitiveClass()) {
    return GeneratePrimMArray(mt, arrayLength, cSTypes, paramPtr, elemType);
  }
  return GenerateObjMArray(mt, arrayLength, cSTypes, paramPtr, elemType);
}

class ReturnInfoClosure {
 public:
  ReturnInfoClosure(vector<char> &mark, vector<MClass*> &cls) : typesMark(mark), cSTypes(cls) {
    returnMark = typesMark.back();
    typesMark.pop_back();
    returnType = cSTypes.back();
    cSTypes.pop_back();
  }
  ~ReturnInfoClosure() noexcept {
    typesMark.push_back(returnMark);
    cSTypes.push_back(returnType);
    returnType = nullptr;
  }

 private:
  vector<char> &typesMark;
  vector<MClass*> &cSTypes;
  char returnMark;
  MClass *returnType;
};

void MethodHandleMpl::PrepareVarg(vector<jvalue> &param, vector<char> &mark,
                                  vector<MClass*> &types, uint32_t &csTypesNum) {
  // do not prepare for invokeExact
  if (isInvokeExact) {
    return;
  }
  MethodMeta *method = GetMeta(transformNum - 1);
  if (method != nullptr && modifier::IsVarargs(method->GetMod())) {
    MethodType mt(typeArray->GetObjectElementNoRc(transformNum - 1));
    constexpr int8_t nParamNum = 2;
    if (1 + mt.GetParamsType().size() == types.size() &&
        (types.size() > 1 && types[types.size() - nParamNum]->IsArrayClass())) {
      // do nothing ? or check last param type isAssignableFrom
    } else if (types.size() < mt.GetParamsType().size() - 1) {
      string exceptionStr = GeneExpectedNotMatchExceptionString(mt, types);
      MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
    } else { // convert callsite param to a java array
      size_t arrayLength; // calculate convert num
      if (mt.GetParamsType().size() == 1) { // one param,  just an object array,  convert all callsite params
        arrayLength = method->IsStatic() ? types.size() - 1 : types.size();
      } else {
        arrayLength = types.size() - mt.GetParamsType().size();
      }
      ObjHandle<MObject> arrayHandle(GenerateMArray(
          mt, static_cast<uint32_t>(arrayLength), types, param, mt.GetParamsType().back()->GetComponentClass()));
      if (arrayHandle() == nullptr) {
        return;
      }
      {
        size_t iter = arrayLength;
        ReturnInfoClosure info(mark, types);
        while (iter-- != 0) {
          param.pop_back();
          mark.pop_back();
          types.pop_back();
        }
        jvalue val;
        val.l = reinterpret_cast<jobject>(arrayHandle());
        param.push_back(val);
        mark.push_back('L');
        types.push_back(mt.GetParamsType().back());
      }
    }
    csTypesNum = static_cast<uint32_t>(param.size());
  }
}

string MethodHandleMpl::GeneNoSuchMethodExceptionString(const MethodType &methodType, const MethodMeta &method) {
  string str = "no such method: ";
  string className;
  MClass *cls = method.GetDeclaringClass();
  cls->GetTypeName(className);
  str += className;
  str += ".";
  str += method.GetName();
  vector<MClass*> types = methodType.GetParamsType();
  size_t typesSize = types.size();
  str += "(";
  for (size_t i = 0; i < typesSize; ++i) {
    className.clear();
    types[i]->GetTypeName(className);
    size_t idx = className.rfind(".");
    str += className.substr(idx + 1);
    if (i != typesSize - 1) {
      str += ",";
    }
  }
  str += ")";
  const MClass *retType = methodType.GetReTType();
  className.clear();
  retType->GetTypeName(className);
  str += className;
  // add invokeVirtual/invokeStatic
  return str;
}

void MethodHandleMpl::CheckReturnType() {
  MethodMeta *method = GetMeta(transformNum - 1);
  MObject *type = typeArray->GetObjectElementNoRc(0);
  if (type != nullptr && method != nullptr) {
    MethodType mt(type);
    if (mt.GetReTType() != method->GetReturnType()) {
      string exceptionStr = GeneNoSuchMethodExceptionString(mt, *method);
      MRT_ThrowNewException("java/lang/NoSuchMethodException", exceptionStr.c_str());
    }
  }
}

jvalue MethodHandleMpl::invoke(vector<jvalue> &paramPtr, uint32_t csTypesNum, vector<char> &typesMark,
                               vector<MClass*> &cSTypes, bool convertRetVal) {
  jvalue result;
  result.l = 0;
  ScopedHandles sHandles;
  PrepareVarg(paramPtr, typesMark, cSTypes, csTypesNum);
  if (MRT_HasPendingException()) {
    return result;
  }
  int32_t *opCArray = reinterpret_cast<int32_t*>(MRT_JavaArrayToCArray(opArray->AsJarray()));
  for (int64_t i = transformNum - 1; i >= 0; --i) {
    uint32_t idx = static_cast<uint32_t>(i);
    MObject *data = dataArray->GetObjectElementNoRc(idx);
    switch ((OptionFlag)opCArray[idx]) {
      case OptionFlag::kDropArguments: {
        if (!CheckParamsType(cSTypes, csTypesNum, paramPtr, idx)) {
          return result;
        }
        DropArguments(paramPtr, csTypesNum, data, typesMark, cSTypes);
        break;
      }
      case OptionFlag::kFinal: {
        if (!CheckParamsType(cSTypes, csTypesNum, paramPtr, idx, false)) {
          return result;
        }
        result = FinalNode(csTypesNum, paramPtr, typesMark, metaArray->GetObjectElementNoRc(idx));
        break;
      }
      case OptionFlag::kFilterReturnValue:
        // filterReturnValueChecks in java, so unnecessary check
        result = FilterReturnValue(paramPtr, csTypesNum, data, typesMark, cSTypes);
        break;
      case OptionFlag::kBindto: {
        BindTo(paramPtr, data, csTypesNum, typesMark, cSTypes);
        if (MRT_HasPendingException()) {
          return result;
        }
        break;
      }
      case OptionFlag::kPermuteArguments:
        // permuteArgumentChecks in java, so unnecessary check
        PermuteArguments(paramPtr, csTypesNum, data, typesMark, cSTypes);
        break;
    }
  }
  MObject *finalType = typeArray->GetObjectElementNoRc(0);
  if (convertRetVal && finalType != nullptr) { // filterReturnValueChecks in java, so unnecessary check
    MethodType finalMT(finalType);
    ConvertReturnValueMPL(finalMT, cSTypes, finalMT.GetReTType(), cSTypes[csTypesNum], &result);
  }
  return result;
}
}
