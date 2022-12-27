/*
 * Copyright (c) [2022] Futurewei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_regsaves.h"
#include "aarch64_cg.h"
#include "aarch64_live.h"
#include "aarch64_cg.h"
#include "aarch64_proepilog.h"
#include "cg_dominance.h"
#include "cg_ssa_pre.h"
#include "cg_ssu_pre.h"

namespace maplebe {

#define RS_DUMP 0
#define M_LOG LogInfo::MapleLogger()

#define SKIP_FPLR(REG) \
  if (REG >= R29 && REG < V8) { \
    continue; \
  }

void AArch64RegSavesOpt::InitData() {
  calleeBitsDef = cgFunc->GetMemoryPool()->NewArray<CalleeBitsType>(cgFunc->NumBBs());
  errno_t retDef = memset_s(calleeBitsDef, cgFunc->NumBBs() * sizeof(CalleeBitsType),
                            0, cgFunc->NumBBs() * sizeof(CalleeBitsType));
  calleeBitsUse = cgFunc->GetMemoryPool()->NewArray<CalleeBitsType>(cgFunc->NumBBs());
  errno_t retUse = memset_s(calleeBitsUse, cgFunc->NumBBs() * sizeof(CalleeBitsType),
                            0, cgFunc->NumBBs() * sizeof(CalleeBitsType));
  calleeBitsAcc = cgFunc->GetMemoryPool()->NewArray<CalleeBitsType>(cgFunc->NumBBs());
  errno_t retAccDef = memset_s(calleeBitsAcc, cgFunc->NumBBs() * sizeof(CalleeBitsType),
                               0, cgFunc->NumBBs() * sizeof(CalleeBitsType));
  CHECK_FATAL(retDef == EOK && retUse == EOK && retAccDef == EOK, "memset_s of calleesBits failed");

  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  const MapleVector<AArch64reg> &sp = aarchCGFunc->GetCalleeSavedRegs();
  if (!sp.empty()) {
    if (std::find(sp.begin(), sp.end(), RFP) != sp.end()) {
      aarchCGFunc->GetProEpilogSavedRegs().push_back(RFP);
    }
    if (std::find(sp.begin(), sp.end(), RLR) != sp.end()) {
      aarchCGFunc->GetProEpilogSavedRegs().push_back(RLR);
    }
  }

  for (auto bb : bfs->sortedBBs) {
    SetId2bb(bb);
  }
}


void AArch64RegSavesOpt::CollectLiveInfo(const BB &bb, const Operand &opnd, bool isDef, bool isUse) {
  if (!opnd.IsRegister()) {
    return;
  }
  const RegOperand &regOpnd = static_cast<const RegOperand&>(opnd);
  regno_t regNO = regOpnd.GetRegisterNumber();
  if (!AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regNO)) ||
      (regNO >= R29 && regNO <= R31)) {
    return;   /* check only callee-save registers */
  }
  RegType regType = regOpnd.GetRegisterType();
  if (regType == kRegTyVary) {
    return;
  }
  if (isDef) {
    /* First def */
    if (!IsCalleeBitSet(GetCalleeBitsDef(), bb.GetId(), regNO)) {
      SetCalleeBit(GetCalleeBitsDef(), bb.GetId(), regNO);
    }
  }
  if (isUse) {
    /* Last use */
    SetCalleeBit(GetCalleeBitsUse(), bb.GetId(), regNO);
  }
}

void AArch64RegSavesOpt::GenerateReturnBBDefUse(const BB &bb) {
  PrimType returnType = cgFunc->GetFunction().GetReturnType()->GetPrimType();
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if (IsPrimitiveFloat(returnType)) {
    Operand &phyOpnd =
        aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(V0), k64BitSize, kRegTyFloat);
    CollectLiveInfo(bb, phyOpnd, false, true);
  } else if (IsPrimitiveInteger(returnType)) {
    Operand &phyOpnd =
        aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R0), k64BitSize, kRegTyInt);
    CollectLiveInfo(bb, phyOpnd, false, true);
  }
}

void AArch64RegSavesOpt::ProcessAsmListOpnd(const BB &bb, const Operand &opnd, uint32 idx) {
  bool isDef = false;
  bool isUse = false;
  switch (idx) {
    case kAsmOutputListOpnd:
    case kAsmClobberListOpnd: {
      isDef = true;
      break;
    }
    case kAsmInputListOpnd: {
      isUse = true;
      break;
    }
    default:
      return;
  }
  auto &listOpnd = static_cast<const ListOperand&>(opnd);
  for (auto &op : listOpnd.GetOperands()) {
    CollectLiveInfo(bb, *op, isDef, isUse);
  }
}

void AArch64RegSavesOpt::ProcessListOpnd(const BB &bb, const Operand &opnd) {
  auto &listOpnd = static_cast<const ListOperand&>(opnd);
  for (auto &op : listOpnd.GetOperands()) {
    CollectLiveInfo(bb, *op, false, true);
  }
}

void AArch64RegSavesOpt::ProcessMemOpnd(const BB &bb, Operand &opnd) {
  auto &memOpnd = static_cast<MemOperand&>(opnd);
  Operand *base = memOpnd.GetBaseRegister();
  Operand *offset = memOpnd.GetIndexRegister();
  if (base != nullptr) {
    CollectLiveInfo(bb, *base, !memOpnd.IsIntactIndexed(), true);
  }
  if (offset != nullptr) {
    CollectLiveInfo(bb, *offset, false, true);
  }
}

void AArch64RegSavesOpt::ProcessCondOpnd(const BB &bb) {
  Operand &rflag = cgFunc->GetOrCreateRflag();
  CollectLiveInfo(bb, rflag, false, true);
}

void AArch64RegSavesOpt::ProcessOperands(const Insn &insn, const BB &bb) {
  const InsnDesc *md = insn.GetDesc();
  bool isAsm = (insn.GetMachineOpcode() == MOP_asm);

  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    auto *regProp = md->opndMD[i];
    bool isDef = regProp->IsRegDef();
    bool isUse = regProp->IsRegUse();
    if (opnd.IsList()) {
      if (isAsm) {
        ProcessAsmListOpnd(bb, opnd, i);
      } else {
        ProcessListOpnd(bb, opnd);
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      ProcessMemOpnd(bb, opnd);
    } else if (opnd.IsConditionCode()) {
      ProcessCondOpnd(bb);
    } else {
      CollectLiveInfo(bb, opnd, isDef, isUse);
    }
  } /* for all operands */
}

static bool IsBackEdge(const BB* bb, BB* targ) {
  CGFuncLoops *loop = bb->GetLoop();
  if (loop != nullptr && loop->GetHeader() == bb) {
    if (find(loop->GetBackedge().begin(), loop->GetBackedge().end(), targ) != loop->GetBackedge().end()) {
      return true;
    }
  }
  return false;
}

void AArch64RegSavesOpt::GenAccDefs() {
  /* Set up accumulated callee def bits in all blocks */
  for (auto bb : bfs->sortedBBs) {
    if (bb->GetPreds().size() == 0) {
      SetCalleeBit(GetCalleeBitsAcc(), bb->GetId(), GetBBCalleeBits(GetCalleeBitsDef(), bb->GetId()));
    } else {
      CalleeBitsType curbbBits = GetBBCalleeBits(GetCalleeBitsDef(), bb->GetId());
      int64 n = -1;
      CalleeBitsType tmp = static_cast<CalleeBitsType>(n);
      for (auto pred : bb->GetPreds()) {
        if (IsBackEdge(bb, pred)) {
          continue;
        }
        tmp &= GetBBCalleeBits(GetCalleeBitsAcc(), pred->GetId());
      }
      SetCalleeBit(GetCalleeBitsAcc(), bb->GetId(), curbbBits | tmp);
    }
  }
}

/* Record in each local BB the 1st def and the last use of a callee-saved
   register  */
void AArch64RegSavesOpt::GenRegDefUse() {
  for (auto bbp : bfs->sortedBBs) {
    BB &bb = *bbp;
    if (bb.GetKind() == BB::kBBReturn) {
      GenerateReturnBBDefUse(bb);
    }
    if (bb.IsEmpty()) {
      continue;
    }

    FOR_BB_INSNS(insn, &bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      ProcessOperands(*insn, bb);
    } /* for all insns */
  } /* for all sortedBBs */

  GenAccDefs();

#if RS_DUMP
  M_LOG << "CalleeBits for " << cgFunc->GetName() << ":\n";
  for (BBID i = 1; i < cgFunc->NumBBs(); ++i) {
    M_LOG << i << " : " << calleeBitsDef[i] << " " << calleeBitsUse[i] << " " << calleeBitsAcc[i] << "\n";
  }
#endif
}

bool AArch64RegSavesOpt::CheckForUseBeforeDefPath() {
  /* Check if any block has a use without a def as shown in its accumulated
     calleeBitsDef from above */
  BBID found = 0;
  CalleeBitsType use;
  CalleeBitsType acc;
  for (BBID bid = 0; bid < cgFunc->NumBBs(); ++bid) {
    use = GetBBCalleeBits(GetCalleeBitsUse(), bid);
    acc = GetBBCalleeBits(GetCalleeBitsAcc(), bid);
    if ((use & acc) != use) {
      found = bid;
      break;
    }
  }
  if (found) {
#if RS_DUMP
  CalleeBitsType mask = 1;
  for (uint32 i = 0; i < static_cast<uint32>(sizeof(CalleeBitsType) << k3BitSize); ++i) {
    regno_t reg = ReverseRegBitMap(i);
    if ((use & mask) != 0 && (acc & mask) == 0) {
      M_LOG << "R" << (reg - 1) << " in BB" << found << " is in a use before def path\n";
    }
    mask <<= 1;
  }
#endif
    return true;
  }
  return false;
}

void AArch64RegSavesOpt::PrintBBs() const {
  M_LOG << "RegSaves LiveIn/Out of BFS nodes:\n";
  for (auto *bb : bfs->sortedBBs) {
    M_LOG << "< === > ";
    M_LOG << bb->GetId();
    M_LOG << " pred:[";
    for (auto predBB : bb->GetPreds()) {
      M_LOG << " " << predBB->GetId();
    }
    M_LOG << "] succs:[";
    for (auto succBB : bb->GetSuccs()) {
      M_LOG << " " << succBB->GetId();
    }
    M_LOG << "]\n LiveIn of [" << bb->GetId() << "]: ";
    for (auto liveIn: bb->GetLiveInRegNO()) {
        M_LOG << liveIn << " ";
    }
    M_LOG << "\n LiveOut of [" << bb->GetId() << "]: ";
    for (auto liveOut: bb->GetLiveOutRegNO()) {
      M_LOG << liveOut  << " ";
    }
    M_LOG << "\n";
  }
}

/* 1st def MUST not have preceding save in dominator list. Each dominator
   block must not have livein or liveout of the register */
int32 AArch64RegSavesOpt::CheckCriteria(BB *bb, regno_t reg) const {
  /* Already a site to save */
  SavedRegInfo *sp = bbSavedRegs[bb->GetId()];
  if (sp != nullptr && sp->ContainSaveReg(reg)) {
    return 1;
  }

  /* This preceding block has livein OR liveout of reg */
  MapleSet<regno_t> &liveIn = bb->GetLiveInRegNO();
  MapleSet<regno_t> &liveOut = bb->GetLiveOutRegNO();
  if (liveIn.find(reg) != liveIn.end() ||
    liveOut.find(reg) != liveOut.end()) {
    return 2;
  }

  return 0;
}

/* Return true if reg is already to be saved in its dominator list */
bool AArch64RegSavesOpt::AlreadySavedInDominatorList(const BB &bb, regno_t reg) const {
  BB *aBB = GetDomInfo()->GetDom(bb.GetId());

#if RS_DUMP
  M_LOG << "Checking dom list starting " << bb.GetId() << " for saved R" << (reg - 1) << ":\n  ";
#endif
  while (!aBB->GetPreds().empty()) {  /* can't go beyond prolog */
#if RS_DUMP
    M_LOG << aBB->GetId() << " ";
#endif
    int t = CheckCriteria(aBB, reg);
    if (t != 0) {
#if RS_DUMP
      std::string str = t == 1 ?  " saved here, skip!\n" : " has livein/out, skip!\n";
      M_LOG << " --R" << (reg - 1) << str;
#endif
      return true;                   /* previously saved, inspect next reg */
    }
    aBB = GetDomInfo()->GetDom(aBB->GetId());
  }
  return false;                      /* not previously saved, to save at bb */
}

BB* AArch64RegSavesOpt::FindLoopDominator(BB *bb, regno_t reg, bool *done) const {
  BB *bbDom = bb;
  while (bbDom->GetLoop() != nullptr) {
    bbDom = GetDomInfo()->GetDom(bbDom->GetId());
    if (CheckCriteria(bbDom, reg) != 0) {
      *done = true;
      break;
    }
    ASSERT(bbDom, "Can't find dominator for save location");
  }
  return bbDom;
}

/* If the newly found blk is a dominator of blk(s) in the current
   to be saved list, remove these blks from bbSavedRegs */
void AArch64RegSavesOpt::CheckAndRemoveBlksFromCurSavedList(SavedBBInfo *sp, const BB *bbDom, regno_t reg) {
  for (BB *sbb : sp->GetBBList()) {
    for (BB *abb = sbb; !abb->GetPreds().empty();) {
      if (abb->GetId() == bbDom->GetId()) {
        /* Found! Don't plan to save in abb */
        sp->RemoveBB(sbb);
        bbSavedRegs[sbb->GetId()]->RemoveSaveReg(reg);
#if RS_DUMP
        M_LOG << " --R" << (reg - 1) << " save removed from BB" << sbb->GetId() << "\n";
#endif
        break;
      }
      abb = GetDomInfo()->GetDom(abb->GetId());
    }
  }
}

/* Determine callee-save regs save locations and record them in bbSavedRegs.
   Save is needed for a 1st def callee-save register at its dominator block
   outside any loop. */
void AArch64RegSavesOpt::DetermineCalleeSaveLocationsDoms() {
#if RS_DUMP
  M_LOG << "Determining regsave sites using dom list for " << cgFunc->GetName() << ":\n";
#endif
  for (auto *bb : bfs->sortedBBs) {
#if RS_DUMP
    M_LOG << "BB: " << bb->GetId() << "\n";
#endif
    CalleeBitsType c = GetBBCalleeBits(GetCalleeBitsDef(), bb->GetId());
    if (c == 0) {
      continue;
    }
    CalleeBitsType mask = 1;
    for (uint32 i = 0; i < static_cast<uint32>(sizeof(CalleeBitsType) << 3); ++i) {
      MapleSet<regno_t> &liveIn = bb->GetLiveInRegNO();
      regno_t reg = ReverseRegBitMap(i);
      if ((c & mask) != 0 && liveIn.find(reg) == liveIn.end()) { /* not livein */
        BB *bbDom = bb;  /* start from current BB */
        bool done = false;
        bbDom = FindLoopDominator(bbDom, reg, &done);
        if (done) {
          mask <<= 1;
          continue;
        }

        /* Check if a dominator of bbDom was already a location to save */
        if (AlreadySavedInDominatorList(*bbDom, reg)) {
          mask <<= 1;
          continue;    /* no need to save again, next reg */
        }

        /* If the newly found blk is a dominator of blk(s) in the current
           to be saved list, remove these blks from bbSavedRegs */
        uint32 creg = i;
        SavedBBInfo *sp = regSavedBBs[creg];
        if (sp == nullptr) {
          regSavedBBs[creg] = memPool->New<SavedBBInfo>(alloc);
        } else {
          CheckAndRemoveBlksFromCurSavedList(sp, bbDom, reg);
        }
        regSavedBBs[creg]->InsertBB(bbDom);

        BBID bid = bbDom->GetId();
#if RS_DUMP
        M_LOG << " --R" << (reg - 1);
        M_LOG << " to save in " << bid << "\n";
#endif
        SavedRegInfo *ctx = GetbbSavedRegsEntry(bid);
        if (!ctx->ContainSaveReg(reg)) {
          ctx->InsertSaveReg(reg);
        }
      }
      mask <<= 1;
      CalleeBitsType t = c;
      t >>= 1;
      if (t == 0) {
        break;  /* short cut */
      }
    }
  }
}

void AArch64RegSavesOpt::DetermineCalleeSaveLocationsPre() {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  MapleAllocator sprealloc(memPool);
#if RS_DUMP
  M_LOG << "Determining regsave sites using ssa_pre for " << cgFunc->GetName() << ":\n";
#endif
  const MapleVector<AArch64reg> &callees = aarchCGFunc->GetCalleeSavedRegs();
  /* do 2 regs at a time to force store pairs */
  for (uint32 i = 0; i < callees.size(); ++i) {
    AArch64reg reg1 = callees[i];
    SKIP_FPLR(reg1);
    AArch64reg reg2 = kRinvalid;
    if ((i + 1) < callees.size()) {
      reg2 = callees[i + 1];
      SKIP_FPLR(reg2);
      ++i;
    }

    SsaPreWorkCand wkCand(&sprealloc);
    for (BBID bid = 1; bid < static_cast<BBID>(bbSavedRegs.size()); ++bid) {
      /* Set the BB occurrences of this callee-saved register */
      if (IsCalleeBitSet(GetCalleeBitsDef(), bid, reg1) ||
          IsCalleeBitSet(GetCalleeBitsUse(), bid, reg1)) {
        (void)wkCand.occBBs.insert(bid);
      }
      if (reg2 != kRinvalid) {
        if (IsCalleeBitSet(GetCalleeBitsDef(), bid, reg2) ||
            IsCalleeBitSet(GetCalleeBitsUse(), bid, reg2)) {
          (void)wkCand.occBBs.insert(bid);
        }
      }
    }
    DoSavePlacementOpt(cgFunc, GetDomInfo(), &wkCand);
    if (wkCand.saveAtEntryBBs.empty()) {
      /* something gone wrong, skip this reg */
      wkCand.saveAtProlog = true;
    }
    if (wkCand.saveAtProlog) {
      /* Save cannot be applied, skip this reg and place save/restore
         in prolog/epilog */
      MapleVector<AArch64reg> &pe = aarchCGFunc->GetProEpilogSavedRegs();
      if (std::find(pe.begin(), pe.end(), reg1) == pe.end()) {
        pe.push_back(reg1);
      }
      if (reg2 != kRinvalid && std::find(pe.begin(), pe.end(), reg2) == pe.end()) {
        pe.push_back(reg2);
      }
#if RS_DUMP
      M_LOG << "Save R" << (reg1 - 1) << " n/a, do in Pro/Epilog\n";
      if (reg2 != kRinvalid) {
        M_LOG << "    R  " << (reg2 - 1) << " n/a, do in Pro/Epilog\n";
      }
#endif
      continue;
    }
    if (!wkCand.saveAtEntryBBs.empty()) {
      for (BBID entBB : wkCand.saveAtEntryBBs) {
#if RS_DUMP
        std::string r = reg1 <= R28 ? "R" : "V";
        M_LOG << "BB " << entBB << " save for : " << r << (reg1 - 1) << "\n";
        if (reg2 != kRinvalid) {
          std::string r2 = reg2 <= R28 ? "R" : "V";
          M_LOG << "              : " << r2 << (reg2 - 1) << "\n";
        }
#endif
        GetbbSavedRegsEntry(entBB)->InsertSaveReg(reg1);
        if (reg2 != kRinvalid) {
          GetbbSavedRegsEntry(entBB)->InsertSaveReg(reg2);
        }
      }
    }
  }
}

void AArch64RegSavesOpt::CheckCriticalEdge(const BB *bb, AArch64reg reg) {
  for (BB *sbb : bb->GetSuccs()) {
    if (sbb->GetPreds().size() > 1) {
      CHECK_FATAL(false, "critical edge detected");
    }
    /* To insert at all succs */
    GetbbSavedRegsEntry(sbb->GetId())->InsertEntryReg(reg);
  }
}

/* Restore cannot be applied, skip this reg and place save/restore
   in prolog/epilog */
void AArch64RegSavesOpt::RevertToRestoreAtEpilog(AArch64reg reg) {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  for (size_t bid = 1; bid < bbSavedRegs.size(); ++bid) {
    SavedRegInfo *sp = bbSavedRegs[bid];
    if (sp != nullptr && !sp->GetSaveSet().empty() && sp->ContainSaveReg(reg)) {
      sp->RemoveSaveReg(reg);
    }
  }
  MapleVector<AArch64reg> &pe = aarchCGFunc->GetProEpilogSavedRegs();
  if (std::find(pe.begin(), pe.end(), reg) == pe.end()) {
    pe.push_back(reg);
  }
#if RS_DUMP
      M_LOG << "Restore R" << (reg - 1) << " n/a, do in Pro/Epilog\n";
#endif
}

/* Determine calleesave regs restore locations by calling ssu-pre,
   previous bbSavedRegs memory is cleared and restore locs recorded in it */
void AArch64RegSavesOpt::DetermineCalleeRestoreLocations() {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  MapleAllocator sprealloc(memPool);
#if RS_DUMP
  M_LOG << "Determining Callee Restore Locations:\n";
#endif
  const MapleVector<AArch64reg> &callees = aarchCGFunc->GetCalleeSavedRegs();
  for (auto reg : callees) {
    SKIP_FPLR(reg);

    SPreWorkCand wkCand(&sprealloc);
    for (BBID bid = 1; bid < static_cast<uint32>(bbSavedRegs.size()); ++bid) {
      /* Set the saved BB locations of this callee-saved register */
      SavedRegInfo *sp = bbSavedRegs[bid];
      if (sp != nullptr) {
        if (sp->ContainSaveReg(reg)) {
          (void)wkCand.saveBBs.insert(bid);
        }
      }
      /* Set the BB occurrences of this callee-saved register */
      if (IsCalleeBitSet(GetCalleeBitsDef(), bid, reg) ||
          IsCalleeBitSet(GetCalleeBitsUse(), bid, reg)) {
        (void)wkCand.occBBs.insert(bid);
      }
    }
    DoRestorePlacementOpt(cgFunc, GetPostDomInfo(), &wkCand);
    if (wkCand.saveBBs.empty()) {
      /* something gone wrong, skip this reg */
      wkCand.restoreAtEpilog = true;
    }
    if (wkCand.restoreAtEpilog) {
      RevertToRestoreAtEpilog(reg);
      continue;
    }
    if (!wkCand.restoreAtEntryBBs.empty() || !wkCand.restoreAtExitBBs.empty()) {
      for (BBID entBB : wkCand.restoreAtEntryBBs) {
#if RS_DUMP
        std::string r = reg <= R28 ? "r" : "v";
        M_LOG << "BB " << entBB << " restore: " << r << (reg - 1) << "\n";
#endif
        GetbbSavedRegsEntry(entBB)->InsertEntryReg(reg);
      }
      for (BBID exitBB : wkCand.restoreAtExitBBs) {
        BB *bb = GetId2bb(exitBB);
        if (bb->GetKind() == BB::kBBIgoto) {
          CHECK_FATAL(false, "igoto detected");
        }
        Insn *lastInsn = bb->GetLastMachineInsn();
        if (lastInsn != nullptr && (lastInsn->IsBranch() || lastInsn->IsTailCall()) &&
            lastInsn->GetOperand(0).IsRegister() &&
            static_cast<RegOperand&>(lastInsn->GetOperand(0)).GetRegisterNumber() == reg) {
          RevertToRestoreAtEpilog(reg);
        }
        if (lastInsn != nullptr && (lastInsn->IsBranch() || lastInsn->IsTailCall())) {
          /* To insert in this block - 1 instr */
          SavedRegInfo *sp = GetbbSavedRegsEntry(exitBB);
          sp->InsertExitReg(reg);
          sp->insertAtLastMinusOne = true;
        } else if (bb->GetSuccs().size() > 1) {
          CheckCriticalEdge(bb, reg);
        } else {
          /* otherwise, BB_FT etc */
          GetbbSavedRegsEntry(exitBB)->InsertExitReg(reg);
        }
#if RS_DUMP
          std::string r = reg <= R28 ? "R" : "V";
          M_LOG << "BB " << exitBB << " restore: " << r << (reg - 1) << "\n";
#endif
      }
    }
  }
}

int32 AArch64RegSavesOpt::FindCalleeBase() const {
  int32 offset = static_cast<int32>(
      (static_cast<AArch64MemLayout*>(cgFunc->GetMemlayout())->RealStackFrameSize()) -
      (static_cast<AArch64CGFunc*>(cgFunc)->SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen) /* FP/LR */) -
      (cgFunc->GetMemlayout()->SizeOfArgsToStackPass()));

  if (cgFunc->GetFunction().GetAttr(FUNCATTR_varargs)) {
    /* GR/VR save areas are above the callee save area */
    AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc->GetMemlayout());
    int saveareasize = static_cast<int>(RoundUp(ml->GetSizeOfGRSaveArea(), GetPointerSize() * k2BitSize) +
        RoundUp(ml->GetSizeOfVRSaveArea(), GetPointerSize() * k2BitSize));
    offset -= saveareasize;
  }
  return offset;
}

void AArch64RegSavesOpt::SetupRegOffsets() {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  const MapleVector<AArch64reg> &proEpilogRegs = aarchCGFunc->GetProEpilogSavedRegs();
  int32 regsInProEpilog = static_cast<int32>(proEpilogRegs.size() - 2);
  const MapleVector<AArch64reg> &callees = aarchCGFunc->GetCalleeSavedRegs();

  int32 offset = FindCalleeBase();
  for (auto reg : callees) {
    SKIP_FPLR(reg);
    if (std::count(proEpilogRegs.begin(), proEpilogRegs.end(), reg) != 0) {
      continue;
    }
    if (regOffset.find(reg) == regOffset.end()) {
      regOffset[reg] = static_cast<uint32>(offset + (regsInProEpilog * kBitsPerByte));
      offset += static_cast<int32>(kIntregBytelen);
    }
  }
}

void AArch64RegSavesOpt::InsertCalleeSaveCode() {
  BBID bid = 0;
  BB *saveBB = cgFunc->GetCurBB();
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);

#if RS_DUMP
  M_LOG << "Inserting Save for " << cgFunc->GetName() << ":\n";
#endif
  int32 offset = FindCalleeBase();
  for (BB *bb : bfs->sortedBBs) {
    bid = bb->GetId();
    aarchCGFunc->SetSplitBaseOffset(0);
    if (bbSavedRegs[bid] != nullptr && !bbSavedRegs[bid]->GetSaveSet().empty()) {
      aarchCGFunc->GetDummyBB()->ClearInsns();
      cgFunc->SetCurBB(*aarchCGFunc->GetDummyBB());
      AArch64reg intRegFirstHalf = kRinvalid;
      AArch64reg fpRegFirstHalf = kRinvalid;
      for (auto areg : bbSavedRegs[bid]->GetSaveSet()) {
        AArch64reg reg = static_cast<AArch64reg>(areg);
        RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
        std::string r = reg <= R28 ? "R" : "V";
        /* If reg not seen before, record offset and then update */
        if (regOffset.find(areg) == regOffset.end()) {
          regOffset[areg] = static_cast<uint32>(offset);
          offset += static_cast<int32>(kIntregBytelen);
        }
        if (firstHalf == kRinvalid) {
          /* 1st half in reg pair */
          firstHalf = reg;
#if RS_DUMP
          M_LOG << r << (reg - 1) << " save in BB" << bid << "  Offset = " << regOffset[reg]<< "\n";
#endif
        } else {
          if (regOffset[reg] == (regOffset[firstHalf] + k8ByteSize)) {
            /* firstHalf & reg consecutive, make regpair */
            AArch64GenProEpilog::AppendInstructionPushPair(*cgFunc, firstHalf, reg, regType,
                                                           static_cast<int32>(regOffset[firstHalf]));
          } else if (regOffset[firstHalf] == (regOffset[reg] + k8ByteSize)) {
            /* reg & firstHalf consecutive, make regpair */
            AArch64GenProEpilog::AppendInstructionPushPair(*cgFunc, reg, firstHalf, regType,
                                                           static_cast<int32>(regOffset[reg]));
          } else {
            /* regs cannot be paired */
            AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc, firstHalf, regType,
                                                             static_cast<int32>(regOffset[firstHalf]));
            AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc, reg, regType,
                                                             static_cast<int32>(regOffset[reg]));
          }
          firstHalf = kRinvalid;
#if RS_DUMP
          M_LOG << r << (reg - 1) << " save in BB" << bid << "  Offset = " << regOffset[reg]<< "\n";
#endif
        }
      }

      if (intRegFirstHalf != kRinvalid) {
        AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc,
            intRegFirstHalf, kRegTyInt, static_cast<int32>(regOffset[intRegFirstHalf]));
      }

      if (fpRegFirstHalf != kRinvalid) {
        AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc,
            fpRegFirstHalf, kRegTyFloat, static_cast<int32>(regOffset[fpRegFirstHalf]));
      }
      bb->InsertAtBeginning(*aarchCGFunc->GetDummyBB());
    }
  }
  cgFunc->SetCurBB(*saveBB);
}

void AArch64RegSavesOpt::PrintSaveLocs(AArch64reg reg) {
  M_LOG << "  for save @BB [ ";
  for (size_t b = 1; b < bbSavedRegs.size(); ++b) {
    if (bbSavedRegs[b] != nullptr && bbSavedRegs[b]->ContainSaveReg(reg)) {
      M_LOG << b << " ";
    }
  }
  M_LOG << "]\n";
}

void AArch64RegSavesOpt::InsertCalleeRestoreCode() {
  BBID bid = 0;
  BB *saveBB = cgFunc->GetCurBB();
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);

#if RS_DUMP
  M_LOG << "Inserting Restore: \n";
#endif
  int32 offset = FindCalleeBase();
  for (BB *bb : bfs->sortedBBs) {
    bid = bb->GetId();
    aarchCGFunc->SetSplitBaseOffset(0);
    SavedRegInfo *sp = bbSavedRegs[bid];
    if (sp != nullptr) {
      if (sp->GetEntrySet().empty() && sp->GetExitSet().empty()) {
        continue;
      }

      aarchCGFunc->GetDummyBB()->ClearInsns();
      cgFunc->SetCurBB(*aarchCGFunc->GetDummyBB());
      for (auto areg : sp->GetEntrySet()) {
        AArch64reg reg = static_cast<AArch64reg>(areg);
        offset = static_cast<int32>(regOffset[areg]);
#if RS_DUMP
        std::string r = reg <= R28 ? "R" : "V";
        M_LOG << r << (reg - 1) << " entry restore in BB " << bid << "  Saved Offset = " << offset << "\n";
        PrintSaveLocs(reg);
#endif

        /* restore is always the same from saved offset */
        RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        AArch64GenProEpilog::AppendInstructionPopSingle(*cgFunc, reg, regType, offset);
      }
      FOR_BB_INSNS(insn, aarchCGFunc->GetDummyBB()) {
        insn->SetDoNotRemove(true); /* do not let ebo remove these restores */
      }
      bb->InsertAtBeginning(*aarchCGFunc->GetDummyBB());

      aarchCGFunc->GetDummyBB()->ClearInsns();
      cgFunc->SetCurBB(*aarchCGFunc->GetDummyBB());
      for (auto areg : sp->GetExitSet()) {
        AArch64reg reg = static_cast<AArch64reg>(areg);
        offset = static_cast<int32>(regOffset[areg]);
#if RS_DUMP
        std::string r = reg <= R28 ? "R" : "V";
        M_LOG << r << (reg - 1) << " exit restore in BB " << bid << " Offset = " << offset << "\n";
        PrintSaveLocs(reg);
#endif

        /* restore is always single from saved offset */
        RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        AArch64GenProEpilog::AppendInstructionPopSingle(*cgFunc, reg, regType, offset);
      }
      FOR_BB_INSNS(insn, aarchCGFunc->GetDummyBB()) {
        insn->SetDoNotRemove(true);
      }
      if (sp->insertAtLastMinusOne) {
        bb->InsertAtEndMinus1(*aarchCGFunc->GetDummyBB());
      } else {
        bb->InsertAtEnd(*aarchCGFunc->GetDummyBB());
      }
    }
  }
  cgFunc->SetCurBB(*saveBB);
}

/* Callee-save registers save/restore placement optimization */
void AArch64RegSavesOpt::Run() {
  // DotGenerator::GenerateDot("SR", *cgFunc, cgFunc->GetMirModule(), true, cgFunc->GetName());
  if (Globals::GetInstance()->GetOptimLevel() <= CGOptions::kLevel1 || !cgFunc->GetMirModule().IsCModule()) {
    return;
  }
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);

  Bfs localBfs(*cgFunc, *memPool);
  bfs = &localBfs;
  bfs->ComputeBlockOrder();
#if RS_DUMP
  M_LOG << "##Calleeregs Placement for: " << cgFunc->GetName() << "\n";
  PrintBBs();
#endif

  /* Determined 1st def and last use of all callee-saved registers used
     for all BBs */
  InitData();
  GenRegDefUse();

  if (CGOptions::UseSsaPreSave()) {
    /* Use ssapre */
    if (cgFunc->GetNeedStackProtect() || CheckForUseBeforeDefPath()) {
      for (auto reg : aarchCGFunc->GetCalleeSavedRegs()) {
        if (reg != RFP && reg != RLR) {
          aarchCGFunc->GetProEpilogSavedRegs().push_back(reg);
        }
      }
      return;
    }
    DetermineCalleeSaveLocationsPre();
  } else {
    /* Determine save sites at dominators of 1st def with no live-in and
       not within loop */
    /* Obsolete, to be deleted */
    DetermineCalleeSaveLocationsDoms();
  }

  /* Determine restore sites */
  DetermineCalleeRestoreLocations();

#ifdef VERIFY
  /* Verify saves/restores are in pair */
  std::vector<regno_t> rlist = { R19, R20, R21, R22, R23, R24, R25, R26, R27, R28 };
  for (auto reg : rlist) {
    M_LOG << "Verify calleeregs_placement data for R" << (reg - 1) << ":\n";
    std::set<BB*, BBIdCmp> visited;
    uint32 saveBid = 0;
    uint32 restoreBid = 0;
    Verify(reg, cgFunc->GetFirstBB(), &visited, &saveBid, &restoreBid);
    M_LOG << "\nVerify Done\n";
  }
#endif

  /* Assign stack offset to each shrinkwrapped register, skip over the offsets
     for registers saved in prolog */
  SetupRegOffsets();

  /* Generate callee save instrs at found sites */
  InsertCalleeSaveCode();

  /* Generate callee restores at found sites */
  InsertCalleeRestoreCode();
}
}  /* namespace maplebe */
