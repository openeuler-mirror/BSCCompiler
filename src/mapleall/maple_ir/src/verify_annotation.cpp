/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights isCorrecterved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPisCorrectS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "verify_annotation.h"
#include "global_tables.h"
#include "reflection_analysis.h"

namespace maple {
namespace verifyanno {
constexpr char kDeferredThrowVerifyError[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoThrowVerifyError_3B";
constexpr char kDeferredExtendFinalCheck[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredExtendFinalCheck_3B";
constexpr char kDeferredOverrideFinalCheck[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredOverrideFinalCheck_3B";
constexpr char kDeferredAssignableCheck[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredAssignableCheck_3B";
constexpr char kAssignableChecksContainer[] =
    "Lcom_2Fhuawei_2Fark_2Fannotation_2Fverify_2FVerfAnnoDeferredAssignableChecks_3B";
}

// Convert from Ljava_2Flang_2FStringBuilder_3B to Ljava_2Flang_2FStringBuilder;
inline std::string SimplifyClassName(const std::string &name) {
  const std::string postFix = "_3B";
  if (name.rfind(postFix) == (name.size() - postFix.size())) {
    std::string shortName = name.substr(0, name.size() - postFix.size());
    return shortName.append(";");
  }
  return name;
}

inline MIRStructType *GetOrCreateStructType(const std::string &name, MIRModule &md) {
  return static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetOrCreateClassType(name, md));
}

MIRPragmaElement *NewAnnotationElement(MIRModule &md, const GStrIdx &nameIdx, PragmaValueType type) {
  auto *elem = md.GetMemPool()->New<MIRPragmaElement>(md.GetMPAllocator());
  elem->SetType(type);
  elem->SetNameStrIdx(nameIdx);
  return elem;
}

MIRPragmaElement *NewAnnotationElement(MIRModule &md, const std::string &name, PragmaValueType type) {
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  return NewAnnotationElement(md, nameIdx, type);
}

MIRPragmaElement *NewAnnotationStringElement(MIRModule &md, const std::string &name, const std::string &value) {
  MIRPragmaElement *elem = NewAnnotationElement(md, name, kValueString);
  if (!value.empty()) {
    elem->SetU64Val(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(value).GetIdx());
  }
  return elem;
}

MIRPragma *NewPragmaRTAnnotation(MIRModule &md, const MIRStructType &clsType,
                                 const MIRStructType &aType, PragmaKind kind) {
  auto *pragma = md.GetMemPool()->New<MIRPragma>(md, md.GetMPAllocator());
  pragma->SetStrIdx(clsType.GetNameStrIdx());
  pragma->SetTyIdx(aType.GetTypeIndex());
  pragma->SetKind(kind);
  pragma->SetVisibility(kVisRuntime);
  return pragma;
}

MIRPragma *NewPragmaRTClassAnnotation(MIRModule &md, const MIRStructType &clsType, const std::string &aTypeName) {
  MIRStructType *aType = GetOrCreateStructType(aTypeName, md);
  if (aType->GetPragmaVec().empty()) {
    // not enough to set kVisRuntime,
    // do annotate with Retention(RUNTIME) (see ReflectionAnalysis::RtRetentionPolicyCheck)
    // kJavaLangAnnotationRetentionStr is the same to the one in RtRetentionPolicyCheck in reflection_analysis.cpp
    const std::string kJavaLangAnnotationRetentionStr = "Ljava_2Flang_2Fannotation_2FRetention_3B";
    MIRStructType *retentionType = GetOrCreateStructType(kJavaLangAnnotationRetentionStr, md);
    MIRPragma *pragmaRetention = NewPragmaRTAnnotation(md, *aType, *retentionType, kPragmaClass);
    pragmaRetention->PushElementVector(NewAnnotationStringElement(md, "value", "RUNTIME"));
    aType->PushbackPragma(pragmaRetention);
  }
  return NewPragmaRTAnnotation(md, clsType, *aType, kPragmaClass);
}

// creates pragma not inserted into clsType pragmaVec
MIRPragma *NewAssignableCheckAnno(MIRModule &md, const MIRStructType &clsType, const AssignableCheckPragma &info) {
  MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredAssignableCheck);
  pr->PushElementVector(NewAnnotationStringElement(md, "dst", SimplifyClassName(info.GetToType())));
  pr->PushElementVector(NewAnnotationStringElement(md, "src", SimplifyClassName(info.GetFromType())));
  return pr;
}

void AddVerfAnnoThrowVerifyError(MIRModule &md, const ThrowVerifyErrorPragma &info, MIRStructType &clsType) {
  const auto &pragmaVec = clsType.GetPragmaVec();
  for (auto pragmaPtr : pragmaVec) {
    if (pragmaPtr == nullptr) {
      continue;
    }
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pragmaPtr->GetTyIdx());
    if (mirType != nullptr && mirType->GetName().compare(verifyanno::kDeferredThrowVerifyError) == 0) {
      return;
    }
  }
  MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredThrowVerifyError);
  pr->PushElementVector(NewAnnotationStringElement(md, "msg", info.GetMessage()));
  clsType.PushbackPragma(pr);
}

void AddVerfAnnoAssignableCheck(MIRModule &md, std::vector<const AssignableCheckPragma*> &info,
                                MIRStructType &clsType) {
  if (info.empty()) {
    return;
  }

  if (info.size() == 1) {
    MIRPragma *pr = NewAssignableCheckAnno(md, clsType, *info.front());
    clsType.PushbackPragma(pr);
    return;
  }

  // container pragma
  MIRPragma *prContainer = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kAssignableChecksContainer);
  MIRPragmaElement *elemContainer = NewAnnotationElement(md, "value", kValueArray);

  GStrIdx singleAnnoTypeName = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
      verifyanno::kDeferredAssignableCheck);
  for (auto *singleInfo : info) {
    MIRPragmaElement *elem = NewAnnotationElement(md, singleAnnoTypeName, kValueAnnotation);
    elem->SetTypeStrIdx(singleAnnoTypeName);
    elem->SubElemVecPushBack(NewAnnotationStringElement(md, "dst", SimplifyClassName(singleInfo->GetToType())));
    elem->SubElemVecPushBack(NewAnnotationStringElement(md, "src", SimplifyClassName(singleInfo->GetFromType())));
    elemContainer->SubElemVecPushBack(elem);
  }

  prContainer->PushElementVector(elemContainer);
  clsType.PushbackPragma(prContainer);
}

void AddVerfAnnoExtendFinalCheck(MIRModule &md, MIRStructType &clsType) {
  MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredExtendFinalCheck);
  clsType.PushbackPragma(pr);
}

void AddVerfAnnoOverrideFinalCheck(MIRModule &md, MIRStructType &clsType) {
  MIRPragma *pr = NewPragmaRTClassAnnotation(md, clsType, verifyanno::kDeferredOverrideFinalCheck);
  clsType.PushbackPragma(pr);
}
} // namespace maple
