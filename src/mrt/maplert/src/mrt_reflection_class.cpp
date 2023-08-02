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
#include "mrt_reflection_class.h"
#include <set>
#include <list>
#include "libs.h"
#include "collector/cp_generator.h"
#include "itab_util.h"
#include "utils/name_utils.h"
#include "mrt_class_api.h"
#include "mrt_reflection_api.h"
#include "mclass_inline.h"
#include "mstring_inline.h"
#include "fieldmeta_inline.h"

using namespace maple;

namespace maplert {
using namespace annoconstant;

// This method is only used in cycle pattern load and all patterns are predefined
// Or saved before reboot
bool MRT_ClassSetGctib(jclass cls, char *newBuffer, jint offset) {
  MClass *klass = MClass::JniCastNonNull(cls);
  GCTibGCInfo *gctibInfo = reinterpret_cast<struct GCTibGCInfo*>(klass->GetGctib());
  DCHECK(gctibInfo != nullptr) << "Cyclepattern: gctib is null" << maple::endl;
  DCHECK(newBuffer != nullptr) << "Cyclepattern: newBuffer is null" << maple::endl;
  size_t gctibSize = sizeof(GCTibGCInfo) + (gctibInfo->nBitmapWords * kDWordBytes);
  int32_t tempSize = static_cast<int32_t>(gctibSize) + offset;
  constexpr int32_t kMaxSize = 64 * 1024 * 1024; // 64M
  if (tempSize <= 0 || tempSize > kMaxSize) {
    return false;
  }
  char *newAddress = static_cast<char*>(malloc(tempSize));
  if (newAddress == nullptr) {
    LOG(ERROR) << "MRT_ClassSetGctib malloc error" << maple::endl;
    return false;
  }
  // set header and cyclepattern
  errno_t returnValueOfMemcpyS1 = memcpy_s(newAddress, gctibSize, gctibInfo, gctibSize);
  errno_t returnValueOfMemcpyS2 = memcpy_s(newAddress + gctibSize, offset, newBuffer, offset);
  if (returnValueOfMemcpyS1 != EOK || returnValueOfMemcpyS2 != EOK) {
    free(newAddress);
    return false;
  }
  uint32_t maxRC = 0;
  uint32_t minRC = 0;
  bool valid = ClassCycleManager::GetRCThreshold(maxRC, minRC, newBuffer);
  if (valid == false) {
    LOG(ERROR) << "Cyclepattern: invalid min/max rc threshold " << maple::endl;
    free(newAddress);
    return false;
  }
  auto newGctib = reinterpret_cast<GCTibGCInfo*>(newAddress);
  newGctib->headerProto = SetCycleMaxRC(gctibInfo->headerProto, maxRC) | SetCycleMinRC(gctibInfo->headerProto, minRC) |
                          maplert::kCyclePatternBit;
  LOG2FILE(kLogtypeCycle) << klass->GetName() << " max min: " << maxRC << " " << minRC << std::hex <<
      " " << newGctib->headerProto << std::dec << std::endl;
  if (!ClassCycleManager::CheckValidPattern(klass, newAddress)) {
    LOG2FILE(kLogtypeCycle) << "cycle_check: verify fail " << klass->GetName() << std::endl;
    free(newAddress);
    return false;
  }
  klass->SetGctib(reinterpret_cast<uintptr_t>(newAddress));
  if (ClassCycleManager::HasDynamicLoadPattern(klass)) {
    free(gctibInfo);
  } else {
    ClassCycleManager::AddDynamicLoadPattern(klass, true);
  }
  return true;
}

// interface to access frequently used class-es
jclass MRT_ReflectGetObjectClass(jobject mObj) {
  MObject *obj = MObject::JniCastNonNull(mObj);
  return obj->GetClass()->AsJclass();
}

jclass MRT_ReflectClassForCharName(const char *className, bool init, jobject classLoader, bool internalName) {
  DCHECK(className != nullptr);
  std::string descriptor = internalName ? string(className) : nameutils::DotNameToSlash(className);
  MClass *klass = MClass::JniCastNonNull(MRT_GetClassByClassLoader(classLoader, descriptor));
  if (init && (klass != nullptr)) {
    bool ret = klass->InitClassIfNeeded();
    if (ret) {
      LOG(ERROR) << "MRT_TryInitClass return fail" << maple::endl;
    }
  }
  if (klass != nullptr) {
    return klass->AsJclass();
  }
  return nullptr;
}

bool MRT_ClassIsSuperClassValid(jclass clazz) {
  MClass *cls = MClass::JniCast(clazz);
  if (UNLIKELY(cls == nullptr)) {
    return false;
  }
  if (UNLIKELY(cls->IsInterface())) {
    return true;
  }
  if (UNLIKELY(cls->IsPrimitiveClass())) {
    return true;
  }
  if (UNLIKELY(cls->IsArrayClass())) {
    return true;
  }

  while (cls != WellKnown::GetMClassObject()) {
    uint32_t numOfSuperClass = cls->GetNumOfSuperClasses();
    if (UNLIKELY(numOfSuperClass == 0)) {
      return true;
    }
    MClass **superArray = cls->GetSuperClassArray();
    if (UNLIKELY(superArray == nullptr)) {
      LOG(WARNING) << "\"" << cls->GetName() << "\"'s superclass array should not be null." << maple::endl;
      return false;
    }
    for (uint32_t i = 0; i < numOfSuperClass; ++i) {
      ClassMetadata *tempSuper = reinterpret_cast<ClassMetadata*>(superArray[i]);
      MClass *super = reinterpret_cast<MClass*>(LinkerAPI::Instance().GetSuperClass(&tempSuper));
      if (UNLIKELY(super == nullptr)) {
        LOG(WARNING) << "\"" << cls->GetName() << "\"'s superclass[" << i << "] should not be null." <<
            maple::endl;
        return false;
      }
    }
    cls = superArray[0];
  }
  return true;
}

bool ReflectClassIsDeclaredAnnotationPresent(const MClass &classObj, const MObject *annoObj) {
  string annoStr = classObj.GetAnnotation();
  CHECK(annoObj != nullptr);
  char *annotationTypeName = annoObj->AsMClass()->GetName();
  if (annoStr.empty() || annotationTypeName == nullptr) {
    return false;
  }
  return AnnotationUtil::GetIsAnnoPresent(annoStr, annotationTypeName,
                                          reinterpret_cast<CacheValueType>(const_cast<MClass*>(&classObj)),
                                          annoObj->AsMClass(), kClassAnnoPresent);
}

MObject *ReflectClassGetDeclaredAnnotation(const MClass &classObj, const MClass *annoClass) {
  if (annoClass == nullptr) {
    MRT_ThrowNewException("java/lang/NullPointerException", nullptr);
    return nullptr;
  }
  if (classObj.IsProxy()) {
    return nullptr;
  }
  string annoStr = classObj.GetAnnotation();
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), const_cast<MClass*>(&classObj));
  std::unique_ptr<AnnoParser> parser(&annoParser);
  MObject *ret = parser->AllocAnnoObject(&const_cast<MClass&>(classObj), const_cast<MClass*>(annoClass));
  return ret;
}

jobject MRT_ReflectClassGetDeclaredAnnotations(jclass cls) {
  MClass *classObj = MClass::JniCastNonNull(cls);
  string annoStr = classObj->GetAnnotation();
  CacheValueType placeHolder = nullptr;
  if (annoStr.empty() || !AnnoParser::HasAnnoMember(annoStr) ||
      AnnotationUtil::GetCache(kHasNoDeclaredAnno, classObj, placeHolder)) {
    MArray *nullArray = MArray::NewObjectArray(0, *WellKnown::GetMClassAAnnotation());
    return nullArray->AsJobject();
  }
  VLOG(reflect) << "Enter MRT_ReflectClassGetDeclaredAnnotations, annoStr: " << annoStr << maple::endl;
  return AnnotationUtil::GetDeclaredAnnotations(annoStr, classObj)->AsJobject();
}

MObject *ReflectClassGetAnnotation(const MClass &klass, const MClass *annotationType) {
  DCHECK(annotationType != nullptr) << "ReflectClassGetAnnotation: annotationType nullptr !" << maple::endl;
  // NEED: check annotationType for null
  MObject *anno = ReflectClassGetDeclaredAnnotation(klass, annotationType);
  if (anno != nullptr) {
    return anno;
  }

  if (AnnotationUtil::HasDeclaredAnnotation(const_cast<MClass*>(annotationType), kAnnotationInherited)) {
    // then annotations from super class-chain
    MClass *superKlass = klass.GetSuperClass();
    for (; superKlass != nullptr; superKlass = superKlass->GetSuperClass()) {
      anno = ReflectClassGetDeclaredAnnotation(*superKlass, annotationType);
      if (anno != nullptr) {
        return anno;
      }
    }
  }

  return nullptr;
}

MObject *ReflectClassGetClasses(const MClass &classObj) {
  set<MClass*> metaList;
  AnnotationUtil::GetDeclaredClasses(const_cast<MClass*>(&classObj), metaList);
  MClass *superCl = classObj.GetSuperClass();
  while (superCl != nullptr) {
    AnnotationUtil::GetDeclaredClasses(superCl, metaList);
    superCl = superCl->GetSuperClass();
  }
  auto it = metaList.begin();
  while (it != metaList.end()) {
    if (!(*it)->IsPublic()) {
      it = metaList.erase(it);
    } else {
      ++it;
    }
  }
  uint32_t size = static_cast<uint32_t>(metaList.size());
  MArray *jarray = MArray::NewObjectArray(size, *WellKnown::GetMClassAClass());
  it = metaList.begin();
  for (uint32_t i = 0; i < size; ++i, ++it) {
    jarray->SetObjectElementNoRc(i, *it);
  }
  return jarray;
}

jobjectArray MRT_ReflectClassGetDeclaredClasses(jclass cls) {
  MClass *classObj = MClass::JniCastNonNull(cls);
  set<MClass*> *metalist = reinterpret_cast<set<MClass*>*>(AnnotationUtil::Get(kDeclaredClasses, classObj));

  uint32_t arrSize = static_cast<uint32_t>(metalist->size());
  MArray *arrayObj = MArray::NewObjectArray(arrSize, *WellKnown::GetMClassAClass());
  auto it = metalist->begin();
  for (uint32_t i = 0; i < arrSize; ++i, ++it) {
    arrayObj->SetObjectElementOffHeap(i, *it);
  }
  return arrayObj->AsJobjectArray();
}

MObject *ReflectClassGetInnerClassName(const MClass &classObj) {
  string annoStr = classObj.GetAnnotation();
  if (annoStr.empty()) {
    return nullptr;
  }
  VLOG(reflect) << "Enter __MRT_Reflect_Class_getInnerClassName, annoStr: " << annoStr << maple::endl;
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), const_cast<MClass*>(&classObj));
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t loc = parser->Find(parser->GetInnerClassStr());
  if (loc == kNPos) {
    return nullptr;
  }

  constexpr int kSteps = 4; // jump to InnerClass value
  parser->NextItem(kSteps);
  std::string retArr;
  if (parser->ParseNum(kValueInt) == kValueNull) {
    retArr = "NULL";
  } else {
    retArr = parser->ParseStr(kDefParseStrType);
  }
  MString *res = MString::InternUtf(retArr);
  return res;
}

// the following API get java/lang/reflect/Field Object, Java heap Object
// implement API for java/lang/Class Native
static void ThrowNoSuchFieldException(const MClass &classObj, const MString &fieldName) {
  std::ostringstream msg;
  std::string temp;
  classObj.GetDescriptor(temp);
  std::string fieldCharName = fieldName.GetChars();
  msg << "No field " << fieldCharName << " in class " << temp;
  MRT_ThrowNewException("java/lang/NoSuchFieldException", msg.str().c_str());
}

MObject *ReflectClassGetField(const MClass &classObj, const MString *fieldName) {
  if (UNLIKELY(fieldName == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "name == null");
    return nullptr;
  }

  FieldMeta *field = classObj.GetField(fieldName, true);
  MField *mField = nullptr;
  if (field != nullptr) {
    mField = MField::NewMFieldObject(*field);
  } else {
    ThrowNoSuchFieldException(classObj, *fieldName);
  }
  return mField;
}

MObject *ReflectClassGetPublicFieldRecursive(const MClass &classObj, const MString *fieldName) {
  if (UNLIKELY(fieldName == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "name == null");
    return nullptr;
  }

  FieldMeta *field = classObj.GetField(fieldName, true);
  MField *mField = nullptr;
  if (field != nullptr) {
    mField = MField::NewMFieldObject(*field);
  }
  return mField;
}

MObject *ReflectClassGetDeclaredFields(const MClass &classObj) {
  ScopedHandles sHandles;
  uint32_t numOfField = classObj.GetNumOfFields();
  FieldMeta *fields = classObj.GetFieldMetas();
  ObjHandle<MArray> fieldArray(MArray::NewObjectArray(numOfField, *WellKnown::GetMClassAField()));
  for (uint32_t i = 0; i < numOfField; ++i) {
    FieldMeta *field = fields + i;
    MField *mField = MField::NewMFieldObject(*field);
    if (UNLIKELY(mField == nullptr)) {
      return nullptr;
    }
    fieldArray->SetObjectElementNoRc(i, mField);
  }
  return fieldArray.ReturnObj();
}

MObject *ReflectClassGetDeclaredField(const MClass &classObj, const MString *fieldName) {
  if (UNLIKELY((fieldName) == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "name == null");
    return nullptr;
  }
  if (&classObj == WellKnown::GetMClassString() && fieldName->Cmp("value")) {
    // We log the error for this specific case, as the user might just swallow the exception.
    // This helps diagnose crashes when applications rely on the String#value field being
    // there.
    // Also print on the error stream to test it through run-test.
    std::string message("The String#value field is not present on Android versions >= 6.0");
    LOG(ERROR) << message << maple::endl;
    std::cerr << message << std::endl;
  }

  FieldMeta *fieldMeta = classObj.GetDeclaredField(fieldName);
  if ((fieldMeta != nullptr) && (&classObj == WellKnown::GetMClassClass()) && !fieldMeta->IsStatic()) {
    LOG(ERROR) << "The Class instance field is not present on maple" << maple::endl;
    fieldMeta = nullptr;
  }
  if (fieldMeta == nullptr) {
    ThrowNoSuchFieldException(classObj, *fieldName);
    return nullptr;
  }

  MField *mField = MField::NewMFieldObject(*fieldMeta);
  return mField;
}

static void GetDeclaredFields(const MClass &classObj, std::vector<FieldMeta*> &fieldsVector, bool publicOnly) {
  uint32_t numOfField = classObj.GetNumOfFields();
  FieldMeta *fields = classObj.GetFieldMetas();
  for (uint32_t i = 0; i < numOfField; ++i) {
    FieldMeta *field = fields + i;
    if (!publicOnly || field->IsPublic()) {
      fieldsVector.push_back(field);
    }
  }
}

static MObject *GetFieldsObjectArray(std::vector<FieldMeta*> fieldsVector) {
  ScopedHandles sHandles;
  uint32_t numOfFields = static_cast<uint32_t>(fieldsVector.size());
  uint32_t currentIndex = 0;
  ObjHandle<MArray> fieldArray(MArray::NewObjectArray(numOfFields, *WellKnown::GetMClassAField()));
  for (auto fieldMeta : fieldsVector) {
    MField *mField = MField::NewMFieldObject(*fieldMeta);
    if (UNLIKELY(mField == nullptr)) {
      return nullptr;
    }
    fieldArray->SetObjectElementNoRc(currentIndex++, mField);
  }
  return fieldArray.ReturnObj();
}

MObject *ReflectClassGetDeclaredFieldsUnchecked(const MClass &classObj, bool publicOnly) {
  std::vector<FieldMeta*> fieldsVector;
  GetDeclaredFields(classObj, fieldsVector, publicOnly);
  return GetFieldsObjectArray(fieldsVector);
}

static void GetPublicFieldsRecursive(const MClass &classObj, std::vector<FieldMeta*> &fieldsVector) {
  for (const MClass *super = &classObj; super != nullptr; super = super->GetSuperClass()) {
    GetDeclaredFields(*super, fieldsVector, true);
  }
  // search iftable which has a flattened and uniqued list of interfaces
  std::vector<MClass*> interfaceList;
  classObj.GetInterfaces(interfaceList);
  for (auto itfInfo : interfaceList) {
    GetDeclaredFields(*itfInfo, fieldsVector, true);
  }
}

MObject *ReflectClassGetFields(const MClass &classObj) {
  std::vector<FieldMeta*> fieldsVector;
  GetPublicFieldsRecursive(classObj, fieldsVector);
  return GetFieldsObjectArray(fieldsVector);
}

void ReflectClassGetPublicFieldsRecursive(const MClass &classObj, MObject *listObject) {
  std::vector<FieldMeta*> fieldsVector;
  GetPublicFieldsRecursive(classObj, fieldsVector);
  MClass *collectionsClass = MClass::GetClassFromDescriptor(nullptr, "Ljava/util/ArrayList;");
  MethodMeta *addMethod = collectionsClass->GetMethod("add", "(Ljava/lang/Object;)Z");
  if (UNLIKELY(addMethod == nullptr)) {
    return;
  }

  ScopedHandles sHandles;
  for (auto field : fieldsVector) {
    ObjHandle<MObject> fieldObject(MField::NewMFieldObject(*field));
    if (UNLIKELY(fieldObject() == nullptr)) {
      return;
    }

    jvalue arg[1];
    arg[0].l = fieldObject.AsJObj();
    bool isSuccess = addMethod->Invoke<bool, calljavastubconst::kJvalue>(listObject, arg);
    if (isSuccess == false) {
      return;
    }
  }
}

jobject MRT_ReflectClassGetDeclaredFields(jclass classObj, jboolean publicOnly) {
  MClass *klass = MClass::JniCastNonNull(classObj);
  MObject *ret = ReflectClassGetDeclaredFieldsUnchecked(*klass, publicOnly == JNI_TRUE);
  return ret->AsJobject();
}

MObject *ReflectClassGetPublicDeclaredFields(const MClass &classObj) {
  return ReflectClassGetDeclaredFieldsUnchecked(classObj, true);
}

// the following API get FieldMeta
uint32_t MRT_ReflectClassGetNumofFields(jclass cls) {
  MClass *mClassObj = MClass::JniCastNonNull(cls);
  return mClassObj->GetNumOfFields();
}

jfieldID MRT_ReflectClassGetFieldsPtr(jclass classObj) {
  MClass *mClassObj = MClass::JniCastNonNull(classObj);
  FieldMeta *fields = mClassObj->GetFieldMetas();
  return fields->AsJfieldID();
}

jfieldID MRT_ReflectClassGetIndexField(jfieldID head, int i) {
  FieldMeta *fieldMeta = FieldMeta::JniCast(head) + i;
  return fieldMeta->AsJfieldID();
}

// find fieldMeta recursively in classObj
jfieldID MRT_ReflectGetCharField(jclass classObj, const char *fieldName, const char *fieldType) {
  MClass *mClassObj = MClass::JniCastNonNull(classObj);
  FieldMeta *retObj = mClassObj->GetField(fieldName, fieldType, false);
  return retObj->AsJfieldID();
}

jfieldID MRT_ReflectGetStaticCharField(jclass classObj, const char *fieldName) {
  MClass *mClassObj = MClass::JniCastNonNull(classObj);
  FieldMeta *field = mClassObj->GetField(fieldName, nullptr, false);
  if (field != nullptr && field->IsStatic()) {
    return field->AsJfieldID();
  }
  return nullptr;
}

// find fieldMeta declared *just* in classObj
MObject *ReflectClassGetDeclaredMethodInternal(const MClass &classObj, const MString *methodName,
                                               const MArray *arrayClass) {
  if (UNLIKELY((methodName) == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "name == null");
    return nullptr;
  }
  MethodMeta *methodMeta = classObj.GetDeclaredMethod(methodName, arrayClass);
  if (methodMeta == nullptr) {
    return nullptr;
  }
  MMethod *methodObject = MMethod::NewMMethodObject(*methodMeta);
  return methodObject;
}

static MethodMeta *FindInterfaceMethod(const MClass &classObj, const MString *methodName, const MArray *arrayClass) {
  std::vector<MClass*> interfaceVector;
  MethodMeta *resultMethod = nullptr;
  classObj.GetInterfaces(interfaceVector);
  for (auto interface : interfaceVector) {
    MethodMeta *method = interface->GetDeclaredMethod(methodName, arrayClass);
    if ((method != nullptr) && (method->IsPublic())) {
      if (resultMethod == nullptr) {
        resultMethod = method;
      } else if (resultMethod->GetDeclaringClass()->IsAssignableFrom(*method->GetDeclaringClass())) {
        resultMethod = method;
      }
    }
  }
  return resultMethod;
}

MObject *ReflectClassFindInterfaceMethod(const MClass &classObj, const MString *methodName, const MArray *arrayClass) {
  if (UNLIKELY((methodName) == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "name == null");
    return nullptr;
  }
  MethodMeta *methodMeta = FindInterfaceMethod(classObj, methodName, arrayClass);
  if (methodMeta == nullptr) {
    return nullptr;
  }
  MMethod *methodObject = MMethod::NewMMethodObject(*methodMeta);
  return methodObject;
}

static MethodMeta *GetPublicMethodRecursive(const MClass &classObj, const MString *methodName,
                                            const MArray *arrayClass) {
  MethodMeta *method = nullptr;
  for (const MClass *superClass = &classObj; superClass != nullptr; superClass = superClass->GetSuperClass()) {
    method = superClass->GetDeclaredMethod(methodName, arrayClass);
    if ((method != nullptr) && (method->IsPublic())) {
      return method;
    }
  }
  method = FindInterfaceMethod(classObj, methodName, arrayClass);
  return method;
}

static void ThrowNoSuchMethodException(const MClass &classObj, const MString *methodName,
                                       const MArray *arrayClass, bool isInit = false) {
  std::ostringstream msg;
  std::string className;
  classObj.GetBinaryName(className);
  if (isInit) {
    msg << className << "." << "<init>" << " [";
  } else if (methodName != nullptr) {
    std::string methodCharName = methodName->GetChars();
    msg << className << "." << methodCharName << " [";
  }
  if (arrayClass != nullptr) {
    uint32_t len = arrayClass->GetLength();
    std::string name;
    for (uint32_t i = 0; i < len; ++i) {
      MClass *elementObj = arrayClass->GetObjectElementNoRc(i)->AsMClass();
      name.clear();
      elementObj->GetBinaryName(name);
      if (elementObj->IsInterface()) {
        msg << "interface ";
      } else if (!elementObj->IsPrimitiveClass()) {
        msg << "class ";
      }
      msg << name;
      if (i != (len - 1)) {
        msg << ", ";
      }
    }
  }
  msg << "]";
  MRT_ThrowNewException("java/lang/NoSuchMethodException", msg.str().c_str());
}

static MethodMeta *GetMethod(const MClass &classObj, const MString *methodName,
                             const MArray *arrayClass, bool recursive) {
  if (UNLIKELY((methodName) == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "name == null");
    return nullptr;
  }
  if (UNLIKELY((arrayClass != nullptr) && arrayClass->HasNullElement())) {
    MRT_ThrowNewException("java/lang/NoSuchMethodException", "parameter type is null");
    return nullptr;
  }

  MethodMeta *method = recursive ? GetPublicMethodRecursive(classObj, methodName, arrayClass) :
      classObj.GetDeclaredMethod(methodName, arrayClass);
  if (method != nullptr) {
    return method;
  }
  ThrowNoSuchMethodException(classObj, methodName, arrayClass);
  return nullptr;
}

MObject *ReflectClassGetMethod(const MClass &classObj, const MString *methodName, const MArray *arrayClass) {
  MethodMeta *method = GetMethod(classObj, methodName, arrayClass, true);
  if (method == nullptr) {
    return nullptr;
  }
  MMethod *methodObject = MMethod::NewMMethodObject(*method);
  return methodObject;
}

MObject *ReflectClassGetDeclaredMethod(const MClass &classObj, const MString *methodName, const MArray *arrayClass) {
  MethodMeta *methodMeta = GetMethod(classObj, methodName, arrayClass, false);
  if (methodMeta == nullptr) {
    return nullptr;
  }
  MMethod *methodObject = MMethod::NewMMethodObject(*methodMeta);
  return methodObject;
}

MObject *ReflectClassGetDeclaredMethods(const MClass &classObj) {
  MObject *methodArray = ReflectClassGetDeclaredMethodsUnchecked(classObj, false);
  return methodArray;
}

MObject *ReflectClassGetMethods(const MClass &classObj) {
  std::vector<MethodMeta*> methodsVector;
  for (const MClass *superClass = &classObj; superClass != nullptr; superClass = superClass->GetSuperClass()) {
    superClass->GetDeclaredMethods(methodsVector, true);
  }
  std::vector<MClass*> interfaceList;
  classObj.GetInterfaces(interfaceList);
  for (auto it = interfaceList.begin(); it != interfaceList.end(); ++it) {
    MClass *interface = *it;
    interface->GetDeclaredMethods(methodsVector, true);
  }

  std::vector<MethodMeta*> methodsUnique;
  for (auto mth : methodsVector) {
    char *methodName0 = mth->GetName();
    char *sigName0 = mth->GetSignature();
    auto itMethod = methodsUnique.begin();
    for (; itMethod != methodsUnique.end(); ++itMethod) {
      char *methodName1 = (*itMethod)->GetName();
      char *sigName1 = (*itMethod)->GetSignature();
      if (!strcmp(methodName0, methodName1) && !strcmp(sigName0, sigName1)) {
        break;
      }
    }
    if (itMethod == methodsUnique.end()) {
      methodsUnique.push_back(mth);
    }
  }
  ScopedHandles sHandles;
  uint32_t numOfMethod = static_cast<uint32_t>(methodsUnique.size());
  ObjHandle<MArray> methodArray(MArray::NewObjectArray(numOfMethod, *WellKnown::GetMClassAMethod()));
  uint32_t currentIndex = 0;
  for (auto methodMeta : methodsUnique) {
    MMethod *methodObject = MMethod::NewMMethodObject(*methodMeta);
    if (UNLIKELY(methodObject == nullptr)) {
      return nullptr;
    }
    methodArray->SetObjectElementNoRc(currentIndex++, methodObject);
  }
  return methodArray.ReturnObj();
}

void ReflectClassGetPublicMethodsInternal(const MClass &classObj, MObject *listObject) {
  std::vector<MethodMeta*> methodsVector;
  for (const MClass *superClass = &classObj; superClass != nullptr; superClass = superClass->GetSuperClass()) {
    superClass->GetDeclaredMethods(methodsVector, true);
  }
  std::vector<MClass*> interfaceList;
  classObj.GetInterfaces(interfaceList);
  for (auto interface : interfaceList) {
    interface->GetDeclaredMethods(methodsVector, true);
  }
  MClass *collectionsClass = MClass::GetClassFromDescriptor(nullptr, "Ljava/util/ArrayList;");
  MethodMeta *addMethod = collectionsClass->GetMethod("add", "(Ljava/lang/Object;)Z");
  if (UNLIKELY(addMethod == nullptr)) {
    return;
  }
  ScopedHandles sHandles;
  for (auto m : methodsVector) {
    ObjHandle<MObject> methodObject(MMethod::NewMMethodObject(*m));
    if (UNLIKELY(methodObject() == nullptr)) {
      return;
    }
    jvalue arg[1];
    arg[0].l = methodObject.AsJObj();
    bool isSuccess = addMethod->Invoke<bool, calljavastubconst::kJvalue>(listObject, arg);
    if (isSuccess == false) {
      return;
    }
  }
}

MObject *ReflectClassGetDeclaredMethodsUnchecked(const MClass &classObj, bool publicOnly) {
  ScopedHandles sHandles;
  std::vector<MethodMeta*> methodsVector;
  classObj.GetDeclaredMethods(methodsVector, publicOnly);
  uint32_t numOfMethod = static_cast<uint32_t>(methodsVector.size());
  ObjHandle<MArray> methodArray(MArray::NewObjectArray(numOfMethod, *WellKnown::GetMClassAMethod()));
  uint32_t currentIndex = 0;
  for (auto methodMeta : methodsVector) {
    MMethod *methodObject = MMethod::NewMMethodObject(*methodMeta);
    if (UNLIKELY(methodObject == nullptr)) {
      return nullptr;
    }
    methodArray->SetObjectElementNoRc(currentIndex++, methodObject);
  }
  return methodArray.ReturnObj();
}

jobject MRT_ReflectClassGetDeclaredMethods(jclass classObj, jboolean publicOnly) {
  MClass *klass = MClass::JniCastNonNull(classObj);
  MObject *m = ReflectClassGetDeclaredMethodsUnchecked(*klass, publicOnly == JNI_TRUE);
  return m->AsJobject();
}

MObject *ReflectClassGetInstanceMethod(const MClass &classObj, const MString *methodName, const MArray *arrayClass) {
  if (UNLIKELY((methodName) == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "name == null");
    return nullptr;
  }
  MethodMeta *method = nullptr;
  for (const MClass *superClass = &classObj; superClass != nullptr; superClass = superClass->GetSuperClass()) {
    method = superClass->GetDeclaredMethod(methodName, arrayClass);
    if ((method != nullptr) && (!method->IsStatic()) && (!method->IsConstructor())) {
      MMethod *methodObject = MMethod::NewMMethodObject(*method);
      return methodObject;
    }
  }
  method = FindInterfaceMethod(classObj, methodName, arrayClass);
  if (method == nullptr) {
    return nullptr;
  }
  MMethod *methodObject = MMethod::NewMMethodObject(*method);
  return methodObject;
}

MObject *ReflectClassGetDeclaredConstructorInternal(const MClass &classObj, const MArray *arrayClass) {
  MethodMeta *constructor = classObj.GetDeclaredConstructor(arrayClass);
  if (constructor == nullptr) {
    return nullptr;
  }
  MMethod *methodObject = MMethod::NewMMethodObject(*constructor);
  return methodObject;
}

MObject *ReflectClassGetDeclaredConstructor(const MClass &classObj, const MArray *arrayClass) {
  if (UNLIKELY((arrayClass != nullptr) && arrayClass->HasNullElement())) {
    MRT_ThrowNewException("java/lang/NoSuchMethodException", "parameter type is null");
    return nullptr;
  }

  MethodMeta *method = classObj.GetDeclaredConstructor(arrayClass);
  if (method != nullptr) {
    MMethod *methodObject = MMethod::NewMMethodObject(*method);
    return methodObject;
  }

  ThrowNoSuchMethodException(classObj, nullptr, arrayClass, true);
  return nullptr;
}

static MObject *GetDeclaredConstructors(const MClass &classObj, bool publicOnly) {
  std::vector<MethodMeta*> constructorsVector;
  uint32_t numOfMethod = classObj.GetNumOfMethods();
  MethodMeta *methodS = classObj.GetMethodMetas();
  for (uint32_t i = 0; i < numOfMethod; ++i) {
    MethodMeta *method = &methodS[i];
    if (method->IsConstructor() && !method->IsStatic()) {
      if (!publicOnly || (method->IsPublic())) {
        constructorsVector.push_back(method);
      }
    }
  }
  ScopedHandles sHandles;
  numOfMethod = static_cast<uint32_t>(constructorsVector.size());
  ObjHandle<MArray> constructorArray(MArray::NewObjectArray(numOfMethod, *WellKnown::GetMClassAConstructor()));
  uint32_t currentIndex = 0;
  for (auto methodMeta : constructorsVector) {
    MMethod *constructorObject = MMethod::NewMMethodObject(*methodMeta);
    if (UNLIKELY(constructorObject == nullptr)) {
      return nullptr;
    }
    constructorArray->SetObjectElementNoRc(currentIndex++, constructorObject);
  }
  return constructorArray.ReturnObj();
}

MObject *ReflectClassGetDeclaredConstructorsInternal(const MClass &classObj, bool publicOnly) {
  MObject *constructorArray = GetDeclaredConstructors(classObj, publicOnly);
  return constructorArray;
}

MObject *ReflectClassGetDeclaredConstructors(const MClass &classObj) {
  MObject *constructorArray = GetDeclaredConstructors(classObj, false);
  return constructorArray;
}

jobject MRT_ReflectClassGetDeclaredConstructors(jclass classObj, jboolean publicOnly) {
  MClass *mClassObj = MClass::JniCastNonNull(classObj);
  MObject *ret = GetDeclaredConstructors(*mClassObj, publicOnly == JNI_TRUE);
  return ret->AsJobject();
}

MObject *ReflectClassGetConstructor(const MClass &classObj, const MArray *arrayClass) {
  if (UNLIKELY((arrayClass != nullptr) && arrayClass->HasNullElement())) {
    MRT_ThrowNewException("java/lang/NoSuchMethodException", "parameter type is null");
    return nullptr;
  }

  MethodMeta *constructor = classObj.GetDeclaredConstructor(arrayClass);
  if ((constructor != nullptr) && (constructor->IsPublic())) {
    MMethod *constructorObject = MMethod::NewMMethodObject(*constructor);
    return constructorObject;
  }

  ThrowNoSuchMethodException(classObj, nullptr, arrayClass, true);
  return nullptr;
}

MObject *ReflectClassGetConstructors(const MClass &classObj) {
  MObject *constructorArray = GetDeclaredConstructors(classObj, true);
  return constructorArray;
}

jmethodID MRT_ReflectGetCharMethod(jclass classObj, const char *methodName, const char *signatureName) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  MethodMeta *methodMeta = mClass->GetMethod(methodName, signatureName);
  return methodMeta->AsJmethodID();
}

jmethodID MRT_ReflectGetMethodFromMethodID(jclass clazz, jmethodID methodID, const char *signature ATTR_UNUSED) {
  MethodMeta *methodMeta = MethodMeta::JniCastNonNull(methodID);
  MClass *mClass = MClass::JniCastNonNull(clazz);
  methodMeta = mClass->GetVirtualMethod(*methodMeta);
  CHECK(methodMeta != nullptr);
  return methodMeta->AsJmethodID();
}

jmethodID MRT_ReflectGetStaticCharMethod(jclass classObj, const char *methodName, const char *signatureName) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  MethodMeta *methodMeta = mClass->GetMethod(methodName, signatureName);
  if (methodMeta != nullptr && (methodMeta->IsStatic())) {
    return methodMeta->AsJmethodID();
  }
  return nullptr;
}

jint MRT_ReflectClassGetAccessFlags(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  uint32_t mod = mClass->GetModifier();
  return static_cast<jint>(mod & 0x7FFFFFFF);
}

jclass MRT_ReflectClassGetComponentType(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  MClass *component = mClass->GetComponentClass();
  return component->AsJclass();
}

jclass MRT_ReflectClassGetSuperClass(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  MClass *supercls = mClass->GetSuperClass();
  return supercls->AsJclass();
}

static MArray *GetInterfacesInternal(const MClass &klass) {
  uint32_t numOfInterface = klass.GetNumOfInterface();
  MClass *interfaceVector[numOfInterface];
  klass.GetDirectInterfaces(interfaceVector, numOfInterface);
  MArray *interfacesArray = MArray::NewObjectArray(numOfInterface, *WellKnown::GetMClassAClass());
  for (uint32_t index = 0; index < numOfInterface; ++index) {
    interfacesArray->SetObjectElementOffHeap(index, interfaceVector[index]);
  }
  return interfacesArray;
}

jobjectArray MRT_ReflectClassGetInterfaces(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  MArray *interfacesArray = GetInterfacesInternal(*mClass);
  return interfacesArray->AsJobjectArray();
}

MObject *ReflectClassGetInterfacesInternal(const MClass &classObj) {
  MArray *interfacesArray = nullptr;
  if (!classObj.IsArrayClass()) {
    uint32_t numOfInterface = classObj.GetNumOfInterface();
    if (numOfInterface != 0) {
      interfacesArray = GetInterfacesInternal(classObj);
    }
  }
  return interfacesArray;
}

jint MRT_ReflectClassGetModifiers(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  const uint32_t kJavaFlagsMask = 0xFFFF;
  return mClass->GetModifier() & kJavaFlagsMask;
}

jstring MRT_ReflectClassGetName(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  MString *res = NewStringUtfFromPoolForClassName(*mClass);
  return res->AsJstring();
}

MObject *ReflectClassGetSignatureAnnotation(const MClass &classObj) {
  MObject *ret = classObj.GetSignatureAnnotation();
  return ret;
}

MObject *ReflectClassGetEnclosingMethodNative(const MClass &classObj) {
  MethodMeta *ret = MethodMeta::Cast(AnnotationUtil::Get(kEnclosingMethod, const_cast<MClass*>(&classObj)));
  if ((ret != nullptr) && (!ret->IsConstructor())) {
    MMethod *methodObj = MMethod::NewMMethodObject(*ret);
    return methodObj;
  }
  return nullptr;
}

MObject *ReflectClassGetEnclosingConstructorNative(const MClass &classObj) {
  MethodMeta *mthMeta = MethodMeta::Cast(AnnotationUtil::Get(kEnclosingMethod, const_cast<MClass*>(&classObj)));
  if ((mthMeta != nullptr) && (mthMeta->IsConstructor())) {
    MMethod *methodObj = MMethod::NewMMethodObject(*mthMeta);
    return methodObj;
  }
  return nullptr;
}

MObject *ReflectClassGetEnclosingClass(const MClass &classObj) {
  MObject *result = MObject::Cast<MObject>(AnnotationUtil::Get(kEnclosingClass, const_cast<MClass*>(&classObj)));
  return result;
}

jclass MRT_ReflectClassGetDeclaringClass(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  if (mClass->IsAnonymousClass() || mClass->IsProxy()) {
    VLOG(reflect) << "Enter MRT_ReflectClassGetDeclaringClass, return NULL " << maple::endl;
    return nullptr;
  }
  return MObject::Cast<MClass>(AnnotationUtil::Get(kDeclaringClass, mClass))->AsJclass();
}

bool ReflectClassIsMemberClass(const MClass &classObj) {
  char *annotation = classObj.GetRawAnnotation();
  bool isValid = false;
  bool result = (annotation != nullptr) ? AnnoParser::IsMemberClass(annotation, isValid) :
      AnnoParser::IsMemberClass("", isValid);
  if (isValid) {
    return result;
  }
  return MRT_ReflectClassGetDeclaringClass(classObj) != nullptr;
}


bool ReflectClassIsLocalClass(const MClass &classObj) {
  if (classObj.IsAnonymousClass()) {
    return false;
  }
  bool isValid = modifier::IsLocalClassVaild(classObj.GetModifier());
  if (isValid) {
    return modifier::IsLocalClass(classObj.GetModifier());
  }
  MObject *enclosingMethod = ReflectClassGetEnclosingMethodNative(classObj);
  if (enclosingMethod != nullptr) {
    MRT_DecRef(reinterpret_cast<address_t>(enclosingMethod));
    return true;
  }
  MObject *enclosingConstructor = ReflectClassGetEnclosingConstructorNative(classObj);
  if (enclosingConstructor != nullptr) {
    MRT_DecRef(reinterpret_cast<address_t>(enclosingConstructor));
    return true;
  }
  return false;
}

MObject *ReflectClassGetClassLoader(const MClass &classObj) {
  jobject classLoader = MRT_GetClassLoader(reinterpret_cast<jclass>(&const_cast<MClass&>(classObj)));
  if (classLoader == nullptr) {
    classLoader = MRT_GetBootClassLoader();
  }
  if (classLoader != nullptr) {
    RC_LOCAL_INC_REF(classLoader);
  }
  return MObject::JniCast(classLoader); // inc ref by runtime and dec ref by caller
}

static bool CheckNewInstanceAccess(const MClass &classObj) {
  uint32_t mod = classObj.GetModifier();
  uint32_t flag = classObj.GetFlag();
  if (modifier::IsAbstract(mod) || modifier::IsInterface(mod) ||
      modifier::IsArrayClass(flag) || modifier::IsPrimitiveClass(flag)) {
    std::string classPrettyName;
    std::ostringstream msg;
    classObj.GetPrettyClass(classPrettyName);
    msg << classPrettyName << " cannot be instantiated";
    MRT_ThrowNewException("java/lang/InstantiationException", msg.str().c_str());
    return false;
  }
  // check access the class.
  if (!modifier::IsPublic(mod)) {
    MClass *callerClass = reflection::GetCallerClass(1);
    if (callerClass && !reflection::CanAccess(classObj, *callerClass)) {
      std::string callerClassStr, classObjStr, msg;
      callerClass->GetPrettyClass(callerClassStr);
      classObj.GetPrettyClass(classObjStr);
      msg = classObjStr + " is not accessible from " + callerClassStr;
      MRT_ThrowNewException("java/lang/IllegalAccessException", msg.c_str());
      return false;
    }
  }
  return true;
}

MObject *ReflectClassNewInstance(const MClass &classObj) {
  if (!CheckNewInstanceAccess(classObj)) {
    return nullptr;
  }

  if (UNLIKELY(!classObj.InitClassIfNeeded())) {
    return nullptr;
  }

  // find constructor, an empty argument list
  MethodMeta *constructor = classObj.GetDefaultConstructor();
  if (UNLIKELY(constructor == nullptr)) {
    std::string classPrettyName;
    std::ostringstream msg;
    classObj.GetPrettyClass(classPrettyName);
    msg << classPrettyName << " has no zero argument constructor";
    MRT_ThrowNewException("java/lang/InstantiationException", msg.str().c_str());
    return nullptr;
  }

  // Invoke the string allocator to return an empty string for the string class.
  if (classObj.IsStringClass()) {
    return MString::NewEmptyStringObject();
  }

  {
    ScopedHandles sHandles;
    ObjHandle<MObject> newInstance(MObject::NewObject(classObj));
    // Verify that we can access the constructor.
    if (!constructor->IsPublic()) {
      MClass *callerClass = nullptr;
      if (!reflection::VerifyAccess(newInstance(), &classObj, constructor->GetMod(), callerClass, 1)) {
        std::string constructorStr, msg, callerClassStr;
        constructor->GetPrettyName(true, constructorStr);
        callerClass->GetPrettyClass(callerClassStr);
        msg = constructorStr + " is not accessible from " + callerClassStr;
        MRT_ThrowNewException("java/lang/IllegalAccessException", msg.c_str());
        return nullptr;
      }
    }

     // constructor no return value
    (void)constructor->InvokeJavaMethodFast<int>(newInstance());
    if (!MRT_HasPendingException()) {
      return newInstance.ReturnObj();
    }
  }

  if (UNLIKELY(MRT_HasPendingException())) {
    MRT_CheckThrowPendingExceptionUnw();
  }
  return nullptr;
}

jboolean MRT_ReflectClassIsPrimitive(jclass classObj) {
  MClass *mClassObj = MClass::JniCastNonNull(classObj);
  return mClassObj->IsPrimitiveClass();
}

jboolean MRT_ReflectClassIsInterface(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  return mClass->IsInterface();
}

jboolean MRT_ReflectClassIsArray(jclass classObj) {
  MClass *mClass = MClass::JniCastNonNull(classObj);
  return mClass->IsArrayClass();
}

jboolean MRT_ReflectClassIsAssignableFrom(jclass superClass, jclass subClass) {
  MClass *mSuperClass = MClass::JniCastNonNull(superClass);
  MClass *mSubClass = MClass::JniCastNonNull(subClass);
  return mSuperClass->IsAssignableFrom(*mSubClass);
}

#if PLATFORM_SDK_VERSION >= 27
MObject *ReflectClassGetPrimitiveClass(const MClass &classObj __attribute__((unused)), const MString *name) {
  if (name == nullptr) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  std::string className = name->GetChars();
  char hash = className[0] ^ ((className[1] & 0x10) << 1);
  switch (hash) {
    case 'i':
      return WellKnown::GetMClassI();
    case 'f':
      return WellKnown::GetMClassF();
    case 'B':
      return WellKnown::GetMClassB();
    case 'c':
      return WellKnown::GetMClassC();
    case 's':
      return WellKnown::GetMClassS();
    case 'l':
      return WellKnown::GetMClassJ();
    case 'd':
      return WellKnown::GetMClassD();
    case 'b':
      return WellKnown::GetMClassZ();
    case 'v':
      return WellKnown::GetMClassV();
    default:
      MRT_ThrowNewException("java/lang/ClassNotFoundException", nullptr);
  }
  BUILTIN_UNREACHABLE();
}
#endif

jboolean MRT_ReflectIsString(const jclass classObj) {
  return MClass::JniCastNonNull(classObj)->IsStringClass();
}

jboolean MRT_ReflectIsClass(const jobject obj) {
  return MObject::JniCastNonNull(obj)->IsClass() ? JNI_TRUE : JNI_FALSE;
}

jboolean MRT_ReflectIsInstanceOf(jobject jobj, jclass javaClass) {
  MObject *mObj = MObject::JniCastNonNull(jobj);
  MClass *mJavaClass = MClass::JniCastNonNull(javaClass);
  return mObj->IsInstanceOf(*mJavaClass) ? JNI_TRUE : JNI_FALSE;
}

jint MRT_ReflectGetObjSize(const jclass classObj) {
  return static_cast<jint>(MClass::JniCastNonNull(classObj)->GetObjectSize());
}

jint MRT_ReflectGetArrayIndexScaleForComponentType(jclass componentClassObj) {
  MClass *classObj = MClass::JniCastNonNull(componentClassObj);
  return classObj->IsPrimitiveClass() ? classObj->GetObjectSize() : MObject::GetReffieldSize();
}

jclass MRT_ReflectGetOrCreateArrayClassObj(jclass elementClass) {
  MClass *elementCls = MClass::JniCastNonNull(elementClass);
  MClass *arrayClass = maplert::WellKnown::GetCacheArrayClass(*elementCls);
  return arrayClass->AsJclass();
}

char *MRT_ReflectGetClassCharName(const jclass javaClass) {
  return MClass::JniCastNonNull(javaClass)->GetName();
}

jobject MRT_ReflectAllocObject(const jclass javaClass, bool isJNI) {
  MClass *mClass = MClass::JniCastNonNull(javaClass);
  MObject *o = MObject::NewObject(*mClass, isJNI);
  return o->AsJobject();
}

jobject MRT_ReflectNewObjectA(const jclass javaClass, const jmethodID mid, const jvalue *args, bool isJNI) {
  const MethodMeta *constructor = MethodMeta::JniCastNonNull(mid);
  const MClass *klass = MClass::JniCastNonNull(javaClass);
  MObject *obj = MObject::NewObject(*klass, *constructor, *args, isJNI);
  if (obj == nullptr) {
    return nullptr;
  }
  return obj->AsJobject();
}

#ifdef DISABLE_MCC_FAST_FUNCS
// used in compiler, be careful if change name
void MCC_Array_Boundary_Check(jobjectArray javaArray, jint index) {
  if (javaArray == nullptr) {
    MRT_ThrowNullPointerExceptionUnw();
    return;
  }
  MArray *mArrayObj = MArray::JniCast(javaArray);
  int32_t length = static_cast<int32_t>(mArrayObj->GetLength());
  if (index < 0) {
    MRT_ThrowArrayIndexOutOfBoundsException(length, index);
    return;
  } else if (index >= length) {
    MRT_ThrowArrayIndexOutOfBoundsException(length, index);
    return;
  }
}
#endif // DISABLE_MCC_FAST_FUNCS

void MCC_ThrowCastException(jclass targetClass, jobject castObj) {
  std::ostringstream msg;
  MClass *mTargetClass = MClass::JniCast(targetClass);
  MObject *mCastObj = MObject::JniCast(castObj);
  if (mCastObj == nullptr || mTargetClass == nullptr) {
    return;
  }
  std::string targetClassName, objClassName;
  mTargetClass->GetTypeName(targetClassName);
  mCastObj->GetClass()->GetTypeName(objClassName);
  msg << objClassName << " cannot be cast to " << targetClassName;
  MRT_ThrowClassCastExceptionUnw(msg.str().c_str());
}

static void ReflectThrowCastException(const MClass *sourceInfo, const MObject &targetObject, int dim) {
  std::ostringstream msg;
  if (sourceInfo != nullptr) {
    std::string sourceName, targerName;
    sourceInfo->GetTypeName(sourceName);
    while (dim--) {
      sourceName += "[]";
    }
    targetObject.GetClass()->GetTypeName(targerName);
    msg << targerName << " cannot be cast to " << sourceName;
  }
  MRT_ThrowClassCastExceptionUnw(msg.str().c_str());
}

void MCC_Reflect_ThrowCastException(const jclass sourceinfo, jobject targetObject, jint dim) {
  MObject *mTargetObject = MObject::JniCast(targetObject);
  if (mTargetObject == nullptr) {
    return;
  }
  ReflectThrowCastException(MClass::JniCast(sourceinfo), *mTargetObject, dim);
}

void MCC_Reflect_Check_Arraystore(jobject arrayObject, jobject elemObject){
  if (UNLIKELY(elemObject == nullptr || arrayObject == nullptr)) {
    return;
  }
  MObject *mArrayObject = MObject::JniCast(arrayObject);
  MObject *mElemObject = MObject::JniCast(elemObject);
  MClass *arryElemInfo = mArrayObject->GetClass()->GetComponentClass();
  if (arryElemInfo == nullptr) {
    return;
  }
  if (!mElemObject->IsInstanceOf(*arryElemInfo)) {
    std::string elemName;
    mElemObject->GetClass()->GetTypeName(elemName);
    MRT_ThrowArrayStoreExceptionUnw(elemName.c_str());
  }
}

void MCC_Reflect_Check_Casting_Array(jclass sourceClass, jobject targetObject, jint arrayDim) {
  if (UNLIKELY(targetObject == nullptr || sourceClass == nullptr)) {
    return;
  }
  MClass *sourceInfo = MClass::JniCast(sourceClass);
  MObject *mTargetObject = MObject::JniCast(targetObject);
  MClass *targetInfo = mTargetObject->GetClass();
  int dim = arrayDim;
  while (arrayDim--) {
    uint32_t targetFlag = targetInfo->GetFlag();
    if (!modifier::IsArrayClass(targetFlag)) {
      ReflectThrowCastException(sourceInfo, *mTargetObject, dim);
    }
    targetInfo = targetInfo->GetComponentClass();
  }
  DCHECK(targetInfo != nullptr) << "MCC_Reflect_Check_Casting_Array: targetInfo is nullptr!" << maple::endl;
  if (!sourceInfo->IsAssignableFrom(*targetInfo)) {
    ReflectThrowCastException(sourceInfo, *mTargetObject, dim);
  }
}

void MCC_Reflect_Check_Casting_NoArray(jclass sourceClass, jobject targetObject) {
  MClass *sourceInfo = MClass::JniCastNonNull(sourceClass);
  MObject *mTargetObject = MObject::JniCastNonNull(targetObject);

  if (!mTargetObject->IsInstanceOf(*sourceInfo)) {
    ReflectThrowCastException(sourceInfo, *mTargetObject, 0);
  }
  return;
}

// used in compiler, be careful if change name
jboolean MCC_Reflect_IsInstance(jobject sourceClass, jobject targetObject) {
  if (sourceClass == nullptr || targetObject == nullptr) {
    return JNI_FALSE;
  }
  MObject *o = MObject::JniCastNonNull(targetObject);
  MClass *c = MClass::JniCastNonNull(sourceClass);
  return o->IsInstanceOf(*c);
}

extern "C"
uintptr_t MCC_getFuncPtrFromItabSlow64(const MObject *obj, uintptr_t hashCode,
                                       uintptr_t secondHashCode, const char *signature) {
  if (UNLIKELY(obj == nullptr)) {
    ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassNullPointerException(), "unknown reason");
    return 0;
  }

  const MClass *klass = obj->GetClass();
  auto itab = reinterpret_cast<uintptr_t*>(klass->GetItab());
  auto addr = reinterpret_cast<uintptr_t*>(itab + hashCode);
  if (*addr != 0) {
    return *addr;
  } else {
    return MCC_getFuncPtrFromItabSecondHash64(itab, secondHashCode, signature);
  }
}

static uintptr_t SearchConflictTable64(const uintptr_t *itabConflictBegin, const char *signature) {
  constexpr uint8_t addrOffset = 2;
  constexpr uint8_t slotSize = 2;
  // search conflict table
  const uintptr_t *conflict =
      itabConflictBegin + (((*itabConflictBegin) & kLowBitOfItabLength) * slotSize) + addrOffset;
#if defined(__aarch64__)
  // Get the high 32bit and set the highest bit zero
  uintptr_t conflictLength = (itabConflictBegin[0] & kHighBitOfItabLength) >> 32;
#elif defined(__arm__)
  // Get the high 16bit and set the highest bit zero
  uintptr_t conflictLength = (itabConflictBegin[0] & kHighBitOfItabLength) >> 16;
#endif
  for (uintptr_t index = 0; index < conflictLength * slotSize; index += slotSize) {
    if (!strcmp(signature, reinterpret_cast<char*>(*(conflict + index)))) {
      uintptr_t addr = conflict[index + 1];
      return addr;
    }
  }
  MRT_ThrowNewExceptionUnw("java/lang/AbstractMethodError", signature);
  return 0;
}

extern "C"
uintptr_t MCC_getFuncPtrFromItabSecondHash64(const uintptr_t *itab, uintptr_t hashCode, const char *signature) {
  DCHECK(itab != nullptr) << "MCC_getFuncPtrFromItabSecondHash64: itab is nullptr!" << maple::endl;
  DCHECK(signature != nullptr) << "MCC_getFuncPtrFromItabSecondHash64: signature is nullptr!" << maple::endl;
  constexpr uint8_t itabConflictBeginOffset = 2;
  constexpr uint8_t slotSize = 2;
  auto itabConflictBegin = reinterpret_cast<uintptr_t*>(*(itab + kItabFirstHashSize));
  auto itabActualBegin = itabConflictBegin + itabConflictBeginOffset;
  if (itabConflictBegin == nullptr) {
    MRT_ThrowNewExceptionUnw("java/lang/AbstractMethodError", signature);
    return 0;
  }

  // search normal
  uintptr_t low = 0;
  // Get the low 32bit
  uintptr_t high = itabConflictBegin[0] & kLowBitOfItabLength;
  uintptr_t index = 0;
  while (low <= high) {
    index = (low + high) >> 1;
    uintptr_t srchash = itabActualBegin[index << 1];
    if (srchash == hashCode) {
      // find
      break;
    }
    if (srchash < hashCode) {
      low = index + 1;
    } else {
      high = index - 1;
    }
  }
  uintptr_t addr = itabActualBegin[index * slotSize + 1];
  if (LIKELY(addr != 1)) {
    return addr;
  }
  return SearchConflictTable64(itabConflictBegin, signature);
}

#ifndef USE_ARM32_MACRO
#ifdef USE_32BIT_REF
extern "C"
uintptr_t MCC_getFuncPtrFromItabInlineCache(uint64_t *cacheEntryAddr, const MClass *klass,
                                            uint32_t hashCode, uint32_t secondHashCode, const char *signature) {
  if (UNLIKELY(cacheEntryAddr == nullptr || klass == nullptr || signature == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "cacheEntryAddr or klass or signature == null");
    return 0;
  }
  uint32_t *itab = reinterpret_cast<uint32_t*>(klass->GetItab());
  uint32_t *addr = itab + hashCode;
  uint64_t result = *addr;
  if (result == 0) {
#if defined(__arm__)
    result = MCC_getFuncPtrFromItabSecondHash32(itab, secondHashCode, signature);
#else  // ~__arm__
    result = MCC_getFuncPtrFromItab(itab, secondHashCode, signature);
#endif  // ~__arm__
  }
  *cacheEntryAddr = (reinterpret_cast<uint64_t>(klass)) | (result << leftShift32Bit);
  return static_cast<uintptr_t>(result);
}
#endif  // ~USE_32BIT_REF
#endif  // ~USE_ARM32_MACRO

extern "C"
uintptr_t MCC_getFuncPtrFromItabSlow32(const MObject *obj, uint32_t hashCode, uint32_t secondHashCode,
                                       const char *signature) {
  if (UNLIKELY(obj == nullptr)) {
    ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassNullPointerException(), "unknown reason");
    return 0;
  }

  const MClass *klass = obj->GetClass();
  uint32_t *itab = reinterpret_cast<uint32_t*>(klass->GetItab());
  uint32_t *addr = itab + hashCode;
  if (*addr != 0) {
    return *addr;
  } else {
    return MCC_getFuncPtrFromItabSecondHash32(itab, secondHashCode, signature);
  }
}

static uintptr_t SearchConflictTable32(const uint32_t *itabConflictBegin, const char *signature) {
  constexpr uint8_t addrOffset = 2;
  constexpr uint8_t slotSize = 2;
  // search conflict table
  uint32_t conflictIndex = ((*itabConflictBegin) & static_cast<uint32_t>(kLowBitOfItabLength)) * slotSize + addrOffset;
  auto conflict = itabConflictBegin + conflictIndex;
  uint32_t conflictLength = 0;
  // This check is for compatible the old version
  if (itabConflictBegin[0] & 0x80000000) {
    conflictLength =  (itabConflictBegin[0] & kHighBitOfItabLength) >> 16; // high 16 bit is conflictLength
  } else {
    // The max value
    conflictLength =  0x7fffffff;
  }
  for (uint32_t index = 0; index < conflictLength * slotSize; index += slotSize) {
    if (!strcmp(signature, reinterpret_cast<char*>(*(conflict + index)))) {
      return *(conflict + index + 1);
    }
  }
  MRT_ThrowNewExceptionUnw("java/lang/AbstractMethodError", signature);
  return 0;
}

extern "C"
uintptr_t MCC_getFuncPtrFromItabSecondHash32(const uint32_t *itab, uint32_t hashCode, const char *signature) {
  DCHECK(itab != nullptr) << "MCC_getFuncPtrFromItabSecondHash32: itab is nullptr!" << maple::endl;
  DCHECK(signature != nullptr) << "MCC_getFuncPtrFromItabSecondHash32: signature is nullptr!" << maple::endl;
  constexpr uint8_t itabConflictBeginOffset = 2;
  constexpr uint8_t slotSize = 2;
  uint32_t *itabConflictBegin = reinterpret_cast<uint32_t*>(*(itab + kItabFirstHashSize));
  uint32_t *itabActualBegin = itabConflictBegin + itabConflictBeginOffset;
  if (itabConflictBegin == nullptr) {
    MRT_ThrowNewExceptionUnw("java/lang/AbstractMethodError", signature);
    return 0;
  }
  // search normal
  uint32_t low = 0;
  uint32_t high = itabConflictBegin[0] & kLowBitOfItabLength;
  uint32_t index = 0;
  while (low <= high) {
    index = (low + high) >> 1;
    uint32_t srchash = itabActualBegin[index << 1];
    if (srchash == hashCode) {
      break;
    }
    (srchash < hashCode) ? (low = index + 1) : (high = index - 1);
  }
  uint32_t actualIndex = index * slotSize + 1;
  uint32_t retFunc = *(itabActualBegin + actualIndex);
  if (LIKELY(retFunc != 1)) {
    return retFunc;
  }
  return SearchConflictTable32(itabConflictBegin, signature);
}

#if defined(__arm__)
extern "C" uintptr_t MCC_getFuncPtrFromItab(const uint32_t *itab, const char *signature, uint32_t hashCode) {
  return MCC_getFuncPtrFromItabSecondHash32(itab, hashCode, signature);
}
#endif

template<typename T>
static uintptr_t GetFuncPtrFromVtab(const MObject *obj, uint32_t offset) {
  if (obj == nullptr) {
    MRT_ThrowNullPointerExceptionUnw();
    return 0;
  }

  const MClass *klass = obj->GetClass();
  T *vtab = reinterpret_cast<T*>(klass->GetVtab());
  T *funcAddr = vtab + offset;
  return *funcAddr;
}

extern "C" uintptr_t MCC_getFuncPtrFromVtab64(const MObject *obj, uint32_t offset) {
  return GetFuncPtrFromVtab<uintptr_t>(obj, offset);
}

extern "C" uintptr_t MCC_getFuncPtrFromVtab32(const MObject *obj, uint32_t offset) {
  return GetFuncPtrFromVtab<uint32_t>(obj, offset);
}

jboolean MRT_ReflectIsInit(const jclass classObj) {
  return (MRT_ClassInitialized(classObj) ? JNI_TRUE : JNI_FALSE);
}

jboolean MRT_ReflectInitialized(jclass classObj) {
  MClass *classInfo = MClass::JniCastNonNull(classObj);
  if (classInfo == nullptr) {
    LOG(ERROR) << "NULL class object!" << maple::endl;
    return JNI_FALSE;
  }
  ClassInitState state = MRT_TryInitClass(*classInfo);
  if ((state == kClassUninitialized) || (state == kClassInitFailed)) {
    LOG(ERROR) << "MRT_ReflectInitialized failed! class: " << classInfo->GetName() << maple::endl;
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

bool MRT_IsValidOffHeapObject(jobject obj __MRT_UNUSED) {
#if __MRT_DEBUG
  MObject *mObj = MObject::JniCastNonNull(obj);
  MClass *clazz = mObj->GetClass();
  // NEED: using interface from perm-space allocator
#ifdef __ANDROID__
  return ((clazz == WellKnown::GetMClassString()) ||
          (MRT_IsMetaObject(obj)) ||
          (MRT_IsPermJavaObj(mObj->AsUintptr())));
#else // !__ANDROID__
  return ((MRT_IsMetaObject(obj)) ||
          (clazz == WellKnown::GetMClassString()) ||
          (clazz == WellKnown::GetMClassAI()) ||
          (MRT_IsPermJavaObj(mObj->AsUintptr()))); // used by "native_binding_utils.cpp"
#endif // __ANDROID__
#else
  return true;
#endif // __MRT_DEBUG
}

bool MRT_IsMetaObject(jobject obj) {
  MObject *mObj = MObject::JniCastNonNull(obj);
  return mObj->GetClass() == WellKnown::GetMClassClass();
}

bool MRT_IsValidClass(jclass jclazz) {
  MClass *mClassObj = MClass::JniCastNonNull(jclazz);
  return mClassObj->GetClass() == WellKnown::GetMClassClass();
}

bool MRT_IsValidMethod(jmethodID jmid) {
  MethodMeta *methodMeta = MethodMeta::JniCastNonNull(jmid);
  MClass *declaringClass = methodMeta->GetDeclaringClass();
  return declaringClass->GetClass() == WellKnown::GetMClassClass();
}

bool MRT_IsValidField(jfieldID jfid) {
  FieldMeta *fieldMeta = FieldMeta::JniCastNonNull(jfid);
  MClass *declaringClass = fieldMeta->GetDeclaringclass();
  return declaringClass->GetClass() == WellKnown::GetMClassClass();
}

// guard some macro
#if defined(__aarch64__)
GUARD_OFFSETOF_MEMBER(ClassMetadata, monitor, kLockWordOffset);
#elif defined(__arm__)
#endif

static std::map<MString*, int, MStringMapNotEqual> stringToInt;
void ArrayMapStringIntInit () {
}

void MCC_ArrayMap_String_Int_put(jstring key, jint value) {
  MString *input = MString::JniCast(key);
  unsigned long size = stringToInt.size();
  size_t objSize = MRT_GetStringObjectSize(key);
  MString *localJstring = MObject::Cast<MString>(calloc(sizeof(char), objSize + 1));
  if (localJstring != nullptr) {
    if (memcpy_s(reinterpret_cast<uint8_t*>(localJstring), objSize,
                 reinterpret_cast<uint8_t*>(input), objSize) != EOK) {
      LOG(ERROR) << "copy string" << maple::endl;
    }
    stringToInt[input] = value;
    if (size == stringToInt.size()) {
      free(localJstring);
    }
  }
}

jint MCC_ArrayMap_String_Int_size() {
  return static_cast<jint>(stringToInt.size());
}

jint MCC_ArrayMap_String_Int_getOrDefault(jstring key, jint defaultValue) {
  MString *input = MString::JniCast(key);
  auto it = stringToInt.find(input);
  if (it != stringToInt.end()) {
    return it->second;
  } else {
    return defaultValue;
  }
}

void MCC_ArrayMap_String_Int_clear() {
  std::vector<MString*> tmpVector;
  for (auto it = stringToInt.begin(); it != stringToInt.end(); ++it) {
    tmpVector.push_back(it->first);
  }
  stringToInt.clear();
  for (auto it = tmpVector.begin(); it != tmpVector.end(); ++it) {
    free(*it);
    *it = nullptr;
  }
}

size_t MRT_ReflectClassGetComponentSize(jclass classObj) {
  MClass *mClassObj = MClass::JniCastNonNull(classObj);
  return mClassObj->GetComponentSize();
}
} // namespace maplert
