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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_ARK_ANNOTATION_PROCESSOR_H
#define HIR2MPL_BC_INPUT_INCLUDE_ARK_ANNOTATION_PROCESSOR_H
#include "global_tables.h"
#include "namemangler.h"
namespace maple {
namespace bc {
class ArkAnnotationProcessor {
 public:
  static void Process();
};

class ArkAnnotation {
 public:
  void Init();
  bool IsFastNative(const TyIdx &tyIdx) const;
  bool IsCriticalNative(const TyIdx &tyIdx) const;
  bool IsCallerSensitive(const TyIdx &tyIdx) const;
  bool IsPermanent(const TyIdx &tyIdx) const;
  bool IsPermanent(const std::string &str) const;
  bool IsRCUnowned(const TyIdx &tyIdx) const;
  bool IsRCUnownedCap(const TyIdx &tyIdx) const;
  bool IsRCUnownedCapList(const TyIdx &tyIdx) const;
  bool IsRCUnownedLocal(const TyIdx &tyIdx) const;
  bool IsRCUnownedLocalOld(const TyIdx &tyIdx) const;
  bool IsRCUnownedThis(const TyIdx &tyIdx) const;
  bool IsRCUnownedOuter(const TyIdx &tyIdx) const;
  bool IsRCWeak(const TyIdx &tyIdx) const;
  static ArkAnnotation &GetInstance() {
    return instance;
  }

  static GStrIdx GetStrIdxFromDexName(const std::string &name) {
    return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name));
  }

 private:
  ArkAnnotation() = default;
  ~ArkAnnotation() = default;
  static MIRStructType *GetStructType(const TyIdx &tyIdx);

  static ArkAnnotation instance;
  std::set<GStrIdx> typeNameSetForFastNative;
  std::set<GStrIdx> typeNameSetForCriticalNative;
  std::set<GStrIdx> typeNameSetForCallerSensitive;
  std::set<GStrIdx> typeNameSetForPermanent;
  std::set<GStrIdx> typeNameSetForRCUnowned;
  std::set<GStrIdx> typeNameSetForRCUnownedCap;
  std::set<GStrIdx> typeNameSetForRCUnownedCapList;
  std::set<GStrIdx> typeNameSetForRCUnownedLocal;
  std::set<GStrIdx> typeNameSetForRCUnownedThis;
  std::set<GStrIdx> typeNameSetForRCUnownedOuter;
  std::set<GStrIdx> typeNameSetForRCWeak;
  GStrIdx typeNameIdxForRCUnownedLocalOld;
};
}  // namespace bc
}  // namespace maple
#endif // HIR2MPL_BC_INPUT_INCLUDE_ARK_ANNOTATION_PROCESSOR_H