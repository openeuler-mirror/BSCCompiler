/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ark_annotation_processor.h"
#include "ark_annotation_map.h"
#include "fe_manager.h"
#include "mpl_logging.h"
namespace maple {
namespace bc {
void ArkAnnotationProcessor::Process() {
  ArkAnnotationMap::GetArkAnnotationMap().Init();
  ArkAnnotation::GetInstance().Init();
  const std::set<std::string> &arkAnnotationTypeNames =
      ArkAnnotationMap::GetArkAnnotationMap().GetArkAnnotationTypeNames();
  for (const std::string &arkAnnotationTypeName : arkAnnotationTypeNames) {
    (void)FEManager::GetTypeManager().CreateClassOrInterfaceType(arkAnnotationTypeName, true, FETypeFlag::kSrcUnknown);
  }
}

// ---------- ArkAnnotation ----------
ArkAnnotation ArkAnnotation::instance;

void ArkAnnotation::Init() {
  // FastNative
  typeNameSetForFastNative.insert(GetStrIdxFromDexName("Ldalvik/annotation/optimization/FastNative;"));
  typeNameSetForFastNative.insert(GetStrIdxFromDexName("Lark/annotation/optimization/FastNative;"));
  // CriticalNative
  typeNameSetForCriticalNative.insert(GetStrIdxFromDexName("Ldalvik/annotation/optimization/CriticalNative;"));
  typeNameSetForCriticalNative.insert(GetStrIdxFromDexName("Lark/annotation/optimization/CriticalNative;"));
  // CallerSensitive
  typeNameSetForCallerSensitive.insert(GetStrIdxFromDexName("Lsun/reflect/CallerSensitive;"));
  // Permanent
  typeNameSetForPermanent.insert(GetStrIdxFromDexName("Lcom/huawei/ark/annotation/Permanent;"));
  typeNameSetForPermanent.insert(GetStrIdxFromDexName("Lharmonyos/annotation/Permanent;"));
  typeNameSetForPermanent.insert(GetStrIdxFromDexName("Lark/annotation/Permanent;"));
  // RCU
  typeNameSetForRCUnowned.insert(GetStrIdxFromDexName("Ljava/lang/annotation/RCUnownedRef;"));
  typeNameSetForRCUnowned.insert(GetStrIdxFromDexName("Lcom/huawei/ark/annotation/Unowned;"));
  typeNameSetForRCUnowned.insert(GetStrIdxFromDexName("Lharmonyos/annotation/Unowned;"));
  typeNameSetForRCUnowned.insert(GetStrIdxFromDexName("Lark/annotation/Unowned;"));
  // RCUnownedCap
  typeNameSetForRCUnownedCap.insert(GetStrIdxFromDexName("Ljava/lang/annotation/RCUnownedCapRef;"));
  // RCUnownedCapList
  typeNameSetForRCUnownedCapList.insert(GetStrIdxFromDexName("Ljava/lang/annotation/RCUnownedCapRef$List;"));
  // RCUnownedLocal
  typeNameSetForRCUnownedLocal.insert(GetStrIdxFromDexName("Lcom/huawei/ark/annotation/UnownedLocal;"));
  typeNameSetForRCUnownedLocal.insert(GetStrIdxFromDexName("Lharmonyos/annotation/UnownedLocal;"));
  typeNameSetForRCUnownedLocal.insert(GetStrIdxFromDexName("Lark/annotation/UnownedLocal;"));
  // RCUnownedLocalOld
  typeNameIdxForRCUnownedLocalOld = GetStrIdxFromDexName("Ljava/lang/annotation/RCLocalUnownedRef;");
  // RCUnownedThis
  typeNameSetForRCUnownedThis.insert(GetStrIdxFromDexName("Ljava/lang/annotation/RCUnownedThisRef;"));
  // RCUnownedOuter
  typeNameSetForRCUnownedOuter.insert(GetStrIdxFromDexName("Lcom/huawei/ark/annotation/UnownedOuter;"));
  typeNameSetForRCUnownedOuter.insert(GetStrIdxFromDexName("Lharmonyos/annotation/UnownedOuter;"));
  typeNameSetForRCUnownedOuter.insert(GetStrIdxFromDexName("Lark/annotation/UnownedOuter;"));
  // RCWeak
  typeNameSetForRCWeak.insert(GetStrIdxFromDexName("Ljava/lang/annotation/RCWeakRef;"));
  typeNameSetForRCWeak.insert(GetStrIdxFromDexName("Lcom/huawei/ark/annotation/Weak;"));
  typeNameSetForRCWeak.insert(GetStrIdxFromDexName("Lharmonyos/annotation/Weak;"));
  typeNameSetForRCWeak.insert(GetStrIdxFromDexName("Lark/annotation/Weak;"));
}

bool ArkAnnotation::IsFastNative(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForFastNative.find(sType->GetNameStrIdx()) != typeNameSetForFastNative.end();
}

bool ArkAnnotation::IsCriticalNative(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForCriticalNative.find(sType->GetNameStrIdx()) != typeNameSetForCriticalNative.end();
}

bool ArkAnnotation::IsCallerSensitive(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForCallerSensitive.find(sType->GetNameStrIdx()) != typeNameSetForCallerSensitive.end();
}

bool ArkAnnotation::IsPermanent(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForPermanent.find(sType->GetNameStrIdx()) != typeNameSetForPermanent.end();
}

bool ArkAnnotation::IsPermanent(const std::string &str) const {
  for (auto idx : typeNameSetForPermanent) {
    std::string annoName = GlobalTables::GetStrTable().GetStringFromStrIdx(idx);
    annoName = namemangler::DecodeName(annoName);
    if (annoName.compare(str) == 0) {
      return true;
    }
  }
  return false;
}

bool ArkAnnotation::IsRCUnowned(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForRCUnowned.find(sType->GetNameStrIdx()) != typeNameSetForRCUnowned.end();
}

bool ArkAnnotation::IsRCUnownedCap(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForRCUnownedCap.find(sType->GetNameStrIdx()) != typeNameSetForRCUnownedCap.end();
}

bool ArkAnnotation::IsRCUnownedCapList(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForRCUnownedCapList.find(sType->GetNameStrIdx()) != typeNameSetForRCUnownedCapList.end();
}

bool ArkAnnotation::IsRCUnownedLocal(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForRCUnownedLocal.find(sType->GetNameStrIdx()) != typeNameSetForRCUnownedLocal.end();
}

bool ArkAnnotation::IsRCUnownedLocalOld(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameIdxForRCUnownedLocalOld == sType->GetNameStrIdx();
}

bool ArkAnnotation::IsRCUnownedThis(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForRCUnownedThis.find(sType->GetNameStrIdx()) != typeNameSetForRCUnownedThis.end();
}

bool ArkAnnotation::IsRCUnownedOuter(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForRCUnownedOuter.find(sType->GetNameStrIdx()) != typeNameSetForRCUnownedOuter.end();
}

bool ArkAnnotation::IsRCWeak(const TyIdx &tyIdx) const {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForRCWeak.find(sType->GetNameStrIdx()) != typeNameSetForRCWeak.end();
}

MIRStructType *ArkAnnotation::GetStructType(const TyIdx &tyIdx) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (!type->IsMIRClassType() && !type->IsMIRInterfaceType()) {
    return nullptr;
  }
  return static_cast<MIRStructType*>(type);
}
}  // namespace bc
}  // namespace maple