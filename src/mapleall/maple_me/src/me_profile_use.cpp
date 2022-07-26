/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_profile_use.h"
#include <iostream>
#include "me_cfg.h"
#include "me_option.h"
#include "me_function.h"

namespace maple {
BBUseInfo *MeProfUse::GetOrCreateBBUseInfo(const BB &bb) {
  auto item = bbProfileInfo.find(&bb);
  if (item != bbProfileInfo.end()) {
    return item->second;
  } else {
    BBUseInfo *useInfo = mp->New<BBUseInfo>(*mp);
    bbProfileInfo.insert(std::make_pair(&bb, useInfo));
    return useInfo;
  }
}

BBUseInfo *MeProfUse::GetBBUseInfo(const BB &bb) const {
  auto item = bbProfileInfo.find(&bb);
  ASSERT(item->second != nullptr, "bb info not created");
  return item->second;
}

uint64 MeProfUse::SumEdgesCount(const MapleVector<BBUseEdge*> &edges) const {
  uint64 count = 0;
  for (const auto &e : edges) {
    count += e->GetCount();
  }
  return count;
}

/* create BB use info */
void MeProfUse::InitBBEdgeInfo() {
  for (auto &e : GetAllEdges()) {
    BB *src = e->GetSrcBB();
    BB *dest = e->GetDestBB();
    auto srcUseInfo = GetOrCreateBBUseInfo(*src);
    srcUseInfo->AddOutEdge(e);
    auto destUseInfo = GetOrCreateBBUseInfo(*dest);
    destUseInfo->AddInEdge(e);
  }
  for (auto &e : GetAllEdges()) {
    if (e->IsInMST()) {
      continue;
    }
    BB *src = e->GetSrcBB();
    auto srcUseInfo = GetBBUseInfo(*src);
    if (srcUseInfo->GetStatus() && srcUseInfo->GetOutEdgeSize() == 1) {
      SetEdgeCount(*e, srcUseInfo->GetCount());
    } else {
      BB *dest = e->GetDestBB();
      auto destUseInfo = GetBBUseInfo(*dest);
      if (destUseInfo->GetStatus() && destUseInfo->GetInEdgeSize() == 1) {
        SetEdgeCount(*e, destUseInfo->GetCount());
      }
    }
    if (e->GetStatus()) {
      continue;
    }
    SetEdgeCount(*e, 0);
  }
}

// If all input edges or output edges determined, caculate BB freq
void MeProfUse::ComputeBBFreq(BBUseInfo &bbInfo, bool &changed) {
  uint64 count = 0;
  if (!bbInfo.GetStatus()) {
    if (bbInfo.GetUnknownOutEdges() == 0) {
      count = SumEdgesCount(bbInfo.GetOutEdges());
      bbInfo.SetCount(count);
      changed = true;
    } else if (bbInfo.GetUnknownInEdges() == 0) {
      count = SumEdgesCount(bbInfo.GetInEdges());
      bbInfo.SetCount(count);
      changed = true;
    }
  }
}

/* compute all edge freq in the cfg without consider exception */
void MeProfUse::ComputeEdgeFreq() {
  bool change = true;
  size_t pass = 0;
  MeCFG *cfg = func->GetCfg();
  auto eIt = cfg->valid_end();
  while (change) {
    change = false;
    pass++;
    CHECK_FATAL(pass != UINT8_MAX, "parse all edges fail");
    /*
     * use the bb edge to infer the bb's count,when all bb's count is valid
     * then all edges count is valid
     */
    for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
      auto *bb = *bIt;
      BBUseInfo *useInfo = GetBBUseInfo(*bb);
      if (useInfo == nullptr) {
        continue;
      }
      ComputeBBFreq(*useInfo, change);
      if (useInfo->GetStatus()) {
        if (useInfo->GetUnknownOutEdges() == 1) {
          uint64 total = 0;
          uint64 outCount = SumEdgesCount(useInfo->GetOutEdges());
          if (useInfo->GetCount() > outCount) {
            total = useInfo->GetCount() - outCount;
          }
          SetEdgeCount(useInfo->GetOutEdges(), total);
          change = true;
        }
        if (useInfo->GetUnknownInEdges() == 1) {
          uint64 total = 0;
          uint64 inCount = SumEdgesCount(useInfo->GetInEdges());
          if (useInfo->GetCount() > inCount) {
            total = useInfo->GetCount() - inCount;
          }
          SetEdgeCount(useInfo->GetInEdges(), total);
          change = true;
        }
      }
    }
  }
  if (dump) {
    LogInfo::MapleLogger() << "parse all edges in " << pass << " pass" << '\n';
  }
  if (Options::profileTest) {
    LogInfo::MapleLogger() << func->GetName() << " succ compute all edges " << '\n';
  }
}

/*
 * this used to set the edge count for the unknown edge
 * ensure only one unkown edge in the edges
 */
void MeProfUse::SetEdgeCount(MapleVector<BBUseEdge*> &edges, uint64 value) {
  for (const auto &e : edges) {
    if (!e->GetStatus()) {
      e->SetCount(value);
      BBUseInfo *srcInfo = GetBBUseInfo(*(e->GetSrcBB()));
      BBUseInfo *destInfo = GetBBUseInfo(*(e->GetDestBB()));
      srcInfo->DecreaseUnKnownOutEdges();
      destInfo->DecreaseUnKnownInEdges();
      return;
    }
  }
  CHECK(false, "can't find unkown edge");
}

void MeProfUse::SetEdgeCount(BBUseEdge &edge, size_t value) {
  // edge counter already valid skip
  if (edge.GetStatus()) {
    return;
  }
  edge.SetCount(value);
  BBUseInfo *srcInfo = GetBBUseInfo(*(edge.GetSrcBB()));
  BBUseInfo *destInfo = GetBBUseInfo(*(edge.GetDestBB()));
  srcInfo->DecreaseUnKnownOutEdges();
  destInfo->DecreaseUnKnownInEdges();
  return;
}

// return true,if all counter is zero
bool MeProfUse::IsAllZero(Profile::BBInfo &result) const {
  bool allZero = true;
  for (size_t i = 0; i < result.totalCounter; ++i) {
    if (result.counter[i] != 0) {
      allZero = false;
      break;
    }
  }
  return allZero;
}

bool MeProfUse::BuildEdgeCount() {
  Profile::BBInfo result;
  bool ret = true;
  ret = func->GetMIRModule().GetProfile().GetFunctionBBProf(func->GetName(), result);
  if (!ret) {
    if (dump) {
      LogInfo::MapleLogger() << func->GetName() << " isn't in profile" << '\n';
    }
    return false;
  }
  if (IsAllZero(result)) {
    if (dump) {
      LogInfo::MapleLogger() << func->GetName() << " counter all zero" << '\n';
    }
    return false;
  }
  FindInstrumentEdges();
  uint64 hash = ComputeFuncHash();
  if (hash != result.funcHash) {
    if (dump) {
      LogInfo::MapleLogger() << func->GetName() << " hash doesn't match profile hash "
                             << result.funcHash << " func real hash " << hash << '\n';
    }
    return false;
  }
  std::vector<BB*> instrumentBBs;
  GetInstrumentBBs(instrumentBBs);
  if (dump) {
    DumpEdgeInfo();
  }
  if (instrumentBBs.size() != result.totalCounter) {
    if (dump) {
      LogInfo::MapleLogger() << func->GetName() << " counter doesn't match profile counter "
                             << result.totalCounter << " func real counter " <<  instrumentBBs.size() << '\n';
    }
    return false;
  }
  size_t i = 0;
  for (auto *bb : instrumentBBs) {
    auto *bbUseInfo = GetOrCreateBBUseInfo(*bb);
    bbUseInfo->SetCount(result.counter[i]);
    i++;
  }

  InitBBEdgeInfo();
  ComputeEdgeFreq();
  succCalcuAllEdgeFreq = true;
  return true;
}

void MeProfUse::SetFuncEdgeInfo() {
  MeCFG *cfg = func->GetCfg();
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    auto *bbInfo = GetBBUseInfo(*bb);
    bb->SetFrequency(static_cast<uint32>(bbInfo->GetCount()));
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    bb->InitEdgeFreq();
    auto outEdges = bbInfo->GetOutEdges();
    for (auto *e : outEdges) {
      auto *destBB = e->GetDestBB();
      // common_exit's pred's BB doesn't have succ BB of common exit
      // so skip this edge
      if (destBB == cfg->GetCommonExitBB()) {
        continue;
      }
      bb->SetEdgeFreq(destBB, e->GetCount());
    }
  }
  func->SetProfValid(true);
  func->SetFrequency(cfg->GetCommonEntryBB()->GetFrequency());
  if (Options::genPGOReport) {
    func->GetMIRModule().GetProfile().SetFuncStatus(func->GetName(), true);
  }
}

void MeProfUse::DumpFuncCFGEdgeFreq() const {
  LogInfo::MapleLogger() << "populate status " << succCalcuAllEdgeFreq << "\n";
  if (!succCalcuAllEdgeFreq) {
    return;
  }
  LogInfo::MapleLogger() << "func freq " << func->GetFrequency() << "\n";
  MeCFG *cfg = func->GetCfg();
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    auto bb = *bIt;
    LogInfo::MapleLogger() << bb->GetBBId() << " freq " << bb->GetFrequency() << '\n';
    for (const auto *succBB : bb->GetSucc()) {
      LogInfo::MapleLogger() << "edge " << bb->GetBBId() << "->" << succBB->GetBBId() << " freq "
                             << bb->GetEdgeFreq(succBB) << '\n';
    }
  }
}

bool MeProfUse::Run() {
  if (!BuildEdgeCount()) {
    return false;
  }
  SetFuncEdgeInfo();
  return true;
}

GcovFuncInfo *MeProfUse::GetFuncData() {
  GcovProfileData *gcovData = func->GetMIRModule().GetGcovProfile();
  if (!gcovData) {
    return nullptr;
  }
  GcovFuncInfo *funcData = gcovData->GetFuncProfile(func->GetUniqueID());
  return funcData;
}

bool MeProfUse::CheckSumFail(const uint64 hash, const uint32 expectedCheckSum, const std::string &tag) {
  uint32 curCheckSum = static_cast<uint32>((hash >> 32) ^ (hash & 0xffffffff));
  if (curCheckSum != expectedCheckSum) {
    if (dump) {
      LogInfo::MapleLogger() << func->GetName() << " " << tag << " checksum doesn't match the expected "
                             << expectedCheckSum << " with " << tag << " real hash " << curCheckSum << '\n';
    }
    return true;
  }
  return false;
}

bool MeProfUse::GcovRun() {
  GcovFuncInfo *funcData = GetFuncData();
  if (!funcData) {
    return false;
  }
  func->GetMirFunc()->SetFuncProfData(funcData);
  // early return if lineno fail
  if (CheckSumFail(ComputeLinenoHash(), funcData->lineno_checksum, "lineno")) {
    func->GetMirFunc()->SetFuncProfData(nullptr); // clear func profile data
    return false;
  }
  FindInstrumentEdges();
  // early return if cfgchecksum fail
  if (CheckSumFail(ComputeFuncHash(), funcData->cfg_checksum, "func")) {
    return false;
  }
  std::vector<BB*> instrumentBBs;
  GetInstrumentBBs(instrumentBBs);
  if (dump) {
    DumpEdgeInfo();
  }
  if (instrumentBBs.size() != funcData->num_counts) {
    if (dump) {
      LogInfo::MapleLogger() << func->GetName() << " counter doesn't match profile counter "
                             << funcData->num_counts << " func real counter " <<  instrumentBBs.size() << '\n';
    }
    func->GetMirFunc()->SetFuncProfData(nullptr); // clear func profile data
    return false;
  }
  size_t i = 0;
  for (auto *bb : instrumentBBs) {
    auto *bbUseInfo = GetOrCreateBBUseInfo(*bb);
    bbUseInfo->SetCount(funcData->counts[i]);
    i++;
  }
  InitBBEdgeInfo();
  ComputeEdgeFreq();
  succCalcuAllEdgeFreq = true;
  // save edge frequence to bb
  SetFuncEdgeInfo();
  func->GetCfg()->ConstructStmtFreq();
  return true;
}

void MEProfUse::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.SetPreservedAll();
}

bool MEProfUse::PhaseRun(maple::MeFunction &f) {
  MeProfUse profUse(f, *GetPhaseMemPool(), DEBUGFUNC_NEWPM(f));
  bool result = true;
  if (Options::profileUse) {
    result = profUse.GcovRun();
    f.GetCfg()->VerifyBBFreq();
  } else {
    profUse.Run();
  }

  if (result) {
    LogInfo::MapleLogger() << "******************after profile use  dump function******************\n";
    profUse.DumpFuncCFGEdgeFreq();
    f.GetCfg()->DumpToFile("afterProfileUse", false, true);
  }
  return true;
}
}  // namespace maple
