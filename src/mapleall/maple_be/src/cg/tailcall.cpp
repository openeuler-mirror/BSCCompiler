/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include "tailcall.h"
#include "cgfunc.h"

namespace maplebe {
using namespace maple;

/* tailcallopt cannot be used if stack address of this function is taken and passed,
   not checking the passing for now, just taken */
bool TailCallOpt::IsStackAddrTaken() {
  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS_REV(insn, bb) {
      if (!IsAddOrSubOp(insn->GetMachineOpcode())) {
        continue;
      }
      for (uint32 i = 0; i < insn->GetOperandSize(); i++) {
        if (insn->GetOperand(i).IsRegister()) {
          RegOperand &reg = static_cast<RegOperand&>(insn->GetOperand(i));
          if (OpndIsStackRelatedReg(reg)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

/* there are two stack protector:
 * 1. stack protector all: for all function
 * 2. stack protector strong: for some functon that
 *   <1> invoke alloca functon;
 *   <2> use stack address;
 *   <3> callee use return stack slot;
 *   <4> local symbol is vector type;
 * */
void TailCallOpt::NeedStackProtect() {
  ASSERT(stackProtect == false, "no stack protect default");
  CG *currCG = cgFunc.GetCG();
  if (currCG->IsStackProtectorAll()) {
    stackProtect = true;
    return;
  }

  if (!currCG->IsStackProtectorStrong()) {
    return;
  }

  if (cgFunc.HasAlloca()) {
    stackProtect = true;
    return;
  }

  /* check if function use stack address or callee function return stack slot */
  auto stackProtectInfo = cgFunc.GetStackProtectInfo();
  if ((stackProtectInfo & kAddrofStack) != 0 || (stackProtectInfo & kRetureStackSlot) != 0) {
    stackProtect = true;
    return;
  }

  /* check if local symbol is vector type */
  auto &mirFunction = cgFunc.GetFunction();
  uint32 symTabSize = static_cast<uint32>(mirFunction.GetSymTab()->GetSymbolTableSize());
  for (uint32 i = 0; i < symTabSize; ++i) {
    MIRSymbol *symbol = mirFunction.GetSymTab()->GetSymbolFromStIdx(i);
    if (symbol == nullptr || symbol->GetStorageClass() != kScAuto || symbol->IsDeleted()) {
      continue;
    }
    TyIdx tyIdx = symbol->GetTyIdx();
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (type->GetKind() == kTypeArray) {
      stackProtect = true;
      return;
    }

    if (type->IsStructType() && IncludeArray(*type)) {
      stackProtect = true;
      return;
    }
  }
}

bool TailCallOpt::IncludeArray(const MIRType &type) const {
  ASSERT(type.IsStructType(), "agg must be one of class/struct/union");
  auto &structType = static_cast<const MIRStructType&>(type);
  /* all elements of struct. */
  auto num = static_cast<uint8>(structType.GetFieldsSize());
  for (uint32 i = 0; i < num; ++i) {
    MIRType *elemType = structType.GetElemType(i);
    if (elemType->GetKind() == kTypeArray) {
      return true;
    }
    if (elemType->IsStructType() && IncludeArray(*elemType)) {
      return true;
    }
  }
  return false;
}

/*
 *  Remove redundant mov and mark optimizable bl/blr insn in the BB.
 *  Return value: true to call this modified block again.
 */
bool TailCallOpt::OptimizeTailBB(BB &bb, MapleSet<Insn*> &callInsns, const BB &exitBB) const {
  Insn *lastInsn = bb.GetLastInsn();
  if (bb.NumInsn() == 1 && lastInsn->IsMachineInstruction() && !lastInsn->IsPseudoInstruction() &&
    !InsnIsCallCand(*bb.GetLastInsn())) {
    return false;
  }
  FOR_BB_INSNS_REV_SAFE(insn, &bb, prevInsn) {
    if (!insn->IsMachineInstruction() || insn->IsPseudoInstruction()) {
      continue;
    }
    if (InsnIsLoadPair(*insn)) {
      if (bb.GetKind() == BB::kBBReturn) {
        RegOperand &reg = static_cast<RegOperand &>(insn->GetOperand(0));
        if (OpndIsCalleeSaveReg(reg)) {
          continue;  /* inserted restore from calleeregs-placement, ignore */
        }
      }
      return false;
    } else if (InsnIsMove(*insn)) {
      CHECK_FATAL(insn->GetOperand(0).IsRegister(), "operand0 is not register");
      CHECK_FATAL(insn->GetOperand(1).IsRegister(), "operand1 is not register");
      auto &reg1 = static_cast<RegOperand&>(insn->GetOperand(0));
      auto &reg2 = static_cast<RegOperand&>(insn->GetOperand(1));
      if (!OpndIsR0Reg(reg1) || !OpndIsR0Reg(reg2)) {
        return false;
      }
      bb.RemoveInsn(*insn);
      continue;
    } else if (InsnIsIndirectCall(*insn)) {
      if (insn->GetOperand(0).IsRegister()) {
        RegOperand &reg = static_cast<RegOperand&>(insn->GetOperand(0));
        if (OpndIsCalleeSaveReg(reg)) {
          return false;  /* can't tailcall, register will be overwritten by restore */
        }
      }
      (void)callInsns.insert(insn);
      return false;
    } else if (InsnIsCall(*insn)) {
      (void)callInsns.insert(insn);
      return false;
    } else if (InsnIsUncondJump(*insn)) {
      LabelOperand &bLab = static_cast<LabelOperand&>(insn->GetOperand(0));
      if (exitBB.GetLabIdx() == bLab.GetLabelIndex()) {
        continue;
      }
      return false;
    } else {
      return false;
    }
  }
  return true;
}

/* Recursively invoke this function for all predecessors of exitBB */
void TailCallOpt::TailCallBBOpt(BB &bb, MapleSet<Insn*> &callInsns, BB &exitBB) {
  /* callsite also in the return block as in "if () return; else foo();"
     call in the exit block */
  if (!bb.IsEmpty() && !OptimizeTailBB(bb, callInsns, exitBB)) {
    return;
  }

  for (auto tmpBB : bb.GetPreds()) {
    if (tmpBB->GetSuccs().size() != 1 || !tmpBB->GetEhSuccs().empty() ||
        (tmpBB->GetKind() != BB::kBBFallthru && tmpBB->GetKind() != BB::kBBGoto)) {
      continue;
    }

    if (OptimizeTailBB(*tmpBB, callInsns, exitBB)) {
      TailCallBBOpt(*tmpBB, callInsns, exitBB);
    }
  }
}

/*
 *  If a function without callee-saved register, and end with a function call,
 *  then transfer bl/blr to b/br.
 *  Return value: true if function do not need Prologue/Epilogue. false otherwise.
 */
bool TailCallOpt::DoTailCallOpt() {
  /* Count how many call insns in the whole function. */
  uint32 nCount = 0;
  bool hasGetStackClass = false;

  FOR_ALL_BB(bb, &cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsCall()) {
        if (InsnIsCall(*insn) && IsFuncNeedFrame(*insn)) {
          hasGetStackClass = true;
        }
        ++nCount;
      }
    }
  }
  if ((nCount > 0 && cgFunc.GetFunction().GetAttr(FUNCATTR_interface)) || hasGetStackClass) {
    return false;
  }

  if (nCount == 0) {
    // no bl instr in any bb
    return true;
  }

  size_t exitBBSize = cgFunc.GetExitBBsVec().size();
  /* For now to reduce complexity */

  BB *exitBB = nullptr;
  if (exitBBSize == 0) {
    if (cgFunc.GetLastBB()->GetPrev()->GetFirstStmt() == cgFunc.GetCleanupLabel() &&
        cgFunc.GetLastBB()->GetPrev()->GetPrev() != nullptr) {
      exitBB = cgFunc.GetLastBB()->GetPrev()->GetPrev();
    } else {
      exitBB = cgFunc.GetLastBB()->GetPrev();
    }
  } else {
    exitBB = cgFunc.GetExitBBsVec().front();
  }
  uint32 i = 1;
  size_t optCount = 0;
  do {
    MapleSet<Insn*> callInsns(tmpAlloc.Adapter());
    TailCallBBOpt(*exitBB, callInsns, *exitBB);
    if (callInsns.size() != 0) {
      optCount += callInsns.size();
      (void)exitBB2CallSitesMap.emplace(exitBB, callInsns);
    }
    if (i < exitBBSize) {
      exitBB = cgFunc.GetExitBBsVec()[i];
      ++i;
    } else {
      break;
    }
  } while (true);

  /* unequal means regular calls exist in function */
  return nCount == optCount;
}

void TailCallOpt::ConvertToTailCalls(MapleSet<Insn*> &callInsnsMap) {
  BB *exitBB = GetCurTailcallExitBB();

  /* ExitBB is filled only by now. If exitBB has restore of SP indicating extra stack space has
     been allocated, such as a function call with more than 8 args, argument with large aggr etc */
  int64 argsToStkPassSize = cgFunc.GetMemlayout()->SizeOfArgsToStackPass();
  if (!cgFunc.HasVLAOrAlloca() && argsToStkPassSize > 0) {
    return;
  }
  FOR_BB_INSNS(insn, exitBB) {
    if (InsnIsAddWithRsp(*insn)) {
      return;
    }
  }

  /* Replace all of the call insns. */
  for (Insn *callInsn : callInsnsMap) {
    ReplaceInsnMopWithTailCall(*callInsn);
    BB *bb = callInsn->GetBB();
    if (bb->GetKind() == BB::kBBGoto) {
      bb->SetKind(BB::kBBFallthru);
      if (InsnIsUncondJump(*bb->GetLastInsn())) {
        bb->RemoveInsn(*bb->GetLastInsn());
      }
    }
    ASSERT(bb->GetSuccs().size() <= 1, "expect no succ or single succ");
    for (auto sBB: bb->GetSuccs()) {
      bb->RemoveSuccs(*sBB);
      sBB->RemovePreds(*bb);
      bb->SetKind(BB::kBBReturn);
      cgFunc.PushBackExitBBsVec(*bb);
      cgFunc.GetCommonExitBB()->PushBackPreds(*bb);
      break;
    }
  }
}

void TailCallOpt::TideExitBB() {
  cgFunc.GetTheCFG()->UnreachCodeAnalysis();
  std::vector<BB*> realRets;
  for (auto *exitBB : cgFunc.GetExitBBsVec()) {
    if (!exitBB->GetPreds().empty()) {
      (void)realRets.emplace_back(exitBB);
    }
  }
  cgFunc.ClearExitBBsVec();
  for (auto *cand: realRets) {
    cgFunc.PushBackExitBBsVec(*cand);
  }
}

void TailCallOpt::Run() {
  if (cgFunc.GetCG()->DoTailCall() && !IsStackAddrTaken() && !stackProtect) {
    (void)DoTailCallOpt(); // return value == "no call instr/only or 1 tailcall"
  }
  if (cgFunc.GetMirModule().IsCModule() && !exitBB2CallSitesMap.empty()) {
    cgFunc.GetTheCFG()->InitInsnVisitor(cgFunc);
    for (auto pair : exitBB2CallSitesMap) {
      BB *curExitBB = pair.first;
      MapleSet<Insn*>& callInsnsMap = pair.second;
      SetCurTailcallExitBB(curExitBB);
      ConvertToTailCalls(callInsnsMap);
    }
    TideExitBB();
  }
}

bool CgTailCallOpt::PhaseRun(maplebe::CGFunc &f) {
  TailCallOpt *tailCallOpt = f.GetCG()->CreateCGTailCallOpt(*GetPhaseMemPool(), f);
  tailCallOpt->Run();
  return false;
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgTailCallOpt, tailcallopt)
}  /* namespace maplebe */