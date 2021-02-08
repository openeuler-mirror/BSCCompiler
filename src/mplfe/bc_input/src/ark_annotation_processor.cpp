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
}

bool ArkAnnotation::IsFastNative(TyIdx tyIdx) {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForFastNative.find(sType->GetNameStrIdx()) != typeNameSetForFastNative.end();
}

bool ArkAnnotation::IsCriticalNative(TyIdx tyIdx) {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForCriticalNative.find(sType->GetNameStrIdx()) != typeNameSetForCriticalNative.end();
}

bool ArkAnnotation::IsCallerSensitive(TyIdx tyIdx) {
  MIRStructType *sType = GetStructType(tyIdx);
  return sType == nullptr ?
      false : typeNameSetForCallerSensitive.find(sType->GetNameStrIdx()) != typeNameSetForCallerSensitive.end();
}

MIRStructType *ArkAnnotation::GetStructType(TyIdx tyIdx) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (!type->IsMIRClassType() && !type->IsMIRInterfaceType()) {
    return nullptr;
  }
  return static_cast<MIRStructType*>(type);
}
}  // namespace bc
}  // namespace maple