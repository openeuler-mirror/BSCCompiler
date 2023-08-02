/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cfgo.h"
#include "cgbb.h"
#include "cg.h"
#include "loop.h"
#include "cg_predict.h"
#include "mpl_logging.h"
/*
 * This phase traverses all basic block of cgFunc and finds special
 * basic block patterns, like continuous fallthrough basic block, continuous
 * uncondition jump basic block, unreachable basic block and empty basic block,
 * then do basic mergering, basic block placement transformations,
 * unnecessary jumps elimination, and remove unreachable or empty basic block.
 * This optimization is done on control flow graph basis.
 */
namespace maplebe {
using namespace maple;

#define CFGO_DUMP_NEWPM CG_DEBUG_FUNC(f)

// return true if to is put after from and there is no real insns between from and to
bool ChainingPattern::NoInsnBetween(const BB &from, const BB &to) const {
  const BB *bb = nullptr;
  for (bb = from.GetNext(); bb != nullptr && bb != &to && bb != cgFunc->GetLastBB(); bb = bb->GetNext()) {
    if (!bb->IsEmptyOrCommentOnly() || bb->IsUnreachable() || bb->GetKind() != BB::kBBFallthru) {
      return false;
    }
  }
  return (bb == &to);
}

// return true if insns in bb1 and bb2 are the same except the last goto insn.
bool ChainingPattern::DoSameThing(const BB &bb1, const Insn &last1, const BB &bb2, const Insn &last2) const {
  const Insn *insn1 = bb1.GetFirstMachineInsn();
  const Insn *insn2 = bb2.GetFirstMachineInsn();
  while (insn1 != nullptr && insn1 != last1.GetNext() && insn2 != nullptr && insn2 != last2.GetNext()) {
    if (!insn1->IsMachineInstruction()) {
      insn1 = insn1->GetNext();
      continue;
    }
    if (!insn2->IsMachineInstruction()) {
      insn2 = insn2->GetNext();
      continue;
    }
    if (insn1->GetMachineOpcode() != insn2->GetMachineOpcode()) {
      return false;
    }
    uint32 opndNum = insn1->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
      Operand &op1 = insn1->GetOperand(i);
      Operand &op2 = insn2->GetOperand(i);
      if (&op1 == &op2) {
        continue;
      }
      if (!op1.Equals(op2)) {
        return false;
      }
    }
    insn1 = insn1->GetNextMachineInsn();
    insn2 = insn2->GetNextMachineInsn();
  }
  return (insn1 == last1.GetNextMachineInsn() && insn2 == last2.GetNextMachineInsn());
}

// BB2 can be merged into BB1, if
//   1. BB1's kind is fallthrough;
//   2. BB2 has only one predecessor which is BB1 and BB2 is not the lastbb
//   3. BB2 is neither catch BB nor switch case BB
bool ChainingPattern::MergeFallthuBB(BB &curBB) {
  BB *sucBB = curBB.GetNext();
  if (sucBB == nullptr ||
      IsLabelInLSDAOrSwitchTable(sucBB->GetLabIdx()) ||
      !cgFunc->GetTheCFG()->CanMerge(curBB, *sucBB)) {
    return false;
  }
  if (curBB.IsAtomicBuiltInBB() || sucBB->IsAtomicBuiltInBB()) {
    return false;
  }
  Log(curBB.GetId());
  if (checkOnly) {
    return false;
  }
  if (sucBB == cgFunc->GetLastBB()) {
    cgFunc->SetLastBB(curBB);
  }
  cgFunc->GetTheCFG()->MergeBB(curBB, *sucBB, *cgFunc);
  keepPosition = true;
  return true;
}

bool ChainingPattern::MergeGotoBB(BB &curBB, BB &sucBB) {
  Log(curBB.GetId());
  if (checkOnly) {
    return false;
  }
  cgFunc->GetTheCFG()->MergeBB(curBB, sucBB, *cgFunc);
  keepPosition = true;
  return true;
}

bool ChainingPattern::MoveSuccBBAsCurBBNext(BB &curBB, BB &sucBB) {
  // without the judge below, there is
  // Assembler Error: CFI state restore without previous remember
  if (sucBB.GetHasCfi() || (sucBB.GetFirstInsn() != nullptr && sucBB.GetFirstInsn()->IsCfiInsn())) {
    return false;
  }
  Log(curBB.GetId());
  if (checkOnly) {
    return false;
  }
  // put sucBB as curBB's next.
  ASSERT(sucBB.GetPrev() != nullptr, "the target of current goto BB will not be the first bb");
  sucBB.GetPrev()->SetNext(sucBB.GetNext());
  if (sucBB.GetNext() != nullptr) {
    sucBB.GetNext()->SetPrev(sucBB.GetPrev());
  }
  sucBB.SetNext(curBB.GetNext());
  if (curBB.GetNext() != nullptr) {
    curBB.GetNext()->SetPrev(&sucBB);
  }
  if (sucBB.GetId() == cgFunc->GetLastBB()->GetId()) {
    cgFunc->SetLastBB(*(sucBB.GetPrev()));
  } else if (curBB.GetId() == cgFunc->GetLastBB()->GetId()) {
    cgFunc->SetLastBB(sucBB);
  }
  sucBB.SetPrev(&curBB);
  curBB.SetNext(&sucBB);
  if (curBB.GetLastMachineInsn() != nullptr) {
    curBB.RemoveInsn(*curBB.GetLastMachineInsn());
  }
  curBB.SetKind(BB::kBBFallthru);
  return true;
}

bool ChainingPattern::RemoveGotoInsn(BB &curBB, BB &sucBB) {
  Log(curBB.GetId());
  if (checkOnly) {
    return false;
  }
  if (&sucBB != curBB.GetNext()) {
    ASSERT(curBB.GetNext() != nullptr, "nullptr check");
    curBB.ReplaceSucc(sucBB, *curBB.GetNext());
    curBB.GetNext()->PushBackPreds(curBB);
    sucBB.RemovePreds(curBB);
  }
  if (curBB.GetLastMachineInsn() != nullptr) {
    curBB.RemoveInsn(*curBB.GetLastMachineInsn());
  }
  curBB.SetKind(BB::kBBFallthru);
  return true;
}

bool ChainingPattern::ClearCurBBAndResetTargetBB(BB &curBB, BB &sucBB) {
  if (curBB.GetHasCfi() || (curBB.GetFirstInsn() != nullptr && curBB.GetFirstInsn()->IsCfiInsn())) {
    return false;
  }
  Insn *brInsn = nullptr;
  for (brInsn = curBB.GetLastMachineInsn(); brInsn != nullptr; brInsn = brInsn->GetPrev()) {
    if (brInsn->IsUnCondBranch()) {
      break;
    }
  }
  ASSERT(brInsn != nullptr, "goto BB has no branch");
  BB *newTarget = sucBB.GetPrev();
  ASSERT(newTarget != nullptr, "get prev bb failed in ChainingPattern::ClearCurBBAndResetTargetBB");
  Insn *last1 = newTarget->GetLastMachineInsn();
  if (newTarget->GetKind() == BB::kBBGoto) {
    Insn *br = nullptr;
    for (br = newTarget->GetLastMachineInsn();
         newTarget->GetFirstInsn() != nullptr && br != newTarget->GetFirstInsn()->GetPrev();
         br = br->GetPrev()) {
      if (br->IsUnCondBranch()) {
        break;
      }
    }
    ASSERT(br != nullptr, "goto BB has no branch");
    last1 = br->GetPreviousMachineInsn();
  }
  if (last1 == nullptr) {
    return false;
  }
  if (brInsn->GetPreviousMachineInsn() && !DoSameThing(*newTarget, *last1, curBB, *brInsn->GetPreviousMachineInsn())) {
    return false;
  }

  Log(curBB.GetId());
  if (checkOnly) {
    return false;
  }

  LabelIdx tgtLabIdx = newTarget->GetLabIdx();
  if (newTarget->GetLabIdx() == MIRLabelTable::GetDummyLabel()) {
    tgtLabIdx = cgFunc->CreateLabel();
    newTarget->AddLabel(tgtLabIdx);
    cgFunc->SetLab2BBMap(tgtLabIdx, *newTarget);
  }
  LabelOperand &brTarget = cgFunc->GetOrCreateLabelOperand(tgtLabIdx);
  brInsn->SetOperand(0, brTarget);
  curBB.RemoveInsnSequence(*curBB.GetFirstInsn(), *brInsn->GetPrev());

  curBB.RemoveFromSuccessorList(sucBB);
  curBB.PushBackSuccs(*newTarget);
  sucBB.RemoveFromPredecessorList(curBB);
  newTarget->PushBackPreds(curBB);

  sucBB.GetPrev()->SetUnreachable(false);
  keepPosition = true;
  return true;
}

// Following optimizations are performed:
// 1. Basic block merging
// 2. unnecessary jumps elimination
// 3. Remove duplicates Basic block.
bool ChainingPattern::Optimize(BB &curBB) {
  if (curBB.IsUnreachable()) {
    return false;
  }

  if (curBB.GetKind() == BB::kBBFallthru) {
    return MergeFallthuBB(curBB);
  }

  if (curBB.GetKind() == BB::kBBGoto && !curBB.IsEmpty()) {
    Insn* last = curBB.GetLastMachineInsn();
    if (last->IsTailCall()) {
      return false;
    }

    BB *sucBB = CGCFG::GetTargetSuc(curBB);
    // BB2 can be merged into BB1, if
    //   1. BB1 ends with a goto;
    //   2. BB2 has only one predecessor which is BB1
    //   3. BB2 is of goto kind. Otherwise, the original fall through will be broken
    //   4. BB2 is neither catch BB nor switch case BB
    if (sucBB == nullptr || curBB.GetEhSuccs().size() != sucBB->GetEhSuccs().size()) {
      return false;
    }
    if (!curBB.GetEhSuccs().empty() && (curBB.GetEhSuccs().front() != sucBB->GetEhSuccs().front())) {
      return false;
    }
    if (sucBB->GetKind() == BB::kBBGoto &&
        !IsLabelInLSDAOrSwitchTable(sucBB->GetLabIdx()) &&
        cgFunc->GetTheCFG()->CanMerge(curBB, *sucBB)) {
      // BB9(curBB)   BB1
      //  \           /
      //   BB5(sucBB, gotoBB)
      // for this case, should not merge BB5, BB9
      if (sucBB->GetPreds().size() > 1) {
        return false;
      }
      return MergeGotoBB(curBB, *sucBB);
    } else if (sucBB != &curBB && curBB.GetNext() != sucBB &&
               sucBB != cgFunc->GetLastBB() && !sucBB->IsPredecessor(*sucBB->GetPrev()) &&
               !(sucBB->GetNext() != nullptr && sucBB->GetNext()->IsPredecessor(*sucBB)) &&
               !IsLabelInLSDAOrSwitchTable(sucBB->GetLabIdx()) && sucBB->GetEhSuccs().empty() &&
               sucBB->GetKind() != BB::kBBThrow && curBB.GetNext() != nullptr) {
      return MoveSuccBBAsCurBBNext(curBB, *sucBB);
    }
    // Last goto instruction can be removed, if:
    //  1. The goto target is physically the next one to current BB.
    else if (sucBB == curBB.GetNext() ||
             (NoInsnBetween(curBB, *sucBB) && !IsLabelInLSDAOrSwitchTable(curBB.GetNext()->GetLabIdx()))) {
      return RemoveGotoInsn(curBB, *sucBB);
    }
    // Clear curBB and target it to sucBB->GetPrev()
    //  if sucBB->GetPrev() and curBB's insns are the same.
    //
    // curBB:           curBB:
    //   insn_x0          b prevbb
    //   b sucBB        ...
    // ...         ==>  prevbb:
    // prevbb:            insn_x0
    //   insn_x0        sucBB:
    // sucBB:
    else if (sucBB != curBB.GetNext() && !curBB.IsSoloGoto() &&
             !IsLabelInLSDAOrSwitchTable(curBB.GetLabIdx()) &&
             sucBB->GetKind() == BB::kBBReturn &&
             sucBB->GetPreds().size() > 1 &&
             sucBB->GetPrev() != nullptr &&
             sucBB->IsPredecessor(*sucBB->GetPrev()) &&
             (sucBB->GetPrev()->GetKind() == BB::kBBFallthru || sucBB->GetPrev()->GetKind() == BB::kBBGoto)) {
      return ClearCurBBAndResetTargetBB(curBB, *sucBB);
    }
  }
  return false;
}

// curBB:             curBB:
//   insn_x0            insn_x0
//   b targetBB         b BB
// ...           ==>  ...
// targetBB:          targetBB:
//   b BB               b BB
// ...                ...
// BB:                BB:
// *------------------------------
// curBB:             curBB:
//   insn_x0            insn_x0
//   cond_br brBB       cond_br BB
// ...                ...
// brBB:         ==>  brBB:
//   b BB               b BB
// ...                ...
// BB:                BB:
//
// conditions:
//   1. only goto and comment in brBB;
bool SequentialJumpPattern::Optimize(BB &curBB) {
  if (curBB.IsUnreachable()) {
    return false;
  }
  if (curBB.GetKind() == BB::kBBGoto && !curBB.IsEmpty()) {
    BB *sucBB = CGCFG::GetTargetSuc(curBB);
    CHECK_FATAL(sucBB != nullptr, "sucBB is null in SequentialJumpPattern::Optimize");
    BB *targetBB = CGCFG::GetTargetSuc(*sucBB);
    if (sucBB != &curBB && sucBB->IsSoloGoto() && targetBB != nullptr && targetBB != sucBB && !HasInvalidPred(*sucBB)) {
      Log(curBB.GetId());
      if (checkOnly) {
        return false;
      }
      cgFunc->GetTheCFG()->RetargetJump(*sucBB, curBB);
      SkipSucBB(curBB, *sucBB);
      return true;
    }
  } else if (curBB.GetKind() == BB::kBBIf) {
    for (BB *sucBB : curBB.GetSuccs()) {
      BB *sucTargetBB = CGCFG::GetTargetSuc(*sucBB);
      if (sucBB != curBB.GetNext() && sucBB->IsSoloGoto() && sucTargetBB != nullptr && sucTargetBB != sucBB &&
          !HasInvalidPred(*sucBB)) {
        Log(curBB.GetId());
        if (checkOnly) {
          return false;
        }
        // e.g.
        //                BB12[if]   (curBB)
        //               beq  label:11
        //                 /     \
        //                /     BB25[if] (label: 11)
        //                \      /
        //                 BB13[goto]  (sucBB)
        //                   |
        //                 BB6[ft]   (targetBB)  (label: 6)
        // For the above case, the ifBB can not modify the target label of the conditional jump insn,
        // because the target of the conditional jump insn is the other succBB(BB25).
        BB *ifTargetBB = CGCFG::GetTargetSuc(curBB);
        CHECK_NULL_FATAL(ifTargetBB);
        // In addition, if the targetBB(ifTargetBB) of the ifBB is not the gotoBB(sucBB), and the targetBB(sucTargetBB)
        // of the sucBB is not its next, we can not do the optimization, because it will change the layout of the
        // sucTargetBB, and it does not necessarily improve performance.
        if (ifTargetBB != sucBB && sucBB->GetNext() != sucTargetBB) {
          return false;
        }
        if (ifTargetBB == sucBB) {
          cgFunc->GetTheCFG()->RetargetJump(*sucBB, curBB);
        }
        SkipSucBB(curBB, *sucBB);
        return true;
      }
    }
  } else if (curBB.GetKind() == BB::kBBRangeGoto) {
    bool changed = false;
    for (BB *sucBB : curBB.GetSuccs()) {
      if (sucBB != curBB.GetNext() && sucBB->IsSoloGoto() && !HasInvalidPred(*sucBB) &&
          CGCFG::GetTargetSuc(*sucBB) != nullptr) {
        Log(curBB.GetId());
        if (checkOnly) {
          return false;
        }
        UpdateSwitchSucc(curBB, *sucBB);
        cgFunc->GetTheCFG()->FlushUnReachableStatusAndRemoveRelations(*sucBB, *cgFunc);
        changed = true;
      }
    }
    return changed;
  }
  return false;
}

void SequentialJumpPattern::UpdateSwitchSucc(BB &curBB, BB &sucBB) const {
  BB *gotoTarget = CGCFG::GetTargetSuc(sucBB);
  CHECK_FATAL(gotoTarget != nullptr, "gotoTarget is null in SequentialJumpPattern::UpdateSwitchSucc");
  const MapleVector<LabelIdx> &labelVec = curBB.GetRangeGotoLabelVec();
  bool isPred = false;
  for (auto label: labelVec) {
    if (label == gotoTarget->GetLabIdx()) {
      isPred = true;
      break;
    }
  }
  for (size_t i = 0; i < labelVec.size(); ++i) {
    if (labelVec[i] == sucBB.GetLabIdx()) {
      curBB.SetRangeGotoLabel(static_cast<uint32>(i), gotoTarget->GetLabIdx());
    }
  }
  cgFunc->UpdateEmitSt(curBB, sucBB.GetLabIdx(), gotoTarget->GetLabIdx());

  // connect curBB, gotoTarget
  for (auto it = gotoTarget->GetPredsBegin(); it != gotoTarget->GetPredsEnd(); ++it) {
    if (*it == &sucBB) {
      auto origIt = it;
      if (isPred) {
        break;
      }
      if (origIt != gotoTarget->GetPredsBegin()) {
        --origIt;
        gotoTarget->InsertPred(origIt, curBB);
      } else {
        gotoTarget->PushFrontPreds(curBB);
      }
      break;
    }
  }
  for (auto it = curBB.GetSuccsBegin(); it != curBB.GetSuccsEnd(); ++it) {
    if (*it == &sucBB) {
      auto origIt = it;
      curBB.EraseSuccs(it);
      if (isPred) {
        break;
      }
      if (origIt != curBB.GetSuccsBegin()) {
        --origIt;
        curBB.InsertSucc(origIt, *gotoTarget);
      } else {
        curBB.PushFrontSuccs(*gotoTarget);
      }
      break;
    }
  }
  // cut curBB -> sucBB
  for (auto it = sucBB.GetPredsBegin(); it != sucBB.GetPredsEnd(); ++it) {
    if (*it == &curBB) {
      sucBB.ErasePreds(it);
    }
  }
  for (auto it = curBB.GetSuccsBegin(); it != curBB.GetSuccsEnd(); ++it) {
    if (*it == &sucBB) {
      curBB.EraseSuccs(it);
    }
  }
}

bool SequentialJumpPattern::HasInvalidPred(BB &sucBB) const {
  for (auto predIt = sucBB.GetPredsBegin(); predIt != sucBB.GetPredsEnd(); ++predIt) {
    if ((*predIt)->GetKind() != BB::kBBGoto && (*predIt)->GetKind() != BB::kBBIf &&
        (*predIt)->GetKind() != BB::kBBRangeGoto && (*predIt)->GetKind() != BB::kBBFallthru) {
      return true;
    }
    if ((*predIt)->GetKind() == BB::kBBIf) {
      BB *ifTargetBB = CGCFG::GetTargetSuc(**predIt);
      CHECK_NULL_FATAL(ifTargetBB);
      BB *sucTargetBB = CGCFG::GetTargetSuc(sucBB);
      CHECK_NULL_FATAL(sucTargetBB);
      if (ifTargetBB != &sucBB && sucBB.GetNext() != sucTargetBB) {
        return true;
      }
    }
    if ((*predIt)->GetKind() == BB::kBBIf || (*predIt)->GetKind() == BB::kBBRangeGoto) {
      if ((*predIt)->GetNext() == &sucBB) {
        return true;
      }
    } else if ((*predIt)->GetKind() == BB::kBBGoto) {
      if (*predIt == &sucBB || (*predIt)->IsEmpty()) {
        return true;
      }
    }
  }
  return false;
}

// preCond:
// sucBB is one of curBB's successor.
//
// Change curBB's successor to sucBB's successor
void SequentialJumpPattern::SkipSucBB(BB &curBB, BB &sucBB) const {
  BB *gotoTarget = CGCFG::GetTargetSuc(sucBB);
  CHECK_FATAL(gotoTarget != nullptr, "gotoTarget is null in SequentialJumpPattern::SkipSucBB");
  curBB.ReplaceSucc(sucBB, *gotoTarget);
  sucBB.RemovePreds(curBB);
  gotoTarget->PushBackPreds(curBB);
  // If the sucBB needs to be skipped, all preds of the sucBB must skip it and update cfg info.
  // e.g.
  //             BB3[if]     (curBB)
  //             / \
  //            /   BB6[if]
  //            \   /
  //             BB10[goto]  (sucBB)
  //              |
  //             BB8         (gotoTarget)
  for (auto *predBB : sucBB.GetPreds()) {
    if (predBB->GetKind() == BB::kBBGoto) {
      cgFunc->GetTheCFG()->RetargetJump(sucBB, *predBB);
    } else if (predBB->GetKind() == BB::kBBIf) {
      BB *ifTargetBB = CGCFG::GetTargetSuc(*predBB);
      if (ifTargetBB == &sucBB) {
        cgFunc->GetTheCFG()->RetargetJump(sucBB, *predBB);
      }
    } else if (predBB->GetKind() == BB::kBBFallthru) {
      // e.g.
      //      (curBB) BB70[goto]         BB27[if]
      //                \              /             \
      //                 \            /               \
      //                  \      BB71[ft] (iterPredBB) \
      //                   \     /                      \
      //                    BB48[goto]   (sucBB)      BB28[ft]
      //                      |                     /
      //                      |                    /
      //                    BB29[if]   (gotoTarget)
      ASSERT_NOT_NULL(cgFunc->GetTheCFG()->GetInsnModifier());
      cgFunc->GetTheCFG()->GetInsnModifier()->ModifyFathruBBToGotoBB(*predBB, gotoTarget->GetLabIdx());
    } else if (predBB->GetKind() == BB::kBBRangeGoto) {
      UpdateSwitchSucc(*predBB, sucBB);
    }
    predBB->ReplaceSucc(sucBB, *gotoTarget);
    sucBB.RemovePreds(*predBB);
    gotoTarget->PushBackPreds(*predBB);
  }
  cgFunc->GetTheCFG()->FlushUnReachableStatusAndRemoveRelations(sucBB, *cgFunc);
  // LastBB cannot be removed from the preds of succBB by FlushUnReachableStatusAndRemoveRelations, Why?
  // We'll do a separate process below for the case that sucBB is LastBB.
  if (sucBB.GetKind() == BB::kBBGoto && &sucBB == cgFunc->GetLastBB()) {
    // gotoBB has only one succ.
    ASSERT(sucBB.GetSuccsSize() == 1, "invalid gotoBB");
    sucBB.SetUnreachable(true);
    sucBB.SetFirstInsn(nullptr);
    sucBB.SetLastInsn(nullptr);
    gotoTarget->RemovePreds(sucBB);
    sucBB.RemoveSuccs(*gotoTarget);
  }
  // Remove the unreachableBB which has been skipped
  if (sucBB.IsUnreachable()) {
    cgFunc->GetTheCFG()->RemoveBB(sucBB);
  }
}

// Found pattern
// curBB:                      curBB:
//       ...            ==>          ...
//       cond_br brBB                cond1_br ftBB
// ftBB:                       brBB:
//       bl throwfunc                ...
// brBB:                       retBB:
//       ...                         ...
// retBB:                      ftBB:
//       ...                         bl throwfunc
void FlipBRPattern::RelocateThrowBB(BB &curBB) {
  BB *ftBB = curBB.GetNext();
  CHECK_FATAL(ftBB != nullptr, "ifBB has a fall through BB");
  CGCFG *theCFG = cgFunc->GetTheCFG();
  CHECK_FATAL(theCFG != nullptr, "nullptr check");
  BB *retBB = theCFG->FindLastRetBB();
  retBB = (retBB == nullptr ? cgFunc->GetLastBB() : retBB);
  if (ftBB->GetKind() != BB::kBBThrow || !ftBB->GetEhSuccs().empty() ||
      IsLabelInLSDAOrSwitchTable(ftBB->GetLabIdx()) || !retBB->GetEhSuccs().empty()) {
    return;
  }
  BB *brBB = CGCFG::GetTargetSuc(curBB);
  if (brBB != ftBB->GetNext()) {
    return;
  }

  EHFunc *ehFunc = cgFunc->GetEHFunc();
  if (ehFunc != nullptr && ehFunc->GetLSDACallSiteTable() != nullptr) {
    const MapleVector<LSDACallSite*> &callsiteTable = ehFunc->GetLSDACallSiteTable()->GetCallSiteTable();
    for (size_t i = 0; i < callsiteTable.size(); ++i) {
      LSDACallSite *lsdaCallsite = callsiteTable[i];
      BB *endTry = cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLength.GetEndOffset()->GetLabelIdx());
      BB *startTry = cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLength.GetStartOffset()->GetLabelIdx());
      if (retBB->GetId() >= startTry->GetId() && retBB->GetId() <= endTry->GetId()) {
        if (retBB->GetNext()->GetId() < startTry->GetId() || retBB->GetNext()->GetId() > endTry->GetId() ||
            curBB.GetId() < startTry->GetId() || curBB.GetId() > endTry->GetId()) {
          return;
        }
      } else {
        if ((retBB->GetNext()->GetId() >= startTry->GetId() && retBB->GetNext()->GetId() <= endTry->GetId()) ||
            (curBB.GetId() >= startTry->GetId() && curBB.GetId() <= endTry->GetId())) {
          return;
        }
      }
    }
  }
  // get branch insn of curBB
  Insn *curBBBranchInsn = theCFG->FindLastCondBrInsn(curBB);
  CHECK_FATAL(curBBBranchInsn != nullptr, "curBB(it is a kBBif) has no branch");

  // Reverse the branch
  uint32 targetIdx = GetJumpTargetIdx(*curBBBranchInsn);
  MOperator mOp = FlipConditionOp(curBBBranchInsn->GetMachineOpcode());
  LabelOperand &brTarget = cgFunc->GetOrCreateLabelOperand(*ftBB);
  curBBBranchInsn->SetMOP(cgFunc->GetCG()->GetTargetMd(mOp));
  curBBBranchInsn->SetOperand(targetIdx, brTarget);

  // move ftBB after retBB
  curBB.SetNext(brBB);
  CHECK_NULL_FATAL(brBB);
  brBB->SetPrev(&curBB);

  retBB->GetNext()->SetPrev(ftBB);
  ftBB->SetNext(retBB->GetNext());
  ftBB->SetPrev(retBB);
  retBB->SetNext(ftBB);
}

// 1. relocate goto BB
// Found pattern             (1) ftBB->GetPreds().size() == 1
// curBB:                      curBB: cond1_br target
//       ...            ==>    brBB:
//       cond_br brBB           ...
// ftBB:                       targetBB: (ftBB,targetBB)
//       goto target         (2) ftBB->GetPreds().size() > 1
// brBB:                       curBB : cond1_br ftBB
//       ...                   brBB:
// targetBB                      ...
//                            ftBB
//                            targetBB
//
// loopHeaderBB:              loopHeaderBB:
//       ...                        ...
//       cond_br loopExit:          cond_br loopHeaderBB
// ftBB:                      ftBB:
//       goto loopHeaderBB:         goto loopExit
//
// 3. relocate throw BB in RelocateThrowBB()
bool FlipBRPattern::Optimize(BB &curBB) {
  if (curBB.GetKind() == BB::kBBIf && !curBB.IsEmpty()) {
    BB *ftBB = curBB.GetNext();
    ASSERT(ftBB != nullptr, "ftBB is null in  FlipBRPattern::Optimize");
    BB *brBB = CGCFG::GetTargetSuc(curBB);
    ASSERT(brBB != nullptr, "brBB is null in  FlipBRPattern::Optimize");
    // Check if it can be optimized
    if (ftBB->GetKind() == BB::kBBGoto && ftBB->GetNext() == brBB) {
      if (!ftBB->GetEhSuccs().empty()) {
        return false;
      }
      Insn *curBBBranchInsn = nullptr;
      for (curBBBranchInsn = curBB.GetLastMachineInsn(); curBBBranchInsn != nullptr;
           curBBBranchInsn = curBBBranchInsn->GetPrev()) {
        if (curBBBranchInsn->IsBranch()) {
          break;
        }
      }
      ASSERT(curBBBranchInsn != nullptr, "FlipBRPattern: curBB has no branch");
      Insn *brInsn = nullptr;
      for (brInsn = ftBB->GetLastMachineInsn(); brInsn != nullptr; brInsn = brInsn->GetPrev()) {
        if (brInsn->IsUnCondBranch()) {
          break;
        }
      }
      ASSERT(brInsn != nullptr, "FlipBRPattern: ftBB has no branch");

      // Reverse the branch
      uint32 targetIdx = GetJumpTargetIdx(*curBBBranchInsn);
      MOperator mOp = FlipConditionOp(curBBBranchInsn->GetMachineOpcode());
      if (mOp == 0) {
        return false;
      }
      auto it = ftBB->GetSuccsBegin();
      BB *tgtBB = *it;
      if (ftBB->GetPreds().size() == 1 &&
          (ftBB->IsSoloGoto() ||
           (!IsLabelInLSDAOrSwitchTable(tgtBB->GetLabIdx()) &&
            cgFunc->GetTheCFG()->CanMerge(*ftBB, *tgtBB)))) {
        curBBBranchInsn->SetMOP(cgFunc->GetCG()->GetTargetMd(mOp));
        Operand &brTarget = brInsn->GetOperand(GetJumpTargetIdx(*brInsn));
        curBBBranchInsn->SetOperand(targetIdx, brTarget);
        // Insert ftBB's insn at the beginning of tgtBB.
        if (!ftBB->IsSoloGoto()) {
          ftBB->RemoveInsn(*brInsn);
          tgtBB->InsertAtBeginning(*ftBB);
        }
        // Patch pred and succ lists
        ftBB->EraseSuccs(it);
        ftBB->PushBackSuccs(*brBB);
        it = curBB.GetSuccsBegin();
        CHECK_FATAL(*it != nullptr, "nullptr check");
        if (*it == brBB) {
          curBB.ReplaceSucc(it, *tgtBB);
        } else {
          ++it;
          curBB.ReplaceSucc(it, *tgtBB);
        }
        for (it = tgtBB->GetPredsBegin(); it != tgtBB->GetPredsEnd(); ++it) {
          if (*it == ftBB) {
            tgtBB->ErasePreds(it);
            break;
          }
        }
        tgtBB->PushBackPreds(curBB);
        for (it = brBB->GetPredsBegin(); it != brBB->GetPredsEnd(); ++it) {
          if (*it == &curBB) {
            brBB->ErasePreds(it);
            break;
          }
        }
        brBB->PushFrontPreds(*ftBB);
        // Remove instructions from ftBB so curBB falls thru to brBB
        ftBB->SetFirstInsn(nullptr);
        ftBB->SetLastInsn(nullptr);
        ftBB->SetKind(BB::kBBFallthru);
      } else if (!IsLabelInLSDAOrSwitchTable(ftBB->GetLabIdx()) &&
                 !tgtBB->IsPredecessor(*tgtBB->GetPrev())) {
        curBBBranchInsn->SetMOP(cgFunc->GetCG()->GetTargetMd(mOp));
        LabelIdx tgtLabIdx = ftBB->GetLabIdx();
        if (ftBB->GetLabIdx() == MIRLabelTable::GetDummyLabel()) {
          tgtLabIdx = cgFunc->CreateLabel();
          ftBB->AddLabel(tgtLabIdx);
	        cgFunc->SetLab2BBMap(tgtLabIdx, *ftBB);
        }
        LabelOperand &brTarget = cgFunc->GetOrCreateLabelOperand(tgtLabIdx);
        curBBBranchInsn->SetOperand(targetIdx, brTarget);
        curBB.SetNext(brBB);
        brBB->SetPrev(&curBB);
        ftBB->SetPrev(tgtBB->GetPrev());
        tgtBB->GetPrev()->SetNext(ftBB);
        ftBB->SetNext(tgtBB);
        tgtBB->SetPrev(ftBB);

        ftBB->RemoveInsn(*brInsn);
        ftBB->SetKind(BB::kBBFallthru);
      }
    } else if (GetPhase() == kCfgoPostRegAlloc && ftBB->GetKind() == BB::kBBGoto &&
               loopInfo.GetBBLoopParent(curBB.GetId()) != nullptr &&
               loopInfo.GetBBLoopParent(curBB.GetId()) == loopInfo.GetBBLoopParent(ftBB->GetId()) &&
               ftBB->IsSoloGoto() &&
               &(loopInfo.GetBBLoopParent(ftBB->GetId())->GetHeader()) == *(ftBB->GetSuccsBegin()) &&
               !loopInfo.GetBBLoopParent(curBB.GetId())->Has(
                    (curBB.GetSuccs().front() == ftBB) ? *curBB.GetSuccs().back() : *curBB.GetSuccs().front())) {
      Insn *curBBBranchInsn = nullptr;
      for (curBBBranchInsn = curBB.GetLastMachineInsn(); curBBBranchInsn != nullptr;
           curBBBranchInsn = curBBBranchInsn->GetPrev()) {
        if (curBBBranchInsn->IsBranch()) {
          break;
        }
      }
      ASSERT(curBBBranchInsn != nullptr, "FlipBRPattern: curBB has no branch");
      Insn *brInsn = nullptr;
      for (brInsn = ftBB->GetLastMachineInsn(); brInsn != nullptr; brInsn = brInsn->GetPrev()) {
        if (brInsn->IsUnCondBranch()) {
          break;
        }
      }
      ASSERT(brInsn != nullptr, "FlipBRPattern: ftBB has no branch");
      uint32 condTargetIdx = GetJumpTargetIdx(*curBBBranchInsn);
      LabelOperand &condTarget = static_cast<LabelOperand&>(curBBBranchInsn->GetOperand(condTargetIdx));
      MOperator mOp = FlipConditionOp(curBBBranchInsn->GetMachineOpcode());
      if (mOp == 0) {
        return false;
      }
      uint32 gotoTargetIdx = GetJumpTargetIdx(*brInsn);
      LabelOperand &gotoTarget = static_cast<LabelOperand&>(brInsn->GetOperand(gotoTargetIdx));
      curBBBranchInsn->SetMOP(cgFunc->GetCG()->GetTargetMd(mOp));
      curBBBranchInsn->SetOperand(condTargetIdx, gotoTarget);
      brInsn->SetOperand(gotoTargetIdx, condTarget);
      auto it = ftBB->GetSuccsBegin();
      BB *loopHeadBB = *it;
      curBB.ReplaceSucc(*brBB, *loopHeadBB);
      brBB->RemovePreds(curBB);
      ftBB->ReplaceSucc(*loopHeadBB, *brBB);
      loopHeadBB->RemovePreds(*ftBB);

      loopHeadBB->PushBackPreds(curBB);
      brBB->PushBackPreds(*ftBB);
    } else {
      RelocateThrowBB(curBB);
    }
  }
  return false;
}

// remove a basic block that contains nothing
bool EmptyBBPattern::Optimize(BB &curBB) {
  // Can not remove the BB whose address is referenced by adrp_label insn
  if (curBB.IsUnreachable() || curBB.IsAdrpLabel()) {
    return false;
  }
  // Empty bb and it's not a cleanupBB/returnBB/lastBB/catchBB.
  if (curBB.GetPrev() == nullptr || curBB.IsCleanup() || &curBB == cgFunc->GetLastBB() ||
      curBB.GetKind() == BB::kBBReturn || IsLabelInLSDAOrSwitchTable(curBB.GetLabIdx())) {
    return false;
  }

  if (curBB.GetFirstInsn() == nullptr && curBB.GetLastInsn() == nullptr) {
    // empty BB
    Log(curBB.GetId());
    if (checkOnly) {
      return false;
    }
    BB *sucBB = CGCFG::GetTargetSuc(curBB);
    if (sucBB == nullptr || sucBB->IsCleanup()) {
      return false;
    }
    cgFunc->GetTheCFG()->RemoveBB(curBB);
    // removeBB may do nothing. since no need to repeat, always ret false here.
    return false;
  } else if (!curBB.HasMachineInsn()) {
    // BB only has dbg insn
    Log(curBB.GetId());
    if (checkOnly) {
      return false;
    }
    BB *sucBB = CGCFG::GetTargetSuc(curBB);
    if (sucBB == nullptr || sucBB->IsCleanup()) {
      return false;
    }
    // For Now We try to sink first conservatively.
    // All dbg insns should not be dropped. Later hoist or copy case should be considered.
    if (curBB.NumSuccs() == 1) {
      BB *succBB = curBB.GetSuccs().front();
      succBB->InsertAtBeginning(curBB);
      cgFunc->GetTheCFG()->RemoveBB(curBB);
    }
    return false;
  }
  return false;
}

// remove unreachable BB
// condition:
//   1. unreachable BB can't have cfi instruction when postcfgo.
bool UnreachBBPattern::Optimize(BB &curBB) {
  if (curBB.IsUnreachable()) {
    Log(curBB.GetId());
    if (checkOnly) {
      return false;
    }
    // if curBB in exitbbsvec,return false.
    if (cgFunc->IsExitBB(curBB)) {
      // In C some bb follow noreturn calls should remain unreachable
      curBB.SetUnreachable(cgFunc->GetMirModule().GetSrcLang() == kSrcLangC);
      return false;
    }

    if (curBB.GetHasCfi() || (curBB.GetFirstInsn() != nullptr && curBB.GetFirstInsn()->IsCfiInsn())) {
      return false;
    }

    EHFunc *ehFunc = cgFunc->GetEHFunc();
    // if curBB InLSDA ,replace curBB's label with nextReachableBB before remove it.
    if (ehFunc != nullptr && ehFunc->NeedFullLSDA() &&
        CGCFG::InLSDA(curBB.GetLabIdx(), ehFunc)) {
      // find nextReachableBB
      BB *nextReachableBB = nullptr;
      for (BB *bb = &curBB; bb != nullptr; bb = bb->GetNext()) {
        if (!bb->IsUnreachable()) {
          nextReachableBB = bb;
          break;
        }
      }
      CHECK_FATAL(nextReachableBB != nullptr, "nextReachableBB not be nullptr");
      if (nextReachableBB->GetLabIdx() == 0) {
        LabelIdx labIdx = cgFunc->CreateLabel();
        nextReachableBB->AddLabel(labIdx);
        cgFunc->SetLab2BBMap(labIdx, *nextReachableBB);
      }

      ehFunc->GetLSDACallSiteTable()->UpdateCallSite(curBB, *nextReachableBB);
    }

   // Indicates whether the curBB is removed
    bool isRemoved = true;
    if (curBB.GetPrev() != nullptr) {
      curBB.GetPrev()->SetNext(curBB.GetNext());
    }
    if (curBB.GetNext() != nullptr) {
      curBB.GetNext()->SetPrev(curBB.GetPrev());
    } else {
      cgFunc->SetLastBB(*(curBB.GetPrev()));
    }
    if (cgFunc->GetFirstBB() == cgFunc->GetLastBB() && cgFunc->GetFirstBB() != nullptr) {
      isRemoved = false;
    }
    // flush after remove
    for (BB *bb : curBB.GetSuccs()) {
      bb->RemovePreds(curBB);
      cgFunc->GetTheCFG()->FlushUnReachableStatusAndRemoveRelations(*bb, *cgFunc);
    }
    for (BB *bb : curBB.GetEhSuccs()) {
      bb->RemoveEhPreds(curBB);
      cgFunc->GetTheCFG()->FlushUnReachableStatusAndRemoveRelations(*bb, *cgFunc);
    }
    curBB.ClearSuccs();
    curBB.ClearEhSuccs();
    // return always be false
    if (!isRemoved) {
      return false;
    }
    if (cgFunc->GetCG()->GetCGOptions().WithDwarf() && cgFunc->GetWithSrc()) {
      DebugInfo *di = cgFunc->GetCG()->GetMIRModule()->GetDbgInfo();
      DBGDie *fdie = di->GetFuncDie(&cgFunc->GetFunction());
      for (auto attr : fdie->GetAttrVec()) {
        if (!attr->GetKeep()) {
          continue;
        }
        if ((attr->GetDwAt() == DW_AT_high_pc || attr->GetDwAt() == DW_AT_low_pc) &&
            attr->GetId() == curBB.GetLabIdx()) {
          attr->SetKeep(false);
        }
      }
    }
  }
  return false;
}

// BB_pred1:        BB_pred1:
//   b curBB          insn_x0
// ...                b BB2
// BB_pred2:   ==>  ...
//   b curBB        BB_pred2:
// ...                insn_x0
// curBB:             b BB2
//   insn_x0        ...
//   b BB2          curBB:
//                    insn_x0
//                    b BB2
// condition:
//   1. The number of instruction in curBB is less than THRESHOLD;
//   2. curBB can't have cfi instruction when postcfgo
bool DuplicateBBPattern::Optimize(BB &curBB) {
  if (!cgFunc->IsAfterRegAlloc()) {
    return false;
  }
  if (curBB.IsUnreachable()) {
    return false;
  }
  if (CGOptions::IsNoDupBB() || CGOptions::OptimizeForSize()) {
    return false;
  }

  // curBB can't be in try block
  if (curBB.GetKind() != BB::kBBGoto || IsLabelInLSDAOrSwitchTable(curBB.GetLabIdx()) ||
      !curBB.GetEhSuccs().empty()) {
    return false;
  }

#if defined(TARGARM32) && TARGARM32
  FOR_BB_INSNS(insn, (&curBB)) {
    if (insn->IsPCLoad() || insn->IsClinit()) {
      return false;
    }
  }
#endif
  // It is possible curBB jump to itself
  uint32 numPreds = curBB.NumPreds();
  for (BB *bb : curBB.GetPreds()) {
    if (bb == &curBB) {
      numPreds--;
    }
  }

  if (numPreds > 1 && CGCFG::GetTargetSuc(curBB) != nullptr && CGCFG::GetTargetSuc(curBB)->NumPreds() > 1) {
    std::vector<BB*> candidates;
    for (BB *bb : curBB.GetPreds()) {
      if (bb->GetKind() == BB::kBBGoto && bb->GetNext() != &curBB && bb != &curBB && !bb->IsEmpty()) {
        candidates.emplace_back(bb);
      }
    }
    if (candidates.empty()) {
      return false;
    }
    if (curBB.NumInsn() <= kThreshold) {
      if (curBB.GetHasCfi() || (curBB.GetFirstInsn() != nullptr && curBB.GetFirstInsn()->IsCfiInsn())) {
        return false;
      }
      Log(curBB.GetId());
      if (checkOnly) {
        return false;
      }
      bool changed = false;
      for (BB *bb : candidates) {
        if (curBB.GetEhSuccs().size() != bb->GetEhSuccs().size()) {
          continue;
        }
        if (!curBB.GetEhSuccs().empty() && (curBB.GetEhSuccs().front() != bb->GetEhSuccs().front())) {
          continue;
        }
        bb->RemoveInsn(*bb->GetLastInsn());
        FOR_BB_INSNS(insn, (&curBB)) {
          if (!insn->IsMachineInstruction()) {
            continue;
          }
          Insn *clonedInsn = cgFunc->GetTheCFG()->CloneInsn(*insn);
          clonedInsn->SetPrev(nullptr);
          clonedInsn->SetNext(nullptr);
          clonedInsn->SetBB(nullptr);
          bb->AppendInsn(*clonedInsn);
        }
        bb->RemoveSuccs(curBB);
        for (BB *item : curBB.GetSuccs()) {
          bb->PushBackSuccs(*item);
          item->PushBackPreds(*bb);
        }
        curBB.RemovePreds(*bb);
        changed = true;
      }
      cgFunc->GetTheCFG()->FlushUnReachableStatusAndRemoveRelations(curBB, *cgFunc);
      return changed;
    }
  }
  return false;
}

// === new pm ===
void CgPreCfgo::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLoopAnalysis>();
}

bool CgPreCfgo::PhaseRun(maplebe::CGFunc &f) {
  auto *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
  CFGOptimizer *cfgOptimizer = f.GetCG()->CreateCFGOptimizer(*GetPhaseMemPool(), f, *loopInfo);
  const std::string &funcClass = f.GetFunction().GetBaseClassName();
  const std::string &funcName = f.GetFunction().GetBaseFuncName();
  const std::string &name = funcClass + funcName;
  if (CFGO_DUMP_NEWPM) {
    DotGenerator::GenerateDot("before-precfgo", f, f.GetMirModule());
  }
  cfgOptimizer->Run(name);
  if (CFGO_DUMP_NEWPM) {
    f.GetTheCFG()->CheckCFG();
    DotGenerator::GenerateDot("after-precfgo", f, f.GetMirModule());
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPreCfgo, precfgo)


void CgCfgo::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLoopAnalysis>();
}

bool CgCfgo::PhaseRun(maplebe::CGFunc &f) {
  auto *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
  CFGOptimizer *cfgOptimizer = f.GetCG()->CreateCFGOptimizer(*GetPhaseMemPool(), f, *loopInfo);
  if (f.IsAfterRegAlloc()) {
    cfgOptimizer->SetPhase(kCfgoPostRegAlloc);
  }
  const std::string &funcClass = f.GetFunction().GetBaseClassName();
  const std::string &funcName = f.GetFunction().GetBaseFuncName();
  const std::string &name = funcClass + funcName;
  if (CFGO_DUMP_NEWPM) {
    DotGenerator::GenerateDot("before-cfgo", f, f.GetMirModule());
  }
  cfgOptimizer->Run(name);
  if (CFGO_DUMP_NEWPM) {
    f.GetTheCFG()->CheckCFG();
    DotGenerator::GenerateDot("after-cfgo", f, f.GetMirModule());
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCfgo, cfgo)

void CgPostCfgo::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLoopAnalysis>();
}

bool CgPostCfgo::PhaseRun(maplebe::CGFunc &f) {
  auto *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
  CFGOptimizer *cfgOptimizer = f.GetCG()->CreateCFGOptimizer(*GetPhaseMemPool(), f, *loopInfo);
  const std::string &funcClass = f.GetFunction().GetBaseClassName();
  const std::string &funcName = f.GetFunction().GetBaseFuncName();
  const std::string &name = funcClass + funcName;
  cfgOptimizer->SetPhase(kPostCfgo);
  if (CFGO_DUMP_NEWPM) {
    DotGenerator::GenerateDot("before-postcfgo", f, f.GetMirModule());
  }
  cfgOptimizer->Run(name);
  if (CFGO_DUMP_NEWPM) {
    DotGenerator::GenerateDot("after-postcfgo", f, f.GetMirModule());
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPostCfgo, postcfgo)
}  // namespace maplebe
