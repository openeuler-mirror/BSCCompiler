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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_ARK_ANNOTATION_MAP_H
#define HIR2MPL_BC_INPUT_INCLUDE_ARK_ANNOTATION_MAP_H
#include <map>
#include "global_tables.h"

namespace maple {
namespace bc {
class ArkAnnotationMap {
 public:
  inline static ArkAnnotationMap &GetArkAnnotationMap() {
    return annotationMap;
  }

  void Init();
  const std::string &GetAnnotationTypeName(const std::string &orinName);
  const std::set<std::string> &GetArkAnnotationTypeNames() const {
    return arkAnnotationTypeNames;
  }

 private:
  ArkAnnotationMap() = default;
  ~ArkAnnotationMap() = default;

  static ArkAnnotationMap annotationMap;
  std::map<std::string, std::string> pragmaTypeNameMap;
  std::set<std::string> arkAnnotationTypeNames;
};
}  // namespace bc
}  // namespace maple
#endif  // HIR2MPL_BC_INPUT_INCLUDE_ARK_ANNOTATION_MAP_H