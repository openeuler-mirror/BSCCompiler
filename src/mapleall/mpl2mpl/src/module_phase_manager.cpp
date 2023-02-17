/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "module_phase_manager.h"
#include "me_phase_manager.h"
#include "ipa_phase_manager.h"
#include "ipa_side_effect.h"
#include "mpl_profdata_parser.h"
#include "openProfile.h"
#include "update_mplt.h"
#include "gen_profile.h"

#define JAVALANG (mirModule.IsJavaModule())
#define CLANG (mirModule.IsCModule())

namespace maple {
void DumpModule(const MIRModule &mod, const std::string phaseName, bool isBefore) {
  MIRFunction *func = nullptr;
  if (Options::DumpFunc()) {
    GStrIdx gStrIdxOfFunc = GlobalTables::GetStrTable().GetStrIdxFromName(Options::dumpFunc);
    func = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdxOfFunc)->GetFunction();
  }
  bool dumpPhase = Options::DumpPhase(phaseName);
  if (Options::dumpBefore && dumpPhase && isBefore) {
    LogInfo::MapleLogger() << ">>>>> Dump before " << phaseName << " <<<<<\n";
    if (func != nullptr) {
      func->Dump(false);
    } else {
      mod.Dump();
    }
    LogInfo::MapleLogger() << ">>>>> Dump before End <<<<<\n";
    return;
  }
  if ((Options::dumpAfter || dumpPhase) && !isBefore) {
    LogInfo::MapleLogger() << ">>>>> Dump after " << phaseName << " <<<<<\n";
    if (func != nullptr) {
      func->Dump(false);
    } else {
      mod.Dump();
    }
    LogInfo::MapleLogger() << ">>>>> Dump after End <<<<<\n\n";
  }
}

void MEBETopLevelManager::InitFuncDescWithWhiteList(const maple::MIRModule &mod) {
  if (!Options::sideEffectWhiteList) {
    return;
  }
  for (auto &pair : SideEffect::GetWhiteList()) {
    auto *funcSymbol =
        GlobalTables::GetGsymTable().GetSymbolFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(pair.first));
    MIRFunction *func = nullptr;
    if (funcSymbol != nullptr && funcSymbol->GetSKind() == kStFunc) {
      func = mod.GetMIRBuilder()->GetFunctionFromSymbol(*funcSymbol);
    }
    if (func == nullptr) {
      continue;
    }
    const FuncDesc &desc = pair.second;
    func->SetFuncDesc(desc);
    func->GetFuncDesc().SetConfiged();
    if (desc.IsPure() || desc.IsConst()) {
      func->SetAttr(FUNCATTR_pure);
      func->SetAttr(FUNCATTR_nodefargeffect);
      func->SetAttr(FUNCATTR_nodefeffect);
      func->SetAttr(FUNCATTR_nothrow_exception);
    }
  }
}

void MEBETopLevelManager::Run(maple::MIRModule &mod) {
  if (mod.IsJavaModule()) {
    Options::sideEffectWhiteList = true;
  }
  InitFuncDescWithWhiteList(mod);
  auto admMempool = AllocateMemPoolInPhaseManager("MEBETopLevelManager's Analysis Data Manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  for (size_t i = 0; i < phasesSequence.size(); ++i) {
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "<<<Run Module " << (curPhase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << curPhase->PhaseName()  << " ]>>>\n";
    }
    DumpModule(mod, curPhase->PhaseName(), true);
    if (curPhase->IsAnalysis()) {
      (void)RunAnalysisPhase<MapleModulePhase, MIRModule>(*curPhase, *serialADM, mod);
    } else {
      (void)RunTransformPhase<MapleModulePhase, MIRModule>(*curPhase, *serialADM, mod);
    }
    DumpModule(mod, curPhase->PhaseName(), false);
  }
  serialADM->EraseAllAnalysisPhase();
}

void MEBETopLevelManager::DoPhasesPopulate(const maple::MIRModule &mirModule) {
#define MODULE_PHASE
#include "phases.def"
#undef MODULE_PHASE
}

MAPLE_TRANSFORM_PHASE_REGISTER(MEBETopLevelManager, TopLevelManager)

MAPLE_ANALYSIS_PHASE_REGISTER(IpaSccPM, IpaSccPM)
MAPLE_ANALYSIS_PHASE_REGISTER(M2MClone, clone)
MAPLE_ANALYSIS_PHASE_REGISTER(M2MIpaClone, ipaclone)
MAPLE_ANALYSIS_PHASE_REGISTER(M2MCallGraph, callgraph)
MAPLE_ANALYSIS_PHASE_REGISTER(M2MKlassHierarchy, classhierarchy)
MAPLE_ANALYSIS_PHASE_REGISTER(M2MAnnotationAnalysis, annotationanalysis)
MAPLE_ANALYSIS_PHASE_REGISTER(ProfileGenPM, ProfileGenPM)
MAPLE_ANALYSIS_PHASE_REGISTER(MMplProfDataParser, grofDataParser)

MAPLE_TRANSFORM_PHASE_REGISTER(M2MInline, inline)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MGInline, ginline)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MOutline, outline)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MIPODevirtualize, ipodevirtulize)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MFuncDeleter, funcdeleter)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MMethodReplace, methodreplace)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MScalarReplacement, ScalarReplacement)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MOpenprofile, openprofile)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MPreme, preme)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MPreCheckCast, precheckcast)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MCheckCastGeneration, gencheckcast)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MMuidReplacement, MUIDReplacement)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MGenerateNativeStubFunc, GenNativeStubFunc)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MJavaEHLowerer, javaehlower)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MCodeReLayout, CodeReLayout)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MClassInit, clinit)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MJavaIntrnLowering, javaintrnlowering)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MSimplify, simplify)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MUpdateMplt, updatemplt)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MReflectionAnalysis, reflectionanalysis)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MVtableImpl, VtableImpl)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MExpand128Floats, Expand128Floats)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MConstantFold, ConstantFold)
MAPLE_TRANSFORM_PHASE_REGISTER(M2MVtableAnalysis, vtableanalysis)
}  // namespace maple
