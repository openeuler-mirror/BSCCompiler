/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "cg_sink.h"
#include "live.h"

namespace maplebe {
#define SINK_DUMP CG_DEBUG_FUNC(cgFunc)

static void CollectInsnOpnds(const Insn &insn, std::vector<regno_t> &defOpnds, std::vector<regno_t> &useOpnds) {
  const InsnDesc *md = insn.GetDesc();
  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    auto &opnd = insn.GetOperand(i);
    const auto *opndDesc = md->GetOpndDes(i);
    if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      if (opndDesc->IsDef()) {
        defOpnds.push_back(regOpnd.GetRegisterNumber());
      }
      if (opndDesc->IsUse()) {
        useOpnds.push_back(regOpnd.GetRegisterNumber());
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      if (memOpnd.GetBaseRegister() != nullptr) {
        useOpnds.push_back(memOpnd.GetBaseRegister()->GetRegisterNumber());
        if (memOpnd.GetAddrMode() == MemOperand::kPreIndex || memOpnd.GetAddrMode() == MemOperand::kPostIndex) {
          defOpnds.push_back(memOpnd.GetBaseRegister()->GetRegisterNumber());
        }
      }
      if (memOpnd.GetIndexRegister() != nullptr) {
        useOpnds.push_back(memOpnd.GetIndexRegister()->GetRegisterNumber());
      }
    } else if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto *regOpnd : std::as_const(listOpnd.GetOperands())) {
        if (opndDesc->IsDef()) {
          defOpnds.push_back(regOpnd->GetRegisterNumber());
        }
        if (opndDesc->IsUse()) {
          useOpnds.push_back(regOpnd->GetRegisterNumber());
        }
      }
    }
  }
}

static bool IsInsnReadOrModifyMemory(const Insn &insn) {
  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    if (insn.GetOperand(i).IsMemoryAccessOperand()) {
      return true;
    }
  }
  return false;
}

bool PostRASink::HasRegisterDependency(const std::vector<regno_t> &defRegs,
                                       const std::vector<regno_t> &useRegs) const {
  for (auto defRegNO : defRegs) {
    if (modifiedRegs.count(defRegNO) != 0 || usedRegs.count(defRegNO) != 0) {
      return true;
    }
  }
  for (auto useRegNO : useRegs) {
    if (modifiedRegs.count(useRegNO) != 0) {
      return true;
    }
  }
  return false;
}

void PostRASink::UpdateRegsUsedDefed(const std::vector<regno_t> &defRegs, const std::vector<regno_t> &useRegs) {
  for (auto defRegNO : defRegs) {
    modifiedRegs.insert(defRegNO);
  }
  for (auto useRegNO : useRegs) {
    usedRegs.insert(useRegNO);
  }
}

BB *PostRASink::GetSingleLiveInSuccBB(const BB &curBB, const std::set<BB*> &sinkableBBs,
                                      const std::vector<regno_t> &defRegs) const {
  BB *singleBB = nullptr;
  for (auto defReg : defRegs) {
    // check if any register is live-in in other successors
    for (auto *succ : curBB.GetSuccs()) {
      if (sinkableBBs.count(succ) == 0 && succ->GetLiveInRegNO().count(defReg) != 0) {
        return nullptr;
      }
    }
    for (auto *sinkBB : sinkableBBs) {
      if (sinkBB->GetLiveInRegNO().count(defReg) == 0) {
        continue;
      }
      if (singleBB != nullptr && singleBB != sinkBB) {
        return nullptr;
      }
      singleBB = sinkBB;
    }
  }
  return singleBB;
}

void PostRASink::SinkInsn(Insn &insn, BB &sinkBB) const {
  if (SINK_DUMP) {
    LogInfo::MapleLogger() << "Sink insn : ";
    insn.Dump();
    LogInfo::MapleLogger() << "\t into BB " << sinkBB.GetId();
  }
  insn.GetBB()->RemoveInsn(insn);
  sinkBB.InsertInsnBegin(insn);
}

void PostRASink::UpdateLiveIn(BB &sinkBB, const std::vector<regno_t> &defRegs,
                              const std::vector<regno_t> &useRegs) const {
  for (auto defRegNO : defRegs) {
    sinkBB.GetLiveInRegNO().erase(defRegNO);
  }
  for (auto useRegNO : useRegs) {
    sinkBB.GetLiveInRegNO().insert(useRegNO);
  }
}

bool PostRASink::TryToSink(BB &bb) {
  std::set<BB*> sinkableBBs;
  for (auto *succ : bb.GetSuccs()) {
    if (!succ->GetLiveInRegNO().empty() && succ->GetPreds().size() == 1) {
      sinkableBBs.insert(succ);
    }
  }
  if (sinkableBBs.empty()) {  // no bb can sink
    return false;
  }

  bool changed = false;

  modifiedRegs.clear();
  usedRegs.clear();
  FOR_BB_INSNS_REV(insn, &bb) {
    if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
      continue;
    }
    if (insn->IsCall()) {   // not sink insn cross call
      return changed;
    }

    std::vector<regno_t> defedRegsInInsn;
    std::vector<regno_t> usedRegsInInsn;
    CollectInsnOpnds(*insn, defedRegsInInsn, usedRegsInInsn);

    // Don't sink the insn that will read or modify memory
    if (IsInsnReadOrModifyMemory(*insn)) {
      UpdateRegsUsedDefed(defedRegsInInsn, usedRegsInInsn);
      continue;
    }

    // Don't sink the insn that does not define any register and violates the register dependency
    if (defedRegsInInsn.empty() || HasRegisterDependency(defedRegsInInsn, usedRegsInInsn)) {
      UpdateRegsUsedDefed(defedRegsInInsn, usedRegsInInsn);
      continue;
    }

    auto *sinkBB = GetSingleLiveInSuccBB(bb, sinkableBBs, defedRegsInInsn);
    // Don't sink if we cannot find a single sinkable successor in which Reg is live-in.
    if (sinkBB == nullptr || sinkBB->GetKind() == BB::kBBReturn) {
      UpdateRegsUsedDefed(defedRegsInInsn, usedRegsInInsn);
      continue;
    }
    ASSERT(sinkBB->GetPreds().size() == 1 && *sinkBB->GetPredsBegin() == &bb, "NIY, unexpected sinkBB");

    SinkInsn(*insn, *sinkBB);
    UpdateLiveIn(*sinkBB, defedRegsInInsn, usedRegsInInsn);
    changed = true;
  }
  return changed;
}

void PostRASink::Run() {
  bool changed = false;
  do {
    changed = false;
    FOR_ALL_BB(bb, &cgFunc) {
      if (TryToSink(*bb)) {
        changed = true;
      }
    }
  } while (changed);
}

void CgPostRASink::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}

bool CgPostRASink::PhaseRun(maplebe::CGFunc &f) {
  auto *live = GET_ANALYSIS(CgLiveAnalysis, f);
  CHECK_FATAL(live != nullptr, "null ptr check");
  live->ResetLiveSet();

  PostRASink sink(f, *GetPhaseMemPool());
  sink.Run();

  return false;
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPostRASink, postrasink)
}  // namespace maplebe

