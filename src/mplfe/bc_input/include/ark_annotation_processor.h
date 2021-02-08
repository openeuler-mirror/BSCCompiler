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
#ifndef MPLFE_BC_INPUT_INCLUDE_ARK_ANNOTATION_PROCESSOR_H
#define MPLFE_BC_INPUT_INCLUDE_ARK_ANNOTATION_PROCESSOR_H
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
  bool IsFastNative(TyIdx tyIdx);
  bool IsCriticalNative(TyIdx tyIdx);
  bool IsCallerSensitive(TyIdx tyIdx);
  static ArkAnnotation &GetInstance() {
    return instance;
  }

  static GStrIdx GetStrIdxFromDexName(const std::string &name) {
    return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name));
  }

 private:
  ArkAnnotation() = default;
  ~ArkAnnotation() = default;
  MIRStructType *GetStructType(TyIdx tyIdx);

  static ArkAnnotation instance;
  std::set<GStrIdx> typeNameSetForFastNative;
  std::set<GStrIdx> typeNameSetForCriticalNative;
  std::set<GStrIdx> typeNameSetForCallerSensitive;
};
}  // namespace bc
}  // namespace maple
#endif // MPLFE_BC_INPUT_INCLUDE_ARK_ANNOTATION_PROCESSOR_H