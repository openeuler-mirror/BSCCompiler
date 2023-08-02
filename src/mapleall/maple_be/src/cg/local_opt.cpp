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
#include "local_opt.h"
#include "cg.h"
#include "mpl_logging.h"
#if defined TARGX86_64
#include "x64_reaching.h"
#endif
/*
 * this phase does optimization on local level(single bb or super bb)
 * this phase requires liveanalysis
 */
namespace maplebe {
void LocalOpt::DoLocalCopyPropOptmize() {
  DoLocalCopyProp();
}

void LocalPropOptimizePattern::Run() {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (!CheckCondition(*insn)) {
        continue;
      }
      Optimize(*bb, *insn);
    }
  }
}

bool LocalCopyProp::PhaseRun(maplebe::CGFunc &f) {
  MemPool *mp = GetPhaseMemPool();
  LiveAnalysis *liveInfo = nullptr;
  liveInfo = GET_ANALYSIS(CgLiveAnalysis, f);
  liveInfo->ResetLiveSet();
  auto *reachingDef = f.GetCG()->CreateReachingDefinition(*mp, f);
  LocalOpt *localOpt = f.GetCG()->CreateLocalOpt(*mp, f, *reachingDef);
  localOpt->DoLocalCopyPropOptmize();
  return false;
}

void LocalCopyProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.AddPreserved<CgLiveAnalysis>();
}

bool RedundantDefRemove::CheckCondition(Insn &insn) {
  uint32 opndNum = insn.GetOperandSize();
  const InsnDesc *md = insn.GetDesc();
  std::vector<Operand*> defOpnds;
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    auto *opndDesc = md->opndMD[i];
    if (opndDesc->IsDef() && opndDesc->IsUse()) {
      return false;
    }
    if (opnd.IsList()) {
      continue;
    }
    if (opndDesc->IsDef()) {
      defOpnds.emplace_back(&opnd);
    }
  }
  if (defOpnds.size() != 1 || !defOpnds[0]->IsRegister()) {
    return false;
  }
  auto &regDef = static_cast<RegOperand&>(*defOpnds[0]);
  auto &liveOutRegSet = insn.GetBB()->GetLiveOutRegNO();
  if (liveOutRegSet.find(regDef.GetRegisterNumber()) != liveOutRegSet.end()) {
    return false;
  }
  return true;
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(LocalCopyProp, localcopyprop)
}  /* namespace maplebe */
