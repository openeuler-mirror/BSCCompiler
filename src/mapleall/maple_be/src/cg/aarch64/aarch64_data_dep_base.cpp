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
#include "aarch64_cg.h"
#include "aarch64_operand.h"
#include "pressure.h"
#include "aarch64_data_dep_base.h"

/* For building dependence graph, The entry is AArch64DataDepBase::Run. */
namespace maplebe {
void AArch64DataDepBase::ReplaceDepNodeWithNewInsn(DepNode &firstNode, DepNode &secondNode, Insn& newInsn,
                                                   bool isFromClinit) const {
  if (isFromClinit) {
    firstNode.AddClinitInsn(*firstNode.GetInsn());
    firstNode.AddClinitInsn(*secondNode.GetInsn());
    firstNode.SetCfiInsns(secondNode.GetCfiInsns());
  } else {
    for (Insn *insn : secondNode.GetCfiInsns()) {
      firstNode.AddCfiInsn(*insn);
    }
    for (Insn *insn : secondNode.GetComments()) {
      firstNode.AddComments(*insn);
    }
    secondNode.ClearComments();
  }
  firstNode.SetInsn(newInsn);
  Reservation *rev = mad.FindReservation(newInsn);
  CHECK_FATAL(rev != nullptr, "reservation is nullptr.");
  firstNode.SetReservation(*rev);
  firstNode.SetUnits(rev->GetUnit());
  firstNode.SetUnitNum(rev->GetUnitNum());
  newInsn.SetDepNode(firstNode);
}

void AArch64DataDepBase::ClearDepNodeInfo(DepNode &depNode) const {
  Insn &insn = cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_pseudo_none);
  insn.SetDepNode(depNode);
  Reservation *seRev = mad.FindReservation(insn);
  depNode.SetInsn(insn);
  depNode.SetType(kNodeTypeEmpty);
  depNode.SetReservation(*seRev);
  depNode.SetUnitNum(0);
  depNode.ClearCfiInsns();
  depNode.SetUnits(nullptr);
}

/* Combine (adrpldr & clinit_tail) to clinit. */
void AArch64DataDepBase::CombineClinit(DepNode &firstNode, DepNode &secondNode, bool isAcrossSeparator) {
  ASSERT(firstNode.GetInsn()->GetMachineOpcode() == MOP_adrp_ldr, "first insn should be adrpldr");
  ASSERT(secondNode.GetInsn()->GetMachineOpcode() == MOP_clinit_tail, "second insn should be clinit_tail");
  ASSERT(firstNode.GetCfiInsns().empty(), "There should not be any comment/cfi instructions between clinit.");
  ASSERT(secondNode.GetComments().empty(), "There should not be any comment/cfi instructions between clinit.");
  Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(
      MOP_clinit, firstNode.GetInsn()->GetOperand(0), firstNode.GetInsn()->GetOperand(1));
  newInsn.SetId(firstNode.GetInsn()->GetId());
  /* Replace first node with new insn. */
  ReplaceDepNodeWithNewInsn(firstNode, secondNode, newInsn, true);
  /* Clear second node information. */
  ClearDepNodeInfo(secondNode);
  CombineDependence(firstNode, secondNode, isAcrossSeparator);
}

/*
 * Combine memory access pair:
 *   1.ldr to ldp.
 *   2.str to stp.
 */
void AArch64DataDepBase::CombineMemoryAccessPair(DepNode &firstNode, DepNode &secondNode, bool useFirstOffset) {
  ASSERT(firstNode.GetInsn(), "the insn of first Node should not be nullptr");
  ASSERT(secondNode.GetInsn(), "the insn of second Node should not be nullptr");
  MOperator thisMop = firstNode.GetInsn()->GetMachineOpcode();
  MOperator mopPair = GetMopPair(thisMop);
  ASSERT(mopPair != 0, "mopPair should not be zero");
  Operand *opnd0 = nullptr;
  Operand *opnd1 = nullptr;
  Operand *opnd2 = nullptr;
  if (useFirstOffset) {
    opnd0 = &(firstNode.GetInsn()->GetOperand(0));
    opnd1 = &(secondNode.GetInsn()->GetOperand(0));
    opnd2 = &(firstNode.GetInsn()->GetOperand(1));
  } else {
    opnd0 = &(secondNode.GetInsn()->GetOperand(0));
    opnd1 = &(firstNode.GetInsn()->GetOperand(0));
    opnd2 = &(secondNode.GetInsn()->GetOperand(1));
  }
  Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mopPair, *opnd0, *opnd1, *opnd2);
  newInsn.SetId(firstNode.GetInsn()->GetId());
  std::string newComment;
  const MapleString &comment = firstNode.GetInsn()->GetComment();
  if (comment.c_str() != nullptr) {
    newComment += comment.c_str();
  }
  const MapleString &secondComment = secondNode.GetInsn()->GetComment();
  if (secondComment.c_str() != nullptr) {
    newComment += "  ";
    newComment += secondComment.c_str();
  }
  if ((newComment.c_str() != nullptr) && (strlen(newComment.c_str()) > 0)) {
    newInsn.SetComment(newComment);
  }
  /* Replace first node with new insn. */
  ReplaceDepNodeWithNewInsn(firstNode, secondNode, newInsn, false);
  /* Clear second node information. */
  ClearDepNodeInfo(secondNode);
  CombineDependence(firstNode, secondNode, false, true);
}

bool AArch64DataDepBase::IsFrameReg(const RegOperand &opnd) const {
  return (opnd.GetRegisterNumber() == RFP) || (opnd.GetRegisterNumber() == RSP);
}

MemOperand *AArch64DataDepBase::BuildNextMemOperandByByteSize(const MemOperand &aarchMemOpnd, uint32 byteSize) const {
  MemOperand *nextMemOpnd = nullptr;
  RegOperand *nextBaseReg = aarchMemOpnd.GetBaseRegister();
  CHECK_NULL_FATAL(nextBaseReg);
  ImmOperand *offsetOperand = static_cast<ImmOperand*>(aarchMemOpnd.GetOffset());
  int32 offsetVal = offsetOperand ? static_cast<int32>(offsetOperand->GetValue()) : 0;
  int32 nextOffVal = offsetVal + static_cast<int32>(byteSize);
  ImmOperand *nextOffOperand = memPool.New<ImmOperand>(nextOffVal, k32BitSize, true);
  CHECK_NULL_FATAL(nextOffOperand);
  nextMemOpnd = memPool.New<MemOperand>(aarchMemOpnd.GetSize(), *nextBaseReg,
                                        *nextOffOperand, aarchMemOpnd.GetAddrMode());
  return nextMemOpnd;
}

/* Get the second memory access operand of stp/ldp instructions. */
MemOperand *AArch64DataDepBase::GetNextMemOperand(const Insn &insn, const MemOperand &aarchMemOpnd) const {
  MemOperand *nextMemOpnd = nullptr;
  switch (insn.GetMachineOpcode()) {
    case MOP_wldp:
    case MOP_sldp:
    case MOP_xldpsw:
    case MOP_wstp:
    case MOP_sstp: {
      nextMemOpnd = BuildNextMemOperandByByteSize(aarchMemOpnd, k4ByteSize);
      break;
    }
    case MOP_xldp:
    case MOP_dldp:
    case MOP_xstp:
    case MOP_dstp: {
      nextMemOpnd = BuildNextMemOperandByByteSize(aarchMemOpnd, k8ByteSize);
      break;
    }
    default:
      break;
  }

  return nextMemOpnd;
}

/*
 * Build data dependence of symbol memory access.
 * Memory access with symbol must be a heap memory access.
 */
void AArch64DataDepBase::BuildDepsAccessStImmMem(Insn &insn, bool isDest) {
  if (isDest) {
    AddDependence4InsnInVectorByType(curCDGNode->GetHeapUseInsns(), insn, kDependenceTypeAnti);
    /* Build output dependence */
    AddDependence4InsnInVectorByType(curCDGNode->GetHeapDefInsns(), insn, kDependenceTypeOutput);
    curCDGNode->AddHeapDefInsn(&insn);
  } else {
    AddDependence4InsnInVectorByType(curCDGNode->GetHeapDefInsns(), insn, kDependenceTypeTrue);
    curCDGNode->AddHeapUseInsn(&insn);
  }
  Insn *membarInsn = curCDGNode->GetMembarInsn();
  if (membarInsn != nullptr) {
    AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
  }
}

/* Build data dependence of memory bars instructions */
void AArch64DataDepBase::BuildDepsMemBar(Insn &insn) {
  if (IsIntraBlockAnalysis()) {
    AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetStackUseInsns(), insn, kDependenceTypeMembar);
    AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetHeapUseInsns(), insn, kDependenceTypeMembar);
    AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetStackDefInsns(), insn, kDependenceTypeMembar);
    AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetHeapDefInsns(), insn, kDependenceTypeMembar);
  } else {
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kStackUses);
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kHeapUses);
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kStackDefs);
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kHeapDefs);
  }
  curCDGNode->SetMembarInsn(&insn);
}

/* Build data dependence of stack memory and heap memory uses */
void AArch64DataDepBase::BuildDepsUseMem(Insn &insn, MemOperand &aarchMemOpnd) {
  aarchMemOpnd.SetAccessSize(insn.GetMemoryByteSize());
  RegOperand *baseRegister = aarchMemOpnd.GetBaseRegister();
  MemOperand *nextMemOpnd = GetNextMemOperand(insn, aarchMemOpnd);
  if (IsIntraBlockAnalysis()) {
    /* Stack memory address */
    MapleVector<Insn*> stackDefs = curCDGNode->GetStackDefInsns();
    for (auto defInsn : stackDefs) {
      if (defInsn->IsCall() || NeedBuildDepsMem(aarchMemOpnd, nextMemOpnd, *defInsn)) {
        AddDependence(*defInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeTrue);
      }
    }
    /* Heap memory address */
    MapleVector<Insn*> heapDefs = curCDGNode->GetHeapDefInsns();
    AddDependence4InsnInVectorByType(heapDefs, insn, kDependenceTypeTrue);
  } else {
    BuildInterBlockMemDefUseDependency(*insn.GetDepNode(), aarchMemOpnd, nextMemOpnd, false);
  }
  if (((baseRegister != nullptr) && IsFrameReg(*baseRegister)) || aarchMemOpnd.IsStackMem()) {
    curCDGNode->AddStackUseInsn(&insn);
  } else {
    curCDGNode->AddHeapUseInsn(&insn);
  }
  Insn *membarInsn = curCDGNode->GetMembarInsn();
  if (membarInsn != nullptr) {
    AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
  } else if (!IsIntraBlockAnalysis()) {
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false,
                                             kDependenceTypeMembar, kMembar);
  }
}

/* Build data dependence of stack memory and heap memory definitions */
void AArch64DataDepBase::BuildDepsDefMem(Insn &insn, MemOperand &aarchMemOpnd) {
  RegOperand *baseRegister = aarchMemOpnd.GetBaseRegister();
  ASSERT(baseRegister != nullptr, "baseRegister shouldn't be null here");
  MemOperand *nextMemOpnd = GetNextMemOperand(insn, aarchMemOpnd);
  aarchMemOpnd.SetAccessSize(insn.GetMemoryByteSize());

  if (IsIntraBlockAnalysis()) {
    /* Build anti dependence */
    MapleVector<Insn*> stackUses = curCDGNode->GetStackUseInsns();
    for (auto *stackUse : stackUses) {
      if (NeedBuildDepsMem(aarchMemOpnd, nextMemOpnd, *stackUse)) {
        AddDependence(*stackUse->GetDepNode(), *insn.GetDepNode(), kDependenceTypeAnti);
      }
    }
    /* Build output dependence */
    MapleVector<Insn*> stackDefs = curCDGNode->GetStackDefInsns();
    for (auto stackDef : stackDefs) {
      if (stackDef->IsCall() || NeedBuildDepsMem(aarchMemOpnd, nextMemOpnd, *stackDef)) {
        AddDependence(*stackDef->GetDepNode(), *insn.GetDepNode(), kDependenceTypeOutput);
      }
    }
    /* Heap memory
   * Build anti dependence
   */
    MapleVector<Insn*> heapUses = curCDGNode->GetHeapUseInsns();
    AddDependence4InsnInVectorByType(heapUses, insn, kDependenceTypeAnti);
    /* Build output dependence */
    MapleVector<Insn*> heapDefs = curCDGNode->GetHeapDefInsns();
    AddDependence4InsnInVectorByType(heapDefs, insn, kDependenceTypeOutput);

    /* Memory definition can not across may-throw insns */
    MapleVector<Insn*> mayThrows = curCDGNode->GetMayThrowInsns();
    AddDependence4InsnInVectorByType(mayThrows, insn, kDependenceTypeThrow);
  } else {
    BuildInterBlockMemDefUseDependency(*insn.GetDepNode(), aarchMemOpnd, nextMemOpnd, true);
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeThrow, kMayThrows);
  }

  if (baseRegister->GetRegisterNumber() == RSP) {
    Insn *lastCallInsn = curCDGNode->GetLastCallInsn();
    if (lastCallInsn != nullptr) {
      /* Build a dependence between stack passed arguments and call */
      AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeControl);
    } else if (!IsIntraBlockAnalysis()) {
      BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeControl, kLastCall);
    }
  }

  Insn *membarInsn = curCDGNode->GetMembarInsn();
  if (membarInsn != nullptr) {
    AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
  } else if (!IsIntraBlockAnalysis()) {
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeMembar, kMembar);
  }

  /* Update cur cdgNode info */
  if (IsFrameReg(*baseRegister) || aarchMemOpnd.IsStackMem()) {
    curCDGNode->AddStackDefInsn(&insn);
  } else {
    curCDGNode->AddHeapDefInsn(&insn);
  }
}

static bool NoAlias(const MemOperand &leftOpnd, const MemOperand &rightOpnd) {
  if (leftOpnd.GetAddrMode() == MemOperand::kBOI && rightOpnd.GetAddrMode() == MemOperand::kBOI) {
    if (leftOpnd.GetBaseRegister()->GetRegisterNumber() == RFP ||
        rightOpnd.GetBaseRegister()->GetRegisterNumber() == RFP) {
      Operand *ofstOpnd = leftOpnd.GetOffsetOperand();
      Operand *rofstOpnd = rightOpnd.GetOffsetOperand();
      ASSERT(ofstOpnd != nullptr, "offset operand should not be null.");
      ASSERT(rofstOpnd != nullptr, "offset operand should not be null.");
      auto *ofst = static_cast<ImmOperand*>(ofstOpnd);
      auto *rofst = static_cast<ImmOperand*>(rofstOpnd);
      ASSERT(ofst != nullptr, "CG internal error, invalid type.");
      ASSERT(rofst != nullptr, "CG internal error, invalid type.");
      return (!ofst->ValueEquals(*rofst));
    }
  }
  return false;
}

static bool NoOverlap(const MemOperand &leftOpnd, const MemOperand &rightOpnd) {
  if (leftOpnd.GetAddrMode() != MemOperand::kBOI || rightOpnd.GetAddrMode() != MemOperand::kBOI ||
      !leftOpnd.IsIntactIndexed() || !rightOpnd.IsIntactIndexed()) {
    return false;
  }
  if (leftOpnd.GetBaseRegister()->GetRegisterNumber() != RFP ||
      rightOpnd.GetBaseRegister()->GetRegisterNumber() != RFP) {
    return false;
  }
  int64 ofset1 = leftOpnd.GetOffsetOperand()->GetValue();
  int64 ofset2 = rightOpnd.GetOffsetOperand()->GetValue();
  if (ofset1 < ofset2) {
    return ((ofset1 + static_cast<int8>(leftOpnd.GetAccessSize())) <= ofset2);
  } else {
    return ((ofset2 + static_cast<int8>(rightOpnd.GetAccessSize())) <= ofset1);
  }
}

/* Return true if memInsn's memOpnd no alias with memOpnd and nextMemOpnd */
bool AArch64DataDepBase::NeedBuildDepsMem(const MemOperand &memOpnd,
                                          const MemOperand *nextMemOpnd,
                                          const Insn &memInsn) const {
  auto *memOpndOfmemInsn = static_cast<MemOperand*>(memInsn.GetMemOpnd());
  if (!NoAlias(memOpnd, *memOpndOfmemInsn) ||
      ((nextMemOpnd != nullptr) && !NoAlias(*nextMemOpnd, *memOpndOfmemInsn))) {
    return true;
  }
  if (cgFunc.GetMirModule().GetSrcLang() == kSrcLangC && !memInsn.IsCall()) {
    CHECK_FATAL(memInsn.GetMemOpnd() != nullptr, "invalid MemOpnd");
    static_cast<MemOperand*>(memInsn.GetMemOpnd())->SetAccessSize(memInsn.GetMemoryByteSize());
    return (!NoOverlap(memOpnd, *memOpndOfmemInsn));
  }
  MemOperand *nextMemOpndOfmemInsn = GetNextMemOperand(memInsn, *memOpndOfmemInsn);
  if (nextMemOpndOfmemInsn != nullptr) {
    if (!NoAlias(memOpnd, *nextMemOpndOfmemInsn) ||
        ((nextMemOpnd != nullptr) && !NoAlias(*nextMemOpnd, *nextMemOpndOfmemInsn))) {
      return true;
    }
  }
  return false;
}

/*
 * Build dependence of call instructions.
 * caller-saved physical registers will be defined by a call instruction.
 * also a conditional register may be modified by a call.
 */
void AArch64DataDepBase::BuildCallerSavedDeps(Insn &insn) {
  /* Build anti dependence and output dependence. */
  for (uint32 i = R0; i <= R7; ++i) {
    BuildDepsDefReg(insn, i);
  }
  for (uint32 i = V0; i <= V7; ++i) {
    BuildDepsDefReg(insn, i);
  }
  if (!beforeRA) {
    for (uint32 i = R8; i <= R18; ++i) {
      BuildDepsDefReg(insn, i);
    }
    for (uint32 i = RLR; i <= RSP; ++i) {
      BuildDepsUseReg(insn, i);
    }
    for (uint32 i = V16; i <= V31; ++i) {
      BuildDepsDefReg(insn, i);
    }
  }
  /* For condition operand, such as NE, EQ, and so on. */
  if (cgFunc.GetRflag() != nullptr) {
    BuildDepsDefReg(insn, kRFLAG);
  }
}

/*
 * Build dependence between stack-define-instruction that deal with call-insn's args and a call-instruction.
 * insn : a call instruction (call/tail-call)
 */
void AArch64DataDepBase::BuildStackPassArgsDeps(Insn &insn) {
  MapleVector<Insn*> stackDefs = curCDGNode->GetStackDefInsns();
  for (auto stackDefInsn : stackDefs) {
    if (stackDefInsn->IsCall()) {
      continue;
    }
    Operand *opnd = stackDefInsn->GetMemOpnd();
    ASSERT(opnd->IsMemoryAccessOperand(), "make sure opnd is memOpnd");
    auto *memOpnd = static_cast<MemOperand*>(opnd);
    RegOperand *baseReg = memOpnd->GetBaseRegister();
    if ((baseReg != nullptr) && (baseReg->GetRegisterNumber() == RSP)) {
      AddDependence(*stackDefInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeControl);
    }
  }
}

/* Some insns may dirty all stack memory, such as "bl MCC_InitializeLocalStackRef" */
void AArch64DataDepBase::BuildDepsDirtyStack(Insn &insn) {
  /* Build anti dependence */
  MapleVector<Insn*> stackUses = curCDGNode->GetStackUseInsns();
  AddDependence4InsnInVectorByType(stackUses, insn, kDependenceTypeAnti);
  /* Build output dependence */
  MapleVector<Insn*> stackDefs = curCDGNode->GetStackDefInsns();
  AddDependence4InsnInVectorByType(stackDefs, insn, kDependenceTypeOutput);
  curCDGNode->AddStackDefInsn(&insn);
}

/* Some call insns may use all stack memory, such as "bl MCC_CleanupLocalStackRef_NaiveRCFast" */
void AArch64DataDepBase::BuildDepsUseStack(Insn &insn) {
  /* Build true dependence */
  MapleVector<Insn*> stackDefs = curCDGNode->GetStackDefInsns();
  AddDependence4InsnInVectorByType(stackDefs, insn, kDependenceTypeTrue);
}

/* Some insns may dirty all heap memory, such as a call insn */
void AArch64DataDepBase::BuildDepsDirtyHeap(Insn &insn) {
  /* Build anti dependence */
  MapleVector<Insn*> heapUses = curCDGNode->GetHeapUseInsns();
  AddDependence4InsnInVectorByType(heapUses, insn, kDependenceTypeAnti);
  /* Build output dependence */
  MapleVector<Insn*> heapDefs = curCDGNode->GetHeapDefInsns();
  AddDependence4InsnInVectorByType(heapDefs, insn, kDependenceTypeOutput);
  Insn *membarInsn = curCDGNode->GetMembarInsn();
  if (membarInsn != nullptr) {
    AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
  }
  curCDGNode->AddHeapDefInsn(&insn);
}

/* Analysis live-in registers in catch bb and cleanup bb */
void AArch64DataDepBase::AnalysisAmbiInsns(BB &bb) {
  curCDGNode->SetHasAmbiRegs(false);
  if (bb.GetEhSuccs().empty()) {
    return;
  }
  MapleSet<regno_t> &ehInRegs = curCDGNode->GetEhInRegs();

  /* Union all catch bb */
  for (auto succBB : bb.GetEhSuccs()) {
    const MapleSet<regno_t> &liveInRegSet = succBB->GetLiveInRegNO();
    (void)set_union(liveInRegSet.begin(), liveInRegSet.end(), ehInRegs.begin(), ehInRegs.end(),
                    inserter(ehInRegs, ehInRegs.begin()));
  }

  /* Union cleanup entry bb */
  const MapleSet<regno_t> &regNOSet = cgFunc.GetCleanupBB()->GetLiveInRegNO();
  (void)std::set_union(regNOSet.begin(), regNOSet.end(), ehInRegs.begin(), ehInRegs.end(),
                       inserter(ehInRegs, ehInRegs.begin()));

  /* Subtract R0 and R1, that is defined by eh runtime */
  (void)ehInRegs.erase(R0);
  (void)ehInRegs.erase(R1);
  if (ehInRegs.empty()) {
    return;
  }
  curCDGNode->SetHasAmbiRegs(true);
}

/*
 * It is a yieldpoint if loading from a dedicated
 * register holding polling page address:
 * ldr  wzr, [RYP]
 */
static bool IsYieldPoint(const Insn &insn) {
  if (insn.IsLoad() && !insn.IsLoadLabel()) {
    auto mem = static_cast<MemOperand*>(insn.GetMemOpnd());
    return (mem != nullptr && mem->GetBaseRegister() != nullptr && mem->GetBaseRegister()->GetRegisterNumber() == RYP);
  }
  return false;
}

/*
 * Build data dependence of memory operand.
 * insn : an instruction with the memory access operand.
 * opnd : the memory access operand.
 * regProp : operand property of the memory access operand.
 */
void AArch64DataDepBase::BuildMemOpndDependency(Insn &insn, Operand &opnd, const OpndDesc &regProp) {
  ASSERT(opnd.IsMemoryAccessOperand(), "opnd must be memory Operand");
  auto *memOpnd = static_cast<MemOperand*>(&opnd);
  RegOperand *baseRegister = memOpnd->GetBaseRegister();
  if (baseRegister != nullptr) {
    regno_t regNO = baseRegister->GetRegisterNumber();
    BuildDepsUseReg(insn, regNO);
    if (memOpnd->IsPostIndexed() || memOpnd->IsPreIndexed()) {
      /* Base operand has changed. */
      BuildDepsDefReg(insn, regNO);
    }
  }
  RegOperand *indexRegister = memOpnd->GetIndexRegister();
  if (indexRegister != nullptr) {
    regno_t regNO = indexRegister->GetRegisterNumber();
    BuildDepsUseReg(insn, regNO);
  }
  if (regProp.IsUse()) {
    BuildDepsUseMem(insn, *memOpnd);
  } else {
    BuildDepsDefMem(insn, *memOpnd);
    BuildDepsAmbiInsn(insn);
  }
  if (IsYieldPoint(insn)) {
    BuildDepsMemBar(insn);
    BuildDepsDefReg(insn, kRFLAG);
  }
}

/* Build Dependency for each operand of insn */
void AArch64DataDepBase::BuildOpndDependency(Insn &insn) {
  const InsnDesc* md = insn.GetDesc();
  MOperator mOp = insn.GetMachineOpcode();
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    const OpndDesc *regProp = md->opndMD[i];
    if (opnd.IsMemoryAccessOperand()) {
      BuildMemOpndDependency(insn, opnd, *regProp);
    } else if (opnd.IsStImmediate()) {
      if (mOp != MOP_xadrpl12) {
        BuildDepsAccessStImmMem(insn, false);
      }
    } else if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      regno_t regNO = regOpnd.GetRegisterNumber();
      if (regProp->IsUse()) {
        BuildDepsUseReg(insn, regNO);
      }
      if (regProp->IsDef()) {
        BuildDepsDefReg(insn, regNO);
      }
    } else if (opnd.IsConditionCode()) {
      /* For condition operand, such as NE, EQ, and so on. */
      if (regProp->IsUse()) {
        BuildDepsUseReg(insn, kRFLAG);
        BuildDepsBetweenControlRegAndCall(insn, false);
      }
      if (regProp->IsDef()) {
        BuildDepsDefReg(insn, kRFLAG);
        BuildDepsBetweenControlRegAndCall(insn, true);
      }
    } else if (opnd.IsList()) {
      auto &listOpnd = static_cast<const ListOperand&>(opnd);
      /* Build true dependences */
      for (auto &lst : listOpnd.GetOperands()) {
        regno_t regNO = lst->GetRegisterNumber();
        BuildDepsUseReg(insn, regNO);
      }
    }
  }
}

static bool IsLazyLoad(MOperator op) {
  return (op == MOP_lazy_ldr) || (op == MOP_lazy_ldr_static) || (op == MOP_lazy_tail);
}

/*
 * Build dependences in some special issue (stack/heap/throw/clinit/lazy binding/control flow).
 * insn :  an instruction.
 * depNode : insn's depNode.
 * nodes : the dependence nodes include insn's depNode.
 */
void AArch64DataDepBase::BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode*> &nodes) {
  const InsnDesc *md = insn.GetDesc();
  MOperator mOp = insn.GetMachineOpcode();
  if (insn.IsCall() || insn.IsTailCall()) {
    /* Caller saved registers */
    BuildCallerSavedDeps(insn);
    BuildStackPassArgsDeps(insn);

    if (mOp == MOP_xbl) {
      auto &target = static_cast<FuncNameOperand&>(insn.GetOperand(0));
      if ((target.GetName() == "MCC_InitializeLocalStackRef") ||
          (target.GetName() == "MCC_ClearLocalStackRef") ||
          (target.GetName() == "MCC_DecRefResetPair")) {
        /* Write stack memory */
        BuildDepsDirtyStack(insn);
      } else if ((target.GetName() == "MCC_CleanupLocalStackRef_NaiveRCFast") ||
                 (target.GetName() == "MCC_CleanupLocalStackRefSkip_NaiveRCFast") ||
                 (target.GetName() == "MCC_CleanupLocalStackRefSkip")) {
        /* Use Stack Memory */
        BuildDepsUseStack(insn);
      } else if (cgFunc.GetMirModule().GetSrcLang() == kSrcLangC) {
        /* potential C aliasing */
        BuildDepsDirtyStack(insn);
      }
    }
    BuildDepsDirtyHeap(insn);
    BuildDepsAmbiInsn(insn);
    BuildDepsLastCallInsn(insn);
  } else if (insn.IsClinit() || IsLazyLoad(insn.GetMachineOpcode()) ||
             insn.GetMachineOpcode() == MOP_arrayclass_cache_ldr) {
    BuildDepsDirtyHeap(insn);
    BuildDepsDefReg(insn, kRFLAG);
    if (insn.GetMachineOpcode() != MOP_adrp_ldr) {
      BuildDepsDefReg(insn, R16);
      BuildDepsDefReg(insn, R17);
    }
  } else if ((mOp == MOP_xret) || md->IsBranch()) {
    BuildDepsControlAll(insn, nodes);
  } else if (insn.IsMemAccessBar()) {
    BuildDepsMemBar(insn);
  } else if (insn.IsSpecialIntrinsic()) {
    BuildDepsDirtyHeap(insn);
  }
}

void AArch64DataDepBase::UpdateRegUseAndDef(Insn &insn, const DepNode &depNode, MapleVector<DepNode*> &nodes) {
  /* Update reg use */
  const auto &useRegnos = depNode.GetUseRegnos();
  if (beforeRA) {
    depNode.InitRegUsesSize(useRegnos.size());
  }
  for (auto regNO : useRegnos) {
    // Update reg use for cur depInfo
    curCDGNode->AppendUseInsnChain(regNO, &insn, memPool, beforeRA);
    if (beforeRA) {
      CHECK_FATAL(curCDGNode->GetUseInsnChain(regNO)->insn != nullptr, "get useInsn failed");
      depNode.SetRegUses(*curCDGNode->GetUseInsnChain(regNO));
      if (curCDGNode->GetLatestDefInsn(regNO) == nullptr) {
        curCDGNode->SetLatestDefInsn(regNO, nodes[separatorIndex]->GetInsn());
        nodes[separatorIndex]->AddDefReg(regNO);
        nodes[separatorIndex]->SetRegDefs(nodes[separatorIndex]->GetDefRegnos().size(),
                                          curCDGNode->GetUseInsnChain(regNO));
      }
    }
  }

  /* Update reg def */
  const auto &defRegnos = depNode.GetDefRegnos();
  size_t i = 0;
  if (beforeRA) {
    depNode.InitRegDefsSize(defRegnos.size());
  }
  for (const auto regNO : defRegnos) {
    // Update reg def for cur depInfo
    curCDGNode->SetLatestDefInsn(regNO, &insn);
    curCDGNode->ClearUseInsnChain(regNO);
    if (beforeRA) {
      depNode.SetRegDefs(i, nullptr);
      if (regNO >= R0 && regNO <= R3) {
        depNode.SetHasPreg(true);
      } else if (regNO == R8) {
        depNode.SetHasNativeCallRegister(true);
      }
    }
    ++i;
  }
}

/* Build a pseudo node to separate data dependence graph */
DepNode *AArch64DataDepBase::BuildSeparatorNode() {
  Insn &pseudoSepInsn = cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_pseudo_dependence_seperator);
  auto *separatorNode = memPool.New<DepNode>(pseudoSepInsn, alloc);
  separatorNode->SetType(kNodeTypeSeparator);
  pseudoSepInsn.SetDepNode(*separatorNode);
  if (beforeRA) {
    auto *regPressure = memPool.New<RegPressure>(alloc);
    separatorNode->SetRegPressure(*regPressure);
    separatorNode->InitPressure();
  }
  return separatorNode;
}

void AArch64DataDepBase::BuildInterBlockMemDefUseDependency(DepNode &depNode, MemOperand &memOpnd,
                                                            MemOperand *nextMemOpnd, bool isMemDef) {
  CHECK_FATAL(!IsIntraBlockAnalysis(), "must be inter block data dependence analysis");
  BB *curBB = curCDGNode->GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
  CDGRegion *curRegion = curCDGNode->GetRegion();
  CHECK_FATAL(curRegion != nullptr, "get region from cdgNode failed");
  std::vector<bool> visited(curRegion->GetMaxBBIdInRegion(), false);
  if (isMemDef) {
    BuildPredPathMemDefDependencyDFS(*curBB, visited, depNode, memOpnd, nextMemOpnd);
  } else {
    BuildPredPathMemUseDependencyDFS(*curBB, visited, depNode, memOpnd, nextMemOpnd);
  }
}

void AArch64DataDepBase::BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                                          MemOperand &memOpnd, MemOperand *nextMemOpnd) {
  if (visited[curBB.GetId()]) {
    return;
  }
  CDGNode *cdgNode = curBB.GetCDGNode();
  CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
  CDGRegion *curRegion = cdgNode->GetRegion();
  CHECK_FATAL(curRegion != nullptr, "get region from cdgNode failed");
  if (curRegion->GetRegionId() != curCDGNode->GetRegion()->GetRegionId()) {
    return;
  }
  visited[curBB.GetId()] = true;
  MapleVector<Insn*> stackUses = cdgNode->GetStackUseInsns();
  for (auto *stackUse : stackUses) {
    if (NeedBuildDepsMem(memOpnd, nextMemOpnd, *stackUse)) {
      AddDependence(*stackUse->GetDepNode(), depNode, kDependenceTypeAnti);
    }
  }
  /* Build output dependence */
  MapleVector<Insn*> stackDefs = cdgNode->GetStackDefInsns();
  for (auto stackDef : stackDefs) {
    if (stackDef->IsCall() || NeedBuildDepsMem(memOpnd, nextMemOpnd, *stackDef)) {
      AddDependence(*stackDef->GetDepNode(), depNode, kDependenceTypeOutput);
    }
  }
  /* Heap memory
   * Build anti dependence
   */
  MapleVector<Insn*> heapUses = curCDGNode->GetHeapUseInsns();
  AddDependence4InsnInVectorByType(heapUses, *depNode.GetInsn(), kDependenceTypeAnti);
  /* Build output dependence */
  MapleVector<Insn*> heapDefs = curCDGNode->GetHeapDefInsns();
  AddDependence4InsnInVectorByType(heapDefs, *depNode.GetInsn(), kDependenceTypeOutput);
  for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
    BuildPredPathMemDefDependencyDFS(**predIt, visited, depNode, memOpnd, nextMemOpnd);
  }
}

void AArch64DataDepBase::BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                                          MemOperand &memOpnd, MemOperand *nextMemOpnd) {
  if (visited[curBB.GetId()]) {
    return;
  }
  CDGNode *cdgNode = curBB.GetCDGNode();
  CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
  CDGRegion *curRegion = cdgNode->GetRegion();
  CHECK_FATAL(curRegion != nullptr, "get region from cdgNode failed");
  if (curRegion->GetRegionId() != curCDGNode->GetRegion()->GetRegionId()) {
    return;
  }
  visited[curBB.GetId()] = true;
  /* Stack memory address */
  MapleVector<Insn*> stackDefs = cdgNode->GetStackDefInsns();
  for (auto stackDef : stackDefs) {
    if (stackDef->IsCall() || NeedBuildDepsMem(memOpnd, nextMemOpnd, *stackDef)) {
      AddDependence(*stackDef->GetDepNode(), depNode, kDependenceTypeTrue);
    }
  }
  /* Heap memory address */
  MapleVector<Insn*> heapDefs = cdgNode->GetHeapDefInsns();
  AddDependence4InsnInVectorByType(heapDefs, *depNode.GetInsn(), kDependenceTypeTrue);
  for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
    BuildPredPathMemUseDependencyDFS(**predIt, visited, depNode, memOpnd, nextMemOpnd);
  }
}
}  /* namespace maplebe */
