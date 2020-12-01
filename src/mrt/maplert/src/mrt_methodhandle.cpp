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
#include "mrt_methodhandle.h"
#include "mrt_handleutil.h"
#include "mstring_inline.h"

namespace maplert {
constexpr char MethodHandle::kInvokeMtd[];
constexpr char MethodHandle::kInvokeExactMtd[];

MethodMeta *RefineTargetMethod(Kind handleKind, MethodMeta *method, const MClass *referClass) {
  MClass *decClass = method->GetDeclaringClass();
  if (handleKind == kSuperCall) {
    if (decClass == referClass) {
      return method;
    } else {
      if (!MRT_ReflectClassIsInterface(*decClass)) {
        CHECK_E_P(referClass == nullptr, "RefineTargetMethod: referClass is nullptr.");
        MClass *superClass = referClass->GetSuperClass();
        if (superClass != nullptr) {
          return superClass->GetDeclaredMethod(method->GetName(), method->GetSignature());
        }
      }
    }
  } else if (handleKind == kVirtualCall || handleKind == kInterfaceCall) {
    if (decClass != referClass && referClass != nullptr) {
      MethodMeta *realMthd = referClass->GetMethod(method->GetName(), method->GetSignature());
      if (realMthd == nullptr) {
        string msg = GeneIllegalArgumentExceptionString(referClass, decClass);
        MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.c_str());
        return method;
      }
      return realMthd;
    }
  } else if (handleKind == kDirectCall && method->IsConstructor() && decClass == WellKnown::GetMClassString()) {
    if (StringToStringFactoryMap.size() == 0) {
      MClass *strFactory =
          MClass::JniCast(MRT_GetClassByContextClass(NULL, "Ljava_2Flang_2FStringFactory_3B"));
      CHECK_E_P(strFactory == nullptr, "MRT_GetClassByContextClass return nullptr.");
      std::vector<MethodMeta*> methodsVec;
      reinterpret_cast<MClass*>(strFactory)->GetDeclaredMethods(methodsVec, false);
      std::lock_guard<std::mutex> guard(mtx);
      std::for_each(methodsVec.begin(), methodsVec.end(), [](MethodMeta *mthd) {
        if (stringInitMethodSet.find(mthd->GetName()) != stringInitMethodSet.end()) {
          StringToStringFactoryMap[mthd->GetSignature()] = mthd;
        }
      });
    }
    string originSig = method->GetSignature();
    originSig.replace(originSig.size() - 1, 1, "Ljava/lang/String;");
    MethodMeta *realMthd = StringToStringFactoryMap[originSig];
    return realMthd;
  }
  return method;
}

std::string MethodType::ToString() const noexcept {
  string s = "(";
  string className;
  size_t size = paramsType.size();
  for (size_t i = 0; i < size; ++i) {
    if (paramsType[i] != nullptr) {
      className.clear();
      reinterpret_cast<MClass*>(paramsType[i])->GetTypeName(className);
      s += className;
    } else {
      s += "void";
    }
    if (i != size - 1) {
      s += ", ";
    }
  }
  s += ")";
  if (returnType != nullptr) {
    className.clear();
    reinterpret_cast<MClass*>(returnType)->GetTypeName(className);
    s += className;
  } else {
    s += "void";
  }
  return s;
}

std::string MethodHandle::ValidTypeToString() const noexcept {
  return validMethodTypeMplObjCache->ToString();
}

MethodMeta *MethodHandle::GetRealMethod(MObject *obj) const noexcept {
  MethodMeta *realMethod = nullptr;
  auto getRealMethod = std::bind(RefineTargetMethod, handleKind,
                                 reinterpret_cast<MethodMeta*>(mplFieldOrMethod), placeholders::_1);
  if (handleKind == kSuperCall) {
    realMethod = getRealMethod(GetMethodTypeMplObj()->GetParamsType()[0]);
  } else {
    bool isLiteralString = false;
    if (obj != nullptr && obj->GetClass() == WellKnown::GetMClassString()) {
      MString *jstr = reinterpret_cast<MString*>(obj);
      isLiteralString = jstr->IsLiteral();
    }
    if (IS_HEAP_ADDR(reinterpret_cast<address_t>(obj)) || isLiteralString) {
      realMethod = getRealMethod(obj->GetClass());
    } else {
      realMethod = getRealMethod(nullptr);
    }
  }
  if (realMethod == nullptr) {
    LOG(ERROR) << "GetRealMethod return null" << maple::endl;
  }
  return realMethod;
}

char MethodHandle::GetReturnTypeMark(char markInProto) const noexcept {
  char mark;
  const MClass *retType = methodTypeMplObjCache->GetReTType();
  if (handleKind != kCallSiteTransformCall) {
    mark = *(retType->GetName());
  } else {
    mark = markInProto;
  }
  return mark;
}

MObject *MethodHandle::CreateEmStackFrame(const MArray *bArray, const MObject *refArray,
                                          MClass **types, uint32_t arraySize) const {
  ScopedHandles sHandles;
  ObjHandle<MObject> callerType(MObject::NewObject(*WellKnown::GetMClassMethodType()));
  if (callerType == 0) {
    LOG(FATAL) << "CreateEmStackFrame: callerType is nullptr" << maple::endl;
  }
  callerType->StoreObjectNoRc(WellKnown::GetMFieldMethodHandlepRTypeOffset(), types[arraySize - 1]);
  ObjHandle<MArray> paramArray(MRT_NewObjArray(static_cast<int>(arraySize) - 1, *WellKnown::GetMClassClass(), nullptr));
  for (uint32_t i = 0; i < arraySize - 1; ++i) {
    paramArray->SetObjectElement(i, types[i]);
  }
  callerType()->StoreObject(WellKnown::GetMFieldMethodHandlePTypesOffset(), paramArray());

  ObjHandle<MObject> EmStackFrameObj(MObject::NewObject(*WellKnown::GetMClassEmulatedStackFrame()));
  EmStackFrameObj->StoreObject(WellKnown::GetMFieldEmStackFrameCallsiteOffset(), callerType());

  if (handleKind == kCallSiteTransformCall) {
    EmStackFrameObj->StoreObject(WellKnown::GetMFieldEmStackFrameTypeOffset(), callerType());
  } else {
    EmStackFrameObj->StoreObject(WellKnown::GetMFieldEmStackFrameTypeOffset(), GetMethodTypeJavaObj());
  }
  EmStackFrameObj->StoreObject(WellKnown::GetMFieldEmStackFrameStackFrameOffset(), bArray);
  EmStackFrameObj->StoreObject(WellKnown::GetMFieldEmStackFrameReferencesOffset(), refArray);
  return EmStackFrameObj.ReturnObj();
}

bool MethodHandle::FieldSGet(MClass **parameterTypes, bool doConvert, uint32_t arraySize, jvalue &result) {
  char shortFieldType = *(GetFieldMeta()->GetTypeName());
  jfieldID field = reinterpret_cast<jfieldID>(GetFieldMeta());
  switch (shortFieldType) {
    case 'Z':
      result.z = MRT_ReflectGetFieldjboolean(field, nullptr);
      break;
    case 'B':
      result.b = MRT_ReflectGetFieldjbyte(field, nullptr);
      break;
    case 'C':
      result.c = MRT_ReflectGetFieldjchar(field, nullptr);
      break;
    case 'S':
      result.s = MRT_ReflectGetFieldjshort(field, nullptr);
      break;
    case 'I':
      result.i = MRT_ReflectGetFieldjint(field, nullptr);
      break;
    case 'J':
      result.j = MRT_ReflectGetFieldjlong(field, nullptr);
      break;
    case 'F':
      result.f = MRT_ReflectGetFieldjfloat(field, nullptr);
      break;
    case 'D':
      result.d = MRT_ReflectGetFieldjdouble(field, nullptr);
      break;
    case 'V':
      LOG(FATAL) << "Unreachable: " << shortFieldType;
      break;
    default: {  // reference
      result.l = MRT_ReflectGetFieldjobject(field, nullptr);
      break;
    }
  }
  if (doConvert && !ConvertReturnValue(*this, parameterTypes, arraySize, result)) {
    return false;
  }
  return true;
}

bool MethodHandle::FieldSPut(MClass **parameterTypes, bool doConvert, uint32_t arraySize) {
  jvalue setVal;
  setVal.j = 0;
  constexpr size_t paramTypeIndex = 0;
  GetValFromVargs(setVal, *parameterTypes[paramTypeIndex]->GetName());
  vector<MClass*> ptypesVal = MethodType(GetMethodTypeJavaObj()).GetParamsType();
  if (doConvert) {
    MClass *from = parameterTypes[paramTypeIndex];
    MClass *to = ptypesVal[0];
    if (from != to) {
      ConvertJStruct convertJStruct = { parameterTypes, arraySize, &setVal };
      STATEMENTCHECK(!ConvertJvalue(convertJStruct, *this, from, to))
    }
  }
  jfieldID field = reinterpret_cast<jfieldID>(GetFieldMeta());
  switch (*(GetFieldMeta()->GetTypeName())) {
    case 'Z':
      MRT_ReflectSetFieldjboolean(field, nullptr, setVal.z);
      break;
    case 'B':
      MRT_ReflectSetFieldjbyte(field, nullptr, setVal.b);
      break;
    case 'C':
      MRT_ReflectSetFieldjchar(field, nullptr, setVal.c);
      break;
    case 'S':
      MRT_ReflectSetFieldjshort(field, nullptr, setVal.s);
      break;
    case 'I':
      MRT_ReflectSetFieldjint(field, nullptr, setVal.i);
      break;
    case 'J':
      MRT_ReflectSetFieldjlong(field, nullptr, setVal.j);
      break;
    case 'F':
      MRT_ReflectSetFieldjfloat(field, nullptr, setVal.f);
      break;
    case 'D':
      MRT_ReflectSetFieldjdouble(field, nullptr, setVal.d);
      break;
    case 'V':
      LOG(FATAL) << "Unreachable: " << *(GetFieldMeta()->GetTypeName());
      break;
    default: {  // reference
      MRT_ReflectSetFieldjobject(field, nullptr, setVal.l);
      break;
    }
  }
  return true;
}

bool MethodHandle::FieldPut(MClass **parameterTypes, bool doConvert, uint32_t arraySize) {
  jobject obj = *args.GetObject();
  jvalue setVal;
  setVal.j = 0;
  constexpr size_t paramTypeIndex = 1;
  GetValFromVargs(setVal, *parameterTypes[paramTypeIndex]->GetName());
  vector<MClass*> ptypesVal = MethodType(GetMethodTypeJavaObj()).GetParamsType();
  if (doConvert) {
    MClass *from = parameterTypes[paramTypeIndex];
    MClass *to = ptypesVal[1];
    ConvertJStruct convertJStruct = { parameterTypes, arraySize, &setVal };
    STATEMENTCHECK(from != to && !ConvertJvalue(convertJStruct, *this, from, to))
  }
  char shortFieldType = *(GetFieldMeta()->GetTypeName());
  jfieldID field = reinterpret_cast<jfieldID>(GetFieldMeta());
  switch (shortFieldType) {
    case 'Z':
      MRT_ReflectSetFieldjboolean(field, obj, setVal.z);
      break;
    case 'B':
      MRT_ReflectSetFieldjbyte(field, obj, setVal.b);
      break;
    case 'C':
      MRT_ReflectSetFieldjchar(field, obj, setVal.c);
      break;
    case 'S':
      MRT_ReflectSetFieldjshort(field, obj, setVal.s);
      break;
    case 'I':
      MRT_ReflectSetFieldjint(field, obj, setVal.i);
      break;
    case 'J':
      MRT_ReflectSetFieldjlong(field, obj, setVal.j);
      break;
    case 'F':
      MRT_ReflectSetFieldjfloat(field, obj, setVal.f);
      break;
    case 'D':
      MRT_ReflectSetFieldjdouble(field, obj, setVal.d);
      break;
    case 'V':
      LOG(FATAL) << "Unreachable: " << shortFieldType;
      break;
    default: {  // reference
      MRT_ReflectSetFieldjobject(field, obj, setVal.l);
      break;
    }
  }
  return true;
}

bool MethodHandle::FieldGet(MClass **parameterTypes, bool doConvert, uint32_t arraySize, jvalue &result) {
  jobject obj = *args.GetObject();
  char shortFieldType = *(GetFieldMeta()->GetTypeName());
  jfieldID field = reinterpret_cast<jfieldID>(GetFieldMeta());
  switch (shortFieldType) {
    case 'Z':
      result.z = MRT_ReflectGetFieldjbooleanUnsafe(field, obj);
      break;
    case 'B':
      result.b = MRT_ReflectGetFieldjbyteUnsafe(field, obj);
      break;
    case 'C':
      result.c = MRT_ReflectGetFieldjcharUnsafe(field, obj);
      break;
    case 'S':
      result.s = MRT_ReflectGetFieldjshortUnsafe(field, obj);
      break;
    case 'I':
      result.i = MRT_ReflectGetFieldjintUnsafe(field, obj);
      break;
    case 'J':
      result.j = MRT_ReflectGetFieldjlongUnsafe(field, obj);
      break;
    case 'F':
      result.f = MRT_ReflectGetFieldjfloatUnsafe(field, obj);
      break;
    case 'D':
      result.d = MRT_ReflectGetFieldjdoubleUnsafe(field, obj);
      break;
    case 'V':
      LOG(FATAL) << "Unreachable: " << shortFieldType;
      break;
    default: {  // reference
      result.l = MRT_ReflectGetFieldjobject(field, obj);
      break;
    }
  }
  STATEMENTCHECK(doConvert && !ConvertReturnValue(*this, parameterTypes, arraySize, result))
  return true;
}

void MethodHandle::GetValFromVargs(jvalue &value, char shortFieldType) {
  switch (shortFieldType) {
    case 'Z':
      value.z = static_cast<jboolean>(args.GetJint());
      break;
    case 'B':
      value.b = static_cast<jbyte>(args.GetJint());
      break;
    case 'C':
      value.c = static_cast<jchar>(args.GetJint());
      break;
    case 'S':
      value.s = static_cast<jshort>(args.GetJint());
      break;
    case 'I':
      value.i = args.GetJint();
      break;
    case 'J':
      value.j = args.GetJlong();
      break;
    case 'F':
      value.f = args.GetJfloat();
      break;
    case 'D':
      value.d = args.GetJdouble();
      break;
    case 'V':
      LOG(FATAL) << "Unreachable: " << shortFieldType;
      break;
    default: {  // reference
      value.l = *args.GetObject();
      break;
    }
  }
}

bool MethodHandle::FieldAccess(MClass **paramTypes, bool doConvert, uint32_t arraySize, jvalue &result) {
  switch (handleKind) {
    case Kind::kInstanceGet: {
      return FieldGet(paramTypes, doConvert, arraySize, result);
    }
    case Kind::kStaticGet: {
      STATEMENTCHECK(!ClinitCheck(GetFieldMeta()->GetDeclaringclass()))
      return FieldSGet(paramTypes, doConvert, arraySize, result);
    }
    case Kind::kInstancePut: {
      return FieldPut(paramTypes, doConvert, arraySize);
    }
    case Kind::kStaticPut: {
      STATEMENTCHECK(!ClinitCheck(GetFieldMeta()->GetDeclaringclass()))
      return FieldSPut(paramTypes, doConvert, arraySize);
    }
    default:
      LOG(FATAL) << "Unreachable: " << static_cast<uint32_t>(handleKind);
      BUILTIN_UNREACHABLE();
  }
}

bool MethodHandle::IsExactMatch() const noexcept {  // check for type and nominalType
  vector<MClass*> ptypes = methodTypeMplObjCache->GetParamsType();
  vector<MClass*> ptypesNominal = validMethodTypeMplObjCache->GetParamsType();
  STATEMENTCHECK(ptypes.size() != ptypesNominal.size())
  STATEMENTCHECK(!std::equal(ptypes.begin(), ptypes.end(), ptypesNominal.begin()))
  return true;
}

bool MethodHandle::IsExactMatch(MClass **types, uint32_t arraySize, bool isNominal) const noexcept {
  const MethodType *methodTypeMplObj = nullptr;
  if (isNominal) {
    methodTypeMplObj = GetValidMethodTypeMplObj();
  } else {
    methodTypeMplObj = GetMethodTypeMplObj();
  }
  vector<MClass*> ptypesVal = methodTypeMplObj->GetParamsType();
  size_t calleePtypeSize = arraySize - 1;  // remove return type
  STATEMENTCHECK(calleePtypeSize != ptypesVal.size())
  STATEMENTCHECK(!std::equal(ptypesVal.begin(), ptypesVal.end(), types))
  return methodTypeMplObj->GetReTType() == types[calleePtypeSize];
}

bool MethodHandle::ExactInvokeCheck(const MString *calleeName, MClass **parameterTypes, uint32_t arraySize) const {
  DCHECK(calleeName != nullptr);
  char *calleeNameStr = reinterpret_cast<char*>(calleeName->GetContentsPtr());
  if (!strcmp(calleeNameStr, kInvokeExactMtd)) {
    constexpr bool isNominal = true;
    if (GetNominalTypeJavaObj() != nullptr) {
      if (!IsExactMatch(parameterTypes, arraySize, isNominal)) {
        string exceptionStr = GeneExceptionString(*this, parameterTypes, arraySize);
        MRT_ThrowNewExceptionUnw("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
      }
      if (IsExactMatch() && IsExactMatch(parameterTypes, arraySize)) {
        return true;
      }
    } else {
      if (!IsExactMatch(parameterTypes, arraySize)) {
        string exceptionStr = GeneExceptionString(*this, parameterTypes, arraySize);
        MRT_ThrowNewExceptionUnw("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
      }
      return true;
    }
  }
  return false;
}

bool MethodHandle::FillInvokeArgs(const ArgsWrapper &argsWrapper, CallParam &paramStruct,
                                  BaseArgValue &paramArray, ScopedHandles &sHandles) const {
  if (paramStruct.isExactMatched) {
    FillArgsInfoNoCheck(argsWrapper, paramStruct.typesMark, paramStruct.arraySize, paramArray, paramStruct.beginIndex);
    return true;
  }
  return ConvertParams(paramStruct, *this, argsWrapper, paramArray, sHandles);
}

void MethodHandle::DirectCall(CallParam paramStruct, jvalue &result) {
  ArgValue paramArray(0);
  ArgValueInterp paramArrayInterp(0);
  MethodMeta *method = GetMethodMeta();
  ArgsWrapper argsWrapper(args);
  ScopedHandles sHandles;

  if (!paramStruct.isStaticMethod) {
    // get first arg when arraySize >= 2
    if (paramStruct.arraySize >= 2) {
      uint32_t argArraySize = paramStruct.arraySize;
      paramStruct.arraySize = 2; // Only get receiver
      if (!FillInvokeArgs(argsWrapper, paramStruct, paramArray, sHandles)) {
        return;
      }
      paramArrayInterp.AddReference(paramArray.GetReceiver());
      paramStruct.arraySize = argArraySize;
      paramStruct.beginIndex = 1; // Skip receiver
    }
    // GetReceiver will return nullptr if arraySize < 2
    method = GetRealMethod(paramArray.GetReceiver());
  }
  CHECK_E_V(method == nullptr, "method is nullptr");

  BaseArgValue *param = method->NeedsInterp() ? reinterpret_cast<BaseArgValue*>(&paramArrayInterp) :
                                                reinterpret_cast<BaseArgValue*>(&paramArray);
  // get remaining args.
  if (!FillInvokeArgs(argsWrapper, paramStruct, *param, sHandles)) {
    return;
  }

  if (method->NeedsInterp()) {
    if (method->IsStatic()) {
      result = maplert::interpreter::InterpJavaMethod<calljavastubconst::kJvalue>(method,
          nullptr, reinterpret_cast<uintptr_t>(paramArrayInterp.GetData()));
    } else {
      result = maplert::interpreter::InterpJavaMethod<calljavastubconst::kJvalue>(method,
          paramArrayInterp.GetReceiver(), reinterpret_cast<uintptr_t>(paramArrayInterp.GetData() + 1));
    }
  } else {
    DoInvoke(*method, result, paramArray);
  }
}

bool MethodHandle::NoParamFastCall(CallParam &paramStruct, jvalue &result) {
  vector<MClass*> handleTypes = GetMethodTypeMplObj()->GetParamsType();
  MethodMeta *mplFieldOrMethodMeta = GetMethodMeta();
  uint32_t arraySize = paramStruct.paramNum + 1;
  if (paramStruct.paramNum == 1 && !paramStruct.isStaticMethod) {
    jvalue thisObj;
    thisObj.l = *args.GetObject();
    // NPE can not be thrown without this check when isExactMatched is true
    if (paramStruct.isExactMatched == true && thisObj.l == nullptr) {
      MRT_ThrowNewException("java/lang/NullPointerException", "");
    }
    MethodMeta *realMethod = nullptr;
    ConvertJStruct convertJStruct = { paramStruct.paramTypes, arraySize, &thisObj };
    if (!paramStruct.isExactMatched && !handleTypes.empty() &&
        !ConvertJvalue(convertJStruct, *this, paramStruct.paramTypes[0], handleTypes[0])) {
      MRT_CheckThrowPendingExceptionUnw();
    }
    realMethod = reinterpret_cast<MethodMeta*>(GetRealMethod(reinterpret_cast<MObject*>(thisObj.l)));
    result = InvokeMethodNoParameter(reinterpret_cast<MObject*>(thisObj.l), *realMethod);
    return true;
  } else if (paramStruct.paramNum == 0 && paramStruct.isStaticMethod) {
    result = InvokeMethodNoParameter(nullptr, *mplFieldOrMethodMeta);
    return true;
  }
  return false;
}

void EMSFWriter::WriteByMark(char mark, int8_t *cArray, uint32_t &byteIdx, const jvalue &val, bool isDec) {
  switch (mark) { // full through
    case 'Z':
    case 'B':
    case 'C':
    case 'S':
    case 'I':
      WriteToEmStackFrame(cArray, byteIdx, val.i);
      break;
    case 'F':
      WriteToEmStackFrame(cArray, byteIdx, val.f);
      break;
    case 'J':
      WriteToEmStackFrame(cArray, byteIdx, val.j);
      break;
    case 'D':
      WriteToEmStackFrame(cArray, byteIdx, val.d);
      break;
    default:
      if (isDec) {
        RC_LOCAL_DEC_REF(val.l);
      }
      LOG(ERROR) << "WriteByMark() don't match mark: " << mark;
  }
}

void DoCalculate(char name, uint32_t &byteArrSize, uint32_t &refNum) {
  switch (name) { // full through
    case 'I':
    case 'B':
    case 'F':
    case 'C':
    case 'S':
    case 'Z':
    case 'V':
      byteArrSize += sizeof(int);
      break;
    case 'J':
    case 'D':
      byteArrSize += sizeof(double);
      break;
    default:
      ++refNum;
      break;
  }
}

jvalue MethodHandle::InvokeMethodNoParameter(MObject *obj, MethodMeta &mthd) {
  if (mthd.NeedsInterp()) {
    if ((obj == nullptr) && (!mthd.IsStatic())) {
      LOG(ERROR) << "obj can not be nullptr!" << maple::endl;
      if (MRT_HasPendingException()) {
        MRT_CheckThrowPendingExceptionUnw();
      }
      MRT_ThrowNullPointerExceptionUnw();
    }
    return interpreter::InterpJavaMethod<calljavastubconst::ArgsType::kNone>(
        &mthd, obj, reinterpret_cast<uintptr_t>(nullptr));
  } else {
    MClass *methodReturnType = mthd.GetReturnType();
    if (methodReturnType == WellKnown::GetMClassD() || methodReturnType == WellKnown::GetMClassF()) {
      jvalue v;
      v.d = RuntimeStub<jdouble>::FastCallCompiledMethod(mthd.GetFuncAddress(), obj);
      return v;
    } else {
      return RuntimeStub<jvalue>::FastCallCompiledMethodJ(mthd.GetFuncAddress(), obj);
    }
  }
}

bool MethodHandle::FillEmStackFrameArray(const char *typesMark, MClass **parameterTypes,
                                         int8_t *cArray, const MArray *refArray) {
  uint32_t refIdx = 0;
  const vector<MClass*> &ptypesVal = methodTypeMplObjCache->GetParamsType();
  uint32_t returnTypeIdx = realParamNum;
  for (uint32_t i = 0; i < returnTypeIdx; ++i) {
    MClass *from = parameterTypes[i];
    jvalue val;
    val.j = 0;
    GetValFromVargs(val, *from->GetName());
    if (handleKind == kCallSiteTransformCall) { // write according by from type
      if (typesMark[i] != 'L') {
        EMSFWriter::WriteByMark(*parameterTypes[i]->GetName(), cArray, emByteIdx, val);
      } else {
        refArray->SetObjectElement(refIdx++, reinterpret_cast<MObject*>(val.l));
      }
      continue;
    }

    MClass *to = ptypesVal[i];
    ConvertJStruct convertJStruct = { parameterTypes, returnTypeIdx + 1, &val };
    if (from != to) {
      STATEMENTCHECK(!ConvertJvalue(convertJStruct, *this, from, to))
    }

    if (MRT_ReflectClassIsPrimitive(*to)) {
      EMSFWriter::WriteByMark(*to->GetName(), cArray, emByteIdx, val);
    } else {
      refArray->SetObjectElement(refIdx++, reinterpret_cast<MObject*>(val.l));
    }

    if (typesMark[i] != 'L' && !MRT_ReflectClassIsPrimitive(*to)) {
      RC_LOCAL_DEC_REF(val.l);
    }
  }
  return true;
}

jvalue EMSFReader::GetRetValFromEmStackFrame(const MArray *refArray, uint32_t refNum, uint32_t idx,
                                             const int8_t *cArray, char mark) {
  jvalue retVal;
  retVal.j = 0UL;
  switch (mark) {
    case 'L':
    case '[':
      retVal.l = reinterpret_cast<jobject>(refArray->GetObjectElement(refNum - 1));
      break;
    case 'J':
      retVal.j = GetLongFromEmStackFrame(cArray, idx);
      break;
    case 'D': {
      DoubleLongConvert u;
      u.j = GetLongFromEmStackFrame(cArray, idx);
      retVal.d = u.d;
      break;
    }
    case 'F': {
      FloatIntConvert u;
      u.i = GetIntFromEmStackFrame(cArray, idx);
      retVal.f = u.f;
      break;
    }
    default: { // I B C S Z
      retVal.i = GetIntFromEmStackFrame(cArray, idx);
      break;
    }
  }
  return retVal;
}

void MethodHandle::InvokeTransformVirtualMethod(MObject *emStFrameObj, jvalue &retVal) const {
  MethodMeta *transformMethod = GetHandleJClassType()->GetMethod("transform", "(Ldalvik/system/EmulatedStackFrame;)V");
  CHECK_E_V(transformMethod == nullptr, "Class.GetMethod return nullptr");
  ArgValue xregValue(0);
  xregValue.AddReference(const_cast<MObject*>(GetHandleJavaObj()));
  xregValue.AddReference(emStFrameObj);
  if (transformMethod->NeedsInterp()) {
    jvalue temp;
    temp.l = reinterpret_cast<jobject>(emStFrameObj);
    retVal = interpreter::InterpJavaMethod<calljavastubconst::kJvalue>(transformMethod,
        const_cast<MObject*>(GetHandleJavaObj()), reinterpret_cast<uintptr_t>(&temp));
  } else {
    RuntimeStub<void>::SlowCallCompiledMethod(transformMethod->GetFuncAddress(), xregValue.GetData(), 0, 0);
  }
}

// transform invoke, marshal EmStackFrame and invoke transform
bool MethodHandle::InvokeTransform(const SizeInfo &sz, const char *typesMark, // 120
                                   MClass **parameterTypes, jvalue &retVal) {
  if (handleKind != kCallSiteTransformCall && sz.arraySize - 1 != methodTypeMplObjCache->GetParamsType().size()) {
    if (!MRT_HasPendingException()) {
      string exceptionStr = GeneExceptionString(*this, parameterTypes, sz.arraySize);
      MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
    }
    return false;
  }
  ScopedHandles sHandles;
  ObjHandle<MArray> byteArray(
      MRT_NewPrimitiveArray(static_cast<int>(sz.byteArrSize), maple::Primitive::kByte, sizeof(int8_t)));
  CHECK_E_B(byteArray() == 0, "MRT_NewPrimitiveArray return nullptr");
  ObjHandle<MArray> referenceArray(
      MRT_NewObjArray(static_cast<int>(sz.refNum), *WellKnown::GetMClassObject(), nullptr));
  CHECK_E_B(referenceArray() == 0, "MRT_NewObjArray return nullptr");
  int8_t *cArray = reinterpret_cast<int8_t*>(byteArray.AsArray()->ConvertToCArray());
  realParamNum = sz.arraySize - 1;
  if (!FillEmStackFrameArray(typesMark, parameterTypes, cArray, referenceArray.AsArray())) {
    return false;
  }
  ObjHandle<MObject> emStFrameObj(
      CreateEmStackFrame(byteArray.AsArray(), referenceArray.AsObject(), parameterTypes, sz.arraySize));
  CHECK_E_B(emStFrameObj() == 0, "CreateEmStackFrame return nullptr");
  InvokeTransformVirtualMethod(emStFrameObj.AsObject(), retVal);
  if (UNLIKELY(MRT_HasPendingException())) {
    return false;
  }
  MClass *returnType = parameterTypes[sz.arraySize - 1];
  if (MRT_ReflectClassIsPrimitive(*returnType)) {
    emByteIdx = (returnType == WellKnown::GetMClassD() || returnType == WellKnown::GetMClassJ()) ?
        sz.byteArrSize - sizeof(jlong) : sz.byteArrSize - sizeof(int);
  }
  retVal = EMSFReader::GetRetValFromEmStackFrame(
      referenceArray.AsArray(), sz.refNum, emByteIdx, cArray, GetReturnTypeMark(typesMark[sz.arraySize - 1]));
  if (handleKind != kCallSiteTransformCall) {
    return ConvertReturnValue(*this, parameterTypes, sz.arraySize, retVal);
  } else {
    return !MRT_HasPendingException();
  }
}

bool MethodHandle::ParamsConvert(const MClass *from, const MClass *to, MClass **parameterTypes,
                                 uint32_t arraySize, jvalue &internVal) const {
  ConvertJStruct convertJStruct = { parameterTypes, arraySize, &internVal };
  STATEMENTCHECK(to != from && !ConvertJvalue(convertJStruct, *this, from, to))
  return true;
}

static void GetReturnValue(MObject *emStFrameObj, jvalue &result) {
    EmStackFrame emStackFrameMplObj = EmStackFrame(emStFrameObj);
    MObject *callsiteObj = emStFrameObj->LoadObjectNoRc(WellKnown::GetMFieldEmStackFrameCallsiteOffset());
    MClass *retFromType =
       reinterpret_cast<MClass*>(callsiteObj->LoadObjectNoRc(WellKnown::GetMFieldMethodHandlepRTypeOffset()));
    int8_t *cArray = emStackFrameMplObj.GetStackFrameNativeBytes();
    uint32_t cArrayLen = emStackFrameMplObj.GetStackFrameNativeBytesCount();

    int8_t constexpr doubleOrLongLength = 8;
    int8_t constexpr intOrFloatLength = 4;
    char mark = *(retFromType->GetName());
    uint32_t idx;
    switch (mark) {
      case 'L':
      case '[': {
        result.l = reinterpret_cast<jobject>(emStackFrameMplObj.GetReferencesReturnObj());
        break;
      }
      case 'D': {
        DoubleLongConvert u;
        idx = cArrayLen - doubleOrLongLength;
        u.j = EMSFReader::GetLongFromEmStackFrame(cArray, idx);
        result.d = u.d;
        break;
      }
      case 'F': {
        FloatIntConvert u;
        idx = cArrayLen - intOrFloatLength;
        u.i = EMSFReader::GetIntFromEmStackFrame(cArray, idx);
        result.f = u.f;
        break;
      }
      case 'J': {
        idx = cArrayLen - doubleOrLongLength;
        result.j = EMSFReader::GetLongFromEmStackFrame(cArray, idx);
        break;
      }
      case 'V': {
        break;
      }
      default: {
        idx = cArrayLen - intOrFloatLength;
        result.i = EMSFReader::GetIntFromEmStackFrame(cArray, idx);
        break;
      }
    }
}

static bool RecursiveInvokePoly(MObject *emStFrameObj, const MethodHandle &methodHandleMplObj, jvalue &result) {
  Kind handleKind = methodHandleMplObj.GetHandleKind();
  if (handleKind == kTransformCall || handleKind == kCallSiteTransformCall) {
    ArgValue xregValue(0);
    xregValue.AddReference(const_cast<MObject*>(methodHandleMplObj.GetHandleJavaObj()));
    xregValue.AddReference(emStFrameObj);
    MethodMeta *transformMethod = methodHandleMplObj.GetHandleJClassType()->GetMethod(
        "transform", "(Ldalvik/system/EmulatedStackFrame;)V");
    __MRT_ASSERT(transformMethod != nullptr, "RecursiveInvokePoly: transformMethod is null");
    if (transformMethod->NeedsInterp()) {
      result = interpreter::InterpJavaMethod<calljavastubconst::kJvalue>(transformMethod,
          reinterpret_cast<MObject*>(const_cast<MObject*>(methodHandleMplObj.GetHandleJavaObj())),
          reinterpret_cast<uintptr_t>(emStFrameObj));
    } else {
      RuntimeStub<void>::SlowCallCompiledMethod(transformMethod->GetFuncAddress(), xregValue.GetData(), 0, 0);
    }
    MRT_CheckThrowPendingExceptionUnw();
    GetReturnValue(emStFrameObj, result);
    return true;
  }
  return false;
}

bool EmStackFrameInvoker::FillParamsForEmStackFrame(BaseArgValue &paramArray) {
  int8_t *cArray = emStackFrameMplObj->GetStackFrameNativeBytes(); // object might moving in ParamsConvert
  for (uint32_t i = 0; i < paramLength; ++i) {
    MClass *from = typesArr[i];
    MClass *to = (*ptypesOfHandle)[i];
    switch (*to->GetName()) {
      case 'I':
      case 'B':
      case 'C':
      case 'S':
      case 'Z': {
        paramArray.AddInt32(EMSFReader::GetIntFromEmStackFrame(cArray, byteIdx));
        break;
      }
      case 'D': {
        DoubleLongConvert u;
        u.j = EMSFReader::GetLongFromEmStackFrame(cArray, byteIdx);
        paramArray.AddDouble(u.d);
        break;
      }
      case 'F': {
        FloatIntConvert u;
        u.i = EMSFReader::GetIntFromEmStackFrame(cArray, byteIdx);
        paramArray.AddFloat(u.f);
        break;
      }
      case 'J': {
        paramArray.AddInt64(EMSFReader::GetLongFromEmStackFrame(cArray, byteIdx));
        break;
      }
      default: {
        jvalue value;
        value.l = reinterpret_cast<jobject>(emStackFrameMplObj->GetReferencesObj(refIdx++));
        STATEMENTCHECK(!methodHandleMplObj.ParamsConvert(from, to, typesArr, paramLength, value));
        paramArray.AddReference(reinterpret_cast<MObject*>(value.l));
        break;
      }
    }
  }
  return true;
}

bool EmStackFrameInvoker::InvokeInterpMethod(jvalue &result, MethodMeta &realMethod) {
  ArgValueInterp paramArray(0);
  STATEMENTCHECK(!FillParamsForEmStackFrame(paramArray));
  if (MRT_HasPendingException()) {
    return false;
  }
  if (realMethod.IsStatic()) {
    result = maplert::interpreter::InterpJavaMethod<calljavastubconst::kJvalue>(&realMethod,
        nullptr, reinterpret_cast<uintptr_t>(paramArray.GetData()));
  } else {
    result = maplert::interpreter::InterpJavaMethod<calljavastubconst::kJvalue>(&realMethod,
        paramArray.GetReceiver(), reinterpret_cast<uintptr_t>(paramArray.GetData() + 1));
  }
  return true;
}

bool EmStackFrameInvoker::InvokeStaticCmpileMethod(jvalue &result, const MethodMeta &realMethod) {
  ArgValue paramArray(0);
  STATEMENTCHECK(!FillParamsForEmStackFrame(paramArray));
  if (MRT_HasPendingException()) {
    return false;
  }
  DoInvoke(realMethod, result, paramArray);
  return true;
}

bool EmStackFrameInvoker::Invoke(jvalue &result) {
  if (RecursiveInvokePoly(emStFrameObj, methodHandleMplObj, result)) {
    return true;
  }
  // check pramater length
  if (paramLength != ptypesOfHandle->size()) {
    string exceptionStr = GeneExceptionString(methodHandleMplObj, typesArr, paramLength);
    MRT_ThrowNewException("java/lang/invoke/WrongMethodTypeException", exceptionStr.c_str());
    return false;
  }  // check end
  MObject *param0 = emStackFrameMplObj->GetReferencesObj(0);
  if (!mplFieldOrMethodMeta->IsStatic() && param0 == nullptr) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "null receiver");
  }
  MethodMeta *realMethod = methodHandleMplObj.GetRealMethod(param0);
  STATEMENTCHECK(UNLIKELY(MRT_HasPendingException()))
  STATEMENTCHECK(!ClinitCheck(realMethod->GetDeclaringClass()))
  // interpreter path
  if (realMethod->NeedsInterp()) {
    // Interp
    STATEMENTCHECK(!InvokeInterpMethod(result, *realMethod));
  } else {
    // O2
    STATEMENTCHECK(!InvokeStaticCmpileMethod(result, *realMethod));
  }
  STATEMENTCHECK(UNLIKELY(MRT_HasPendingException()))
  MClass *retType = mplFieldOrMethodMeta->GetReturnType();
  if (retType == WellKnown::GetMClassV()) {
    return true;
  }
  MObject *callsiteObj = emStFrameObj->LoadObjectNoRc(WellKnown::GetMFieldEmStackFrameCallsiteOffset());
  MClass *retFromType =
      reinterpret_cast<MClass*>(callsiteObj->LoadObjectNoRc(WellKnown::GetMFieldMethodHandlepRTypeOffset()));
  STATEMENTCHECK(retFromType != WellKnown::GetMClassV() && !ConvertReturnValue(methodHandleMplObj,
      typesArr, paramLength + 1, result))

  // set return value to EMStackFrame
  if (!MRT_ReflectClassIsPrimitive(*retFromType)) {
    emStackFrameMplObj->PutReferencesObj(reinterpret_cast<MObject*>(result.l), refIdx);
    RC_LOCAL_DEC_REF(result.l);
  } else {
    char typeMark = *(retFromType->GetName());
    int8_t *cArray = emStackFrameMplObj->GetStackFrameNativeBytes();
    EMSFWriter::WriteByMark(typeMark, cArray, byteIdx, result, !mplFieldOrMethodMeta->IsConstructor());
  }
  return true;
}

bool MethodHandle::InvokeWithEmStackFrame(MObject *emStFrameObj, jvalue &result) {
  CHECK_E_B(emStFrameObj == nullptr, "InvokeWithEmStackFrame : emStFrameObj is nullptr");
  EmStackFrameInvoker invoker(emStFrameObj, *this);
  return invoker.Invoke(result);
}

MObject *MethodHandle::GetMemberInternal(const MObject *methodHandle) {
  DCHECK(methodHandle != nullptr);
  Kind tkind = static_cast<Kind>(methodHandle->Load<int>(WellKnown::GetMFieldMethodHandleHandleKindOffset(), false));
  long fieldOrMethodMeta = methodHandle->Load<long>(WellKnown::GetMFieldMethodHandleArtFieldOrMethodOffset(), false);
  if (tkind >= Kind::kInstanceGet) {
    FieldMeta *fieldMeta = reinterpret_cast<FieldMeta*>(fieldOrMethodMeta);
    MObject *mField = MField::NewMFieldObject(*fieldMeta);
    return mField;
  } else {
    MethodMeta *methodMeta = reinterpret_cast<MethodMeta*>(fieldOrMethodMeta);
    MObject *mMethod = MMethod::NewMMethodObject(*methodMeta);
    return mMethod;
  }
}

extern "C" jobject MRT_MethodHandleImplGetMemberInternal(const jobject methodHandle) {
  MObject *internal = MethodHandle::GetMemberInternal(MObject::JniCast(methodHandle));
  DCHECK(internal != nullptr);
  return internal->AsJobject();
}
}  // namespace maple
