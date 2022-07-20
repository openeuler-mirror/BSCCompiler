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
#include "ark_annotation_map.h"

namespace maple {
namespace bc {
ArkAnnotationMap ArkAnnotationMap::annotationMap;

void ArkAnnotationMap::Init() {
  // AnnotationDefault
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FAnnotationDefault_3B",
                                          "Lark_2Fannotation_2FAnnotationDefault_3B"));
  // EnclosingClass
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FEnclosingClass_3B",
                                          "Lark_2Fannotation_2FEnclosingClass_3B"));
  // EnclosingMethod
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FEnclosingMethod_3B",
                                          "Lark_2Fannotation_2FEnclosingMethod_3B"));
  // InnerClass
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FInnerClass_3B",
                                          "Lark_2Fannotation_2FInnerClass_3B"));
  // KnownFailure
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FKnownFailure_3B",
                                          "Lark_2Fannotation_2FKnownFailure_3B"));
  // MemberClasses
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FMemberClasses_3B",
                                          "Lark_2Fannotation_2FMemberClasses_3B"));
  // MethodParameters
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FMethodParameters_3B",
                                          "Lark_2Fannotation_2FMethodParameters_3B"));
  // Signature
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FSignature_3B",
                                          "Lark_2Fannotation_2FSignature_3B"));
  // SourceDebugExtension
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FSourceDebugExtension_3B",
                                          "Lark_2Fannotation_2FSourceDebugExtension_3B"));
  // TestTargetClass
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FTestTargetClass_3B",
                                          "Lark_2Fannotation_2FTestTargetClass_3B"));
  // TestTarget
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FTestTarget_3B",
                                          "Lark_2Fannotation_2FTestTarget_3B"));
  // Throws
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2FThrows_3B",
                                          "Lark_2Fannotation_2FThrows_3B"));
  // compact
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2Fcompat_2FUnsupportedAppUsage_3B",
                                          "Lark_2Fannotation_2Fcompat_2FUnsupportedAppUsage_3B"));
  // codegen
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2Fcodegen_2FCovariantReturnType_3B",
                                          "Lark_2Fannotation_2Fcodegen_2FCovariantReturnType_3B"));
  // optimization
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2Foptimization_2FCriticalNative_3B",
                                          "Lark_2Fannotation_2Foptimization_2FCriticalNative_3B"));
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2Foptimization_2FDeadReferenceSafe_3B",
                                          "Lark_2Fannotation_2Foptimization_2FDeadReferenceSafe_3B"));
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2Foptimization_2FFastNative_3B",
                                          "Lark_2Fannotation_2Foptimization_2FFastNative_3B"));
  pragmaTypeNameMap.insert(std::make_pair("Ldalvik_2Fannotation_2Foptimization_2FReachabilitySensitive_3B",
                                          "Lark_2Fannotation_2Foptimization_2FReachabilitySensitive_3B"));
  for (auto &it : pragmaTypeNameMap) {
    (void)arkAnnotationTypeNames.insert(it.second);
  }
}

const std::string &ArkAnnotationMap::GetAnnotationTypeName(const std::string &orinName) {
  std::map<std::string, std::string>::iterator it = pragmaTypeNameMap.find(orinName);
  if (it == pragmaTypeNameMap.end()) {
    return orinName;
  } else {
    return it->second;
  }
}
}  // namespace bc
}  // namespace maple