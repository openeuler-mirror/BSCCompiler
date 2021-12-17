/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_ssa.h"
#include "cg.h"

#include "optimize_common.h"

namespace maplebe {
uint32 CGSSAInfo::SSARegNObase = 100;
void VRegVersion::SetNewSSAOpnd(RegOperand &regOpnd) {
  ssaRegOpnd = &regOpnd;
}

void CGSSAInfo::ConstructSSA() {
  InsertPhiInsn();
  /* Rename variables */
  RenameVariablesForBB(domInfo->GetCommonEntryBB().GetId());
}

void CGSSAInfo::InsertPhiInsn() {
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb){
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      std::set<uint32> defRegNOs = insn->GetDefRegs();
      for (auto vRegNO : defRegNOs) {
        RegOperand *virtualOpnd = cgFunc->GetVirtualRegisterOperand(vRegNO);
        if (virtualOpnd != nullptr) {
          PrunedPhiInsertion(*bb, *virtualOpnd);
        }
      }
    }
  }
}

void CGSSAInfo::PrunedPhiInsertion(BB &bb, RegOperand &virtualOpnd) {
  regno_t vRegNO = virtualOpnd.GetRegisterNumber();
  MapleVector<uint32> frontiers = domInfo->GetDomFrontier(bb.GetId());
  for (auto i : frontiers) {
    BB *phiBB = cgFunc->GetBBFromID(i);
    CHECK_FATAL(phiBB != nullptr, "get phiBB failed change to ASSERT");
    if (phiBB->HasPhiInsn(vRegNO)) {
      continue;
    }
    /* change to TESTBIT */
    if (phiBB->GetLiveInRegNO().count(vRegNO) ) {
      CG *codeGen = cgFunc->GetCG();
      PhiOperand &phiList = codeGen->CreatePhiOperand(*memPool, ssaAlloc);
      /* do not insert phi opnd when insert phi insn? */
      for (auto prevBB : phiBB->GetPreds()) {
        if (prevBB->GetLiveOut()->TestBit(vRegNO)) {
          auto *paraOpnd = static_cast<RegOperand*>(virtualOpnd.Clone(*tempMp));
          phiList.InsertOpnd(prevBB->GetId(), *paraOpnd);
        } else {
          CHECK_FATAL(false, "multipule BB in");
        }
      }
      Insn &phiInsn = codeGen->BuildPhiInsn(virtualOpnd, phiList);
      phiBB->InsertInsnBegin(phiInsn);
      phiBB->AddPhiInsn(vRegNO, phiInsn);
      PrunedPhiInsertion(*phiBB, virtualOpnd);
    }
  }
}

void CGSSAInfo::RenameVariablesForBB(uint32 bbID) {
  RenameBB(*cgFunc->GetBBFromID(bbID)); /* rename first BB */
  const auto &domChildren = domInfo->GetDomChildren(bbID);
  for (const auto &child : domChildren) {
    RenameBB(*cgFunc->GetBBFromID(child));
  }
}

void CGSSAInfo::RenameBB(BB &bb) {
  if (IsBBRenamed(bb.GetId())) {
    return;
  }
  AddRenamedBB(bb.GetId());
  /* record version stack size */
  std::map<regno_t, uint32> oriStackSize;
  for (auto it : vRegStk) {
    oriStackSize.emplace(it.first, it.second.size());
  }
  RenamePhi(bb);
  FOR_BB_INSNS(insn, &bb) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    RenameInsn(*insn);
  }
  RenameSuccPhiUse(bb);
  RenameVariablesForBB(bb.GetId());
  /* stack pop up */
  for (auto &it : vRegStk) {
    if (oriStackSize.count(it.first)) {
      while (it.second.size() > oriStackSize[it.first]) {
        ASSERT(!it.second.empty(), "empty stack");
        it.second.pop();
      }
    }
  }
}

void CGSSAInfo::RenamePhi(BB &bb) {
  for (auto phiInsnIt : bb.GetPhiInsns()) {
    Insn *phiInsn = phiInsnIt.second;
    CHECK_FATAL(phiInsn != nullptr, "get phi insn failed");
    auto *phiDefOpnd = static_cast<RegOperand*>(&phiInsn->GetOperand(kInsnFirstOpnd));
    VRegVersion *newVst = CreateNewVersion(*phiDefOpnd, *phiInsn, true);
    phiInsn->SetOperand(kInsnFirstOpnd, *newVst->GetSSAvRegOpnd());
  }
}

void CGSSAInfo::RenameSuccPhiUse(BB &bb) {
  for (auto *sucBB : bb.GetSuccs()) {
    for (auto phiInsnIt : sucBB->GetPhiInsns()) {
      Insn *phiInsn = phiInsnIt.second;
      CHECK_FATAL(phiInsn != nullptr, "get phi insn failed");
      Operand *phiListOpnd = &phiInsn->GetOperand(kInsnSecondOpnd);
      CHECK_FATAL(phiListOpnd->IsPhi(), "unexpect phi operand");
      MapleMap<uint32, RegOperand*> &philist = static_cast<PhiOperand*>(phiListOpnd)->GetOperands();
      ASSERT(philist.size() <= sucBB->GetPreds().size(), "unexpect phiList size need check");
      for (auto phiOpndIt : philist) {
        if (phiOpndIt.first == bb.GetId()) {
          RegOperand *renamedOpnd = GetRenamedOperand(*(phiOpndIt.second), false, *phiInsn);
          philist[phiOpndIt.first] = renamedOpnd;
        }
      }
    }
  }
}

uint32 CGSSAInfo::IncreaseVregCount(regno_t vRegNO) {
  if (!vRegDefCount.count(vRegNO)) {
    vRegDefCount.emplace(vRegNO, 0);
  } else {
    vRegDefCount[vRegNO]++;
  }
  return vRegDefCount[vRegNO];
}

bool CGSSAInfo::IncreaseSSAOperand(regno_t vRegNO, VRegVersion *vst) {
  if (allSSAOperands.count(vRegNO)) {
    return false;
  }
  allSSAOperands.emplace(vRegNO, vst);
  return true;
}

VRegVersion *CGSSAInfo::CreateNewVersion(RegOperand &virtualOpnd, Insn &defInsn, bool isDefByPhi) {
  regno_t vRegNO = virtualOpnd.GetRegisterNumber();
  uint32 verionIdx = IncreaseVregCount(vRegNO);
  RegOperand *ssaOpnd = CreateSSAOperand(virtualOpnd);
  auto *newVst = memPool->New<VRegVersion>(ssaAlloc, *ssaOpnd, verionIdx, vRegNO);
  newVst->SetDefInsn(&defInsn, isDefByPhi ? kDefByPhi : kDefByInsn);
  if (!IncreaseSSAOperand(ssaOpnd->GetRegisterNumber(), newVst)) {
    CHECK_FATAL(false, "insert ssa operand failed");
  }
  auto it = vRegStk.find(vRegNO);
  if (it == vRegStk.end()) {
    MapleStack<VRegVersion*> vRegVersionStack(ssaAlloc.Adapter());
    auto ret = vRegStk.insert(std::pair<regno_t, MapleStack<VRegVersion*>>(vRegNO, vRegVersionStack));
    CHECK_FATAL(ret.second, "insert failed");
    it = ret.first;
  }

  it->second.push(newVst);
  return newVst;
}

VRegVersion *CGSSAInfo::GetVersion(RegOperand &virtualOpnd) {
  regno_t vRegNO = virtualOpnd.GetRegisterNumber();
  auto vRegIt = vRegStk.find(vRegNO);
  return vRegIt != vRegStk.end() ? vRegIt->second.top() : nullptr;
}

VRegVersion *CGSSAInfo::FindSSAVersion(regno_t ssaRegNO) {
  auto it = allSSAOperands.find(ssaRegNO);
  return it != allSSAOperands.end() ? it->second : nullptr;
}

void CGSSAInfo::DumpFuncCGIRinSSAForm() const {
  LogInfo::MapleLogger() << "\n******  SSA CGIR for " << cgFunc->GetName() << " *******\n";
  FOR_ALL_BB_CONST(bb, cgFunc) {
    LogInfo::MapleLogger() << "=== BB " << " <" << bb->GetKindName();
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      LogInfo::MapleLogger() << "[labeled with " << bb->GetLabIdx();
      LogInfo::MapleLogger() << " ==> @" << cgFunc->GetFunction().GetLabelName(bb->GetLabIdx()) << "]";
    }

    LogInfo::MapleLogger() << "> <" << bb->GetId() << "> ";
    if (bb->IsCleanup()) {
      LogInfo::MapleLogger() << "[is_cleanup] ";
    }
    if (bb->IsUnreachable()) {
      LogInfo::MapleLogger() << "[unreachable] ";
    }
    if (bb->GetFirstStmt() == cgFunc->GetCleanupLabel()) {
      LogInfo::MapleLogger() << "cleanup ";
    }
    if (!bb->GetSuccs().empty()) {
      LogInfo::MapleLogger() << "succs: ";
      for (auto *succBB : bb->GetSuccs()) {
        LogInfo::MapleLogger() << succBB->GetId() << " ";
      }
    }
    if (!bb->GetEhSuccs().empty()) {
      LogInfo::MapleLogger() << "eh_succs: ";
      for (auto *ehSuccBB : bb->GetEhSuccs()) {
        LogInfo::MapleLogger() << ehSuccBB->GetId() << " ";
      }
    }
    LogInfo::MapleLogger() << "===\n";
    LogInfo::MapleLogger() << "frequency:" << bb->GetFrequency() << "\n";

    FOR_BB_INSNS_CONST(insn, bb) {
      if (insn->IsCfiInsn() && insn->IsDbgInsn()) {
        insn->Dump();
      } else {
        CHECK_FATAL(!insn->IsDMBInsn(), "NYI");
        DumpInsnInSSAForm(*insn);
      }
    }
  }
}

void CgSSAConstruct::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgDomAnalysis>();
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}

bool CgSSAConstruct::PhaseRun(maplebe::CGFunc &f) {
  if (CG_DEBUG_FUNC(f)) {
    DotGenerator::GenerateDot("beforessa", f, f.GetMirModule(), true);
  }
  MemPool *ssaMemPool = GetPhaseMemPool();
  MemPool *ssaTempMp = ApplyTempMemPool(); /* delete after ssa construct */
  DomAnalysis *domInfo = nullptr;
  domInfo = GET_ANALYSIS(CgDomAnalysis, f);
  LiveAnalysis *liveInfo = nullptr;
  liveInfo = GET_ANALYSIS(CgLiveAnalysis, f);
  liveInfo->ResetLiveSet();
  ssaInfo = f.GetCG()->CreateCGSSAInfo(*ssaMemPool, f, *domInfo, *ssaTempMp);
  ssaInfo->ConstructSSA();
  if (CG_DEBUG_FUNC(f)) {
    LogInfo::MapleLogger() << "******** CG IR After ssaconstruct in ssaForm : *********" << "\n";
    ssaInfo->DumpFuncCGIRinSSAForm();
  }
  if (liveInfo != nullptr) {
    liveInfo->ClearInOutDataInfo();
  }
  return true;
}
}
