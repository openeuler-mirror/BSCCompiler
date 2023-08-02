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
#include "mrt_reflection_proxy.h"
#include <algorithm>
#include <securec.h>
#include "itab_util.h"
#include "mclass_inline.h"
#include "mstring_inline.h"
#include "mmethod_inline.h"
#include "methodmeta_inline.h"
#include "exception/mrt_exception.h"
#include "mrt_reflection_stubfuncforproxy.def"
#ifdef USE_32BIT_REF
using VOID_PTR = uint32_t;
#else
using VOID_PTR = uint64_t;
#endif

using namespace maple;
namespace maplert {
MethodMeta *GetProxySuperMethod(const MObject &obj, uint32_t num);
jvalue ProcessProxyMethodInvoke(MObject &obj, const MethodMeta &proxySuperMethod, jvalue args[]);
MObject *InvokeInvocationHandler(MObject &obj, jvalue args[], const MethodMeta &proxySuperMethod);
void DoProxyThrowableException(const MObject &proxyObj, const MObject &exceptionObj, const MethodMeta &method);
extern "C"
int64_t EnterProxyMethodInvoke(intptr_t *stack, uint32_t num) {
  // this proxy object always put index 0
  CHECK(stack != nullptr);
  MObject *obj = reinterpret_cast<MObject*>(stack[0]);
  CHECK(obj != nullptr);
  MethodMeta *proxySuperMethod = GetProxySuperMethod(*obj, num);
  CHECK(proxySuperMethod != nullptr) << "proxySuperMethod return nullptr." << maple::endl;
  jvalue ret;
  ret.j = 0L;
  {
    DecodeStackArgs stackArgs(stack);
    std::string prefix("L");
    proxySuperMethod->BuildJValuesArgsFromStackMemeryPrefixSigNature(stackArgs, prefix);
    ret = ProcessProxyMethodInvoke(*obj, *proxySuperMethod, &(stackArgs.GetData()[prefix.length()]));
  }
  if (MRT_HasPendingException()) {
    MRT_CheckThrowPendingExceptionUnw();
    return ret.j;
  }
  return ret.j;
}

jvalue ProcessProxyMethodInvoke(MObject &obj, const MethodMeta &proxySuperMethod, jvalue args[]) {
  ScopedHandles sHandles;
  jvalue retValue;
  retValue.l = 0UL;
  MObject *value = InvokeInvocationHandler(obj, args, proxySuperMethod);
  ObjHandle<MObject> valueRef(value);
  if (UNLIKELY(MRT_HasPendingException())) {
    ObjHandle<MObject> th(MRT_PendingException());
    DoProxyThrowableException(obj, *(th()), proxySuperMethod);
    return retValue;
  }
  char retPrimitiveType = proxySuperMethod.GetReturnPrimitiveType();
  switch (retPrimitiveType) {
    case 'V':
      return retValue;
    case 'L':
    case '[': {
      MClass *retType = proxySuperMethod.GetReturnType();
      if (retType == nullptr) {
        return retValue;
      }
      if (valueRef() != 0 && !valueRef->IsInstanceOf(*retType)) {
        MRT_ThrowNewException("java/lang/ClassCastException", nullptr);
        return retValue;
      }
      retValue.l = valueRef.ReturnJObj();
      return retValue;
    }
    default: { // primitive value
      if (UNLIKELY(valueRef() == 0)) {
        MRT_ThrowNewException("java/lang/NullPointerException", nullptr);
        return retValue;
      }
      if (!primitiveutil::IsBoxObject(*value, retPrimitiveType)) {
        MRT_ThrowNewException("java/lang/ClassCastException", nullptr);
        return retValue;
      }
      bool success = primitiveutil::UnBoxPrimitive(*value, retValue);
      if (!success) {
        MRT_ThrowNewException("java/lang/ClassCastException", nullptr);
        return retValue;
      }
      return retValue;
    }
  }
}

static void ThrowUndeclaredThrowableException(const MObject &exceptionObj) {
  MRT_ClearPendingException();
  MClass *exceptionClass = WellKnown::GetMClassUndeclaredThrowableException();
  MethodMeta *exceptionConstruct = exceptionClass->GetDeclaredConstructor("(Ljava/lang/Throwable;)V");
  DCHECK(exceptionConstruct != nullptr) << "exception Construct nullptr." << maple::endl;
  MObject *exceptionInstance = MObject::NewObject(*exceptionClass, exceptionConstruct, &exceptionObj);
  if (UNLIKELY(exceptionInstance == nullptr)) {
    return;
  }
  MRT_ThrowExceptionSafe(exceptionInstance->AsJobject());
  RC_LOCAL_DEC_REF(exceptionInstance);
  return;
}

void DoProxyThrowableException(const MObject &proxyObj, const MObject &exceptionObj, const MethodMeta &method) {
  if (exceptionObj.IsInstanceOf(*WellKnown::GetMClassError()) ||
      exceptionObj.IsInstanceOf(*WellKnown::GetMClassRuntimeException())) {
    return;
  }
  ScopedHandles sHandles;
  MClass *proxyCls = proxyObj.GetClass();
  char *methodName = method.GetName();
  char *methodSig = method.GetSignature();
  MClass **superClassArray = proxyCls->GetSuperClassArray();
  DCHECK(superClassArray != nullptr) << "Proxy Cls super class array nullptr." << maple::endl;
  uint32_t numofsuper = proxyCls->GetNumOfSuperClasses();
  uint32_t assignableNum = 0;
  uint32_t duplicatedMethodNum = 0;
  for (uint32_t i = 1; i < numofsuper; ++i) {
    MClass *interfaceclass = MClass::ResolveSuperClass(&superClassArray[i]);
    MethodMeta *methodMeta = interfaceclass->GetDeclaredMethod(methodName, methodSig);
    if (methodMeta != nullptr) {
      ++duplicatedMethodNum;
      std::vector<MClass*> types;
      methodMeta->GetExceptionTypes(types);
      bool isCurrentAssignable = false;
      size_t numOfException = types.size();
      for (size_t j = 0; j < numOfException; ++j) {
        MClass *decExCls = types[j];
        if (exceptionObj.IsInstanceOf(*decExCls)) {
          isCurrentAssignable = true;
          break;
        }
      }
      if (isCurrentAssignable == true) {
        ++assignableNum;
      }
    }
  }
  if (assignableNum != duplicatedMethodNum) {
    ThrowUndeclaredThrowableException(exceptionObj);
  }
  return;
}

static MethodMeta *GetHandlerInvokeMethod(const MClass &handlerClass) {
  MethodMeta *invokeMethod = handlerClass.GetMethod("invoke",
      "(Ljava/lang/Object;Ljava/lang/reflect/Method;[Ljava/lang/Object;)Ljava/lang/Object;");
  if (UNLIKELY(invokeMethod == nullptr || (invokeMethod->GetFuncAddress() == 0))) {
    std::string msg = "No Method: jobject invoke(Object, Method, Object[]) in class ";
    msg += handlerClass.GetName();
    msg += " or super class";
    MRT_ThrowNewException("java/lang/NoSuchMethodError", msg.c_str());
    return nullptr;
  }
  return invokeMethod;
}

static MethodMeta *GetSuperMethod(const MethodMeta &srcMethod, const MClass &proxyClass) {
  MethodMeta *superMethod = nullptr;
  char *methodName = srcMethod.GetName();
  char *methodSig = srcMethod.GetSignature();
  MClass *objectClass = WellKnown::GetMClassObject();
  if (!strcmp("equals", methodName) && !strcmp("(Ljava/lang/Object;)Z", methodSig)) {
    superMethod = objectClass->GetDeclaredMethod(methodName, methodSig);
  } else if (!strcmp("hashCode", methodName) && !strcmp("()I", methodSig)) {
    superMethod = objectClass->GetDeclaredMethod(methodName, methodSig);
  } else if (!strcmp("toString", methodName) && !strcmp("()Ljava/lang/String;", methodSig)) {
    superMethod = objectClass->GetDeclaredMethod(methodName, methodSig);
  } else {
    MClass **superClassArray = proxyClass.GetSuperClassArray();
    CHECK(superClassArray != nullptr) << "Super Class Array is nullptr in proxy class." << maple::endl;
    uint32_t numofsuper = proxyClass.GetNumOfSuperClasses();
    for (uint32_t i = 1; i < numofsuper; ++i) {
      MClass *interfaceclass = MClass::ResolveSuperClass(&superClassArray[i]);
      MethodMeta *method = interfaceclass->GetMethod(methodName, methodSig);
      if (method != nullptr) {
        superMethod = method;
        break;
      }
    }
  }
  return superMethod;
}

MethodMeta *GetProxySuperMethod(const MObject &obj, uint32_t num) {
  MClass *proxyClass = obj.GetClass();
  MethodMeta *proxyMethod = &(proxyClass->GetMethodMetas()[num]);
  MethodMeta *superMethod = GetSuperMethod(*proxyMethod, *proxyClass);
  return superMethod;
}

MObject *InvokeInvocationHandler(MObject &obj, jvalue args[], const MethodMeta &proxySuperMethod) {
  ScopedHandles sHandles;
  FieldMeta *hfield = WellKnown::GetMClassProxy()->GetDeclaredField("h");
  CHECK(hfield != nullptr) << "Can't find h field in proxy class." << maple::endl;
  ObjHandle<MObject> hobj(hfield->GetObjectValue(&obj));
  CHECK(hobj() != 0) << "proxy's h field is nullptr." << maple::endl;
  MethodMeta *invokeMethod = GetHandlerInvokeMethod(*(hobj()->GetClass()));
  if (invokeMethod == nullptr) {
    return nullptr;
  }

  uint32_t parameterSize = proxySuperMethod.GetParameterCount();
  MArray *arrayP = nullptr;
  if (parameterSize > 0) {
    arrayP = MArray::NewObjectArray(parameterSize, *WellKnown::GetMClassAObject());
    CHECK(arrayP != nullptr) << "NewObjectArray return nullptr." << maple::endl;
    proxySuperMethod.BuildMArrayArgsFromJValues(*arrayP, args);
  }
  ObjHandle<MArray> arrayPRef(arrayP);
  ObjHandle<MObject> interfaceMethodObj(MMethod::NewMMethodObject(proxySuperMethod));
  jvalue methodArgs[3]; // invokeMethod has 3 args
  methodArgs[0].l = reinterpret_cast<jobject>(&obj);
  methodArgs[1].l = interfaceMethodObj.AsJObj();
  methodArgs[2].l = arrayPRef.AsJObj();
  jobject retvalue = invokeMethod->Invoke<jobject, calljavastubconst::ArgsType::kJvalue>(hobj(), methodArgs);
  return reinterpret_cast<MObject*>(retvalue);
}

static void SortMethodFromProxyClassbyName(MClass &proxyCls, std::vector<MethodMeta*> &objectMethod) {
  uint32_t numOfMethods = proxyCls.GetNumOfMethods();
  MethodMeta *objectMethods = proxyCls.GetMethodMetas();
  DCHECK(objectMethods != nullptr) << "GetMethodMetas() fail in SortMethodFromProxyClassbyName." << maple::endl;
  for (uint32_t i = 0; i < numOfMethods; ++i) {
    objectMethod.push_back(&objectMethods[i]);
  }

  std::sort(objectMethod.begin(), objectMethod.end(), [](MethodMeta *a, MethodMeta *b) {
    char *nameA = a->GetName();
    std::string sa(nameA);
    char *nameB = b->GetName();
    std::string sb(nameB);
    return sa < sb;
  });
}

static void GenProxyVtab(MClass &proxyCls, MArray &methods) {
  const uint32_t numVMethodsObjectVtab = 11; // 11 is number of methods in objectVtab
  MClass *objectMClass = WellKnown::GetMClassObject();
  const static int16_t equalsVtabSlot = objectMClass->GetMethod("equals", "(Ljava/lang/Object;)Z")->GetVtabIndex();
  const static int16_t hashCodeVtabSlot = objectMClass->GetMethod("hashCode", "()I")->GetVtabIndex();
  const static int16_t toStringVtabSlot = objectMClass->GetMethod("toString", "()Ljava/lang/String;")->GetVtabIndex();
  uint32_t numVirtualMethods = methods.GetLength();
  auto vtab  = reinterpret_cast<VOID_PTR*>(calloc(numVMethodsObjectVtab + numVirtualMethods, sizeof(VOID_PTR)));
  CHECK(vtab != nullptr) << "calloc fail in GenProxyVtab." << maple::endl;
  if (vtab == nullptr) {
    return;
  }

  uint32_t vtabCnt = 0;
  auto objectVtab = reinterpret_cast<VOID_PTR*>(objectMClass->GetVtab());
  DCHECK(objectVtab != nullptr) << "object Vtab is nullptr." << maple::endl;
  for (uint32_t i = 0; i < numVMethodsObjectVtab; ++i) {
    vtab[i] = objectVtab[i];
  }
  vtabCnt += numVMethodsObjectVtab;

  std::vector<MethodMeta*> proxyMethods;
  SortMethodFromProxyClassbyName(proxyCls, proxyMethods);
  for (auto it = proxyMethods.begin(); it != proxyMethods.end(); ++it) {
    MethodMeta *method = *it;
    char *methodName = method->GetName();
    char *sigName = method->GetSignature();
    uintptr_t address = method->GetFuncAddress();
    if (!strcmp("equals", methodName) && !strcmp("(Ljava/lang/Object;)Z", sigName)) {
      vtab[equalsVtabSlot] = static_cast<VOID_PTR>(address);
      continue;
    } else if (!strcmp("hashCode", methodName) && !strcmp("()I", sigName)) {
      vtab[hashCodeVtabSlot] = static_cast<VOID_PTR>(address);
      continue;
    } else if (!strcmp("toString", methodName) && !strcmp("()Ljava/lang/String;", sigName)) {
      vtab[toStringVtabSlot] = static_cast<VOID_PTR>(address);
      continue;
    }
    vtab[vtabCnt++] = static_cast<VOID_PTR>(address);
  }
  proxyCls.SetVtable(reinterpret_cast<uintptr_t>(vtab));
}

static void SortMethodWithHashCode(std::vector<MethodMeta*> &methodVec) {
  std::sort(methodVec.begin(), methodVec.end(), [](const MethodMeta *a, const MethodMeta *b) {
    uint16_t hashCodeA = a->GetHashCode();
    uint32_t hashCodeB = b->GetHashCode();
    if (a->IsFinalizeMethod() && !b->IsFinalizeMethod()) {
      return false;
    }
    if (!a->IsFinalizeMethod() && b->IsFinalizeMethod()) {
      return true;
    }
    return hashCodeA < hashCodeB;
  });
}

static void GenProxyConstructor(MClass &proxyCls, MethodMeta &proxyClsMth, MethodMeta &constructor) {
  char *mthMethodName = constructor.GetName();
  MethodSignature *mthSig = constructor.GetMethodSignature();
  uint32_t modifier = (constructor.GetMod() & (~modifier::kModifierProtected)) | modifier::kModifierPublic;
  proxyClsMth.FillMethodMeta(false, mthMethodName, mthSig, constructor.GetAnnotationRaw(),
                             constructor.GetVtabIndex(), proxyCls,
                             reinterpret_cast<uintptr_t>(constructor.GetFuncAddress()),
                             modifier, constructor.GetFlag(), constructor.GetArgSize());
}

static uintptr_t FillItab(const std::map<unsigned long, std::pair<unsigned long, std::string>> &itabMap,
                          const std::vector<std::pair<unsigned long, std::string>> &itabConflictVector,
                          const std::map<unsigned long, std::pair<unsigned long, std::string>> &itabSecondMap,
                          const std::vector<std::pair<unsigned long, std::string>> &itabSecondConflictVector) {
  size_t size1 = itabConflictVector.size();
  size_t itabSecondMapSize = itabSecondMap.size();
  size_t itabSecondConflictSize = itabSecondConflictVector.size();
  unsigned long itabSize = 0;
  if (size1 == 0) {
    itabSize = kItabFirstHashSize;
  } else {
    // 1 is first tab, 2 is second tab, * 2 for hash value and methods adress in second tab,
    // itabSecondConflict * 2 name and signature, function addr
    itabSize = kItabFirstHashSize + 1 + 2 + itabSecondMapSize * 2 + itabSecondConflictSize * 2;
  }
  VOID_PTR *itab = reinterpret_cast<VOID_PTR*>(MRT_AllocFromMeta(itabSize * sizeof(VOID_PTR), kITabMetaData));
  for (auto item : itabMap) {
    if (item.second.first == 1) {
      continue;
    }
    *(itab + item.first) = item.second.first;
  }
  if (size1 != 0) {
    unsigned long tmp = reinterpret_cast<unsigned long>(itab + kItabFirstHashSize + 1);
    *(itab + kItabFirstHashSize) = tmp;
  }
  uint32_t index = 0;
  if (size1 != 0) {
    index = kItabFirstHashSize + 1;
#ifdef USE_32BIT_REF
    uint64_t shiftCountBit = 4 * 4;
#else
    uint64_t shiftCountBit = 8 * 4;
#endif
    itab[index++] = ((itabSecondConflictSize | (1UL << (shiftCountBit - 1))) << shiftCountBit) + itabSecondMapSize;
    itab[index++] = 1;
    for (auto item : itabSecondMap) {
      itab[index++] = item.first;
      itab[index++] = item.second.first;
    }
    for (auto item : itabSecondConflictVector) {
      std::string methodName = item.second;
      size_t len = methodName.length() + 1;
      char *name = reinterpret_cast<char*>(MRT_AllocFromMeta(len, kNativeStringData));
      if (strcpy_s(name, len, methodName.c_str()) != EOK) {
        LOG(FATAL) << "FillItab strcpy_s() not return 0" << maple::endl;
        return 0;
      }
      itab[index++] = reinterpret_cast<uintptr_t>(name);
      itab[index++] = item.first;
    }
  }
  return reinterpret_cast<uintptr_t>(itab);
}

static uintptr_t GenProxyItab(const std::map<unsigned long, std::pair<unsigned long, std::string>> itabMap,
                              std::vector<std::pair<unsigned long, std::string>> itabConflictVector) {
  size_t size1 = itabConflictVector.size();
  std::map<unsigned long, std::pair<unsigned long, std::string>> itabSecondMap;
  std::vector<std::pair<unsigned long, std::string>> itabSecondConflictVector;
  if (size1 != 0) {
    for (auto item :itabConflictVector) {
      std::string methodName = item.second;
      unsigned long hashIdex = GetSecondHashIndex(methodName.c_str());
      if (itabSecondMap.find(hashIdex) == itabSecondMap.end()) {
        std::pair<unsigned long, std::pair<unsigned long, std::string> > hashPair(hashIdex, item);
        itabSecondMap.insert(hashPair);
      } else {
        if (itabSecondMap[hashIdex].first == 1) {
          itabSecondConflictVector.push_back(item);
        } else {
          itabSecondConflictVector.push_back(item);
          auto oldItem = itabSecondMap[hashIdex];
          itabSecondConflictVector.push_back(oldItem);
          itabSecondMap[hashIdex].first = 1;
        }
      }
    }
  }
  return FillItab(itabMap, itabConflictVector, itabSecondMap, itabSecondConflictVector);
}

static void ResolveConstructHashConflict(MethodMeta &constructor, std::vector<MethodMeta*> &methodVec,
                                         std::vector<MethodMeta*> &methodHashConflict) {
  bool isConflict = false;
  for (auto it = methodVec.begin(); it != methodVec.end(); ++it) {
    MethodMeta *methodMeta = *it;
    if ((methodMeta != &constructor) && ((methodMeta->GetHashCode() == constructor.GetHashCode()) ||
        (methodMeta->GetHashCode() == modifier::kHashConflict))) {
      if (isConflict == false) {
        methodHashConflict.push_back(&constructor);
        isConflict = true;
      }
      methodHashConflict.push_back(methodMeta);
    }
  }

  for (auto methodConflict : methodHashConflict) {
    auto it = std::find(methodVec.begin(), methodVec.end(), methodConflict);
    if (it != methodVec.end()) {
      methodVec.erase(it);
    }
  }
}

static void ReomveRepeatMethod(const MArray &methodsArray, std::vector<MethodMeta*> &methodVec, uint32_t &rmNum) {
  uint32_t numMethods = methodsArray.GetLength();
  for (uint32_t i = numMethods; i > 0; --i) {
    MMethod *method = methodsArray.GetObjectElementNoRc(i - 1)->AsMMethod();
    if (method == nullptr) {
      continue;
    }
    MethodMeta *methodMeta = method->GetMethodMeta();
    char *signature = methodMeta->GetSignature();
    char *name = methodMeta->GetName();
    bool hasGen = false;
    for (auto it = methodVec.begin(); it != methodVec.end(); ++it) {
      MethodMeta *temp = *it;
      if (!strcmp(signature, temp->GetSignature()) && !strcmp(name, temp->GetName())) {
        hasGen = true;
        break ;
      }
    }
    if (hasGen) {
      ++rmNum;
      continue;
    }
    methodVec.push_back(methodMeta);
  }
}

static void GenProxyMethod(const MClass &proxyCls, MethodMeta &proxyClsMth, const MethodMeta &mobj,
                           const uint32_t &currentNum,
                           std::map<unsigned long, std::pair<unsigned long, std::string>> &itabMap,
                           std::vector<std::pair<unsigned long, std::string>> &itabConflictVector) {
  constexpr uint32_t kRemoveFlags = modifier::kModifierAbstract | modifier::kModifierDefault;
  constexpr uint32_t kAddFlags = modifier::kModifierFinal;
  char *methodName = mobj.GetName();
  MethodSignature *mthSig = mobj.GetMethodSignature();
  char *sigName = mobj.GetSignature();
  std::string innerMethodname = std::string(methodName) + "|" + std::string(sigName);
  unsigned long hashIdex = GetHashIndex(innerMethodname.c_str());
  auto address = reinterpret_cast<uintptr_t>(gstubfunc[currentNum]);
  uint32_t modifier = (mobj.GetMod() & (~kRemoveFlags)) | kAddFlags;
  proxyClsMth.FillMethodMeta(false, methodName, mthSig, mobj.GetAnnotationRaw(),
                             mobj.GetVtabIndex(), proxyCls, address, modifier,
                             mobj.GetFlag(), mobj.GetArgSize());
  std::pair<unsigned long, std::string> addressPair(address, innerMethodname);
  auto hasExist = itabMap.find(hashIdex);
  if (hasExist != itabMap.end()) {
    // conflict
    if (hasExist->second.first == 1) {
      itabConflictVector.push_back(addressPair);
    } else {
      auto tmp = hasExist->second;
      itabConflictVector.push_back(tmp);
      hasExist->second.first = 1;
      itabConflictVector.push_back(addressPair);
    }
  } else {
    std::pair<unsigned long, std::pair<unsigned long, std::string>> hashcodePair(hashIdex, addressPair);
    itabMap.insert(hashcodePair);
  }
}

static void GenProxyMethodAndConstructor(MClass &proxyCls, MethodMeta *&proxyClsMth, uint32_t &currentNum,
                                         std::vector<MethodMeta*> &methodVec,
                                         std::map<unsigned long, std::pair<unsigned long, std::string>> &itabMap,
                                         std::vector<std::pair<unsigned long, std::string>> &itabConflictVector,
                                         bool isConflictVector) {
  for (auto mobj : methodVec) {
    if (mobj->IsConstructor()) {
      GenProxyConstructor(proxyCls, *proxyClsMth, *mobj);
    } else {
      GenProxyMethod(proxyCls, *proxyClsMth, *mobj, currentNum, itabMap, itabConflictVector);
    }
    if (isConflictVector) {
      proxyClsMth->SetHashCode(modifier::kHashConflict);
    }
    ++currentNum;
    ++proxyClsMth;
  }
}

static void GenerateProxyMethods(MClass &proxyCls, MArray &methods) {
  uint32_t currentNum = 0;
  const uint32_t numDirectMethods = 1; // for constructor
  uint32_t numVirtualMethods = methods.GetLength();
  uint32_t methodNumSum = numVirtualMethods + numDirectMethods;
  uint32_t rmNum = 0;
  if (methodNumSum > kSupportMaxInterfaceMethod) {
    LOG(FATAL) << " number: " << methodNumSum << maple::endl;
  }

  std::vector<MethodMeta*> methodVec;
  ReomveRepeatMethod(methods, methodVec, rmNum);

  // constructor
  MClass *reflectProxyMCls = WellKnown::GetMClassProxy();
  MethodMeta *constructor = reflectProxyMCls->GetDeclaredConstructor("(Ljava/lang/reflect/InvocationHandler;)V");
  methodVec.push_back(constructor);
  SortMethodWithHashCode(methodVec);
  std::vector<MethodMeta*> methodHashConflict;
  ResolveConstructHashConflict(*constructor, methodVec, methodHashConflict);

  MethodMeta *proxyClsMthBase = MethodMeta::CastNonNull(MRT_AllocFromMeta(methodNumSum * sizeof(MethodMeta),
                                                                          kMethodMetaData));
  MethodMeta *proxyClsMth = proxyClsMthBase;
  std::map<unsigned long, std::pair<unsigned long, std::string>> itabMap;
  std::vector<std::pair<unsigned long, std::string>> itabConflictVector;

  GenProxyMethodAndConstructor(proxyCls, proxyClsMth, currentNum, methodVec, itabMap, itabConflictVector, false);
  GenProxyMethodAndConstructor(proxyCls, proxyClsMth, currentNum, methodHashConflict,
                               itabMap, itabConflictVector, true);
  uintptr_t itab = GenProxyItab(itabMap, itabConflictVector);
  proxyCls.SetItable(itab);
  proxyCls.SetMethods(*proxyClsMthBase);
  proxyCls.SetNumOfMethods(methodNumSum - rmNum);
  GenProxyVtab(proxyCls, methods);
}

static void GenerateProxyFields(MClass &proxyCls) {
  // set proxy declaring field 0
  proxyCls.SetNumOfFields(0);
}

jclass MRT_ReflectProxyGenerateProxy(jstring name, jobjectArray interfaces, jobject loader,
                                     jobjectArray methods, jobjectArray throws __attribute__((unused))) {
  MString *proxyName = MString::JniCastNonNull(name);
  MArray *interfacesArray = MArray::JniCastNonNull(interfaces);
  MArray *methodArray = MArray::JniCastNonNull(methods);
  MClass *proxyCls = MClass::NewMClass();
  std::string classNameStr = proxyName->GetChars();
  const char *classNameStrMeta = strdup(classNameStr.c_str()); // Does not need to be free-ed. See below.
  // Note: Why not std::string?  It is used as the name of a MClass, and it needs to be a C-style char*.
  if (classNameStrMeta == nullptr) {
    int myErrno = errno;
    LOG(FATAL) << "strdup: Failed to allocate classNameStrMeta. errno: " << myErrno << maple::endl;
    MRT_Panic();
  }
  proxyCls->SetName(classNameStrMeta); // This keeps classNameStrMeta alive. proxyCls is permanent.
  constexpr uint32_t accessFlag = modifier::kModifierProxy | modifier::kModifierPublic | modifier::kModifierFinal;
  proxyCls->SetModifier(accessFlag);
  MClass *javaProxy = WellKnown::GetMClassProxy();
  proxyCls->SetObjectSize(javaProxy->GetObjectSize());
  uintptr_t gctib = reinterpret_cast<uintptr_t>(javaProxy->GetGctib());
  proxyCls->SetGctib(gctib);
  // proxy super class always java.lang.proxy.
  uint32_t numOfSuper = interfacesArray->GetLength() + 1;
  MObject **itf = reinterpret_cast<MObject**>(calloc(numOfSuper, sizeof(MObject*)));
  CHECK(itf != nullptr) << "calloc Proxy itf fail." << maple::endl;
  itf[0] = javaProxy;
  // interfaces is class object which is out of heap.
  uint32_t srcIndex = 0;
  for (uint32_t i = 1; i < numOfSuper; ++i) {
    itf[i] = interfacesArray->GetObjectElementOffHeap(srcIndex++);
  }
  proxyCls->SetSuperClassArray(reinterpret_cast<uintptr_t>(itf));
  proxyCls->SetNumOfSuperClasses(numOfSuper);

  GenerateProxyMethods(*proxyCls, *methodArray);
  GenerateProxyFields(*proxyCls);
  // flag 0
  // annotation 0
  // clinitAddr 0
  MRT_RegisterDynamicClass(loader, *proxyCls);
  // mark this generated class as initialized
  proxyCls->SetInitStateRawValue(reinterpret_cast<uintptr_t>(proxyCls));
  std::atomic_thread_fence(std::memory_order_release);
  return proxyCls->AsJclass();
}
} // namespace maplert
