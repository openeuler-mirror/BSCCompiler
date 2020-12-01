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
#include "fieldmeta.h"
#include "mclass_inline.h"
#include "fieldmeta_inline.h"
#include "namemangler.h"
namespace maplert {
std::mutex FieldMetaCompact::resolveMutex;
void FieldMetaCompact::SetCompactFieldMetaOffset(const MClass &cls, uint32_t index, int32_t fieldOffset) {
  FieldMetaCompact *leb = cls.GetCompactFields();
  uint32_t numOfField = cls.GetNumOfFields();
  for (uint32_t i = 0; i < numOfField; ++i) {
    if (i == index) {
      leb->SetBitOffset(static_cast<uint32_t>(fieldOffset));
      return;
    }
    const uint8_t *pLeb = leb->GetpCompact();
    (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
    (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
    (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
    (void)namemangler::GetUnsignedLeb128Decode(&pLeb);
    leb = reinterpret_cast<FieldMetaCompact*>(const_cast<uint8_t*>(pLeb));
  }
  LOG(FATAL) << "FieldMetaCompact::GetCompactFieldMetasOffsetAddr: " << cls.GetName() <<
      ", index:" << index << maple::endl;
}

FieldMetaCompact *FieldMetaCompact::DecodeCompactFieldMetasToVector(
    const MClass &cls, uint32_t vecSize, char **typeNameVec, char **fieldNameVec, size_t *offsetVec,
    uint32_t *modifierVec, FieldMetaCompact **fieldMetaCompact, char **annotationVec) {
  uint32_t numOfField = cls.GetNumOfFields();
  if (numOfField == 0 || numOfField > vecSize) {
    return nullptr;
  }
  FieldMetaCompact *leb = cls.GetCompactFields();
  for (uint32_t i = 0; i < numOfField; ++i) {
    if (fieldMetaCompact != nullptr) {
      fieldMetaCompact[i] = leb;
    }
    const uint8_t *pLeb = leb->GetpCompact();
    uint32_t modifier = namemangler::GetUnsignedLeb128Decode(&pLeb);
    uintptr_t offset = leb->GetOffsetOrAddress(modifier::IsStatic(modifier));
    if (offsetVec != nullptr) {
      offsetVec[i] = offset;
    }

    if (modifierVec != nullptr) {
      modifierVec[i] = modifier;
    }
    uint32_t typeNameIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
    char *typeName = LinkerAPI::Instance().GetCString(cls.AsJclass(), typeNameIndex);
    if (typeNameVec != nullptr) {
      typeNameVec[i] = typeName;
    }
    uint32_t fieldNameIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
    char *fieldName = LinkerAPI::Instance().GetCString(cls.AsJclass(), fieldNameIndex);
    if (fieldNameVec != nullptr) {
      fieldNameVec[i] = fieldName;
    }
    uint32_t annotationIndex = namemangler::GetUnsignedLeb128Decode(&pLeb);
    char *annotation = LinkerAPI::Instance().GetCString(cls.AsJclass(), annotationIndex);
    if (annotationVec != nullptr) {
      annotationVec[i] = annotation;
    }
    leb = reinterpret_cast<FieldMetaCompact*>(const_cast<uint8_t*>(pLeb));
  }
  return cls.GetCompactFields();
}

FieldMeta *FieldMetaCompact::DecodeCompactFieldMetas(MClass &cls) {
  uint32_t numOfField = cls.GetNumOfFields();
  if (numOfField == 0) {
    return nullptr;
  }
  CHECK(numOfField <= (std::numeric_limits<uint32_t>::max() / sizeof(FieldMeta))) <<
      "field count too large. numOfField " << numOfField << maple::endl;
  {
    std::lock_guard<std::mutex> lock(resolveMutex);
    FieldMeta *fields = cls.GetRawFieldMetas();
    if (!cls.IsCompactMetaFields()) {
      return fields;
    }
    // Compact, need resolve
    char *typeNameVec[numOfField];
    char *fieldNameVec[numOfField];
    char *annotationVec[numOfField];
    size_t offsetVec[numOfField];
    uint32_t modifierVec[numOfField];
    (void)DecodeCompactFieldMetasToVector(cls, numOfField, typeNameVec, fieldNameVec, offsetVec,
                                          modifierVec, nullptr, annotationVec);
#ifndef USE_32BIT_REF
    for (uint32_t i = 0; i < numOfField; ++i) {
      char *fieldNameStr = fieldNameVec[i];
      char *annoStr = annotationVec[i];
      size_t fieldNameStrLen = strlen(fieldNameStr) + 1;
      size_t annoStrLen = strlen(annoStr) + 1;
      char *strBuffer = reinterpret_cast<char*>(MRT_AllocFromMeta(fieldNameStrLen + annoStrLen, kNativeStringData));
      char *fieldNameStrBuffer = strBuffer;
      char *annoStrBuffer = strBuffer + fieldNameStrLen;
      errno_t tmpResult1 = memcpy_s(fieldNameStrBuffer, fieldNameStrLen, fieldNameStr, fieldNameStrLen);
      errno_t tmpResult2 = memcpy_s(annoStrBuffer, annoStrLen, annoStr, annoStrLen);
      if (UNLIKELY(tmpResult1 != EOK || tmpResult2 != EOK)) {
        LOG(FATAL) << "FieldMetaCompact::DecodeCompactFieldMetas : memcpy_s() failed" << maple::endl;
        return nullptr;
      }
      fieldNameVec[i] = fieldNameStrBuffer;
      annotationVec[i] = annoStrBuffer;
    }
#endif
    size_t size = sizeof(FieldMeta) * numOfField;
    FieldMeta *fieldMetas = FieldMeta::Cast(MRT_AllocFromMeta(size, kFieldMetaData));
    for (uint32_t i = 0; i < numOfField; ++i) {
      FieldMeta *newFieldMeta = fieldMetas + i;
      newFieldMeta->FillFieldMeta(modifierVec[i], offsetVec[i], typeNameVec[i], modifier::kHashConflict,
          static_cast<uint16_t>(i), fieldNameVec[i], annotationVec[i], cls);
    }
    cls.SetFields(*fieldMetas);
    return fieldMetas;
  }
}

uint8_t *FieldMetaCompact::GetpCompact() {
  return &leb128Start;
}

FieldOffset *FieldMetaCompact::GetFieldpOffset() const {
  // Compact is always ro
  FieldOffset *fieldOffset = pOffset.GetDataRef<FieldOffset*>();
  return fieldOffset;
}

uint32_t FieldMetaCompact::GetBitOffset() const {
  FieldOffset *fieldOffset = GetFieldpOffset();
  DCHECK(fieldOffset != nullptr) << "FieldMetaCompact::GetBitOffset(): fieldOffset is nullptr!" << maple::endl;
  return fieldOffset->GetBitOffset();
}

void FieldMetaCompact::SetBitOffset(uint32_t offset) {
  FieldOffset *fieldOffset = GetFieldpOffset();
  fieldOffset->SetBitOffset(offset);
}

uintptr_t FieldMetaCompact::GetOffsetOrAddress(bool isStatic) const {
  uintptr_t offsetOrAddress;
  FieldOffset *fieldOffset = GetFieldpOffset();
  DCHECK(fieldOffset != nullptr) << "FieldMetaCompact::GetOffsetOrAddress: fieldOffset is nullptr!" << maple::endl;
  if (isStatic) {
    offsetOrAddress = fieldOffset->GetAddress();
  } else {
    offsetOrAddress = fieldOffset->GetBitOffset();
  }
  return offsetOrAddress;
}

void FieldMeta::SetFieldName(const char *name) {
  fieldName.SetDataRef(name);
}

void FieldMeta::SetAnnotation(const char *fieldAnnotation) {
  annotation.SetDataRef(fieldAnnotation);
}

void FieldMeta::SetDeclaringClass(const MClass &dlClass) {
  declaringClass.SetDataRef(&dlClass);
}

std::string FieldMeta::GetFullName(const MClass *dclClass, bool needType) const {
  char *name = GetName();
  DCHECK(name != nullptr) << "FieldMeta::GetFullName:name is nullptr" << maple::endl;
  std::string fullName, declaringClassName;
  if (dclClass != nullptr) {
    dclClass->GetBinaryName(declaringClassName);
  }
  if (needType) {
    std::string typeStr;
    MClass *fieldType = GetType();
    if (fieldType != nullptr) {
      fieldType->GetTypeName(typeStr);
    } else {
      typeStr = GetTypeName();
    }
    fullName = typeStr + " ";
  }
  fullName += declaringClassName;
  fullName += ".";
  fullName += name;
  return fullName;
}

MObject *FieldMeta::GetSignatureAnnotation() const {
  std::string annotationSet = GetAnnotation();
  if (annotationSet.empty()) {
    return nullptr;
  }
  MObject *ret = AnnoParser::GetSignatureValue(annotationSet, GetDeclaringclass());
  return ret;
}

void FieldMeta::FillFieldMeta(uint32_t modifier, size_t offset, const char *srcTypeName, uint16_t hash,
                              uint16_t fieldIndex, const char *name, const char *strAnnotation, const MClass &cls) {
  SetMod(modifier);
  SetOffsetOrAddress(offset);
  SetTypeName(srcTypeName);
  SetHashCode(hash);
  SetIndex(fieldIndex);
  SetFieldName(name);
  SetAnnotation(strAnnotation);
  SetDeclaringClass(cls);
}

static MObject *ReflectGetField(const MObject *o, uint32_t modifier, size_t offset) {
  const bool isVolatile = modifier::IsVolatile(modifier);
  if (Collector::Instance().Type() == kNaiveRC) {
    if (modifier::IsWeakRef(modifier)) {
      if (UNLIKELY(offset == 0)) {
        LOG(FATAL) << "static field is rc weak ref " << maple::endl;
      }
      return MObject::Cast<MObject>(MRT_LoadWeakField(reinterpret_cast<address_t>(o),
          reinterpret_cast<address_t*>(reinterpret_cast<address_t>(o) + offset), isVolatile));
    } else if(modifier::IsUnowned(modifier)) {
      if (UNLIKELY(offset == 0)) {
        LOG(FATAL) << "static field is rc unowned ref " << maple::endl;
      } else if (UNLIKELY(isVolatile)) {
        LOG(FATAL) << "volatile can not be rc unowned " << o->GetClass()->GetName() << " " << offset << maple::endl;
      }
    }
  }
  return o->LoadObject(offset, isVolatile);
}

static void ReflectSetField(const MObject *o, uint32_t modifier, size_t offset, const MObject *newValue) {
  const bool isVolatile = modifier::IsVolatile(modifier);
  if (Collector::Instance().Type() == kNaiveRC) {
    if (modifier::IsWeakRef(modifier)) {
      if (UNLIKELY(offset == 0)) {
        LOG(FATAL) << "static field is rc weak ref " << maple::endl;
      }
      MRT_WriteWeakField(reinterpret_cast<address_t>(o),
          reinterpret_cast<address_t*>(reinterpret_cast<address_t>(o) + offset),
          reinterpret_cast<address_t>(newValue), isVolatile);
      return;
    } else if(modifier::IsUnowned(modifier)) {
      if (UNLIKELY(offset == 0)) {
        LOG(FATAL) << "static field is rc unowned ref " << maple::endl;
      } else if (UNLIKELY(isVolatile)) {
        LOG(FATAL) << "volatile can not be rc unowned " << o->GetClass()->GetName() << " " << offset << maple::endl;
      }
      o->StoreObjectNoRc(offset, newValue);
      return;
    }
  }
  o->StoreObject(offset, newValue, isVolatile);
}

static void ReflectSetFieldStatic(uint32_t modifier, address_t *addr, const MObject *newValue) {
  if (Collector::Instance().Type() == kNaiveRC) {
    if (modifier::IsWeakRef(modifier)) {
      LOG(FATAL) << "static field is rc weak ref " << maple::endl;
    } else if(modifier::IsUnowned(modifier)) {
      LOG(FATAL) << "static field is rc unowned ref " << maple::endl;
    }
  }
  const bool isVolatile = modifier::IsVolatile(modifier);
  if (isVolatile) {
    MRT_STORE_JOBJECT_VOLATILE_STATIC(addr, newValue);
  } else {
    MRT_STORE_JOBJECT_STATIC(addr, newValue);
  }
}

void FieldMeta::SetObjectValue(MObject *o, const MObject *value, bool clinitCheck) const {
  o = GetRealMObject(o, clinitCheck);
  if (UNLIKELY(o == nullptr)) {
    return;
  }
  uint32_t offset = GetOffset();
  uint32_t modifier = GetMod();
  if (IsStatic()) {
    ReflectSetFieldStatic(modifier, reinterpret_cast<address_t*>(o), value);
  } else {
    ReflectSetField(o, modifier, offset, value);
  }
}

MObject *FieldMeta::GetObjectValue(const MObject *o) const {
  o = GetRealMObject(o);
  if (UNLIKELY(o == nullptr)) {
    return nullptr;
  }
  uint32_t offset = GetOffset();
  uint32_t modifier = GetMod();
  return ReflectGetField(o, modifier, offset);
}

void FieldMeta::SetOffsetOrAddress(size_t setOffset) {
  if (IsStatic()) {
    SetStaticAddr(setOffset);
  } else {
    SetBitOffset(static_cast<uint32_t>(setOffset));
  }
}

void FieldMeta::SetStaticAddr(uintptr_t addr) {
  // static field address
  DCHECK(IsStatic()) << "should be static field";
  FieldOffset *fieldOffset = GetFieldpOffset();
  DCHECK(fieldOffset != nullptr) << "fieldOffset is nullptr" << maple::endl;
  fieldOffset->SetAddress(addr);
}
} // namespace maplert
