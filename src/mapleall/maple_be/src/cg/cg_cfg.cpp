/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_cfg.h"
#if TARGAARCH64
#include "aarch64_insn.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_insn.h"
#endif
#if TARGARM32
#include "arm32_insn.h"
#endif
#include "cg_option.h"
#include "mpl_logging.h"
#if TARGX86_64
#include "x64_cgfunc.h"
#include "cg.h"
#endif
#include <cstdlib>

namespace {
static const uint32_t CRCTable[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

using namespace maplebe;
bool CanBBThrow(const BB &bb) {
  FOR_BB_INSNS_CONST(insn, &bb) {
    if (insn->IsTargetInsn() && insn->CanThrow()) {
      return true;
    }
  }
  return false;
}
}

namespace maplebe {
void CGCFG::BuildCFG() const {
  /*
   * Second Pass:
   * Link preds/succs in the BBs
   */
  BB *firstBB = cgFunc->GetFirstBB();
  for (BB *curBB = firstBB; curBB != nullptr; curBB = curBB->GetNext()) {
    BB::BBKind kind = curBB->GetKind();
    switch (kind) {
      case BB::kBBIntrinsic:
        /*
         * An intrinsic BB append a MOP_wcbnz instruction at the end, check
         * AArch64CGFunc::SelectIntrinCall(IntrinsiccallNode *intrinsiccallNode) for details
         */
        if (!curBB->GetLastInsn()->IsBranch()) {
          break;
        }
      /* else fall through */
      [[clang::fallthrough]];
      case BB::kBBIf: {
        BB *fallthruBB = curBB->GetNext();
        curBB->PushBackSuccs(*fallthruBB);
        fallthruBB->PushBackPreds(*curBB);
        Insn *branchInsn = curBB->GetLastMachineInsn();
        CHECK_FATAL(branchInsn != nullptr, "machine instruction must be exist in ifBB");
        ASSERT(branchInsn->IsCondBranch(), "must be a conditional branch generated from an intrinsic");
        /* Assume the last non-null operand is the branch target */
        uint32 opSz = curBB->GetLastMachineInsn()->GetOperandSize();
        ASSERT(opSz >= 1, "lastOpndIndex's opnd is greater than -1");
        Operand &lastOpnd = branchInsn->GetOperand(opSz - 1u);
        ASSERT(lastOpnd.IsLabelOpnd(), "label Operand must be exist in branch insn");
        auto &labelOpnd = static_cast<LabelOperand&>(lastOpnd);
        BB *brToBB = cgFunc->GetBBFromLab2BBMap(labelOpnd.GetLabelIndex());
        if (fallthruBB->GetId() != brToBB->GetId()) {
          curBB->PushBackSuccs(*brToBB);
          brToBB->PushBackPreds(*curBB);
        }
        break;
      }
      case BB::kBBGoto: {
        Insn *insn = curBB->GetLastMachineInsn();
        CHECK_FATAL(insn != nullptr, "machine insn must be exist in gotoBB");
        ASSERT(insn->IsUnCondBranch(), "insn must be a unconditional branch insn");
        LabelIdx labelIdx = static_cast<LabelOperand&>(insn->GetOperand(0)).GetLabelIndex();
        BB *gotoBB = cgFunc->GetBBFromLab2BBMap(labelIdx);
        CHECK_FATAL(gotoBB != nullptr, "gotoBB is null");
        curBB->PushBackSuccs(*gotoBB);
        gotoBB->PushBackPreds(*curBB);
        break;
      }
      case BB::kBBIgoto: {
        for (auto lidx : CG::GetCurCGFunc()->GetMirModule().CurFunction()->GetLabelTab()->GetAddrTakenLabels()) {
          BB *igotobb = cgFunc->GetBBFromLab2BBMap(lidx);
          CHECK_FATAL(igotobb, "igotobb is null");
          curBB->PushBackSuccs(*igotobb);
          igotobb->PushBackPreds(*curBB);
        }
        break;
      }
      case BB::kBBRangeGoto: {
        std::set<BB*, BBIdCmp> bbs;
        for (auto labelIdx : curBB->GetRangeGotoLabelVec()) {
          BB *gotoBB = cgFunc->GetBBFromLab2BBMap(labelIdx);
          bbs.insert(gotoBB);
        }
        for (auto gotoBB : bbs) {
          curBB->PushBackSuccs(*gotoBB);
          gotoBB->PushBackPreds(*curBB);
        }
        break;
      }
      case BB::kBBThrow:
        break;
      case BB::kBBFallthru: {
        BB *fallthruBB = curBB->GetNext();
        if (fallthruBB != nullptr) {
          curBB->PushBackSuccs(*fallthruBB);
          fallthruBB->PushBackPreds(*curBB);
        }
        break;
      }
      default:
        break;
    }  /* end switch */

    EHFunc *ehFunc = cgFunc->GetEHFunc();
    /* Check exception table. If curBB is in a try block, add catch BB to its succs */
    if (ehFunc != nullptr && ehFunc->GetLSDACallSiteTable() != nullptr) {
      /* Determine if insn in bb can actually except */
      if (CanBBThrow(*curBB)) {
        const MapleVector<LSDACallSite*> &callsiteTable = ehFunc->GetLSDACallSiteTable()->GetCallSiteTable();
        for (size_t i = 0; i < callsiteTable.size(); ++i) {
          LSDACallSite *lsdaCallsite = callsiteTable[i];
          BB *endTry = cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLength.GetEndOffset()->GetLabelIdx());
          BB *startTry = cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLength.GetStartOffset()->GetLabelIdx());
          if (curBB->GetId() >= startTry->GetId() && curBB->GetId() <= endTry->GetId() &&
              lsdaCallsite->csLandingPad.GetEndOffset() != nullptr) {
            BB *landingPad = cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLandingPad.GetEndOffset()->GetLabelIdx());
            curBB->PushBackEhSuccs(*landingPad);
            landingPad->PushBackEhPreds(*curBB);
          }
        }
      }
    }
  }
  FindAndMarkUnreachable(*cgFunc);
}

static inline uint32 CRC32Compute(uint32_t crc, uint32 val) {
  crc ^= 0xFFFFFFFFU;
  for (int32 idx = 3; idx >= 0; idx--) {
    uint8 byteVal = (val >> (static_cast<uint32>(idx) * k8BitSize)) & 0xffu;
    int TableIdx = (crc ^ byteVal) & 0xffu;
    crc = CRCTable[TableIdx] ^ (crc >> k8BitSize);
  }
  return crc ^ 0xFFFFFFFFU;
}

uint32 CGCFG::ComputeCFGHash() {
  uint32 hash = 0xfffffffful;
  FOR_ALL_BB(bb, cgFunc) {
    hash = CRC32Compute (hash, bb->GetId());
    for (BB *sucBB : bb->GetSuccs()) {
      hash = CRC32Compute (hash, sucBB->GetId());
    }
  }
  return hash;
}

void CGCFG::CheckCFG() {
  FOR_ALL_BB(bb, cgFunc) {
    for (BB *sucBB : bb->GetSuccs()) {
      bool found = false;
      for (BB *sucPred : sucBB->GetPreds()) {
        if (sucPred == bb) {
          if (found == false) {
            found = true;
          } else {
            LogInfo::MapleLogger() << "dup pred " << sucPred->GetId() << " for sucBB " << sucBB->GetId() << "\n";
            CHECK_FATAL_FALSE("CG_CFG check failed !");
          }
        }
      }
      if (found == false) {
        LogInfo::MapleLogger() << "non pred for sucBB " << sucBB->GetId() << " for BB " << bb->GetId() << "\n";
        CHECK_FATAL_FALSE("CG_CFG check failed !");
      }
    }
  }
  FOR_ALL_BB(bb, cgFunc) {
    for (BB *predBB : bb->GetPreds()) {
      bool found = false;
      for (BB *predSucc : predBB->GetSuccs()) {
        if (predSucc == bb) {
          if (found == false) {
            found = true;
          } else {
            LogInfo::MapleLogger() << "dup succ " << predSucc->GetId() << " for predBB " << predBB->GetId() << "\n";
            CHECK_FATAL_FALSE("CG_CFG check failed !");
          }
        }
      }
      if (found == false) {
        LogInfo::MapleLogger() << "non succ for predBB " << predBB->GetId() << " for BB " << bb->GetId() << "\n";
        CHECK_FATAL_FALSE("CG_CFG check failed !");
      }
    }
  }
}

void CGCFG::CheckCFGFreq() {
  auto verifyBBFreq = [this](const BB *bb, uint32 succFreq) {
    uint32 res = bb->GetFrequency();
    if ((res != 0 && static_cast<uint32>(abs(static_cast<int>(res - succFreq))) / res > 1) ||
        (res == 0 && res != succFreq)) {
      // Not included
      if (bb->GetSuccs().size() > 1 && bb->GetPreds().size() > 1) {
        return;
      }
      LogInfo::MapleLogger() << cgFunc->GetName() << " curBB: " << bb->GetId() << " freq: "
                             << bb->GetFrequency() << std::endl;
      CHECK_FATAL(false, "Verifyfreq failure BB frequency!");
    }
  };
  FOR_ALL_BB(bb, cgFunc) {
    if (bb->IsUnreachable() || bb->IsCleanup()) {
      continue;
    }
    uint32 res = 0;
    if (bb->GetSuccs().size() > 1) {
      for (auto *succBB : bb->GetSuccs()) {
        res += succBB->GetFrequency();
        if (succBB->GetPreds().size() > 1) {
          LogInfo::MapleLogger() << cgFunc->GetName() << " critical edges: curBB: " << bb->GetId() << std::endl;
          CHECK_FATAL(false, "The CFG has critical edges!");
        }
      }
      verifyBBFreq(bb, res);
    } else if (bb->GetSuccs().size() == 1) {
      auto *succBB = bb->GetSuccs().front();
      if (succBB->GetPreds().size() == 1) {
        verifyBBFreq(bb, succBB->GetFrequency());
      } else if (succBB->GetPreds().size() > 1) {
        for (auto *pred : succBB->GetPreds()) {
          res += pred->GetFrequency();
        }
        verifyBBFreq(succBB, res);
      }
    }
  }
  LogInfo::MapleLogger() << "Check Frequency for " << cgFunc->GetName() << " success!\n";
}

InsnVisitor *CGCFG::insnVisitor;

void CGCFG::InitInsnVisitor(CGFunc &func) const {
  insnVisitor = func.NewInsnModifier();
}

Insn *CGCFG::CloneInsn(Insn &originalInsn) const {
  cgFunc->IncTotalNumberOfInstructions();
  return insnVisitor->CloneInsn(originalInsn);
}

RegOperand *CGCFG::CreateVregFromReg(const RegOperand &pReg) const {
  return insnVisitor->CreateVregFromReg(pReg);
}

/*
 * return true if:
 * mergee has only one predecessor which is merger,
 * or mergee has other comments only predecessors & merger is soloGoto
 * mergee can't have cfi instruction when postcfgo.
 */
bool CGCFG::BBJudge(const BB &first, const BB &second) const {
  if (first.GetKind() == BB::kBBReturn || second.GetKind() == BB::kBBReturn) {
    return false;
  }
  if (&first == &second) {
    return false;
  }
  if (second.GetPreds().size() == 1 && second.GetPreds().front() == &first) {
    return true;
  }
  for (BB *bb : second.GetPreds()) {
    if (bb != &first && !AreCommentAllPreds(*bb)) {
      return false;
    }
  }
  return first.IsSoloGoto();
}

/*
 * Check if a given BB mergee can be merged into BB merger.
 * Returns true if:
 * 1. mergee has only one predecessor which is merger, or mergee has
 *   other comments only predecessors.
 * 2. merge has only one successor which is mergee.
 * 3. mergee can't have cfi instruction when postcfgo.
 */
bool CGCFG::CanMerge(const BB &merger, const BB &mergee) const {
  if (!BBJudge(merger, mergee)) {
    return false;
  }
  if (mergee.GetFirstInsn() != nullptr && mergee.GetFirstInsn()->IsCfiInsn()) {
    return false;
  }
  return (merger.GetSuccs().size() == 1) && (merger.GetSuccs().front() == &mergee);
}

/* Check if the given BB contains only comments and all its predecessors are comments */
bool CGCFG::AreCommentAllPreds(const BB &bb) {
  if (!bb.IsCommentBB()) {
    return false;
  }
  for (BB *pred : bb.GetPreds()) {
    if (!AreCommentAllPreds(*pred)) {
      return false;
    }
  }
  return true;
}

/* Merge sucBB into curBB. */
void CGCFG::MergeBB(BB &merger, BB &mergee, CGFunc &func) {
  BB *prevLast = mergee.GetPrev();
  MergeBB(merger, mergee);
  if (func.GetLastBB()->GetId() == mergee.GetId()) {
    func.SetLastBB(*prevLast);
  }
  if (mergee.GetKind() == BB::kBBReturn) {
    auto retIt = func.GetExitBBsVec().begin();
    while (retIt != func.GetExitBBsVec().end()) {
      if (*retIt == &mergee) {
        (void)func.EraseExitBBsVec(retIt);
        break;
      } else {
        ++retIt;
      }
    }
    func.PushBackExitBBsVec(merger);
  }
  if (mergee.GetKind() == BB::kBBNoReturn) {
    auto noRetIt = func.GetNoRetCallBBVec().begin();
    while (noRetIt != func.GetNoRetCallBBVec().end()) {
      if (*noRetIt == &mergee) {
        (void)func.EraseNoReturnCallBB(noRetIt);
        break;
      } else {
        ++noRetIt;
      }
    }
    func.PushBackNoReturnCallBBsVec(merger);
  }
  if (mergee.GetKind() == BB::kBBRangeGoto) {
    func.AddEmitSt(merger.GetId(), *func.GetEmitSt(mergee.GetId()));
    func.DeleteEmitSt(mergee.GetId());
  }
}

void CGCFG::MergeBB(BB &merger, BB &mergee) {
  if (merger.GetKind() == BB::kBBGoto) {
    if (!merger.GetLastInsn()->IsBranch()) {
      CHECK_FATAL(false, "unexpected insn kind");
    }
    merger.RemoveInsn(*merger.GetLastInsn());
  }
  merger.AppendBBInsns(mergee);
  if (mergee.GetPrev() != nullptr) {
    mergee.GetPrev()->SetNext(mergee.GetNext());
  }
  if (mergee.GetNext() != nullptr) {
    mergee.GetNext()->SetPrev(mergee.GetPrev());
  }
  merger.RemoveSuccs(mergee);
  if (!merger.GetEhSuccs().empty()) {
#if DEBUG
    for (BB *bb : merger.GetEhSuccs()) {
      ASSERT((bb != &mergee), "CGCFG::MergeBB: Merging of EH bb");
    }
#endif
  }
  if (!mergee.GetEhSuccs().empty()) {
    for (BB *bb : mergee.GetEhSuccs()) {
      bb->RemoveEhPreds(mergee);
      bb->PushBackEhPreds(merger);
      merger.PushBackEhSuccs(*bb);
    }
  }
  for (BB *bb : mergee.GetSuccs()) {
    bb->RemovePreds(mergee);
    bb->PushBackPreds(merger);
    merger.PushBackSuccs(*bb);
  }
  merger.SetKind(mergee.GetKind());
  merger.SetNeedRestoreCfi(mergee.IsNeedRestoreCfi());
  mergee.SetNext(nullptr);
  mergee.SetPrev(nullptr);
  mergee.ClearPreds();
  mergee.ClearSuccs();
  mergee.ClearEhPreds();
  mergee.ClearEhSuccs();
  mergee.SetFirstInsn(nullptr);
  mergee.SetLastInsn(nullptr);
}

/*
 * Find all reachable BBs by dfs in cgfunc and mark their field<unreachable> false, then all other bbs should be
 * unreachable.
 */
void CGCFG::FindAndMarkUnreachable(CGFunc &func) {
  BB *firstBB = func.GetFirstBB();
  std::stack<BB*> toBeAnalyzedBBs;
  toBeAnalyzedBBs.push(firstBB);
  std::unordered_set<uint32> instackBBs;

  BB *bb = firstBB;
  /* set all bb's unreacable to true */
  while (bb != nullptr) {
    /* Check if bb is the cleanupBB/switchTableBB/firstBB/lastBB of the function */
    if (bb->IsCleanup() || InSwitchTable(bb->GetLabIdx(), func) || bb == func.GetFirstBB() || bb == func.GetLastBB()) {
      toBeAnalyzedBBs.push(bb);
    } else if (bb->IsLabelTaken() == false) {
      bb->SetUnreachable(true);
    }
    bb = bb->GetNext();
  }

  /* do a dfs to see which bbs are reachable */
  while (!toBeAnalyzedBBs.empty()) {
    bb = toBeAnalyzedBBs.top();
    toBeAnalyzedBBs.pop();
    (void)instackBBs.insert(bb->GetId());

    bb->SetUnreachable(false);

    for (BB *succBB : bb->GetSuccs()) {
      if (instackBBs.count(succBB->GetId()) == 0) {
        toBeAnalyzedBBs.push(succBB);
        (void)instackBBs.insert(succBB->GetId());
      }
    }
    for (BB *succBB : bb->GetEhSuccs()) {
      if (instackBBs.count(succBB->GetId()) == 0) {
        toBeAnalyzedBBs.push(succBB);
        (void)instackBBs.insert(succBB->GetId());
      }
    }
  }
  FOR_ALL_BB(tmpBB, &func) {
    for (MapleList<BB*>::iterator predIt = tmpBB->GetPredsBegin(); predIt != tmpBB->GetPredsEnd(); ++predIt) {
      if ((*predIt)->IsUnreachable()) {
        tmpBB->ErasePreds(predIt);
      }
    }
    for (MapleList<BB*>::iterator predIt = tmpBB->GetEhPredsBegin(); predIt != tmpBB->GetEhPredsEnd(); ++predIt) {
      if ((*predIt)->IsUnreachable()) {
        tmpBB->ErasePreds(predIt);
      }
    }
  }
}

/*
 * Theoretically, every time you remove from a bb's preds, you should consider invoking this method.
 *
 * @param bb
 * @param func
 */
void CGCFG::FlushUnReachableStatusAndRemoveRelations(BB &bb, const CGFunc &func) const {
  bool isFirstBBInfunc = (&bb == func.GetFirstBB());
  bool isLastBBInfunc = (&bb == func.GetLastBB());
  /* Check if bb is the cleanupBB/switchTableBB/firstBB/lastBB of the function */
  if (bb.IsCleanup() || InSwitchTable(bb.GetLabIdx(), func) || isFirstBBInfunc || isLastBBInfunc) {
    return;
  }
  std::stack<BB*> toBeAnalyzedBBs;
  toBeAnalyzedBBs.push(&bb);
  std::set<uint32> instackBBs;
  BB *it = nullptr;
  while (!toBeAnalyzedBBs.empty()) {
    it = toBeAnalyzedBBs.top();
    (void)instackBBs.insert(it->GetId());
    toBeAnalyzedBBs.pop();
    /* Check if bb is the first or the last BB of the function */
    isFirstBBInfunc = (it == func.GetFirstBB());
    isLastBBInfunc = (it == func.GetLastBB());
    bool needFlush = !isFirstBBInfunc && !isLastBBInfunc && !it->IsCleanup() &&
                     (it->GetPreds().empty() || (it->GetPreds().size() == 1 && it->GetEhPreds().front() == it)) &&
                     it->GetEhPreds().empty() &&
                     !InSwitchTable(it->GetLabIdx(), *cgFunc) &&
                     !cgFunc->IsExitBB(*it) &&
                     (it->IsLabelTaken() == false);
    if (!needFlush) {
      continue;
    }
    it->SetUnreachable(true);
    it->SetFirstInsn(nullptr);
    it->SetLastInsn(nullptr);
    for (BB *succ : it->GetSuccs()) {
      if (instackBBs.count(succ->GetId()) == 0) {
        toBeAnalyzedBBs.push(succ);
        (void)instackBBs.insert(succ->GetId());
      }
      succ->RemovePreds(*it);
      succ->RemoveEhPreds(*it);
    }
    it->ClearSuccs();
    for (BB *succ : it->GetEhSuccs()) {
      if (instackBBs.count(succ->GetId()) == 0) {
        toBeAnalyzedBBs.push(succ);
        (void)instackBBs.insert(succ->GetId());
      }
      succ->RemoveEhPreds(*it);
      succ->RemovePreds(*it);
    }
    it->ClearEhSuccs();
  }
}

void CGCFG::RemoveBB(BB &curBB, bool isGotoIf) const {
  if (!curBB.IsUnreachable()) {
    BB *sucBB = CGCFG::GetTargetSuc(curBB, false, isGotoIf);
    if (sucBB != nullptr) {
      sucBB->RemovePreds(curBB);
    }

    BB *fallthruSuc = nullptr;
    if (isGotoIf) {
      for (BB *succ : curBB.GetSuccs()) {
        if (succ == sucBB) {
          continue;
        }
        fallthruSuc = succ;
        break;
      }
      ASSERT(fallthruSuc == curBB.GetNext(), "fallthru succ should be its next bb.");
      if (fallthruSuc != nullptr) {
        fallthruSuc->RemovePreds(curBB);
      }
    }
    for (BB *preBB : curBB.GetPreds()) {
      if (preBB->GetKind() == BB::kBBIgoto) {
        sucBB->PushBackPreds(curBB);
        return;
      }
      /*
     * If curBB is the target of its predecessor, change
     * the jump target.
     */
      if (&curBB == GetTargetSuc(*preBB, true, isGotoIf)) {
        LabelIdx targetLabel;
        if (curBB.GetNext()->GetLabIdx() == 0) {
          targetLabel = insnVisitor->GetCGFunc()->CreateLabel();
          curBB.GetNext()->SetLabIdx(targetLabel);
        } else {
          targetLabel = curBB.GetNext()->GetLabIdx();
        }
        insnVisitor->ModifyJumpTarget(targetLabel, *preBB);
      }
      if (fallthruSuc != nullptr && !fallthruSuc->IsPredecessor(*preBB)) {
        preBB->PushBackSuccs(*fallthruSuc);
        fallthruSuc->PushBackPreds(*preBB);
      }
      if (sucBB != nullptr && !sucBB->IsPredecessor(*preBB)) {
        preBB->PushBackSuccs(*sucBB);
        sucBB->PushBackPreds(*preBB);
      }
      preBB->RemoveSuccs(curBB);
    }
  }

  for (BB *ehSucc : curBB.GetEhSuccs()) {
    ehSucc->RemoveEhPreds(curBB);
  }
  for (BB *ehPred : curBB.GetEhPreds()) {
    ehPred->RemoveEhSuccs(curBB);
  }
  if (curBB.GetNext() != nullptr) {
    cgFunc->GetCommonExitBB()->RemovePreds(curBB);
    curBB.GetNext()->RemovePreds(curBB);
    curBB.GetNext()->SetPrev(curBB.GetPrev());
  } else {
    cgFunc->SetLastBB(*curBB.GetPrev());
  }
  curBB.GetPrev()->SetNext(curBB.GetNext());
  cgFunc->ClearBBInVec(curBB.GetId());
  /* remove callsite */
  EHFunc *ehFunc = cgFunc->GetEHFunc();
  /* only java try has ehFunc->GetLSDACallSiteTable */
  if (ehFunc != nullptr && ehFunc->GetLSDACallSiteTable() != nullptr) {
    ehFunc->GetLSDACallSiteTable()->RemoveCallSite(curBB);
  }

  /* If bb is removed, the related die information needs to be updated. */
  if (cgFunc->GetCG()->GetCGOptions().WithDwarf()) {
    DebugInfo *di = cgFunc->GetCG()->GetMIRModule()->GetDbgInfo();
    DBGDie *fdie = di->GetFuncDie(&cgFunc->GetFunction());
    CHECK_FATAL(fdie != nullptr, "fdie should not be nullptr");
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

void CGCFG::UpdateCommonExitBBInfo() {
  BB *commonExitBB = cgFunc->GetCommonExitBB();
  ASSERT_NOT_NULL(commonExitBB);
  commonExitBB->ClearPreds();
  for (BB *exitBB : cgFunc->GetExitBBsVec()) {
    if (!exitBB->IsUnreachable()) {
      commonExitBB->PushBackPreds(*exitBB);
    }
  }
  for (BB *noRetBB : cgFunc->GetNoRetCallBBVec()) {
    if (!noRetBB->IsUnreachable()) {
      commonExitBB->PushBackPreds(*noRetBB);
    }
  }
  WontExitAnalysis();
}

void CGCFG::RetargetJump(BB &srcBB, BB &targetBB) const {
  insnVisitor->ModifyJumpTarget(srcBB, targetBB);
}

BB *CGCFG::GetTargetSuc(BB &curBB, bool branchOnly, bool isGotoIf) {
  switch (curBB.GetKind()) {
    case BB::kBBGoto:
    case BB::kBBIntrinsic:
    case BB::kBBIf: {
      const Insn* origLastInsn = curBB.GetLastMachineInsn();
      ASSERT_NOT_NULL(origLastInsn);
      if (isGotoIf && (curBB.GetPrev() != nullptr) &&
          (curBB.GetKind() == BB::kBBGoto || curBB.GetKind() == BB::kBBIf) &&
          (curBB.GetPrev()->GetKind() == BB::kBBGoto || curBB.GetPrev()->GetKind() == BB::kBBIf)) {
        origLastInsn = curBB.GetPrev()->GetLastMachineInsn();
      }
      LabelIdx label = insnVisitor->GetJumpLabel(*origLastInsn);
      for (BB *bb : curBB.GetSuccs()) {
        if (bb->GetLabIdx() == label) {
          return bb;
        }
      }
      break;
    }
    case BB::kBBIgoto: {
      for (Insn *insn = curBB.GetLastInsn(); insn != nullptr; insn = insn->GetPrev()) {
#if TARGAARCH64
        if (insn->GetMachineOpcode() == MOP_adrp_label) {
          int64 label = static_cast<ImmOperand&>(insn->GetOperand(1)).GetValue();
          for (BB *bb : curBB.GetSuccs()) {
            if (bb->GetLabIdx() == static_cast<LabelIdx>(label)) {
              return bb;
            }
          }
        }
#endif
      }
      /* can also be a MOP_xbr. */
      return nullptr;
    }
    case BB::kBBFallthru: {
      return (branchOnly ? nullptr : curBB.GetNext());
    }
    case BB::kBBThrow:
      return nullptr;
    default:
      return nullptr;
  }
  return nullptr;
}

bool CGCFG::InLSDA(LabelIdx label, const EHFunc *ehFunc) {
  /* the function have no exception handle module */
  if (ehFunc == nullptr) {
    return false;
  }

  if ((label == 0) || ehFunc->GetLSDACallSiteTable() == nullptr) {
    return false;
  }
  if (label == ehFunc->GetLSDACallSiteTable()->GetCSTable().GetEndOffset()->GetLabelIdx() ||
      label == ehFunc->GetLSDACallSiteTable()->GetCSTable().GetStartOffset()->GetLabelIdx()) {
    return true;
  }
  return ehFunc->GetLSDACallSiteTable()->InCallSiteTable(label);
}

bool CGCFG::InSwitchTable(LabelIdx label, const CGFunc &func) {
  if (label == 0) {
    return false;
  }
  return func.InSwitchTable(label);
}

bool CGCFG::IsCompareAndBranchInsn(const Insn &insn) const {
  return insnVisitor->IsCompareAndBranchInsn(insn);
}

bool CGCFG::IsAddOrSubInsn(const Insn &insn) const {
  return insnVisitor->IsAddOrSubInsn(insn);
}

Insn *CGCFG::FindLastCondBrInsn(BB &bb) const {
  if (bb.GetKind() != BB::kBBIf) {
    return nullptr;
  }
  FOR_BB_INSNS_REV(insn, (&bb)) {
    if (insn->IsBranch()) {
      return insn;
    }
  }
  return nullptr;
}

void CGCFG::MarkLabelTakenBB() const {
  if (cgFunc->GetMirModule().GetSrcLang() != kSrcLangC) {
    return;
  }
  for (BB *bb = cgFunc->GetFirstBB(); bb != nullptr; bb = bb->GetNext()) {
    if (cgFunc->GetFunction().GetLabelTab()->GetAddrTakenLabels().find(bb->GetLabIdx()) !=
        cgFunc->GetFunction().GetLabelTab()->GetAddrTakenLabels().end()) {
      cgFunc->SetHasTakenLabel();
      bb->SetLabelTaken();
    }
  }
}

/*
 * analyse the CFG to find the BBs that are not reachable from function entries
 * and delete them
 */
void CGCFG::UnreachCodeAnalysis() const {
  if (cgFunc->GetMirModule().GetSrcLang() == kSrcLangC &&
      (cgFunc->HasTakenLabel() ||
      (cgFunc->GetEHFunc() && cgFunc->GetEHFunc()->GetLSDAHeader()))) {
    return;
  }
  /*
   * Find all reachable BBs by dfs in cgfunc and mark their field<unreachable> false,
   * then all other bbs should be unreachable.
   */
  BB *firstBB = cgFunc->GetFirstBB();
  std::forward_list<BB*> toBeAnalyzedBBs;
  toBeAnalyzedBBs.push_front(firstBB);
  std::set<BB*, BBIdCmp> unreachBBs;

  BB *bb = firstBB;
  /* set all bb's unreacable to true */
  while (bb != nullptr) {
    /* Check if bb is the firstBB/cleanupBB/returnBB/lastBB of the function */
    if (bb->IsCleanup() || InSwitchTable(bb->GetLabIdx(), *cgFunc) ||
        bb == cgFunc->GetFirstBB() || bb == cgFunc->GetLastBB() ||
        (bb->GetKind() == BB::kBBReturn && !cgFunc->GetMirModule().IsCModule())) {
      toBeAnalyzedBBs.push_front(bb);
    } else {
      (void)unreachBBs.insert(bb);
    }
    if (bb->IsLabelTaken() == false) {
      bb->SetUnreachable(true);
    }
    bb = bb->GetNext();
  }

  /* do a dfs to see which bbs are reachable */
  while (!toBeAnalyzedBBs.empty()) {
    bb = toBeAnalyzedBBs.front();
    toBeAnalyzedBBs.pop_front();
    if (!bb->IsUnreachable()) {
      continue;
    }
    bb->SetUnreachable(false);
    for (BB *succBB : bb->GetSuccs()) {
      toBeAnalyzedBBs.push_front(succBB);
      unreachBBs.erase(succBB);
    }
    for (BB *succBB : bb->GetEhSuccs()) {
      toBeAnalyzedBBs.push_front(succBB);
      unreachBBs.erase(succBB);
    }
  }

  /* remove unreachable bb */
  std::set<BB*, BBIdCmp>::iterator it;
  for (it = unreachBBs.begin(); it != unreachBBs.end(); it++) {
    BB *unreachBB = *it;
    ASSERT(unreachBB != nullptr, "unreachBB must not be nullptr");
    if (cgFunc->IsExitBB(*unreachBB) && !cgFunc->GetMirModule().IsCModule()) {
      unreachBB->SetUnreachable(false);
    }
    EHFunc *ehFunc = cgFunc->GetEHFunc();
    /* if unreachBB InLSDA ,replace unreachBB's label with nextReachableBB before remove it. */
    if (ehFunc != nullptr && ehFunc->NeedFullLSDA() &&
        cgFunc->GetTheCFG()->InLSDA(unreachBB->GetLabIdx(), ehFunc)) {
      /* find next reachable BB */
      BB* nextReachableBB = nullptr;
      for (BB* curBB = unreachBB; curBB != nullptr; curBB = curBB->GetNext()) {
        if (!curBB->IsUnreachable()) {
          nextReachableBB = curBB;
          break;
        }
      }
      CHECK_FATAL(nextReachableBB != nullptr, "nextReachableBB not be nullptr");
      if (nextReachableBB->GetLabIdx() == 0) {
        LabelIdx labelIdx = cgFunc->CreateLabel();
        nextReachableBB->AddLabel(labelIdx);
        cgFunc->SetLab2BBMap(labelIdx, *nextReachableBB);
      }

      ehFunc->GetLSDACallSiteTable()->UpdateCallSite(*unreachBB, *nextReachableBB);
    }

    unreachBB->GetPrev()->SetNext(unreachBB->GetNext());
    cgFunc->GetCommonExitBB()->RemovePreds(*unreachBB);
    unreachBB->GetNext()->SetPrev(unreachBB->GetPrev());

    for (BB *sucBB : unreachBB->GetSuccs()) {
      sucBB->RemovePreds(*unreachBB);
    }
    for (BB *ehSucBB : unreachBB->GetEhSuccs()) {
      ehSucBB->RemoveEhPreds(*unreachBB);
    }

    unreachBB->ClearSuccs();
    unreachBB->ClearEhSuccs();

    cgFunc->ClearBBInVec(unreachBB->GetId());

    /* Clear insns in GOT Map. */
    cgFunc->ClearUnreachableGotInfos(*unreachBB);
    cgFunc->ClearUnreachableConstInfos(*unreachBB);
  }
}

void CGCFG::FindWillExitBBs(BB *bb, std::set<BB*, BBIdCmp> *visitedBBs) {
  if (visitedBBs->count(bb) != 0) {
    return;
  }
  visitedBBs->insert(bb);
  for (BB *predbb : bb->GetPreds()) {
    FindWillExitBBs(predbb, visitedBBs);
  }
}

/*
 * analyse the CFG to find the BBs that will not reach any function exit; these
 * are BBs inside infinite loops; mark their wontExit flag and create
 * artificial edges from them to commonExitBB
 */
void CGCFG::WontExitAnalysis() {
  std::set<BB*, BBIdCmp> visitedBBs;
  FindWillExitBBs(cgFunc->GetCommonExitBB(), &visitedBBs);
  BB *bb = cgFunc->GetFirstBB();
  while (bb != nullptr) {
    if (visitedBBs.count(bb) == 0) {
      bb->SetWontExit(true);
      if (bb->GetKind() == BB::kBBGoto || bb->GetKind() == BB::kBBThrow) {
        // make this bb a predecessor of commonExitBB
        cgFunc->GetCommonExitBB()->PushBackPreds(*bb);
      }
    }
    bb = bb->GetNext();
  }
}

BB *CGCFG::FindLastRetBB() {
  FOR_ALL_BB_REV(bb, cgFunc) {
    if (bb->GetKind() == BB::kBBReturn) {
      return bb;
    }
  }
  return nullptr;
}

void CGCFG::UpdatePredsSuccsAfterSplit(BB &pred, BB &succ, BB &newBB) const {
  /* connext newBB -> succ */
  for (auto it = succ.GetPredsBegin(); it != succ.GetPredsEnd(); ++it) {
    if (*it == &pred) {
      auto origIt = it;
      succ.ErasePreds(it);
      if (origIt != succ.GetPredsBegin()) {
        --origIt;
        succ.InsertPred(origIt, newBB);
      } else {
        succ.PushFrontPreds(newBB);
      }
      break;
    }
  }
  newBB.PushBackSuccs(succ);

  /* connext pred -> newBB */
  for (auto it = pred.GetSuccsBegin(); it != pred.GetSuccsEnd(); ++it) {
    if (*it == &succ) {
      auto origIt = it;
      pred.EraseSuccs(it);
      if (origIt != succ.GetSuccsBegin()) {
        --origIt;
        pred.InsertSucc(origIt, newBB);
      } else {
        pred.PushFrontSuccs(newBB);
      }
      break;
    }
  }
  newBB.PushBackPreds(pred);

  /* maintain eh info */
  for (auto it = pred.GetEhSuccs().begin(); it != pred.GetEhSuccs().end(); ++it) {
    newBB.PushBackEhSuccs(**it);
  }
  for (auto it = pred.GetEhPredsBegin(); it != pred.GetEhPredsEnd(); ++it) {
    newBB.PushBackEhPreds(**it);
  }

  /* update phi */
  for (const auto phiInsnIt : succ.GetPhiInsns()) {
    auto &phiList = static_cast<PhiOperand&>(phiInsnIt.second->GetOperand(kInsnSecondOpnd));
    for (const auto phiOpndIt : phiList.GetOperands()) {
      uint32 fBBId = phiOpndIt.first;
      ASSERT(fBBId != 0, "GetFromBBID = 0");
      BB *predBB = cgFunc->GetBBFromID(fBBId);
      if (predBB == &pred) {
        phiList.UpdateOpnd(fBBId, newBB.GetId(), *phiOpndIt.second);
        break;
      }
    }
  }
  if (cgFunc->GetFunction().GetFuncProfData() != nullptr) {
    newBB.InitEdgeProfFreq();
    FreqType newBBFreq = pred.GetEdgeProfFreq(succ);
    newBB.SetProfFreq(newBBFreq);
    pred.SetEdgeProfFreq(&newBB, newBBFreq);
    newBB.SetEdgeProfFreq(&succ, newBBFreq);
  }
}

#if TARGAARCH64
BB *CGCFG::BreakCriticalEdge(BB &pred, BB &succ) const {
  LabelIdx newLblIdx = cgFunc->CreateLabel();
  BB *newBB = cgFunc->CreateNewBB(newLblIdx, false, BB::kBBGoto, pred.GetFrequency());
  newBB->SetCritical(true);
  bool isFallThru = pred.GetNext() == &succ;
  /* set prev, next */
  if (isFallThru) {
    BB *origNext = pred.GetNext();
    origNext->SetPrev(newBB);
    newBB->SetNext(origNext);
    pred.SetNext(newBB);
    newBB->SetPrev(&pred);
    newBB->SetKind(BB::kBBFallthru);
  } else {
    BB *exitBB = cgFunc->GetExitBBsVec().size() == 0 ? nullptr : cgFunc->GetExitBB(0);
    if (exitBB == nullptr || exitBB->IsUnreachable()) {
      cgFunc->GetLastBB()->AppendBB(*newBB);
      cgFunc->SetLastBB(*newBB);
    } else {
      exitBB->AppendBB(*newBB);
      if (cgFunc->GetLastBB() == exitBB) {
        cgFunc->SetLastBB(*newBB);
      }
    }
    newBB->AppendInsn(
        cgFunc->GetInsnBuilder()->BuildInsn(MOP_xuncond, cgFunc->GetOrCreateLabelOperand(succ.GetLabIdx())));
  }

  /* update offset if succ is goto target */
  if (pred.GetKind() == BB::kBBIf) {
    Insn *brInsn = FindLastCondBrInsn(pred);
    ASSERT(brInsn != nullptr, "null ptr check");
    LabelOperand &brTarget = static_cast<LabelOperand&>(brInsn->GetOperand(AArch64isa::GetJumpTargetIdx(*brInsn)));
    if (brTarget.GetLabelIndex() == succ.GetLabIdx()) {
      brInsn->SetOperand(AArch64isa::GetJumpTargetIdx(*brInsn), cgFunc->GetOrCreateLabelOperand(newLblIdx));
    }
  } else if (pred.GetKind() == BB::kBBRangeGoto) {
    const MapleVector<LabelIdx> &labelVec = pred.GetRangeGotoLabelVec();
    for (size_t i = 0; i < labelVec.size(); ++i) {
      if (labelVec[i] == succ.GetLabIdx()) {
        /* single edge for multi jump target, so have to replace all. */
        pred.SetRangeGotoLabel(i, newLblIdx);
      }
    }
    cgFunc->UpdateEmitSt(pred, succ.GetLabIdx(), newLblIdx);
  } else {
    ASSERT(0, "unexpeced bb kind in BreakCriticalEdge");
  }

  /* update pred, succ */
  UpdatePredsSuccsAfterSplit(pred, succ, *newBB);
  return newBB;
}

void CGCFG::ReverseCriticalEdge(BB &cbb) {
  CHECK_FATAL(cbb.GetPreds().size() == 1, "critical edge bb has more than 1 preds");
  CHECK_FATAL(cbb.GetSuccs().size() == 1, "critical edge bb has more than 1 succs");
  BB *pred = *cbb.GetPreds().begin();
  BB *succ = *cbb.GetSuccs().begin();

  if (pred->GetKind() == BB::kBBIf) {
    Insn *brInsn = FindLastCondBrInsn(*pred);
    CHECK_FATAL(brInsn != nullptr, "null ptr check");
    LabelOperand &brTarget = static_cast<LabelOperand&>(brInsn->GetOperand(AArch64isa::GetJumpTargetIdx(*brInsn)));
    if (brTarget.GetLabelIndex() == cbb.GetLabIdx()) {
      CHECK_FATAL(succ->GetLabIdx() != MIRLabelTable::GetDummyLabel(), "unexpect label");
      brInsn->SetOperand(AArch64isa::GetJumpTargetIdx(*brInsn), cgFunc->GetOrCreateLabelOperand(succ->GetLabIdx()));
    } else {
      if (pred->GetNext() != &cbb) {
        CHECK_FATAL(false, "pred of critical edge bb do not goto cbb");
      }
    }
  } else if (pred->GetKind() == BB::kBBRangeGoto) {
    const MapleVector<LabelIdx> &labelVec = pred->GetRangeGotoLabelVec();
    uint32 index = 0;
    CHECK_FATAL(succ->GetLabIdx() != MIRLabelTable::GetDummyLabel(), "unexpect label");
    for (auto label: labelVec) {
      /* single edge for multi jump target, so have to replace all. */
      if (label == cbb.GetLabIdx()) {
        pred->SetRangeGotoLabel(index, succ->GetLabIdx());
      }
      index++;
    }
    MIRSymbol *st = cgFunc->GetEmitSt(pred->GetId());
    MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st->GetKonst());
    MIRType *etype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64));
    MIRConst *mirConst = cgFunc->GetMemoryPool()->New<MIRLblConst>(succ->GetLabIdx(), cgFunc->GetFunction().GetPuidx(),
        *etype);
    for (size_t i = 0; i < arrayConst->GetConstVec().size(); ++i) {
      CHECK_FATAL(arrayConst->GetConstVecItem(i)->GetKind() == kConstLblConst, "not a kConstLblConst");
      MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
      if (cbb.GetLabIdx() == lblConst->GetValue()) {
        arrayConst->SetConstVecItem(i, *mirConst);
      }
    }
  } else {
    ASSERT(0, "unexpeced bb kind in BreakCriticalEdge");
  }

  pred->RemoveSuccs(cbb);
  pred->PushBackSuccs(*succ);
  succ->RemovePreds(cbb);
  succ->PushBackPreds(*pred);
}
#endif

bool CgHandleCFG::PhaseRun(maplebe::CGFunc &f) {
  CGCFG *cfg = f.GetMemoryPool()->New<CGCFG>(f);
  f.SetTheCFG(cfg);
  cfg->MarkLabelTakenBB();
  /* build control flow graph */
  f.GetTheCFG()->BuildCFG();
  f.HandleFuncCfg(cfg);
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgHandleCFG, handlecfg)

}  /* namespace maplebe */
