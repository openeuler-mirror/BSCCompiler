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
#ifndef HIR2MPL_INCLUDE_FE_TYPE_HIERARCHY_H
#define HIR2MPL_INCLUDE_FE_TYPE_HIERARCHY_H
#include <map>
#include <set>
#include "mir_type.h"

namespace maple {
class FETypeHierarchy {
 public:
  static FETypeHierarchy &GetInstance() {
    return instance;
  }
  void InitByGlobalTable();
  void AddMIRType(const MIRClassType &type);
  void AddMIRType(const MIRInterfaceType &type);
  bool IsParentOf(const GStrIdx &parentIdx, const GStrIdx &childIdx);

 private:
  static FETypeHierarchy instance;
  std::map<GStrIdx, std::set<GStrIdx>> mapIdxChildParent;
  std::set<std::pair<GStrIdx, GStrIdx>> cache;
  FETypeHierarchy() = default;
  ~FETypeHierarchy() = default;
  void AddParentChildRelation(const GStrIdx &parentIdx, const GStrIdx &childIdx);
};  // class FETypeHierarchy
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FE_TYPE_HIERARCHY_H