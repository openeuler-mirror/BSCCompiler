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
#include "methodmeta_inline.h"
#include "exception/mrt_exception.h"
namespace maplert {
using namespace annoconstant;
std::mutex MethodMetaCompact::resolveMutex;
bool MethodMetaBase::IsMethodMetaCompact() const {
  return (compactMetaFlag & modifier::kMethodMetaCompact) == modifier::kMethodMetaCompact;
}

char *MethodMetaBase::GetName() const {
  __MRT_Profile_MethodMeta(this);
  bool isMethodMetaCompact = IsMethodMetaCompact();
  char *name = nullptr;
  if (isMethodMetaCompact) {
    const MethodMetaCompact *methodMetaCompact = reinterpret_cast<const MethodMetaCompact*>(this);
    name = methodMetaCompact->GetName();
  } else {
    const MethodMeta *methodMeta = reinterpret_cast<const MethodMeta*>(this);
    name = methodMeta->GetName();
  }
  return name;
}

void MethodMetaBase::GetSignature(std::string &signature) const {
  bool isMethodMetaCompact = IsMethodMetaCompact();
  if (isMethodMetaCompact) {
    const MethodMetaCompact *methodMetaCompact = reinterpret_cast<const MethodMetaCompact*>(this);
    methodMetaCompact->GetSignature(signature);
  } else {
    const MethodMeta *methodMeta = reinterpret_cast<const MethodMeta*>(this);
    signature = methodMeta->GetSignature();
  }
}

MClass *MethodMetaBase::GetDeclaringClass() const {
  bool isMethodMetaCompact = IsMethodMetaCompact();
  MClass *dclClass = nullptr;
  if (isMethodMetaCompact) {
    const MethodMetaCompact *methodMetaCompact = reinterpret_cast<const MethodMetaCompact*>(this);
    dclClass = methodMetaCompact->GetDeclaringClass();
  } else {
    const MethodMeta *methodMeta = reinterpret_cast<const MethodMeta*>(this);
    dclClass = methodMeta->GetDeclaringClass();
  }
  return dclClass;
}

uint16_t MethodMetaBase::GetFlag() const {
  bool isMethodMetaCompact = IsMethodMetaCompact();
  uint16_t flag = 0;
  if (isMethodMetaCompact) {
    const MethodMetaCompact *methodMetaCompact = reinterpret_cast<const MethodMetaCompact*>(this);
    flag = methodMetaCompact->GetFlag();
  } else {
    const MethodMeta *methodMeta = reinterpret_cast<const MethodMeta*>(this);
    flag = methodMeta->GetFlag();
  }
  return flag;
}

uint32_t MethodMetaBase::GetMod() const {
  bool isMethodMetaCompact = IsMethodMetaCompact();
  uint32_t mod = 0;
  if (isMethodMetaCompact) {
    const MethodMetaCompact *methodMetaCompact = reinterpret_cast<const MethodMetaCompact*>(this);
    mod = methodMetaCompact->GetMod();
  } else {
    const MethodMeta *methodMeta = reinterpret_cast<const MethodMeta*>(this);
    mod = methodMeta->GetMod();
  }
  return mod;
}

int16_t MethodMetaBase::GetVtabIndex() const {
  bool isMethodMetaCompact = IsMethodMetaCompact();
  int16_t vtabIndex = 0;
  if (isMethodMetaCompact) {
    const MethodMetaCompact *methodMetaCompact = reinterpret_cast<const MethodMetaCompact*>(this);
    vtabIndex = methodMetaCompact->GetVtabIndex();
  } else {
    const MethodMeta *methodMeta = reinterpret_cast<const MethodMeta*>(this);
    vtabIndex = methodMeta->GetVtabIndex();
  }
  return vtabIndex;
}

uintptr_t MethodMetaBase::GetFuncAddress() const {
  bool isMethodMetaCompact = IsMethodMetaCompact();
  uintptr_t funcAddr = 0;
  if (isMethodMetaCompact) {
    const MethodMetaCompact *methodMetaCompact = reinterpret_cast<const MethodMetaCompact*>(this);
    funcAddr = methodMetaCompact->GetFuncAddress();
  } else {
    const MethodMeta *methodMeta = reinterpret_cast<const MethodMeta*>(this);
    funcAddr = methodMeta->GetFuncAddress();
  }
  return funcAddr;
}

int16_t MethodMetaCompact::GetVtabIndex() const {
  return vtabIndex;
}

uint16_t MethodMetaCompact::GetFlag() const {
  return compactMetaFlag;
}

int32_t MethodMetaCompact::GetDefTabIndex() const {
  MethodAddress *pMethodAddress = GetpMethodAddress();
  if (pMethodAddress == nullptr) {
    return -1;
  }
  return pMethodAddress->GetDefTabIndex();
}

MethodAddress *MethodMetaCompact::GetpMethodAddress() const {
  if (IsAbstract()) {
    return nullptr;
  }
  uintptr_t value = reinterpret_cast<const DataRefOffset32*>(&leb128Start)->GetDataRef<uintptr_t>();
  // compile will +2 as flag, here need -2 to remove it
  return reinterpret_cast<MethodAddress*>(value - 2);
}

uintptr_t MethodMetaCompact::GetFuncAddress() const {
  if (IsAbstract()) {
    return 0;
  }
  MethodAddress *pMethodAddress = GetpMethodAddress();
  if (pMethodAddress == nullptr) {
    return 0;
  }
  return pMethodAddress->GetAddr();
}

void MethodMetaCompact::SetFuncAddress(uintptr_t address) {
  MethodAddress *pMethodAddress = GetpMethodAddress();
  if (pMethodAddress != nullptr) {
    pMethodAddress->SetAddr(address);
  }
}

bool MethodMetaCompact::IsAbstract() const {
  uint16_t flag = GetFlag();
  return ((flag & modifier::kMethodAbstract) == modifier::kMethodAbstract);
}

const uint8_t *MethodMetaCompact::GetpCompact() const {
  return &leb128Start;
}

char *MethodMetaCompact::GetName() const {
  const uint8_t *pLeb = GetpCompact();
  pLeb = IsAbstract() ? pLeb : (pLeb + sizeof(int32_t));
  // ingore declaring Class index
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // modifier
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // methodname
  uint32_t methodNameIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
  char *method = LinkerAPI::Instance().GetCString(reinterpret_cast<jclass>(GetDeclaringClass()), methodNameIndex);
  __MRT_Profile_CString(method);
  return method;
}

void MethodMetaCompact::GetSignature(std::string &signature) const {
  const uint8_t *pLeb = GetpCompact();
  pLeb = IsAbstract() ? pLeb : (pLeb + sizeof(int32_t));
  // ingore declaring Class index
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // modifier
  uint32_t modifier = namemangler::GetUnsignedLeb128Decode(&pLeb);
  // methodname
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // args size
  uint32_t argsSize = namemangler::GetUnsignedLeb128Decode(&pLeb);
  // methodsignature
  signature = "(";
  // retrun type +1
  uint32_t typeSize = modifier::IsStatic(modifier) ? argsSize + 1 : argsSize;
  MClass *cls = GetDeclaringClass();
  for (uint32_t i = 0; i < typeSize; ++i) {
    uint32_t typeIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
    char *typeCName = LinkerAPI::Instance().GetCString(reinterpret_cast<jclass>(cls), typeIndex);
    if (UNLIKELY(typeCName == nullptr)) {
      LOG(FATAL) << "MethodMetaCompact::GetSignature: typeName must not be nullptr!" << maple::endl;
    }
    std::string typeName = typeCName;
    __MRT_Profile_CString(typeName.c_str());
    if (i == (typeSize - 1)) {
      signature += ")";
      signature += typeName;
    } else {
      signature += typeName;
    }
  }
}

uint32_t MethodMetaCompact::GetMod() const {
  const uint8_t *pLeb = GetpCompact();
  pLeb = IsAbstract() ? pLeb : (pLeb + sizeof(int32_t));
  // ingore declaring Class index
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // modifier
  uint32_t modifier = namemangler::GetUnsignedLeb128Decode(&pLeb);
  return modifier;
}

MClass *MethodMetaCompact::GetDeclaringClass() const {
  const uint8_t *pLeb = GetpCompact();
  pLeb = IsAbstract() ? pLeb : (pLeb + sizeof(int32_t));
  const uint8_t *prePleb = pLeb;
  uint32_t declClassOffset = namemangler::GetUnsignedLeb128Decode(&pLeb);
  const DataRefOffset32 *declClassAddr = reinterpret_cast<const DataRefOffset32*>(prePleb - declClassOffset);
  MClass *declClass = declClassAddr->GetDataRef<MClass*>();
  return declClass;
}

static MethodMetaCompact *Leb128DecodeOneByOne(MethodMetaCompact *leb) {
  // start decode
  const uint8_t *pLeb = leb->GetpCompact();
  pLeb = leb->IsAbstract() ? pLeb : (pLeb + sizeof(int32_t));
  // ingore declaring Class index
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // modifier
  uint32_t modifier = namemangler::GetUnsignedLeb128Decode(&pLeb);
  // methodname
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // args size
  uint32_t argsSize = namemangler::GetUnsignedLeb128Decode(&pLeb);
  // methodsignature
  uint32_t typeSize = modifier::IsStatic(modifier) ? (argsSize + 1) : argsSize;
  for (uint32_t j = 0; j < typeSize; ++j) {
    (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  }
  // annotation
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  leb = reinterpret_cast<MethodMetaCompact*>(const_cast<uint8_t*>(pLeb));
  return leb;
}

uintptr_t MethodMetaCompact::GetCompactFuncAddr(const MClass &cls, uint32_t index) {
  uint32_t numOfMethod = cls.GetNumOfMethods();
  MethodMetaCompact *leb = cls.GetCompactMethods();
  for (uint32_t i = 0; i < numOfMethod; ++i) {
    if (i == index) {
      return leb->GetFuncAddress();
    }
    leb = Leb128DecodeOneByOne(leb);
  }
  LOG(FATAL) << "MethodMetaCompact::GetCompactFuncAddr class: " << cls.GetName() <<
      ", index:" << index << maple::endl;
  return 0;
}

MethodMetaCompact *MethodMetaCompact::GetMethodMetaCompact(const MClass &cls, uint32_t index) {
  uint32_t numOfMethod = cls.GetNumOfMethods();
  MethodMetaCompact *leb = cls.GetCompactMethods();
  for (uint32_t i = 0; i < numOfMethod; ++i) {
    if (i == index) {
      return leb;
    }
    leb = Leb128DecodeOneByOne(leb);
  }
  LOG(FATAL) << "MethodMetaCompact::GetMethodMetaCompact class: " << cls.GetName() <<
      ", index:" << index << maple::endl;
  return 0;
}

uint8_t *MethodMetaCompact::DecodeCompactMethodMeta(const MClass &cls, uintptr_t &funcAddress, uint32_t &modifier,
                                                    std::string &methodName, std::string &signatureName,
                                                    std::string &annotationValue, int32_t &methodInVtabIndex,
                                                    uint16_t &flags, uint16_t &argsSize) const {
  methodInVtabIndex = GetVtabIndex();
  flags = GetFlag();
  funcAddress = reinterpret_cast<uintptr_t>(GetFuncAddress());
  // start decode
  const uint8_t *pLeb = GetpCompact();
  pLeb = IsAbstract() ? pLeb : (pLeb + sizeof(int32_t));
  // ingore declaring Class index
  (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
  // modifier
  modifier = namemangler::GetUnsignedLeb128Decode(&pLeb);
  // methodname
  uint32_t methodNameIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
  methodName = LinkerAPI::Instance().GetCString(cls.AsJclass(), methodNameIndex);
  // args size
  argsSize = static_cast<uint16_t>(namemangler::GetUnsignedLeb128Decode(&pLeb));
  // methodsignature
  std::string signature("(");
  // retrun type +1
  uint32_t typeSize = modifier::IsStatic(modifier) ? (argsSize + 1) : argsSize;
  uint32_t parameterSize = typeSize - 1;
  for (uint32_t j = 0; j < typeSize; ++j) {
    uint32_t typeIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
    std::string typeName = LinkerAPI::Instance().GetCString(cls.AsJclass(), typeIndex);
    __MRT_Profile_CString(typeName.c_str());
    if (j == parameterSize) {
      signature += ")";
      signature += typeName;
    } else {
      signature += typeName;
    }
  }
  signatureName = signature;
  // annotation
  uint32_t annotationIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
  annotationValue = LinkerAPI::Instance().GetCString(cls.AsJclass(), annotationIndex);
  return const_cast<uint8_t*>(pLeb);
}

MethodMeta *MethodMetaCompact::DecodeCompactMethodMetas(MClass &cls) {
  uint32_t numOfMethod = cls.GetNumOfMethods();
  if (numOfMethod == 0) {
    return nullptr;
  }
  {
    std::lock_guard<std::mutex> lock(resolveMutex);
    MethodMeta *methods = cls.GetRawMethodMetas();
    if (!cls.IsCompactMetaMethods()) {
      return methods;
    }
    // Compact, need resolve
    MethodMetaCompact *leb = cls.GetCompactMethods();

    CHECK(numOfMethod < (std::numeric_limits<uint32_t>::max() / sizeof(MethodMeta))) <<
        "method count too large. numOfMethod " << numOfMethod << maple::endl;
    MethodMeta *methodMetas = reinterpret_cast<MethodMeta*>(
        MRT_AllocFromMeta(sizeof(MethodMeta) * numOfMethod, kMethodMetaData));
    MethodMeta *methodMeta = methodMetas;
    for (uint32_t i = 0; i < numOfMethod; ++i) {
      uintptr_t funcAddress = 0;
      uint32_t modifier = 0;
      std::string methodName;
      std::string signatureName;
      std::string annotationValue;
      int32_t methodInVtabIndex = 0;
      uint16_t flags = 0;
      uint16_t argsSize = 0;
      uint8_t *pLeb = leb->DecodeCompactMethodMeta(cls, funcAddress, modifier, methodName, signatureName,
                                                   annotationValue, methodInVtabIndex, flags, argsSize);
      MethodSignature *mthSig = methodMeta->CreateMethodSignatureByName(signatureName.c_str());
      methodMeta->FillMethodMeta(true, methodName.c_str(), mthSig, annotationValue.c_str(),
                                 methodInVtabIndex, cls, funcAddress, modifier, flags, argsSize);
      methodMeta++;
      leb = reinterpret_cast<MethodMetaCompact*>(pLeb);
    }
    cls.SetMethods(*methodMetas);
    return methodMetas;
  }
}

MethodSignature *MethodMeta::CreateMethodSignatureByName(const char *signature) {
  size_t sigStrLen = strlen(signature) + 1;
  char *signatureBuffer = reinterpret_cast<char*>(MRT_AllocFromMeta(sigStrLen, kNativeStringData));
  errno_t tmpResult = memcpy_s(signatureBuffer, sigStrLen, signature, sigStrLen);
  if (UNLIKELY(tmpResult != EOK)) {
    LOG(FATAL) << "MethodMeta::CreateMethodSignatureByName : memcpy_s() failed" << maple::endl;
  }
  signature = signatureBuffer;
  MethodSignature *pMethodSignature = reinterpret_cast<MethodSignature*>(MRT_AllocFromMeta(sizeof(MethodSignature),
                                                                         kMethodMetaData));
  pMethodSignature->SetSignature(signature);
  return pMethodSignature;
}

void MethodMeta::FillMethodMeta(bool copySignature, const char *name,
    const MethodSignature *methodSignature, const char *annotation, int32_t inVtabIndex,
    const MClass &methodDclClass, const uintptr_t funcAddr,
    uint32_t modifier, uint16_t methodFlag, uint16_t argSize) {
  DCHECK(name != nullptr) << "MethodMeta::FillMethodMeta: name is nullptr!" << maple::endl;
  DCHECK(methodSignature != nullptr) << "MethodMeta::FillMethodMeta: methodSignature is nullptr!" << maple::endl;
  DCHECK(annotation != nullptr) << "MethodMeta::FillMethodMeta: annotation is nullptr!" << maple::endl;
#ifndef USE_32BIT_REF
  size_t methodNameStrLen = strlen(name) + 1;
  size_t annotationStrLen = strlen(annotation) + 1;
  char *strBuffer = reinterpret_cast<char*>(MRT_AllocFromMeta(annotationStrLen + methodNameStrLen, kNativeStringData));
  char *methodNameBuffer = strBuffer;
  char *annoBuffer = strBuffer + methodNameStrLen;
  errno_t tmpResult1 = memcpy_s(methodNameBuffer, methodNameStrLen, name, methodNameStrLen);
  errno_t tmpResult2 = memcpy_s(annoBuffer, annotationStrLen, annotation, annotationStrLen);
  if (UNLIKELY(tmpResult1 != EOK || tmpResult2 != EOK)) {
    LOG(FATAL) << "MethodMeta::FillMethodMeta : memcpy_s() failed" << maple::endl;
  }
  name = methodNameBuffer;
  annotation = annoBuffer;
  methodFlag &= ~modifier::kMethodParametarType;
  if (!copySignature) {
    char *signature = methodSignature->GetSignature();
    methodSignature = CreateMethodSignatureByName(signature);
  }
#endif
  (void)copySignature;
  SetMod(modifier);
  SetName(name);
  SetFlag(methodFlag);
  SetMethodSignature(methodSignature);
  SetAnnotation(annotation);
  SetAddress(funcAddr);
  SetDeclaringClass(methodDclClass);
  SetVtabIndex(inVtabIndex);
  SetArgsSize(argSize);
}

MClass *MethodMeta::GetReturnType() const {
  __MRT_Profile_MethodParameterTypes(*this);
  bool isEnableParametarType = IsEnableParametarType();
  if (isEnableParametarType) {
    MetaRef *pTypes = GetMethodSignature()->GetTypes();
    uint32_t returnTypeIndex = GetParameterCount();
    if (pTypes[returnTypeIndex] != 0) {
      return reinterpret_cast<MClass*>(pTypes[returnTypeIndex]);
    }
  }

  char *rtTypeName = GetReturnTypeName();
  MClass *declClass = GetDeclaringClass();
  MClass *returnType = MClass::GetClassFromDescriptor(declClass, rtTypeName);
  if (isEnableParametarType) {
    MetaRef *pTypes = GetMethodSignature()->GetTypes();
    uint32_t returnTypeIndex = GetParameterCount();
    pTypes[returnTypeIndex] = static_cast<MetaRef>(returnType->AsUintptr());
  }
  return returnType;
}

static void GetDescriptorBySignature(char *descriptor, char *&methodSig, char &c) {
  char *tmpDescriptor = descriptor;
  if (c == '[') {
    while (c == '[') {
      *tmpDescriptor++ = c;
      c = *methodSig++;
    }
  }
  if (c == 'L') {
    while (c != ';') {
      *tmpDescriptor++ = c;
      c = *methodSig++;
    }
  }
  *tmpDescriptor++ = c;
  c = *methodSig++;
  *tmpDescriptor = '\0';
}

bool MethodMeta::GetParameterTypesUtil(MClass *parameterTypes[], uint32_t size) const {
  uint32_t parameterCount = GetParameterCount();
  CHECK(parameterCount == size) << "size is wrong." << maple::endl;
  if (parameterCount == 0) {
    return true;
  }
  uint32_t index = 0;
  char *methodSig = GetSignature();
  __MRT_Profile_MethodParameterTypes(*this);
  MClass *declClass = GetDeclaringClass();
  size_t len = strlen(methodSig) + 1;
  // skip first '('
  methodSig++;
  char descriptor[len];
  char c = *methodSig++;
  while (c != ')') {
    GetDescriptorBySignature(descriptor, methodSig, c);
    MClass *ptype = MClass::GetClassFromDescriptor(declClass, descriptor);
    if (UNLIKELY(ptype == nullptr)) {
      return false;
    }
    parameterTypes[index++] = ptype;
  }
  return true;
}

void MethodMeta::GetParameterTypes(std::vector<MClass*> &parameterTypes) const {
  uint32_t parameterCount = GetParameterCount();
  MClass *types[parameterCount];
  bool isSuccess = GetParameterTypes(types, parameterCount);
  if (!isSuccess) {
    return;
  }
  for (uint32_t i = 0; i < parameterCount; ++i) {
    parameterTypes.push_back(types[i]);
  }
}

MArray *MethodMeta::GetParameterTypes() const {
  std::vector<MClass*> parameterTypes;
  GetParameterTypes(parameterTypes);
  if (MRT_HasPendingException()) {
    return nullptr;
  }
  uint32_t size = static_cast<uint32_t>(parameterTypes.size());
  MArray *parameterTypesArray = MArray::NewObjectArray(size, *WellKnown::GetMClassAClass());
  uint32_t currentIndex = 0;
  for (auto parameterType : parameterTypes) {
    parameterTypesArray->SetObjectElementOffHeap(currentIndex++, parameterType);
  }
  return parameterTypesArray;
}

void MethodMeta::GetParameterTypesDescriptor(std::vector<std::string> &descriptors) const {
  char *methodSig = GetSignature();
  size_t len = strlen(methodSig) + 1;
  // skip first '('
  methodSig++;
  char descriptor[len];
  char c = *methodSig++;
  while (c != ')') {
    GetDescriptorBySignature(descriptor, methodSig, c);
    descriptors.push_back(descriptor);
  }
}

MObject *MethodMeta::GetSignatureAnnotation() const {
  std::string methodAnnoStr = GetAnnotation();
  if (methodAnnoStr.empty()) {
    return nullptr;
  }
  MObject *ret = AnnoParser::GetSignatureValue(methodAnnoStr, GetDeclaringClass());
  return ret;
}

void MethodMeta::GetExceptionTypes(std::vector<MClass*> &types) const {
  std::string annoStr = GetAnnotation();
  if (annoStr.empty()) {
    return;
  }
  MClass *cl = GetDeclaringClass();
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), cl);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t loc = parser->Find(parser->GetThrowsClassStr());
  if (loc != kNPos) {
    std::string exception = parser->ParseStr(kDefParseStrType);
    int64_t annoType = parser->ParseNum(kValueInt);
    if (exception == "value" && annoType == kValueArray) {
      size_t start = annoStr.find(parser->GetAnnoArrayStartDelimiter(), parser->GetIdx());
      size_t end = annoStr.find(parser->GetAnnoArrayEndDelimiter(), parser->GetIdx());
      if ((start != std::string::npos) && (end != std::string::npos)) {
        parser->SetIdx(static_cast<uint32_t>(start - kLabelSize));
        int64_t exceptionNum = parser->ParseNum(kValueInt);
        parser->SkipNameAndType();
        std::string exceptionStr;
        for (int64_t i = 0; i < exceptionNum; ++i) {
          exceptionStr.clear();
          exceptionStr = parser->ParseStr(kDefParseStrType);
          MClass *exceptionClass = MClass::GetClassFromDescriptor(cl, exceptionStr.c_str(), false);
          if (exceptionClass != nullptr) {
            types.push_back(exceptionClass);
          } else {
            MRT_ThrowNewException("java/lang/TypeNotPresentException", nullptr);
            break;
          }
        }
      }
    }
  }
  return;
}

MArray *MethodMeta::GetExceptionTypes() const {
  std::vector<MClass*> exceptionTypes;
  GetExceptionTypes(exceptionTypes);
  if (MRT_HasPendingException()) {
    return nullptr;
  }
  uint32_t size = static_cast<uint32_t>(exceptionTypes.size());
  MArray *excetpionArray = MArray::NewObjectArray(size, *WellKnown::GetMClassAClass());
  uint32_t index = 0;
  for (auto type : exceptionTypes) {
    excetpionArray->SetObjectElementOffHeap(index++, type);
  }
  return excetpionArray;
}

void MethodMeta::GetPrettyName(bool needSignature, std::string &dstName) const {
  char *name = GetName();
  MClass *declClass = GetDeclaringClass();
  std::string declaringClassName;
  declClass->GetTypeName(declaringClassName);
  if (needSignature) {
    std::string retTypeStr;
    MClass::ConvertDescriptorToTypeName(GetReturnTypeName(), retTypeStr);
    dstName = retTypeStr + " ";
  }
  dstName += declaringClassName + "." + name;
  if (needSignature) {
    dstName += "(";
    std::vector<std::string> parameterTypes;
    GetParameterTypesDescriptor(parameterTypes);
    uint64_t numOfParam = static_cast<uint32_t>(parameterTypes.size());
    auto it = parameterTypes.begin();
    uint64_t i = 0;
    std::string paramStr;
    for (; it != parameterTypes.end(); ++it, ++i) {
      paramStr.clear();
      MClass::ConvertDescriptorToTypeName((*it).c_str(), paramStr);
      dstName += paramStr;
      if (i != (numOfParam - 1)) {
        dstName += ", ";
      }
    }
    dstName += ")";
  }
}

// ([ZJSIDF[Ljava/lang/String;Ljava/lang/String;)V  -> [JSIDF[L
void MethodMeta::GetShortySignatureUtil(const char srcSignature[], char shorty[], const uint32_t size) {
  DCHECK(shorty != nullptr);
  DCHECK(srcSignature != nullptr);
  const char *name = srcSignature;
  char *ret = shorty;
  name++; // skip '('
  while ((*name != '\0') && (*name != ')')) {
    if (*name == '[') {
      while (*name == '[') {
        name++;
      }
      if (*name == 'L') {
        while (*name != ';') {
          name++;
        }
      }
      name++;
      *ret++ = '[';
      continue;
    }

    if (*name == 'L') {
      while (*name != ';') {
        name++;
      }
      name++;
      *ret++ = 'L';
      continue;
    }
    *ret++ = *name++;
  }
  CHECK((ret - shorty) <= static_cast<const int32_t>(size));
}

void MethodMeta::SetMod(const uint32_t modifier) {
  mod = modifier;
}

void MethodMeta::SetName(const char *name) {
  methodName.SetDataRef(name);
}

void MethodMeta::SetSignature(const char *signature) {
  GetMethodSignature()->SetSignature(signature);
}

void MethodMeta::SetMethodSignature(const MethodSignature *methodSignature) {
  if (IsEnableParametarType()) {
    pMethodSignature.SetDataRef(methodSignature);
  } else {
    char *signature = methodSignature->GetSignature();
    SetSignature(signature);
  }
}

void MethodMeta::SetAddress(const uintptr_t address) {
  MethodAddress *pMethodAddress = GetpMethodAddress();
  pMethodAddress->SetAddr(address);
}

void MethodMeta::SetAnnotation(const char *annotation) {
  annotationValue.SetDataRef(annotation);
}

void MethodMeta::SetDeclaringClass(const MClass &dlClass) {
  declaringClass.SetDataRef(&dlClass);
}

void MethodMeta::SetFlag(const uint16_t methodFlag) {
  flag = methodFlag;
}

void MethodMeta::SetArgsSize(const uint16_t methodArgSize) {
  argumentSize = methodArgSize;
}

void MethodMeta::SetVtabIndex(const int32_t methodVtabIndex) {
  vtabIndex = methodVtabIndex;
}

void MethodMeta::GetJavaMethodFullName(std::string &name) const {
  MClass *cls = GetDeclaringClass();
  if (cls != nullptr) {
    cls->GetBinaryName(name);
  } else {
    name.append("figo.internal.class");
  }
  char *mathodName = GetName();
  char *sigName = GetSignature();
  name.append(".").append(mathodName).append(sigName);
}

void MethodMeta::GetJavaClassName(std::string &name) const {
  MClass *cls = GetDeclaringClass();
  if (cls != nullptr) {
    cls->GetBinaryName(name);
  } else {
    name.append("fig.internal.class");
  }
}

void MethodMeta::GetJavaMethodName(std::string &name) const {
  name = GetName();
}

bool MethodMeta::IsMethodAccessible(const MClass &curClass) const {
  if (IsPublic() || IsProtected()) {
    return true;
  }
  if (IsPrivate()) {
    return false;
  }
  // package
  return reflection::IsInSamePackage(curClass, *GetDeclaringClass());
}

// overrideMethod can be overrided by this method.
bool MethodMeta::IsOverrideMethod(const MethodMeta &overrideMethod) const {
  if (&overrideMethod == this) {
    return true;
  }
  if (IsPrivate()) {
    return false;
  }
  DCHECK(!overrideMethod.IsPrivate());
  if (overrideMethod.IsPublic() || overrideMethod.IsProtected()) {
    return true;
  }

  // package
  return reflection::IsInSamePackage(*overrideMethod.GetDeclaringClass(), *GetDeclaringClass());
}

void MethodMeta::BuildJValuesArgsFromVaList(jvalue argsJvalue[], va_list args) const {
  uint32_t parameterSize = GetParameterCount();
  char retTypeNames[parameterSize];
  GetShortySignature(retTypeNames, parameterSize);
  char *result = retTypeNames;
  DCHECK(argsJvalue != nullptr);
  for (uint32_t i = 0; i < parameterSize; ++i) {
    argsJvalue[i].j = 0;
    switch (result[i]) {
      case 'Z':
        argsJvalue[i].z = static_cast<uint8_t>(va_arg(args, int32_t));
        break;
      case 'B':
        argsJvalue[i].b = static_cast<int8_t>(va_arg(args, int32_t));
        break;
      case 'C':
        argsJvalue[i].c = static_cast<uint16_t>(va_arg(args, int32_t));
        break;
      case 'S':
        argsJvalue[i].s = static_cast<int16_t>(va_arg(args, int32_t));
        break;
      case 'I':
        argsJvalue[i].i = va_arg(args, int32_t);
        break;
      case 'J':
        argsJvalue[i].j = static_cast<int64_t>(va_arg(args, int64_t));
        break;
      case 'F': {
        jvalue tmp;
        tmp.d = va_arg(args, double);
        argsJvalue[i].f = tmp.f;
        break;
      }
      case 'D':
        argsJvalue[i].d = va_arg(args, double);
        break;
      default:
        argsJvalue[i].l = va_arg(args, jobject);
    }
  }
}

void MethodMeta::BuildMArrayArgsFromJValues(MArray &targetValue, jvalue args[]) const {
  uint32_t parameterSize = GetParameterCount();
  char retTypeNames[parameterSize];
  GetShortySignature(retTypeNames, parameterSize);
  char *result = retTypeNames;
  MObject *obj = nullptr;
  for (uint32_t i = 0; i < parameterSize; ++i) {
    switch (result[i]) {
      case 'B':
        obj = primitiveutil::BoxPrimitiveJbyte(args[i].b);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      case 'C':
        obj = primitiveutil::BoxPrimitiveJchar(args[i].c);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      case 'D':
        obj = primitiveutil::BoxPrimitiveJdouble(args[i].d);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      case 'F':
        obj = primitiveutil::BoxPrimitiveJfloat(args[i].f);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      case 'I':
        obj = primitiveutil::BoxPrimitiveJint(args[i].i);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      case 'J':
        obj = primitiveutil::BoxPrimitiveJlong(args[i].j);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      case 'S':
        obj = primitiveutil::BoxPrimitiveJshort(args[i].s);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      case 'Z':
        obj = primitiveutil::BoxPrimitiveJboolean(args[i].z);
        targetValue.SetObjectElementNoRc(i, obj);
        break;
      default: // default ref
        obj = reinterpret_cast<MObject*>(args[i].l);
        targetValue.SetObjectElement(i, obj);
    }
  }
}

void MethodMeta::BuildJValuesArgsFromStackMemeryPrefixSigNature(DecodeStackArgs &args, std::string prefix) {
  uint32_t parameterSize = GetParameterCount();
  size_t oldLen = prefix.length();
  prefix.resize(prefix.length() + parameterSize);
  char *prefixCStr = const_cast<char*>(prefix.c_str());
  GetShortySignature(prefixCStr + oldLen, parameterSize);
  BuildJValuesArgsFromStackMemery(args, prefix);
}

void MethodMeta::BuildJValuesArgsFromStackMemery(DecodeStackArgs &args, std::string &shorty) {
  size_t len = shorty.length();
  for (size_t i = 0; i < len; ++i) {
    switch (shorty[i]) {
      case 'F':
        args.DecodeFloat();
        break;
      case 'D':
        args.DecodeDouble();
        break;
      case 'J':
        args.DecodeInt64();
        break;
      case 'L':
      case '[':
        args.DecodeReference();
        break;
      default:
        args.DecodeInt32();
    }
  }
}
} // namespace maplert
