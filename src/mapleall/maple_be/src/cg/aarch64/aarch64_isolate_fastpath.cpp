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
#include "aarch64_isolate_fastpath.h"
#include "aarch64_cg.h"
#include "cgfunc.h"
#include "cg_irbuilder.h"

namespace maplebe {
using namespace maple;

bool AArch64IsolateFastPath::FindRegs(Operand &op, std::set<regno_t> &vecRegs) const {
  Operand *opnd = &op;
  if (opnd == nullptr || vecRegs.empty()) {
    return false;
  }
  if (opnd->IsList()) {
    MapleList<RegOperand*> pregList = static_cast<ListOperand *>(opnd)->GetOperands();
    for (auto *preg : as_const(pregList)) {
      if (preg->GetRegisterNumber() == R29 ||
          vecRegs.find(preg->GetRegisterNumber()) != vecRegs.end()) {
        return true;  /* the opReg will overwrite or reread the vecRegs */
      }
    }
  }
  if (opnd->IsMemoryAccessOperand()) {  /* the registers of kOpdMem are complex to be detected */
    RegOperand *baseOpnd = static_cast<MemOperand *>(opnd)->GetBaseRegister();
    RegOperand *indexOpnd = static_cast<MemOperand *>(opnd)->GetIndexRegister();
    if ((baseOpnd != nullptr && baseOpnd->GetRegisterNumber() == R29) ||
        (indexOpnd != nullptr && indexOpnd->GetRegisterNumber() == R29)) {
      return true;  /* Avoid modifying data on the stack */
    }
    if ((baseOpnd != nullptr && vecRegs.find(baseOpnd->GetRegisterNumber()) != vecRegs.end()) ||
        (indexOpnd != nullptr && vecRegs.find(indexOpnd->GetRegisterNumber()) != vecRegs.end())) {
      return true;
    }
  }
  if (opnd->IsRegister()) {
    RegOperand *regOpnd = static_cast<RegOperand *>(opnd);
    if (regOpnd->GetRegisterNumber() == R29 ||
        vecRegs.find(regOpnd->GetRegisterNumber()) != vecRegs.end()) {
      return true;  /* dst is a target register, result_dst is a target register */
    }
  }
  return false;
}

bool AArch64IsolateFastPath::InsertOpndRegs(Operand &op, std::set<regno_t> &vecRegs) const {
  Operand *opnd = &op;
  if (opnd->IsList()) {
    MapleList<RegOperand *> pregList = static_cast<ListOperand *>(opnd)->GetOperands();
    for (auto *preg : as_const(pregList)) {
      if (preg != nullptr) {
        (void)vecRegs.insert(preg->GetRegisterNumber());
      }
    }
  }
  if (opnd->IsMemoryAccessOperand()) { /* the registers of kOpdMem are complex to be detected */
    RegOperand *baseOpnd = static_cast<MemOperand *>(opnd)->GetBaseRegister();
    if (baseOpnd != nullptr) {
      (void)vecRegs.insert(baseOpnd->GetRegisterNumber());
    }
    RegOperand *indexOpnd = static_cast<MemOperand *>(opnd)->GetIndexRegister();
    if (indexOpnd != nullptr) {
      (void)vecRegs.insert(indexOpnd->GetRegisterNumber());
    }
  }
  if (opnd->IsRegister()) {
    RegOperand *preg = static_cast<RegOperand *>(opnd);
    if (preg != nullptr) {
      (void)vecRegs.insert(preg->GetRegisterNumber());
    }
  }
  return true;
}

bool AArch64IsolateFastPath::InsertInsnRegs(Insn &insn, bool insertSource, std::set<regno_t> &vecSourceRegs,
                                            bool insertTarget, std::set<regno_t> &vecTargetRegs) const {
  Insn *curInsn = &insn;
  for (uint32 o = 0; o < curInsn->GetOperandSize(); ++o) {
    Operand &opnd = curInsn->GetOperand(o);
    if (insertSource && curInsn->OpndIsUse(o)) {
      (void)InsertOpndRegs(opnd, vecSourceRegs);
    }
    if (insertTarget && curInsn->OpndIsDef(o)) {
      (void)InsertOpndRegs(opnd, vecTargetRegs);
    }
  }
  return true;
}

bool AArch64IsolateFastPath::BackwardFindDependency(BB &ifbb, std::set<regno_t> &vecReturnSourceRegs,
                                                    std::list<Insn*> &existingInsns,
                                                    std::list<Insn*> &moveInsns) const {
  /*
   * Pattern match,(*) instruction are moved down below branch.
   *   ********************
   *   curInsn: <instruction> <target> <source>
   *   <existingInsns> in predBB
   *   <existingInsns> in ifBB
   *   <existingInsns> in returnBB
   *   *********************
   *                        list: the insns can be moved into the coldBB
   * (1) the instruction is neither a branch nor a call, except for the ifbb.GetLastInsn()
   *     As long as a branch insn exists,
   *     the fast path finding fails and the return value is false,
   *     but the code sinking can be continued.
   * (2) the predBB is not a ifBB,
   *     As long as a ifBB in preds exists,
   *     the code sinking fails,
   *     but fast path finding can be continued.
   * (3) the targetRegs of insns in existingInsns can neither be reread or overwrite
   * (4) the sourceRegs of insns in existingInsns can not be overwrite
   * (5) the sourceRegs of insns in returnBB can neither be reread or overwrite
   * (6) the targetRegs and sourceRegs cannot be R29 R30, to protect the stack
   * (7) modified the reg when:
   *     --------------
   *     curInsn: move R2,R1
   *     <existingInsns>: <instruction>s <target>s <source>s
   *                      <instruction>s <target>s <source-R2>s
   *                      -> <instruction>s <target>s <source-R1>s
   *     ------------
   *     (a) all targets cannot be R1, all sources cannot be R1
   *         all targets cannot be R2, all return sources cannot be R2
   *     (b) the targetRegs and sourceRegs cannot be list or MemoryAccess
   *     (c) no ifBB in preds, no branch insns
   *     (d) the bits of source-R2 must be equal to the R2
   *     (e) replace the R2 with R1
   */
  BB *pred = &ifbb;
  std::set<regno_t> vecTargetRegs; /* the targrtRegs of existingInsns */
  std::set<regno_t> vecSourceRegs; /* the soureRegs of existingInsns */
  bool ifPred = false; /* Indicates whether a ifBB in pred exists */
  bool bl = false; /* Indicates whether a branch insn exists */
  do {
    FOR_BB_INSNS_REV(insn, pred) {
      /* code sinking */
      if (insn->IsImmaterialInsn()) {
        moveInsns.push_back(insn);
        continue;
      }
      /* code sinking */
      if (!insn->IsMachineInstruction()) {
        moveInsns.push_back(insn);
        continue;
      }
      /* code sinking fails, the insns must be retained in the ifBB */
      if (ifPred || insn == ifbb.GetLastInsn() || insn->IsBranch() || insn->IsCall() ||
          insn->IsStore() || insn->IsStorePair()) {
        /* fast path finding fails */
        if (insn != ifbb.GetLastInsn() && (insn->IsBranch() || insn->IsCall() ||
            insn->IsStore() || insn->IsStorePair() || insn->IsSpecialCall())) {
          bl = true;
        }
        (void)InsertInsnRegs(*insn, true, vecSourceRegs, true, vecTargetRegs);
        existingInsns.push_back(insn);
        continue;
      }
      bool allow = true;  /* whether allow this insn move into the codeBB */
      for (uint32 o = 0; allow && o < insn->GetOperandSize(); ++o) {
        Operand &opnd = insn->GetOperand(o);
        if (insn->OpndIsDef(o)) {
          allow = allow && !FindRegs(opnd, vecTargetRegs);
          allow = allow && !FindRegs(opnd, vecSourceRegs);
          allow = allow && !FindRegs(opnd, vecReturnSourceRegs);
        }
        if (insn->OpndIsUse(o)) {
          allow = allow && !FindRegs(opnd, vecTargetRegs);
        }
      }
      /* if a result_dst not allowed, this insn can be allowed on the condition of mov Rx,R0/R1,
       * and tje existing insns cannot be blr
       * RLR 31, RFP 32, RSP 33, RZR 34 */
      if (!ifPred && !bl && !allow && (insn->GetMachineOpcode() == MOP_xmovrr ||
          insn->GetMachineOpcode() == MOP_wmovrr)) {
        Operand *resultOpnd = &(insn->GetOperand(0));
        Operand *srcOpnd = &(insn->GetOperand(1));
        regno_t resultNO = static_cast<RegOperand *>(resultOpnd)->GetRegisterNumber();
        regno_t srcNO = static_cast<RegOperand *>(srcOpnd)->GetRegisterNumber();
        if (!FindRegs(*resultOpnd, vecTargetRegs) && !FindRegs(*srcOpnd, vecTargetRegs) &&
            !FindRegs(*srcOpnd, vecSourceRegs) && !FindRegs(*srcOpnd, vecReturnSourceRegs) &&
            (srcNO < RLR || srcNO > RZR)) {
          allow = true; /* allow on the conditional mov Rx,Rxx */
          for (auto *exit : as_const(existingInsns)) {
            /* the registers of kOpdMem are complex to be detected */
            for (uint32 o = 0; o < exit->GetOperandSize(); ++o) {
              if (!exit->OpndIsUse(o)) {
                continue;
              }
              Operand *opd = &(exit->GetOperand(o));
              if (opd->IsList() || opd->IsMemoryAccessOperand()) {
                allow = false;
                break;
              }
              /* Distinguish between 32-bit regs and 64-bit regs */
              if (opd->IsRegister() &&
                  static_cast<RegOperand *>(opd)->GetRegisterNumber() == resultNO &&
                  opd != resultOpnd) {
                allow = false;
                break;
              }
            }
          }
        }
        /* replace the R2 with R1 */
        if (allow) {
          for (auto *exit : existingInsns) {
            for (uint32 o = 0; o < exit->GetOperandSize(); ++o) {
              if (!exit->OpndIsUse(o)) {
                continue;
              }
              Operand *opd = &(exit->GetOperand(o));
              if (opd->IsRegister() && (opd == resultOpnd)) {
                exit->SetOperand(o, *srcOpnd);
              }
            }
          }
        }
      }
      if (!allow) { /* all result_dsts are not target register */
        /* code sinking fails */
        (void)InsertInsnRegs(*insn, true, vecSourceRegs, true, vecTargetRegs);
        existingInsns.push_back(insn);
      } else {
        moveInsns.push_back(insn);
      }
    }
    if (pred->GetPreds().empty()) {
      break;
    }
    if (!ifPred) {
      for (auto *tmPred : pred->GetPreds()) {
        pred = tmPred;
        /* try to find the BB without branch */
        if (tmPred->GetKind() == BB::kBBGoto || tmPred->GetKind() == BB::kBBFallthru) {
          ifPred = false;
          break;
        } else {
          ifPred = true;
        }
      }
    }
  } while (pred != nullptr);
  for (std::set<regno_t>::iterator it = vecTargetRegs.begin(); it != vecTargetRegs.end(); ++it) {
    if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(*it))) { /* flag register */
      return false;
    }
  }
  return !bl;
}

void AArch64IsolateFastPath::IsolateFastPathOpt() {
  /*
   * Detect "if (cond) return" fast path, and move extra instructions
   * to the slow path.
   * Must match the following block structure. BB1 can be a series of
   * single-pred/single-succ blocks.
   *     BB1 ops1 cmp-br to BB3        BB1 cmp-br to BB3
   *     BB2 ops2 br to retBB    ==>   BB2 ret
   *     BB3 slow path                 BB3 ops1 ops2
   * if the detect is successful, BB3 will be used to generate prolog stuff.
   */
  BB &bb = *cgFunc.GetFirstBB();
  if (bb.GetPrev() != nullptr) {
    return;
  }
  BB *ifBB = nullptr;
  BB *returnBB = nullptr;
  BB *coldBB = nullptr;
  {
    BB *curBB = &bb;
    /* Look for straight line code */
    while (1) {
      if (!curBB->GetEhSuccs().empty()) {
        return;
      }
      if (curBB->GetSuccs().size() == 1) {
        if (curBB->HasCall()) {
          return;
        }
        BB *succ = curBB->GetSuccs().front();
        if (succ->GetPreds().size() != 1 || !succ->GetEhPreds().empty()) {
          return;
        }
        curBB = succ;
      } else if (curBB->GetKind() == BB::kBBIf) {
        ifBB = curBB;
        break;
      } else {
        return;
      }
    }
  }
  /* targets of if bb can only be reached by if bb */
  {
    CHECK_FATAL(!ifBB->GetSuccs().empty(), "null succs check!");
    BB *first = ifBB->GetSuccs().front();
    BB *second = ifBB->GetSuccs().back();
    if (first->GetPreds().size() != 1 || !first->GetEhPreds().empty()) {
      return;
    }
    if (second->GetPreds().size() != 1 || !second->GetEhPreds().empty()) {
      return;
    }
    /* One target of the if bb jumps to a return bb */
    if (first->GetKind() != BB::kBBGoto && first->GetKind() != BB::kBBFallthru) {
      return;
    }
    if (first->GetSuccs().size() != 1) {
      return;
    }
    if (first->GetSuccs().front()->GetKind() != BB::kBBReturn) {
      return;
    }
    if (first->GetSuccs().front()->GetPreds().size() != 1) {
      return;
    }
    constexpr int32 maxNumInsn = 2;
    if (first->GetSuccs().front()->NumInsn() > maxNumInsn) { /* avoid a insn is used to debug */
      return;
    }
    if (second->GetSuccs().empty()) {
      return;
    }
    returnBB = first;
    coldBB = second;
  }
  /* Search backward looking for dependencies for the cond branch */
  std::list<Insn*> existingInsns; /* the insns must be retained in the ifBB (and the return BB) */
  std::list<Insn*> moveInsns; /* instructions to be moved to coldbb */
  /*
   * The control flow matches at this point.
   * Make sure the SourceRegs of the insns in returnBB (vecReturnSourceReg) cannot be overwrite.
   * the regs in insns have three forms: list, MemoryAccess, or Register.
   */
  CHECK_FATAL(returnBB != nullptr, "null ptr check");
  std::set<regno_t> vecReturnSourceRegs;
  FOR_BB_INSNS_REV(insn, returnBB) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    if (insn->IsBranch() || insn->IsCall() || insn->IsStore() || insn->IsStorePair()) {
      return;
    }
    (void)InsertInsnRegs(*insn, true, vecReturnSourceRegs, false, vecReturnSourceRegs);
    existingInsns.push_back(insn);
  }
  FOR_BB_INSNS_REV(insn, returnBB->GetSuccs().front()) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    if (insn->IsBranch() || insn->IsCall() || insn->IsStore() || insn->IsStorePair()) {
      return;
    }
    (void)InsertInsnRegs(*insn, true, vecReturnSourceRegs, false, vecReturnSourceRegs);
    existingInsns.push_back(insn);
  }
  /*
   * The mv is the 1st move using the parameter register leading to the branch
   * The ld is the load using the parameter register indirectly for the branch
   * The depMv is the move which preserves the result of the load but might
   *    destroy a parameter register which will be moved below the branch.
   */
  bool fast = BackwardFindDependency(*ifBB, vecReturnSourceRegs, existingInsns, moveInsns);
  /* move extra instructions to the slow path */
  if (!fast) {
    return;
  }
  for (auto &in : as_const(moveInsns)) {
    in->GetBB()->RemoveInsn(*in);
    CHECK_FATAL(coldBB != nullptr, "null ptr check");
    coldBB->InsertInsnBegin(*in);
  }
  /* All instructions are in the right place, replace branch to ret bb to just ret. */
  /* Remove the lastInsn of gotoBB */
  if (returnBB->GetKind() == BB::kBBGoto) {
    returnBB->RemoveInsn(*returnBB->GetLastInsn());
  }
  BB *tgtBB = returnBB->GetSuccs().front();
  CHECK_FATAL(tgtBB->GetKind() == BB::kBBReturn, "check return bb of isolatefastpatch");
  CHECK_FATAL(tgtBB != nullptr, "null ptr check");
  FOR_BB_INSNS(insn, tgtBB) {
    returnBB->AppendInsn(*insn);  /* add the insns such as MOP_xret */
  }
  returnBB->AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_xret));
  /* bb is now a retbb and has no succ. */
  returnBB->SetKind(BB::kBBReturn);

  // add to exitvec and common exitpreds
  BB* commonExit = cgFunc.GetCommonExitBB();
  auto cEpredtgt = std::find(commonExit->GetPredsBegin(), commonExit->GetPredsEnd(), tgtBB);
  const auto cEpredreturn = std::find(commonExit->GetPredsBegin(), commonExit->GetPredsEnd(), returnBB);
  if (cEpredtgt == commonExit->GetPredsEnd() || cEpredreturn != commonExit->GetPredsEnd()) {
    CHECK_FATAL(false, "check case in isolatefast");
  }
  commonExit->RemovePreds(*tgtBB);
  commonExit->PushBackPreds(*returnBB);

  auto commonExitVecTgt = std::find(cgFunc.GetExitBBsVec().begin(), cgFunc.GetExitBBsVec().end(), tgtBB);
  auto commonExitVecRet = std::find(cgFunc.GetExitBBsVec().begin(), cgFunc.GetExitBBsVec().end(), returnBB);
  if (commonExitVecTgt == cgFunc.GetExitBBsVec().end() || commonExitVecRet != cgFunc.GetExitBBsVec().end()) {
    CHECK_FATAL(false, "check case in isolatefast");
  }
  (void)cgFunc.GetExitBBsVec().erase(commonExitVecTgt);
  cgFunc.GetExitBBsVec().push_back(returnBB);

  MapleList<BB*>::const_iterator predIt = std::find(tgtBB->GetPredsBegin(), tgtBB->GetPredsEnd(), returnBB);
  (void)tgtBB->ErasePreds(predIt);
  tgtBB->ClearInsns();
  returnBB->ClearSuccs();
  if (tgtBB->GetPrev() != nullptr && tgtBB->GetNext() != nullptr) {
    tgtBB->GetPrev()->SetNext(tgtBB->GetNext());
    tgtBB->GetNext()->SetPrev(tgtBB->GetPrev());
  }
  SetFastPathReturnBB(returnBB);
  cgFunc.SetPrologureBB(*coldBB);
}

void AArch64IsolateFastPath::Run() {
  IsolateFastPathOpt();
}
}  /* namespace maplebe */
