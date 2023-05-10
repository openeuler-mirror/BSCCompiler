/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_JAVA_INTRN_LOWERING_H
#define MPL2MPL_INCLUDE_JAVA_INTRN_LOWERING_H
#include <map>
#include <unordered_set>
#include "phase_impl.h"
#include "maple_phase_manager.h"

namespace maple {
class JavaIntrnLowering : public FuncOptimizeImpl {
 public:
  JavaIntrnLowering(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~JavaIntrnLowering() override {
    classForName3Func = nullptr;
    classLoaderPointerToType = nullptr;
    getCurrentClassLoaderFunc = nullptr;
    classForName1Func = nullptr;
  }

  FuncOptimizeImpl *Clone() override {
    return new JavaIntrnLowering(*this);
  }

 private:
  void InitTypes();
  void InitFuncs();
  void InitLists();
  void ProcessStmt(StmtNode &stmt) override;
  void ProcessJavaIntrnMerge(StmtNode &assignNode, const IntrinsicopNode &intrinNode) const;
  BaseNode *JavaIntrnMergeToCvtType(PrimType destType, PrimType srcType, BaseNode *src) const;
  void LoadClassLoaderInvocation(const std::string &list);
  void CheckClassLoaderInvocation(const CallNode &callNode) const;
  void DumpClassLoaderInvocation(const CallNode &callNode);
  void ProcessForNameClassLoader(CallNode &callNode);
  void ProcessJavaIntrnFillNewArray(IntrinsiccallNode &intrinCall) const;
  std::string outFileName;
  std::unordered_set<std::string> clInterfaceSet;
  std::multimap<std::string, std::string> clInvocationMap;
  MIRFunction *classForName1Func = nullptr;
  MIRFunction *classForName3Func = nullptr;
  MIRFunction *getCurrentClassLoaderFunc = nullptr;
  MIRType *classLoaderPointerToType = nullptr;
};

MAPLE_MODULE_PHASE_DECLARE(M2MJavaIntrnLowering)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_JAVA_INTRN_LOWERING_H
