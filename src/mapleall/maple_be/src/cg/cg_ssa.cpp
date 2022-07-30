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
uint32 CGSSAInfo::ssaRegNObase = 100;
void CGSSAInfo::ConstructSSA() {
  InsertPhiInsn();
  /* Rename variables */
  RenameVariablesForBB(domInfo->GetCommonEntryBB().GetId());
#if DEBUG
  /* Check phiListOpnd, must be ssaForm */
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsPhi()) {
        continue;
      }
      Operand &phiListOpnd = insn->GetOperand(kInsnSecondOpnd);
      CHECK_FATAL(phiListOpnd.IsPhi(), "unexpect phi operand");
      MapleMap<uint32, RegOperand*> &phiList = static_cast<PhiOperand&>(phiListOpnd).GetOperands();
      for (auto &phiOpndIt : phiList) {
        if (!phiOpndIt.second->IsSSAForm()) {
          CHECK_FATAL(false, "phiOperand is not ssaForm!");
        }
      }
    }
  }
#endif
  cgFunc->SetSSAvRegCount(static_cast<uint32>(GetAllSSAOperands().size()) + ssaRegNObase + 1);
  /* save reversePostOrder of bbs for rectify validbit */
  SetReversePostOrder();
}

void CGSSAInfo::MarkInsnsInSSA(Insn &insn) {
  CHECK_FATAL(insn.GetId() == 0, "insn is not clean !!"); /* change to assert */
  insnCount += 2;
  insn.SetId(static_cast<uint32>(insnCount));
}

void CGSSAInfo::InsertPhiInsn() {
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
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

void CGSSAInfo::PrunedPhiInsertion(const BB &bb, RegOperand &virtualOpnd) {
  regno_t vRegNO = virtualOpnd.GetRegisterNumber();
  MapleVector<uint32> frontiers = domInfo->GetDomFrontier(bb.GetId());
  for (auto i : frontiers) {
    BB *phiBB = cgFunc->GetBBFromID(i);
    CHECK_FATAL(phiBB != nullptr, "get phiBB failed change to ASSERT");
    if (phiBB->HasPhiInsn(vRegNO)) {
      continue;
    }
    if (phiBB->GetLiveIn()->TestBit(vRegNO)) {
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
      MarkInsnsInSSA(phiInsn);
      bool insertSuccess = false;
      FOR_BB_INSNS(insn, phiBB) {
        if (insn->IsMachineInstruction()) {
          (void)phiBB->InsertInsnBefore(*insn, phiInsn);
          insertSuccess = true;
          break;
        }
      }
      if (!insertSuccess) {
        phiBB->InsertInsnBegin(phiInsn);
      }
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
  size_t tempSize = vRegStk.empty() ? allSSAOperands.size() + cgFunc->GetFirstMapleIrVRegNO() + 1 :
      vRegStk.rbegin()->first + 1;
  std::vector<int32> oriStackSize(tempSize, -1);
  for (auto it : vRegStk) {
    ASSERT(it.first < oriStackSize.size(), "out of range");
    oriStackSize[it.first] = static_cast<int32>(it.second.size());
  }
  RenamePhi(bb);
  FOR_BB_INSNS(insn, &bb) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    MarkInsnsInSSA(*insn);
    RenameInsn(*insn);
  }
  RenameSuccPhiUse(bb);
  RenameVariablesForBB(bb.GetId());
  /* stack pop up */
  for (auto &it : vRegStk) {
    if (it.first < oriStackSize.size() && oriStackSize[it.first] >= 0) {
      while (static_cast<int32>(it.second.size()) > oriStackSize[static_cast<uint64>(it.first)]) {
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
    VRegVersion *newVst = CreateNewVersion(*phiDefOpnd, *phiInsn, kInsnFirstOpnd, true);
    phiInsn->SetOperand(kInsnFirstOpnd, *newVst->GetSSAvRegOpnd());
  }
}

void CGSSAInfo::RenameSuccPhiUse(const BB &bb) {
  for (auto *sucBB : bb.GetSuccs()) {
    for (auto phiInsnIt : sucBB->GetPhiInsns()) {
      Insn *phiInsn = phiInsnIt.second;
      CHECK_FATAL(phiInsn != nullptr, "get phi insn failed");
      Operand *phiListOpnd = &phiInsn->GetOperand(kInsnSecondOpnd);
      CHECK_FATAL(phiListOpnd->IsPhi(), "unexpect phi operand");
      MapleMap<uint32, RegOperand*> &phiList = static_cast<PhiOperand*>(phiListOpnd)->GetOperands();
      ASSERT(phiList.size() <= sucBB->GetPreds().size(), "unexpect phiList size need check");
      for (auto phiOpndIt = phiList.begin(); phiOpndIt != phiList.end(); ++phiOpndIt) {
        if (phiOpndIt->first == bb.GetId()) {
          RegOperand *renamedOpnd = GetRenamedOperand(*(phiOpndIt->second), false, *phiInsn, kInsnSecondOpnd);
          phiList[phiOpndIt->first] = renamedOpnd;
        }
      }
    }
  }
}

uint32 CGSSAInfo::IncreaseVregCount(regno_t vRegNO) {
  if (vRegDefCount.count(vRegNO) == 0) {
    vRegDefCount.emplace(vRegNO, 0);
  } else {
    vRegDefCount[vRegNO]++;
  }
  return vRegDefCount[vRegNO];
}

bool CGSSAInfo::IncreaseSSAOperand(regno_t vRegNO, VRegVersion *vst) {
  if (allSSAOperands.count(vRegNO) != 0) {
    return false;
  }
  allSSAOperands.emplace(vRegNO, vst);
  return true;
}

VRegVersion *CGSSAInfo::CreateNewVersion(RegOperand &virtualOpnd, Insn &defInsn, uint32 idx, bool isDefByPhi) {
  regno_t vRegNO = virtualOpnd.GetRegisterNumber();
  uint32 verionIdx = IncreaseVregCount(vRegNO);
  RegOperand *ssaOpnd = CreateSSAOperand(virtualOpnd);
  auto *newVst = memPool->New<VRegVersion>(ssaAlloc, *ssaOpnd, verionIdx, vRegNO);
  auto *defInfo = CreateDUInsnInfo(&defInsn, idx);
  newVst->SetDefInsn(defInfo, isDefByPhi ? kDefByPhi : kDefByInsn);
  if (!IncreaseSSAOperand(ssaOpnd->GetRegisterNumber(), newVst)) {
    CHECK_FATAL(false, "insert ssa operand failed");
  }
  auto it = vRegStk.find(vRegNO);
  if (it == vRegStk.end()) {
    MapleStack<VRegVersion*> vRegVersionStack(ssaAlloc.Adapter());
    auto ret = vRegStk.emplace(std::pair<regno_t, MapleStack<VRegVersion*>>(vRegNO, vRegVersionStack));
    CHECK_FATAL(ret.second, "insert failed");
    it = ret.first;
  }
  it->second.push(newVst);
  return newVst;
}

VRegVersion *CGSSAInfo::GetVersion(const RegOperand &virtualOpnd) {
  regno_t vRegNO = virtualOpnd.GetRegisterNumber();
  auto vRegIt = vRegStk.find(vRegNO);
  return vRegIt != vRegStk.end() ? vRegIt->second.top() : nullptr;
}

VRegVersion *CGSSAInfo::FindSSAVersion(regno_t ssaRegNO) {
  auto it = allSSAOperands.find(ssaRegNO);
  return it != allSSAOperands.end() ? it->second : nullptr;
}

PhiOperand &CGSSAInfo::CreatePhiOperand() {
  return cgFunc->GetCG()->CreatePhiOperand(*memPool, ssaAlloc);
}

void CGSSAInfo::SetReversePostOrder() {
  MapleVector<BB*> &reverse = domInfo->GetReversePostOrder();
  for (auto *bb : reverse) {
    if (bb != nullptr) {
      reversePostOrder.emplace_back(bb->GetId());
    }
  }
}

Insn *CGSSAInfo::GetDefInsn(const RegOperand &useReg) {
  if (!useReg.IsSSAForm()) {
    return nullptr;
  }
  regno_t useRegNO = useReg.GetRegisterNumber();
  VRegVersion *useVersion = FindSSAVersion(useRegNO);
  ASSERT(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
  CHECK_FATAL(!useVersion->IsDeleted(), "deleted version");
  DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
  return defInfo == nullptr ? nullptr : defInfo->GetInsn();
}

void CGSSAInfo::DumpFuncCGIRinSSAForm() const {
  LogInfo::MapleLogger() << "\n******  SSA CGIR for " << cgFunc->GetName() << " *******\n";
  FOR_ALL_BB_CONST(bb, cgFunc) {
    cgFunc->DumpBBInfo(bb);
    FOR_BB_INSNS_CONST(insn, bb) {
      if (insn->IsCfiInsn() && insn->IsDbgInsn()) {
        insn->Dump();
      } else {
        DumpInsnInSSAForm(*insn);
      }
    }
  }
}

void VRegVersion::AddUseInsn(CGSSAInfo &ssaInfo, Insn &useInsn, uint32 idx) {
  ASSERT(useInsn.GetId() > 0, "insn should be marked during ssa");
  auto useInsnIt = useInsnInfos.find(useInsn.GetId());
  if (useInsnIt != useInsnInfos.end()) {
    useInsnIt->second->IncreaseDU(idx);
  } else {
    useInsnInfos.insert(std::make_pair(useInsn.GetId(), ssaInfo.CreateDUInsnInfo(&useInsn, idx)));
  }
}

void VRegVersion::RemoveUseInsn(const Insn &useInsn, uint32 idx) {
  auto useInsnIt = useInsnInfos.find(useInsn.GetId());
  ASSERT(useInsnIt != useInsnInfos.end(), "use Insn not found");
  useInsnIt->second->DecreaseDU(idx);
  if (useInsnIt->second->HasNoDU()) {
    useInsnInfos.erase(useInsnIt);
  }
}

void VRegVersion::CheckDeadUse(const Insn &useInsn) {
  auto useInsnIt = useInsnInfos.find(useInsn.GetId());
  ASSERT(useInsnIt != useInsnInfos.end(), "use Insn not found");
  if (useInsnIt->second->HasNoDU()) {
    useInsnInfos.erase(useInsnIt);
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
  MemPool *ssaTempMp = ApplyTempMemPool();
  DomAnalysis *domInfo = nullptr;
  domInfo = GET_ANALYSIS(CgDomAnalysis, f);
  LiveAnalysis *liveInfo = nullptr;
  liveInfo = GET_ANALYSIS(CgLiveAnalysis, f);
  ssaInfo = f.GetCG()->CreateCGSSAInfo(*ssaMemPool, f, *domInfo, *ssaTempMp);
  ssaInfo->ConstructSSA();

  if (CG_DEBUG_FUNC(f)) {
    LogInfo::MapleLogger() << "******** CG IR After ssaconstruct in ssaForm: *********" << "\n";
    ssaInfo->DumpFuncCGIRinSSAForm();
  }
  if (liveInfo != nullptr) {
    liveInfo->ClearInOutDataInfo();
  }
  /* due to change of register number */
  GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLiveAnalysis::id);
  return true;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgSSAConstruct, cgssaconstruct) /* both transform & analysis */
}
