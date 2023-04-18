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
#ifndef MPL2MPL_INCLUDE_VTABLE_IMPL_H
#define MPL2MPL_INCLUDE_VTABLE_IMPL_H
#include "phase_impl.h"

namespace maple {
enum CallKind {
  kStaticCall = 0,
  kVirtualCall = 1,
  kSuperCall = 2
};

class VtableImpl : public FuncOptimizeImpl {
 public:
  VtableImpl(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~VtableImpl() override {
    mirModule = nullptr;
    klassHierarchy = nullptr;
    mccItabFunc = nullptr;
  }

  void ProcessFunc(MIRFunction *func) override;
  FuncOptimizeImpl *Clone() override {
    return new VtableImpl(*this);
  }
#ifndef USE_ARM32_MACRO
#ifdef USE_32BIT_REF
  void Finish() override;
#endif  // ~USE_32BIT_REF
#endif  // ~USE_ARM32_MACRO

 private:
  void ReplaceResolveInterface(StmtNode &stmt, const ResolveFuncNode &resolveNode) const;
  void ItabProcess(const StmtNode &stmt, const ResolveFuncNode &resolveNode, const std::string &signature,
                   const PregIdx &pregFuncPtr, const MIRType &compactPtrType, const PrimType &compactPtrPrim) const;
  bool Intrinsify(MIRFunction &func, CallNode &cnode) const;
#ifndef USE_ARM32_MACRO
#ifdef USE_32BIT_REF
  void InlineCacheinit();
  void GenInlineCacheTableSymbol();
  void InlineCacheProcess(StmtNode &stmt, const ResolveFuncNode &resolveNode,
                          const std::string &signature, PregIdx &pregFuncPtr);
  void CallMrtInlineCacheFun(StmtNode &stmt, const ResolveFuncNode &resolveNode,
                             RegreadNode &regReadNodeTmp, int64 hashCode, uint64 secondHashCode,
                             const std::string &signature, PregIdx &pregFuncPtr, IfStmtNode &ifStmt);
  void ResolveInlineCacheTable();
#endif  // ~USE_32BIT_REF
#endif  // ~USE_ARM32_MACRO
  void DeferredVisit(CallNode &stmt, CallKind kind);
  void DeferredVisitCheckFloat(CallNode &stmt, const MIRFunction &mirFunc) const;
  MIRModule *mirModule;
  KlassHierarchy *klassHierarchy;
#ifndef USE_ARM32_MACRO
#ifdef USE_32BIT_REF
  MIRFunction *mccItabFuncInlineCache;
  uint32 numOfInterfaceCallSite = 0;
  MIRStructType *inlineCacheTableEntryType = nullptr;
  MIRArrayType *inlineCacheTableType = nullptr;
  MIRSymbol *inlineCacheTableSym = nullptr;
#endif  // ~USE_32BIT_REF
#endif  // ~USE_ARM32_MACRO
  MIRFunction *mccItabFunc = nullptr;
};

MAPLE_MODULE_PHASE_DECLARE(M2MVtableImpl)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_VTABLE_IMPL_H
