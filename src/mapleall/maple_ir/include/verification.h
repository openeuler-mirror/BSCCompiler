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
#ifndef MAPLEIR_VERIFICATION_PHASE_H
#define MAPLEIR_VERIFICATION_PHASE_H
#include "class_hierarchy.h"
#include "verify_pragma_info.h"

namespace maple {
using ClassVerifyPragmas = MapleUnorderedMap<std::string, std::vector<const VerifyPragmaInfo*>>;

class VerifyResult {
 public:
  VerifyResult(const MIRModule &module, const KlassHierarchy &klassHierarchy, MemPool &memPool)
      : module(module),
        klassHierarchy(klassHierarchy),
        allocator(&memPool),
        classesCorrectness(allocator.Adapter()),
        classesPragma(allocator.Adapter()) {}

  ~VerifyResult() = default;

  const KlassHierarchy &GetKlassHierarchy() const {
    return klassHierarchy;
  }

  const MIRModule &GetMIRModule() const {
    return module;
  }

  const MIRFunction *GetCurrentFunction() const {
    return module.GetFunctionList().front();
  }

  const std::string &GetCurrentClassName() const {
    return GetCurrentFunction()->GetClassType()->GetName();
  }

  const ClassVerifyPragmas &GetDeferredClassesPragma() const {
    return classesPragma;
  }

  void AddPragmaVerifyError(const std::string &className, std::string errMsg);
  void AddPragmaAssignableCheck(const std::string &className, std::string fromType, std::string toType);
  void AddPragmaExtendFinalCheck(const std::string &className);
  void AddPragmaOverrideFinalCheck(const std::string &className);

  const MapleUnorderedMap<std::string, bool> &GetResultMap() const {
    return classesCorrectness;
  }
  void SetClassCorrectness(const std::string &className, bool result) {
    classesCorrectness[className] = result;
  }

  bool HasErrorNotDeferred() const {
    for (auto &classResult : classesCorrectness) {
      if (!classResult.second) {
        if (classesPragma.find(classResult.first) == classesPragma.end()) {
          // Verify result is not OK, but has no deferred check or verify error in runtime
          return true;
        }
      }
    }
    return false;
  }

 private:
  bool HasVerifyError(const std::vector<const VerifyPragmaInfo*> &pragmaInfoPtrVec) const;
  bool HasSamePragmaInfo(const std::vector<const VerifyPragmaInfo*> &pragmaInfoPtrVec,
                         const VerifyPragmaInfo &verifyPragmaInfo) const;

  const MIRModule &module;
  const KlassHierarchy &klassHierarchy;
  MapleAllocator allocator;
  // classesCorrectness<key: className, value: correctness>, correctness is true only if the class is verified OK
  MapleUnorderedMap<std::string, bool> classesCorrectness;
  // classesPragma<key: className, value: PragmaInfo for deferred check>
  ClassVerifyPragmas classesPragma;
};

class VerificationPhaseResult : public AnalysisResult {
 public:
  VerificationPhaseResult(MemPool &mp, const VerifyResult &verifyResult)
      : AnalysisResult(&mp), verifyResult(verifyResult) {}
  ~VerificationPhaseResult() override = default;

  const ClassVerifyPragmas &GetDeferredClassesPragma() const {
    return verifyResult.GetDeferredClassesPragma();
  }

 private:
  const VerifyResult &verifyResult;
};

#ifdef NOT_USED
class DoVerification : public ModulePhase {
 public:
  explicit DoVerification(ModulePhaseID id) : ModulePhase(id) {}

  AnalysisResult *Run(MIRModule *module, ModuleResultMgr *mgr) override;
  std::string PhaseName() const override {
    return "verification";
  }

  ~DoVerification() = default;

 private:
  void VerifyModule(MIRModule &module, VerifyResult &result) const;
  void DeferredCheckFinalClassAndMethod(VerifyResult &result) const;
  bool IsLazyBindingOrDecouple(const KlassHierarchy &klassHierarchy) const;
  bool NeedRuntimeFinalCheck(const KlassHierarchy &klassHierarchy, const std::string &className) const;
  void CheckExtendFinalClass(VerifyResult &result) const;
};
#endif
}  // namespace maple
#endif  // MAPLEIR_VERIFICATION_PHASE_H
