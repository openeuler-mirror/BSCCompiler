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
#include "me_profile_gen.h"
#include <iostream>
#include <algorithm>
#include "me_cfg.h"
#include "me_option.h"
#include "me_function.h"
#include "me_irmap_build.h"
#include "mir_builder.h"
#include "gen_profile.h"
#include "itab_util.h"

/*
 * This phase do CFG edge profile using a minimum spanning tree based
 * on the following paper:
 * [1] Donald E. Knuth, Francis R. Stevenson. Optimal measurement of points
 * FOR program frequency counts. BIT Numerical Mathematics 1973, Volume 13,
 * Issue 3, pp 313-322
 * The count of edge on spanning tree can be derived from
 * those edges not on the spanning tree. Knuth proves this method instruments
 * the minimum number of edges.
 */

namespace maple {
uint64 MeProfGen::counterIdx = 0;
uint64 MeProfGen::totalBB = 0;
uint64 MeProfGen::instrumentBB = 0;
uint64 MeProfGen::totalFunc = 0;
uint64 MeProfGen::instrumentFunc = 0;
bool MeProfGen::firstRun = true;
MIRSymbol *MeProfGen::bbCounterTabSym = nullptr;

void MeProfGen::DumpSummary() {
  LogInfo::MapleLogger() << "instrument BB " << instrumentBB << " total BB " << totalBB << std::setprecision(2)
                         << " ratio "  << (static_cast<float>(instrumentBB) / totalBB) << "\n";
  LogInfo::MapleLogger() << "instrument func " << instrumentFunc << " total BB " << totalFunc << std::setprecision(2)
                         << " ratio " <<  (static_cast<float>(instrumentFunc) / totalFunc) << "\n";
}

void MeProfGen::IncTotalFunc() {
  totalFunc++;
}

void MeProfGen::Init() {
  if (Options::profileGen) {
    return;
  }
  if (!firstRun) {
    return;
  }
  MIRArrayType &muidIdxArrayType =
      *GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetUInt32(), 0);
  std::string bbProfileName = namemangler::kBBProfileTabPrefixStr + func->GetMIRModule().GetFileNameAsPostfix();
  bbCounterTabSym = func->GetMIRModule().GetMIRBuilder()->CreateGlobalDecl(bbProfileName.c_str(), muidIdxArrayType);
  MIRAggConst *bbProfileTab = func->GetMIRModule().GetMemPool()->New<MIRAggConst>(
      func->GetMIRModule(), *GlobalTables::GetTypeTable().GetUInt32());
  bbCounterTabSym->SetKonst(bbProfileTab);
  bbCounterTabSym->SetStorageClass(kScFstatic);
  firstRun = false;
}

void MeProfGen::InstrumentBB(BB &bb) {
  /* append intrinsic call at the begin of bb */
  CHECK_FATAL(counterIdx != (UINT32_MAX / GetPrimTypeSize(PTY_u32)), "profile counter overflow");
  MeExpr *arg0 = hMap->CreateIntConstMeExpr(static_cast<int64>(counterIdx), PTY_u32);
  std::vector<MeExpr*> opnds = { arg0 };
  IntrinsiccallMeStmt *counterIncCall = hMap->CreateIntrinsicCallMeStmt(INTRN_MPL_PROF_COUNTER_INC, opnds);
  bb.AddMeStmtFirst(counterIncCall);
  bb.SetAttributes(kBBAttrIsInstrument);
  counterIdx++;
  if (dump) {
    LogInfo::MapleLogger() << "instrument on BB" << bb.GetBBId() << "\n";
  }
}

void MeProfGen::SaveProfile() const {
  if (!Options::profileTest) {
    return;
  }
  if (func->GetName().find("main") != std::string::npos) {
    std::vector<MeExpr*> opnds;
    IntrinsiccallMeStmt *saveProfCall = hMap->CreateIntrinsicCallMeStmt(INTRN_MCCSaveProf, opnds);
    for (BB *exitBB : func->GetCfg()->GetCommonExitBB()->GetPred()) {
      exitBB->AddMeStmtFirst(saveProfCall);
    }
  }
}

void MeProfGen::InstrumentFunc() {
  FindInstrumentEdges();
  std::vector<BB*> instrumentBBs;
  GetInstrumentBBs(instrumentBBs);

  if (Options::profileGen) {
    counterIdx = 0;
    MIRModule *mod = func->GetMirFunc()->GetModule();
    uint32 nCtrs = static_cast<uint32>(instrumentBBs.size());
    func->GetMirFunc()->SetNumCtrs(nCtrs);
    if (nCtrs != 0) {
      MIRType *arrOfInt64Ty =
          GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetInt64(), nCtrs);
      // flatten the counter table name
      std::string ctrTblName = namemangler::kprefixProfCtrTbl +
                               func->GetMIRModule().GetFileName() + "_" +
                               func->GetMirFunc()->GetName();
      std::replace(ctrTblName.begin(), ctrTblName.end(), '.', '_');
      std::replace(ctrTblName.begin(), ctrTblName.end(), '-', '_');
      std::replace(ctrTblName.begin(), ctrTblName.end(), '/', '_');

      MIRSymbol *ctrTblSym = mod->GetMIRBuilder()->CreateGlobalDecl(ctrTblName, *arrOfInt64Ty, kScFstatic);
      ctrTblSym->SetSKind(kStVar);
      func->GetMirFunc()->SetProfCtrTbl(ctrTblSym);
    }
    for (auto *bb : instrumentBBs) {
      InstrumentBB(*bb);
    }

    // Checksums are relatively sensitive to source changes
    // Generate the function's lineno checksum
    std::string fileName = func->GetMIRModule().GetFileName();
    uint64 fileNameHash = DJBHash(fileName.c_str());
    std::string lineNo = std::to_string(func->GetMirFunc()->GetSrcPosition().LineNum());
    uint64 linenoHash = (fileNameHash << 32) | DJBHash(lineNo.c_str());
    func->GetMirFunc()->SetFileLineNoChksum(linenoHash);

    // Generate The function's CFG checksum
    uint64 cfgCheckSum = ComputeFuncHash();
    func->GetMirFunc()->SetCFGChksum(cfgCheckSum);

    if (dump) {
      LogInfo::MapleLogger() << "******************after profileGen dump function******************\n";
      func->Dump(true);
    }
    return;
  }

  // record the current counter start, used in the function profile description
  uint32 counterStart = static_cast<uint32>(counterIdx);
  MIRAggConst *bbProfileTab = safe_cast<MIRAggConst>(bbCounterTabSym->GetKonst());
  for (auto *bb : instrumentBBs) {
    MIRIntConst *indexConst =
        func->GetMIRModule().GetMemPool()->New<MIRIntConst>(0, *GlobalTables::GetTypeTable().GetUInt32());
    bbProfileTab->AddItem(indexConst, 0);
    InstrumentBB(*bb);
  }

  uint32 counterEnd = static_cast<uint32>(counterIdx - 1);
  uint64 hash = ComputeFuncHash();
  func->AddProfileDesc(hash, counterStart, counterEnd);
  instrumentBB += instrumentBBs.size();
  totalBB += GetAllBBs();
  instrumentFunc++;
  SaveProfile();
  if (dump) {
    LogInfo::MapleLogger() << "******************after profile gen  dump function******************\n";
    func->Dump(true);
  }
  if (Options::profileTest || dump) {
    LogInfo::MapleLogger() << func->GetName() << " profile description info: "
                           << "func hash " << std::hex << hash << std::dec << " counter range ["
                           << counterStart << "," << counterEnd << "]\n";
  }
}

// if function have try can't instrument
// if function have infinite loop, can't instrument,because it may cause counter
// overflow
bool MeProfGen::CanInstrument() const {
  MeCFG *cfg = func->GetCfg();
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    if (bb->GetAttributes(kBBAttrIsTry) || bb->GetAttributes(kBBAttrWontExit)) {
      if (func->GetMIRModule().IsCModule()) {
        // Consider the case of non-exit as a special program
        continue;
      } else {
        return false;
      }
    }
  }
  return true;
}

bool MEProfGen::PhaseRun(maple::MeFunction &f) {
  if (!f.GetCfg()->empty()) {
    MeProfGen::IncTotalFunc();
  }
  // function with try can't determine the instrument BB,because
  // there have critial-edge which can't be split
  if (f.HasException()) {
    return false;
  }

  auto *hMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(hMap != nullptr, "hssamap is nullptr");
  MeProfGen profGen(f, *GetPhaseMemPool(), *hMap, DEBUGFUNC_NEWPM(f));
  if (!profGen.CanInstrument()) {
    return false;
  }
  profGen.InstrumentFunc();

  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "dump edge info in profile gen phase " << f.GetMirFunc()->GetName() << std::endl;
    f.GetCfg()->DumpToFile("afterProfileGen", false);
    profGen.DumpEdgeInfo();
  }
  return true;
}

void MEProfGen::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}
}  // namespace maple
