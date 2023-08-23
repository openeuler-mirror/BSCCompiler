/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INLINE_MPLT_H
#define MPL2MPL_INLINE_MPLT_H
#include "types_def.h"
#include "mir_type.h"
#include "mir_symbol.h"
#include "mir_module.h"
#include "mir_function.h"

namespace maple {
struct FuncComparator {
  bool operator()(const MIRFunction *lhs, const MIRFunction *rhs) const {
    return lhs->GetPuidx() < rhs->GetPuidx();
  }
};

class InlineMplt {
 public:
  explicit InlineMplt(MIRModule &module) : mirModule(module) {}
  virtual ~InlineMplt() = default;

  // level: the max level that we export static function.
  // 1 means that if a small global function invokes a static leaf function
  // or a static function which never invoke other static fucntions, we will export this static function too.
  bool CollectInlineInfo(uint32 inlineSize, uint32 level = 2);
  bool Forbidden(BaseNode *node, const std::pair<uint32, uint32> &inlineConditions,
                 std::vector<MIRFunction*> &staticFuncs, std::set<uint32_t> &globalSymbols);
  void CollectStructOrUnionTypes(const MIRType *baseType);
  void GetElememtTypesFromDerivedType(const MIRType *baseType, std::set<const MIRType*> &elementTypes);
  void CollectTypesForOptimizedFunctions();
  void CollectTypesForInliningGlobals();
  void CollectTypesForSingleFunction(const MIRFunction &func);
  void CollectTypesForGlobalVar(const MIRSymbol &globalSymbol);
  void DumpInlineCandidateToFile(const std::string &fileNameStr);
  void DumpOptimizedFunctionTypes();
  uint32 GetFunctionSize(MIRFunction &mirFunc) const;

 private:
  std::set<MIRFunction*, FuncComparator> optimizedFuncs;
  std::set<TyIdx> optimizedFuncsType;
  std::set<uint32_t> inliningGlobals;
  MIRModule &mirModule;
};
} // namespace maple
#endif // MPL2MPL_INLINE_MPLT_H
