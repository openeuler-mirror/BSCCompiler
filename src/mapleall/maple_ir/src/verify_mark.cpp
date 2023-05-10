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
#include "verify_mark.h"
#include "verification.h"

namespace maple {
#ifdef NOT_USED
AnalysisResult *DoVerifyMark::Run(MIRModule *module, ModuleResultMgr *mgr) {
  LogInfo::MapleLogger() << "Start marking verification result" << '\n';

  CHECK_FATAL(module != nullptr, "Module should not be null");

  auto chaAnalysis = mgr->GetAnalysisResult(MoPhase_CHA, module);
  if (chaAnalysis == nullptr) {
    LogInfo::MapleLogger() << "Can't find class hierarchy result" << '\n';
    return nullptr;
  }
  auto verifyAnalysis = mgr->GetAnalysisResult(MoPhase_VERIFICATION, module);
  if (verifyAnalysis == nullptr) {
    LogInfo::MapleLogger() << "Can't find verification result" << '\n';
    return nullptr;
  }

  auto klassHierarchy = static_cast<KlassHierarchy*>(chaAnalysis);
  const auto *verifyResult = static_cast<VerificationPhaseResult*>(verifyAnalysis);
  const auto &verifyResultMap = verifyResult->GetDeferredClassesPragma();
  for (auto &classResult : verifyResultMap) {
    if (IsSystemPreloadedClass(classResult.first)) {
      continue;
    }
    Klass &klass = utils::ToRef(klassHierarchy->GetKlassFromName(classResult.first));
    klass.SetFlag(kClassRuntimeVerify);
    LogInfo::MapleLogger() << "class " << klass.GetKlassName() << " is Set as NOT VERIFIED\n";
    AddAnnotations(*module, klass, classResult.second);
  }

  LogInfo::MapleLogger() << "Finished marking verification result" << '\n';
  return nullptr;
}

void DoVerifyMark::AddAnnotations(MIRModule &module, const Klass &klass,
                                  const std::vector<const VerifyPragmaInfo*> &pragmaInfoVec) {
  MIRStructType *mirStructType = klass.GetMIRStructType();
  if (mirStructType == nullptr || pragmaInfoVec.empty()) {
    return;
  }
  const auto &className = klass.GetKlassName();
  std::vector<const AssignableCheckPragma*> assignableCheckPragmaVec;
  for (auto *pragmaItem : pragmaInfoVec) {
    switch (pragmaItem->GetPragmaType()) {
      case kThrowVerifyError: {
        auto *pragma = static_cast<const ThrowVerifyErrorPragma*>(pragmaItem);
        AddVerfAnnoThrowVerifyError(module, *pragma, *mirStructType);
        LogInfo::MapleLogger() << "\tInserted throw verify error annotation for class " << className << std::endl;
        LogInfo::MapleLogger() << "\tError: " << pragma->GetMessage() << std::endl;
        break;
      }
      case kAssignableCheck: {
        auto *pragma = static_cast<const AssignableCheckPragma*>(pragmaItem);
        assignableCheckPragmaVec.push_back(pragma);
        LogInfo::MapleLogger() << "\tInserted deferred assignable check from " << pragma->GetFromType() <<
            " to " << pragma->GetToType() << " in class " << className << '\n';
        break;
      }
      case kExtendFinalCheck: {
        AddVerfAnnoExtendFinalCheck(module, *mirStructType);
        break;
      }
      case kOverrideFinalCheck: {
        AddVerfAnnoOverrideFinalCheck(module, *mirStructType);
        break;
      }
      default:
        CHECK_FATAL(false, "\nError: Unknown Verify PragmaInfoType");
    }
  }
  AddVerfAnnoAssignableCheck(module, assignableCheckPragmaVec, *mirStructType);
}
#endif
} // namespace maple
