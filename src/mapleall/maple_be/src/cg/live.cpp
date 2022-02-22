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
#include "live.h"
#include <set>
#include "cg.h"
#include "cg_option.h"
#include "cgfunc.h"

/*
 * This phase build two sets: liveOutRegno and liveInRegno of each BB.
 * This algorithm mainly include 3 parts:
 * 1. initialize and get def[]/use[] of each BB;
 * 2. build live_in and live_out based on this algorithm
 *   Out[B] = U In[S] //S means B's successor;
 *   In[B] = use[B] U (Out[B]-def[B]);
 * 3. deal with cleanup BB.
 */
namespace maplebe {
#define LIVE_ANALYZE_DUMP_NEWPM CG_DEBUG_FUNC(f)

void LiveAnalysis::InitAndGetDefUse() {
  FOR_ALL_BB(bb, cgFunc) {
    if (!bb->GetEhPreds().empty()) {
      InitEhDefine(*bb);
    }
    InitBB(*bb);
    GetBBDefUse(*bb);
    if (bb->GetEhPreds().empty()) {
      continue;
    }
    bb->RemoveInsn(*bb->GetFirstInsn()->GetNext());
    cgFunc->DecTotalNumberOfInstructions();
    bb->RemoveInsn(*bb->GetFirstInsn());
    cgFunc->DecTotalNumberOfInstructions();
  }
}

/* Out[BB] = Union all of In[Succs(BB)] */
bool LiveAnalysis::GenerateLiveOut(BB &bb) {
  const MapleVector<uint64>bbLiveOutBak(bb.GetLiveOut()->GetInfo());
  for (auto succBB : bb.GetSuccs()) {
    if (succBB->GetLiveInChange() && !succBB->GetLiveIn()->NoneBit()) {
      bb.LiveOutOrBits(*succBB->GetLiveIn());
    }
    if (!succBB->GetEhSuccs().empty()) {
      for (auto ehSuccBB : succBB->GetEhSuccs()) {
        bb.LiveOutOrBits(*ehSuccBB->GetLiveIn());
      }
    }
  }
  for (auto ehSuccBB : bb.GetEhSuccs()) {
    if (ehSuccBB->GetLiveInChange() && !ehSuccBB->GetLiveIn()->NoneBit()) {
      bb.LiveOutOrBits(*ehSuccBB->GetLiveIn());
    }
  }
  return !bb.GetLiveOut()->IsEqual(bbLiveOutBak);
}

/* In[BB] = use[BB] Union (Out[BB]-def[BB]) */
bool LiveAnalysis::GenerateLiveIn(BB &bb) {
  LocalMapleAllocator allocator(stackMp);
  const MapleVector<uint64> bbLiveInBak(bb.GetLiveIn()->GetInfo(), allocator.Adapter());
  if (!bb.GetInsertUse()) {
    bb.SetLiveInInfo(*bb.GetUse());
    bb.SetInsertUse(true);
  }
  DataInfo &bbLiveOut = bb.GetLiveOut()->Clone(allocator);
  if (!bbLiveOut.NoneBit()) {
    bbLiveOut.Difference(*bb.GetDef());
    bb.LiveInOrBits(bbLiveOut);
  }

  if (!bb.GetEhSuccs().empty()) {
    /* If bb has eh successors, check if multi-gen exists. */
    DataInfo allInOfEhSuccs(cgFunc->GetMaxVReg(), allocator);
    for (auto ehSucc : bb.GetEhSuccs()) {
      allInOfEhSuccs.OrBits(*ehSucc->GetLiveIn());
    }
    allInOfEhSuccs.AndBits(*bb.GetDef());
    bb.LiveInOrBits(allInOfEhSuccs);
  }

  if (!bb.GetLiveIn()->IsEqual(bbLiveInBak)) {
    return true;
  }
  return false;
}

/* building liveIn and liveOut of each BB. */
void LiveAnalysis::BuildInOutforFunc() {
  iteration = 0;
  bool hasChange;
  do {
    ++iteration;
    hasChange = false;
    FOR_ALL_BB_REV(bb, cgFunc) {
      if (!GenerateLiveOut(*bb) && bb->GetInsertUse()) {
        continue;
      }
      if (GenerateLiveIn(*bb)) {
        bb->SetLiveInChange(true);
        hasChange = true;
      } else {
        bb->SetLiveInChange(false);
      }
    }
  } while (hasChange);
}

/*  reset to liveout/in_regno */
void LiveAnalysis::ResetLiveSet() {
  FOR_ALL_BB(bb, cgFunc) {
    bb->GetLiveIn()->GetBitsOfInfo<MapleSet<uint32>>(bb->GetLiveInRegNO());
    bb->GetLiveOut()->GetBitsOfInfo<MapleSet<uint32>>(bb->GetLiveOutRegNO());
  }
}

/* entry function for LiveAnalysis */
void LiveAnalysis::AnalysisLive() {
  InitAndGetDefUse();
  BuildInOutforFunc();
  InsertInOutOfCleanupBB();
}

void LiveAnalysis::DealWithInOutOfCleanupBB() {
  const BB *cleanupBB = cgFunc->GetCleanupEntryBB();
  if (cleanupBB == nullptr) {
    return;
  }
  for (size_t i = 0; i != cleanupBB->GetLiveIn()->Size(); ++i) {
    if (!cleanupBB->GetLiveIn()->TestBit(i)) {
      continue;
    }
    if (CleanupBBIgnoreReg(regno_t(i))) {
      continue;
    }
    /*
     * a param vreg may used in cleanup bb. So this param vreg will live on the whole function
     * since everywhere in function body may occur exceptions.
     */
    FOR_ALL_BB(bb, cgFunc) {
      if (bb->IsCleanup()) {
        continue;
      }
      /* If bb is not a cleanup bb, then insert reg to both livein and liveout. */
      if ((bb != cgFunc->GetFirstBB()) && !bb->GetDef()->TestBit(i)) {
        bb->SetLiveInBit(i);
      }
      bb->SetLiveOutBit(i);
    }
  }
}

void LiveAnalysis::InsertInOutOfCleanupBB() {
  const BB *cleanupBB = cgFunc->GetCleanupEntryBB();
  if (cleanupBB == nullptr) {
    return;
  }
  if (cleanupBB->GetLiveIn() == nullptr || cleanupBB->GetLiveIn()->NoneBit()) {
    return;
  }
  DataInfo cleanupBBLi = *(cleanupBB->GetLiveIn());
  /* registers need to be ignored: (reg < 8) || (29 <= reg && reg <= 32) */
  for (uint32 i = 1; i < 8; ++i) {
    cleanupBBLi.ResetBit(i);
  }
  for (uint32 j = 29; j <= 32; ++j) {
    cleanupBBLi.ResetBit(j);
  }

  FOR_ALL_BB(bb, cgFunc) {
    if (bb->IsCleanup()) {
      continue;
    }
    if (bb != cgFunc->GetFirstBB()) {
      cleanupBBLi.Difference(*bb->GetDef());
      bb->LiveInOrBits(cleanupBBLi);
    }
    bb->LiveOutOrBits(cleanupBBLi);
  }
}

/* dump the current info of def/use/livein/liveout */
void LiveAnalysis::Dump() const {
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc->GetFunction().GetStIdx().Idx());
  LogInfo::MapleLogger() << "\n---------  liveness for " << funcSt->GetName() << "  iteration ";
  LogInfo::MapleLogger() << iteration << " ---------\n";
  FOR_ALL_BB(bb, cgFunc) {
    LogInfo::MapleLogger() << "  === BB_" << bb->GetId() << " (" << std::hex << bb << ") "
                           << std::dec << " <" << bb->GetKindName();
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      LogInfo::MapleLogger() << "[labeled with " << bb->GetLabIdx() << "]";
    }
    LogInfo::MapleLogger() << "> idx " << bb->GetId() << " ===\n";

    if (!bb->GetPreds().empty()) {
      LogInfo::MapleLogger() << "    pred [ ";
      for (auto *pred : bb->GetPreds()) {
        LogInfo::MapleLogger() << pred->GetId() << " (" << std::hex << pred << ") " << std::dec << " ";
      }
      LogInfo::MapleLogger() << "]\n";
    }
    if (!bb->GetSuccs().empty()) {
      LogInfo::MapleLogger() << "    succ [ ";
      for (auto *succ : bb->GetSuccs()) {
        LogInfo::MapleLogger() << succ->GetId() << " (" << std::hex << succ << ") " << std::dec << " ";
      }
      LogInfo::MapleLogger() << "]\n";
    }

    const DataInfo *infoDef = nullptr;
    LogInfo::MapleLogger() << "    DEF: ";
    infoDef = bb->GetDef();
    DumpInfo(*infoDef);

    const DataInfo *infoUse = nullptr;
    LogInfo::MapleLogger() << "\n    USE: ";
    infoUse = bb->GetUse();
    DumpInfo(*infoUse);

    const DataInfo *infoLiveIn = nullptr;
    LogInfo::MapleLogger() << "\n    Live IN: ";
    infoLiveIn = bb->GetLiveIn();
    DumpInfo(*infoLiveIn);

    const DataInfo *infoLiveOut = nullptr;
    LogInfo::MapleLogger() << "\n    Live OUT: ";
    infoLiveOut = bb->GetLiveOut();
    DumpInfo(*infoLiveOut);
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "---------------------------\n";
}

void LiveAnalysis::DumpInfo(const DataInfo &info) const {
  uint32 count = 1;
  for (size_t i = 0; i != info.Size(); ++i) {
    if (info.TestBit(i)) {
      ++count;
      LogInfo::MapleLogger() << i << " ";
      /* 20 output one line */
      if ((count % 20) == 0) {
        LogInfo::MapleLogger() << "\n";
      }
    }
  }

  LogInfo::MapleLogger() << '\n';
}

/* initialize dependent info and container of BB. */
void LiveAnalysis::InitBB(BB &bb) {
  bb.SetLiveInChange(true);
  bb.SetInsertUse(false);
  bb.ClearLiveInRegNO();
  bb.ClearLiveOutRegNO();
  const uint32 maxRegCount = cgFunc->GetMaxVReg();
  bb.SetLiveIn(*NewLiveIn(maxRegCount));
  bb.SetLiveOut(*NewLiveOut(maxRegCount));
  bb.SetDef(*NewDef(maxRegCount));
  bb.SetUse(*NewUse(maxRegCount));
}

void LiveAnalysis::ClearInOutDataInfo() {
  FOR_ALL_BB(bb, cgFunc) {
    bb->SetLiveInChange(false);
    bb->DefClearDataInfo();
    bb->UseClearDataInfo();
    bb->LiveInClearDataInfo();
    bb->LiveOutClearDataInfo();
  }
}

void LiveAnalysis::EnlargeSpaceForLiveAnalysis(BB &currBB) {
  regno_t currMaxVRegNO = cgFunc->GetMaxVReg();
  if (currMaxVRegNO >= currBB.GetLiveIn()->Size()) {
    FOR_ALL_BB(bb, cgFunc) {
      bb->LiveInEnlargeCapacity(currMaxVRegNO);
      bb->LiveOutEnlargeCapacity(currMaxVRegNO);
    }
  }
}

bool CgLiveAnalysis::PhaseRun(maplebe::CGFunc &f) {
  MemPool *liveMemPool = GetPhaseMemPool();
  live = f.GetCG()->CreateLiveAnalysis(*liveMemPool, f);
  live->AnalysisLive();
  if (LIVE_ANALYZE_DUMP_NEWPM) {
    live->Dump();
  }
  return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgLiveAnalysis, liveanalysis)
}  /* namespace maplebe */
