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
#ifndef MPL2MPL_INCLUDE_CLONE_H
#define MPL2MPL_INCLUDE_CLONE_H
#include "mir_module.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "class_hierarchy_phase.h"
#include "me_ir.h"
#include "maple_phase_manager.h"

static constexpr char kFullNameStr[] = "INFO_fullname";
static constexpr char kClassNameStr[] = "INFO_classname";
static constexpr char kFuncNameStr[] = "INFO_funcname";
static constexpr char kVoidRetSuffix[] = "CLONEDignoreret";
namespace maple {
class ReplaceRetIgnored {
 public:
  explicit ReplaceRetIgnored(MemPool *memPool);
  ~ReplaceRetIgnored() = default;

  bool ShouldReplaceWithVoidFunc(const CallMeStmt &stmt, const MIRFunction &calleeFunc) const;
  std::string GenerateNewBaseName(const MIRFunction &originalFunc) const;
  std::string GenerateNewFullName(const MIRFunction &originalFunc) const;
  const MapleSet<MapleString> *GetTobeClonedFuncNames() const {
    return &toBeClonedFuncNames;
  }

  bool IsInCloneList(const std::string &funcName) const {
    return toBeClonedFuncNames.find(MapleString(funcName, memPool)) != toBeClonedFuncNames.end();
  }

  static bool IsClonedFunc(const std::string &funcName) {
    return funcName.find(kVoidRetSuffix) != std::string::npos;
  }

 private:
  MemPool *memPool;
  maple::MapleAllocator allocator;
  MapleSet<MapleString> toBeClonedFuncNames;
  bool RealShouldReplaceWithVoidFunc(Opcode op, size_t nRetSize, const MIRFunction &calleeFunc) const;
};

class Clone : public AnalysisResult {
 public:
  Clone(MIRModule *mod, MemPool *memPool, MIRBuilder &builder, KlassHierarchy *kh)
      : AnalysisResult(memPool), mirModule(mod), allocator(memPool), mirBuilder(builder), kh(kh),
        replaceRetIgnored(memPool->New<ReplaceRetIgnored>(memPool)) {}

  ~Clone() override {
    mirModule = nullptr;
    kh = nullptr;
    replaceRetIgnored = nullptr;
  }

  static MIRSymbol *CloneLocalSymbol(const MIRSymbol &oldSym, const MIRFunction &newFunc);
  static void CloneSymbols(MIRFunction &newFunc, const MIRFunction &oldFunc);
  static void CloneLabels(MIRFunction &newFunc, const MIRFunction &oldFunc);
  MIRFunction *CloneFunction(MIRFunction &originalFunction, const std::string &newBaseFuncName,
      MIRType *returnType = nullptr) const;
  MIRFunction *CloneFunctionNoReturn(MIRFunction &originalFunction);
  void DoClone();
  void UpdateFuncInfo(MIRFunction &newFunc);
  void CloneArgument(MIRFunction &originalFunction, ArgVector &argument) const;
  const ReplaceRetIgnored &GetReplaceRetIgnored() const {
    return *replaceRetIgnored;
  }

  void UpdateReturnVoidIfPossible(CallMeStmt *callMeStmt, const MIRFunction &targetFunc) const;

 private:
  MIRModule *mirModule;
  MapleAllocator allocator;
  MIRBuilder &mirBuilder;
  KlassHierarchy *kh;
  ReplaceRetIgnored *replaceRetIgnored;
};

inline void CopyFuncInfo(MIRFunction &originalFunction, MIRFunction &newFunc, const MIRBuilder &mirBuilder) {
  auto funcNameIdx = newFunc.GetBaseFuncNameStrIdx();
  auto fullNameIdx = newFunc.GetNameStrIdx();
  auto classNameIdx = newFunc.GetBaseClassNameStrIdx();
  auto metaFullNameIdx = mirBuilder.GetOrCreateStringIndex(kFullNameStr);
  auto metaClassNameIdx = mirBuilder.GetOrCreateStringIndex(kClassNameStr);
  auto metaFuncNameIdx = mirBuilder.GetOrCreateStringIndex(kFuncNameStr);
  MIRInfoVector &fnInfo = originalFunction.GetInfoVector();
  const MapleVector<bool> &infoIsString = originalFunction.InfoIsString();
  size_t size = fnInfo.size();
  for (size_t i = 0; i < size; ++i) {
    if (fnInfo[i].first == metaFullNameIdx) {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, fullNameIdx));
    } else if (fnInfo[i].first == metaFuncNameIdx) {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, funcNameIdx));
    } else if (fnInfo[i].first == metaClassNameIdx) {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, classNameIdx));
    } else {
      newFunc.PushbackMIRInfo(std::pair<GStrIdx, uint32>(fnInfo[i].first, fnInfo[i].second));
    }
    newFunc.PushbackIsString(infoIsString[i]);
  }
}

MAPLE_MODULE_PHASE_DECLARE_BEGIN(M2MClone)
  Clone *GetResult() {
    return cl;
  }
  Clone *cl = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_MODULE_PHASE_DECLARE_END
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CLONE_H
