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
#include "mclass.h"
#include "mclass_inline.h"
#include "fieldmeta_inline.h"
#include "mrt_cyclequeue.h"
#include "exception/mrt_exception.h"
#include <algorithm>
namespace maplert {
void MClass::GetPrettyClass(std::string &dstName) const {
  std::string clsName;
  GetBinaryName(clsName);
  dstName = "java.lang.Class<";
  dstName += clsName;
  dstName += ">";
}

void MClass::GetBinaryName(std::string &dstName) const {
  char *className = GetName();
  if (IsProxy()) {
    dstName = className;
  } else {
    ConvertDescriptorToBinaryName(className, dstName);
  }
}

void MClass::GetTypeName(std::string &dstName) const {
  ConvertDescriptorToTypeName(GetName(), dstName);
}

void MClass::ConvertDescriptorToBinaryName(const std::string &descriptor, std::string &binaryName) {
  if (descriptor[0] != 'L' && descriptor[0] != '[') {
    maple::Primitive::GetPrimitiveClassName(descriptor.c_str(), binaryName);
  } else {
    binaryName = descriptor;
    std::replace(binaryName.begin(), binaryName.end(), '/', '.');
    size_t len = binaryName.length();
    // descriptor must > 2, like:LA;
    if (binaryName[0] == 'L') {
      binaryName = binaryName.substr(1, len - 2); // remove char 'L', ';'
    }
  }
}

void MClass::ConvertDescriptorToTypeName(const std::string &descriptor, std::string &defineName) {
  uint32_t dim = GetDimensionFromDescriptor(descriptor);
  ConvertDescriptorToBinaryName(descriptor.substr(dim), defineName);
  for (uint32_t i = 0; i < dim; ++i) {
    defineName += "[]";
  }
}

void MClass::GetDescriptor(std::string &dstName) const {
  dstName = GetName();
}

bool MClass::IsInnerClass() const {
  std::string annoStr = GetAnnotation();
  AnnoParser &parser = AnnoParser::ConstructParser(annoStr.c_str(), const_cast<MClass*>(this));
  int32_t loc = parser.Find(parser.GetInnerClassStr());
  delete &parser;
  return loc != annoconstant::kNPos;
}

std::string MClass::GetAnnotation() const {
  char *annotation = GetRawAnnotation();
  return AnnotationUtil::GetAnnotationUtil(annotation);
}

FieldMetaCompact *MClass::GetCompactFields() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  FieldMetaCompact *compactFields = classInfoRo->fields.GetCompactData<FieldMetaCompact*>();
  return compactFields;
}

MethodMetaCompact *MClass::GetCompactMethods() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  MethodMetaCompact *methodsCompact = classInfoRo->methods.GetCompactData<MethodMetaCompact*>();
  return methodsCompact;
}

bool MClass::IsCompactMetaMethods() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->methods.IsCompact();
}

bool MClass::IsCompactMetaFields() const {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  return classInfoRo->fields.IsCompact();
}

void MClass::GetDeclaredMethods(std::vector<MethodMeta*> &methodsVector, bool publicOnly) const {
  uint32_t numOfMethod = GetNumOfMethods();
  MethodMeta *methodS = GetMethodMetas();
  constexpr int kReserveTw = 20;
  methodsVector.reserve(kReserveTw);
  for (uint32_t i = 0; i < numOfMethod; ++i) {
    MethodMeta *method = &methodS[i];
    if (method->IsConstructor()) {
      continue;
    }
    if (!publicOnly || (method->IsPublic())) {
      methodsVector.push_back(method);
    }
  }
}

void MClass::GetDirectInterfaces(MClass *interfaceVector[], uint32_t size) const {
  uint32_t numOfInterface = GetNumOfInterface();
  CHECK(size <= numOfInterface) << "buffer size is wrong." << maple::endl;
  if (IsArrayClass()) {
    interfaceVector[0] = WellKnown::arrayInterface[0];
    interfaceVector[1] = WellKnown::arrayInterface[1];
  } else {
    MClass **supers = GetSuperClassArray();
    MClass **pInterfaces = IsInterface() ? supers : supers + 1;
    for (uint32_t index = 0; index < numOfInterface; ++index) {
      MClass *interface = ResolveSuperClass(pInterfaces + index);
      interfaceVector[index] = interface;
    }
  }
}

void MClass::GetSuperClassInterfaces(uint32_t numOfSuperClass, MClass **superArray,
                                     std::vector<MClass*> &interfaceVector, uint32_t firstSuperClass) const {
  std::vector<MClass*> tempVector;
  for (uint32_t i = firstSuperClass; i < numOfSuperClass; ++i) {
    MClass *super = ResolveSuperClass(superArray + i);
    std::vector<MClass*>::iterator it = std::find(interfaceVector.begin(), interfaceVector.end(), super);
    if (it == interfaceVector.end()) {
      interfaceVector.push_back(super);
      tempVector.push_back(super);
    }
  }
  for (auto itf : tempVector) {
    itf->GetInterfaces(interfaceVector);
  }
}

void MClass::GetInterfaces(std::vector<MClass*> &interfaceVector) const {
  if (IsPrimitiveClass()) {
    return;
  } else if (IsArrayClass()) {
    interfaceVector.push_back(WellKnown::arrayInterface[0]);
    interfaceVector.push_back(WellKnown::arrayInterface[1]);
    return;
  } else {
    uint32_t numOfSuperClass = GetNumOfSuperClasses();
    MClass **superArray = GetSuperClassArray();
    if (superArray == nullptr) {
      return;
    } else if (IsInterface()) {
      GetSuperClassInterfaces(numOfSuperClass, superArray, interfaceVector, 0);
    } else { // is class
      GetSuperClassInterfaces(numOfSuperClass, superArray, interfaceVector, 1);

      if (numOfSuperClass > 0) {
        MClass *info = GetSuperClass();
        if (info != nullptr) {
          info->GetInterfaces(interfaceVector);
        }
      }
    }
  }
}

MethodMeta *MClass::GetInterfaceMethod(const char *methodName, const char *signatureName) const {
  MethodMeta *method = nullptr;
  std::vector<MClass*> interfaceVector;
  constexpr int kReserveTw = 20;
  interfaceVector.reserve(kReserveTw);
  GetInterfaces(interfaceVector);
  MethodMeta *resultMethod = nullptr;
  for (auto interface : interfaceVector) {
    method = interface->GetDeclaredMethod(methodName, signatureName);
    if (method != nullptr) {
      if (resultMethod == nullptr) {
        resultMethod = method;
      } else if (resultMethod->GetDeclaringClass()->IsAssignableFrom(*method->GetDeclaringClass())) {
        resultMethod = method;
      }
    }
  }
  return resultMethod;
}

MethodMeta *MClass::GetMethod(const char *methodName, const char *signatureName) const {
  MethodMeta *method = nullptr;
  const MClass *superClass = this;
  while (superClass != nullptr) {
    method = superClass->GetDeclaredMethod(methodName, signatureName);
    if (method != nullptr) {
      return method;
    } else {
      superClass = superClass->GetSuperClass();
    }
  }
  return GetInterfaceMethod(methodName, signatureName);
}

MethodMeta *MClass::GetMethodForVirtual(const MethodMeta &srcMethod) const {
  MethodMeta *method = nullptr;
  const MClass *superClass = this;
  char *methodName = srcMethod.GetName();
  char *signatureName = srcMethod.GetSignature();
  while (superClass != nullptr) {
    method = superClass->GetDeclaredMethod(methodName, signatureName);
    if (method != nullptr && method->IsOverrideMethod(srcMethod)) {
      return method;
    } else {
      superClass = superClass->GetSuperClass();
    }
  }
  return GetInterfaceMethod(methodName, signatureName);
}

MethodMeta *MClass::GetDeclaredFinalizeMethod() const {
  MethodMeta *methodS = GetMethodMetas();
  uint32_t numOfMethod = GetNumOfMethods();
  if (numOfMethod == 0) {
    return nullptr;
  }
  MethodMeta *method = &methodS[numOfMethod - 1];
  return method->IsFinalizeMethod() ? method : nullptr;
}

MethodMeta *MClass::GetFinalizeMethod() const {
  MethodMeta *finalizeMethod = nullptr;
  const MClass *superClass = this;
  while (superClass != nullptr) {
    finalizeMethod = superClass->GetDeclaredFinalizeMethod();
    if (finalizeMethod != nullptr) {
      break;
    }
    superClass = superClass->GetSuperClass();
  }
  return finalizeMethod;
}

bool MClass::IsAssignableFromInterface(const MClass &cls) const {
  if (cls.IsArrayClass()) {
    // array class has 2 interface
    return (this == WellKnown::arrayInterface[0]) ? true : (this == WellKnown::arrayInterface[1]);
  }
  CycleQueue<const MClass*> interfaceQueue;
  interfaceQueue.Push(&cls);
  while (!interfaceQueue.Empty()) {
    const MClass *tmpClass = interfaceQueue.Front();
    interfaceQueue.Pop();
    uint32_t numofsuperclass = tmpClass->GetNumOfSuperClasses();
    MClass **superArray = tmpClass->GetSuperClassArray();
    for (uint32_t i = 0; i < numofsuperclass; ++i) {
      MClass *super = ResolveSuperClass(superArray + i);
      if (super == this) {
        return true;
      }
      interfaceQueue.Push(super);
    }
  }
  return false;
}

bool MClass::IsAssignableFromImpl(const MClass &cls) const {
  if (&cls == this) {
    return true;
  } else if (WellKnown::GetMClassObject() == this) {
    return !cls.IsPrimitiveClass();
  } else if (IsArrayClass()) {
    return cls.IsArrayClass() && GetComponentClass()->IsAssignableFromImpl(*cls.GetComponentClass());
  } else if (IsInterface()) {
    return IsAssignableFromInterface(cls);
  } else if (!cls.IsInterface()) {
    MClass **superClassArray = cls.GetSuperClassArrayPtr();
    MClass *super = cls.GetNumOfSuperClasses() == 0 ? nullptr : ResolveSuperClass(superClassArray);
    while (super != nullptr) {
      if (super == this) {
        return true;
      }
      superClassArray = super->GetSuperClassArrayPtr();
      super = super->GetNumOfSuperClasses() == 0 ? nullptr : ResolveSuperClass(superClassArray);
    }
  }
  return false;
}

void MClass::SetName(const char *name) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->className.SetRef(name);
}

void MClass::SetModifier(uint32_t newMod) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->mod = newMod;
}

void MClass::SetGctib(uintptr_t newGctib) {
  ClassMetadata *cls = GetClassMeta();
  cls->gctib.SetGctibRef(newGctib);
}

void MClass::SetSuperClassArray(uintptr_t newValue) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->superclass.SetDataRef(newValue);
}

void MClass::SetObjectSize(uint32_t newSize) {
  ClassMetadata *cls = GetClassMeta();
  cls->sizeInfo.objSize = newSize;
}

void MClass::SetItable(uintptr_t itab) {
  ClassMetadata *cls = GetClassMeta();
  cls->iTable.SetDataRef(itab);
}

void MClass::SetVtable(uintptr_t vtab) {
  ClassMetadata *cls = GetClassMeta();
  cls->vTable.SetDataRef(vtab);
}

void MClass::SetNumOfFields(uint32_t newValue) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->numOfFields = newValue;
}

void MClass::SetNumOfMethods(uint32_t newValue) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->numOfMethods = newValue;
}

void MClass::SetMethods(const MethodMeta &newMethods) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->methods.SetDataRef(&newMethods);
}

void MClass::SetFields(const FieldMeta &newFields) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->fields.SetDataRef(&newFields);
}

void MClass::SetInitStateRawValue(uintptr_t newValue) {
  ClassMetadata *cls = GetClassMeta();
  cls->SetInitStateRawValue(newValue);
}

MClass *MClass::NewMClass(uintptr_t newClsMem) {
  if (newClsMem == 0) {
    newClsMem = reinterpret_cast<uintptr_t>(
        MRT_AllocFromMeta(sizeof(ClassMetadata) + sizeof(ClassMetadataRO), kClassMetaData));
  }
  MClass *newMClass = MObject::Cast<MClass>(newClsMem);
  uintptr_t newClsRo = newClsMem + sizeof(ClassMetadata);
  newMClass->SetClassMetaRoData(newClsRo);
  MClass *mClassClass = WellKnown::GetMClassClass();
  // store java.lang.Class
  newMClass->StoreObjectNoRc(0, mClassClass);
  return newMClass;
}

MClass *MClass::GetArrayClass(const MClass &componentClass) {
  return maplert::WellKnown::GetCacheArrayClass(componentClass);
}

MClass *MClass::GetClassFromDescriptorUtil(const MObject *context, const char *descriptor, bool throwException) {
  CHECK(descriptor != nullptr);
  // get context class
  const MClass *contextClass = context != nullptr ?
      (context->GetClass() == WellKnown::GetMClassClass() ?
      static_cast<const MClass*>(context) : context->GetClass()) : nullptr;
  MClass *cls = MClass::JniCast(MRT_GetClassByContextClass(*contextClass, descriptor));
  if (UNLIKELY(throwException && (cls == nullptr))) {
    std::string msg = "Failed resolution of: ";
    msg += descriptor;
    MRT_ThrowNewException("java/lang/NoClassDefFoundError", msg.c_str());
    return nullptr;
  }
  return cls;
}

MObject *MClass::GetSignatureAnnotation() const {
  std::string annotationSet = GetAnnotation();
  if (annotationSet.empty()) {
    return nullptr;
  }
  VLOG(reflect) << "Enter GetSignatureAnnotation, annoStr: " << annotationSet << maple::endl;
  MObject *ret = AnnoParser::GetSignatureValue(annotationSet, const_cast<MClass*>(this));
  return ret;
}

uint32_t MClass::GetArrayModifiers() const {
  const MClass *componentClass = this;
  while (componentClass->IsArrayClass()) {
    componentClass = componentClass->GetComponentClass();
  }
  uint32_t componentModifiers = componentClass->GetModifier() & 0xFFFF;
  if (modifier::IsInterface(componentModifiers)) {
    componentModifiers &= ~(modifier::GetInterfaceModifier() | modifier::GetStaticModifier());
  }
  return modifier::GetAbstractModifier() | modifier::GetFinalModifier() | componentModifiers;
}

MethodMeta *MClass::GetClinitMethodMeta() const {
  uint32_t numOfMethods = GetNumOfMethods();
  MethodMeta *methodS = GetMethodMetas();
  for (uint32_t i = 0; i < numOfMethods; ++i) {
    MethodMeta *method = methodS + i;
    bool isConstructor = method->IsConstructor();
    bool isStatic = method->IsStatic();
    if (isConstructor && isStatic) {
      return method;
    }
  }
  return nullptr;
}

void MClass::ResolveVtabItab() {
  ClassMetadata *cls = GetClassMeta();
  uint8_t *itab = cls->iTable.GetDataRef<uint8_t*>();
  cls->iTable.SetDataRef(itab);

  uint8_t *vtab = cls->vTable.GetDataRef<uint8_t*>();
  cls->vTable.SetDataRef(vtab);
}

void MClass::SetComponentClass(const MClass &klass) {
  ClassMetadataRO *classInfoRo = GetClassMetaRo();
  classInfoRo->componentClass.SetDataRef(&klass);
}
} // namespace maplert
