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
#ifndef MAPLE_ME_INCLUDE_PROFILEUSE_H
#define MAPLE_ME_INCLUDE_PROFILEUSE_H

#include "me_pgo_instrument.h"
#include "bb.h"
#include "maple_phase_manager.h"

namespace maple {
class BBUseEdge : public BBEdge {
 public:
  BBUseEdge(BB *src, BB *dest, uint64 w = 1, bool isCritical = false, bool isFake = false)
      : BBEdge(src, dest, w, isCritical, isFake) {}
  virtual ~BBUseEdge() = default;

  void SetCount(FreqType value) {
    countValue = value;
    valid = true;
  }

  FreqType GetCount() const {
    return countValue;
  }

  bool GetStatus() const {
    return valid;
  }
 private:
  bool valid = false;
  FreqType countValue = 0;
};


class BBUseInfo {
 public:
  explicit BBUseInfo(MemPool &tmpPool)
      : tmpAlloc(&tmpPool), inEdges(tmpAlloc.Adapter()), outEdges(tmpAlloc.Adapter()) {}
  virtual ~BBUseInfo() = default;

  void SetCount(FreqType value) {
    countValue = value;
    valid = true;
  }
  FreqType GetCount() const {
    return countValue;
  }

  bool GetStatus() const {
    return valid;
  }

  void AddOutEdge(BBUseEdge *e) {
    outEdges.push_back(e);
    if (!e->GetStatus()) {
      unknownOutEdges++;
    }
  }

  void AddInEdge(BBUseEdge *e) {
    inEdges.push_back(e);
    if (!e->GetStatus()) {
      unknownInEdges++;
    }
  }

  const MapleVector<BBUseEdge*> &GetInEdges() const {
    return inEdges;
  }

  MapleVector<BBUseEdge*> &GetInEdges() {
    return inEdges;
  }

  size_t GetInEdgeSize() const {
    return inEdges.size();
  }

  const MapleVector<BBUseEdge*> &GetOutEdges() const {
    return outEdges;
  }

  MapleVector<BBUseEdge*> &GetOutEdges() {
    return outEdges;
  }

  size_t GetOutEdgeSize() const {
    return outEdges.size();
  }

  void DecreaseUnKnownOutEdges() {
    unknownOutEdges--;
  }

  void DecreaseUnKnownInEdges() {
    unknownInEdges--;
  }

  uint32 GetUnknownOutEdges() const {
    return unknownOutEdges;
  }

  uint32 GetUnknownInEdges() const {
    return unknownInEdges;
  }

 private:
  bool valid = false;
  FreqType countValue = 0;
  uint32 unknownInEdges = 0;
  uint32 unknownOutEdges = 0;
  MapleAllocator tmpAlloc;
  MapleVector<BBUseEdge*> inEdges;
  MapleVector<BBUseEdge*> outEdges;
};

class MeProfUse : public PGOInstrument<BBUseEdge> {
 public:
  MeProfUse(MeFunction &func, MemPool &mp, bool dump) : PGOInstrument(func, mp, dump), func(&func), mp(&mp) {}
  virtual ~MeProfUse() = default;
  bool BuildEdgeCount();
  void SetFuncEdgeInfo();
  void DumpFuncCFGEdgeFreq() const;
  bool Run();
  bool IsSuccUseProf() const {
    return succCalcuAllEdgeFreq;
  }
  bool MapleProfRun();
  void CheckSumFail(const uint64 hash, const uint32 expectedCheckSum, const std::string &tag);
 private:
  bool IsAllZero(Profile::BBInfo &result) const;
  void SetEdgeCount(BBUseEdge &edge, FreqType value);
  void SetEdgeCount(MapleVector<BBUseEdge*> &edges, FreqType value);
  void ComputeEdgeFreq();
  void InitBBEdgeInfo();
  void ComputeBBFreq(BBUseInfo &bbInfo, bool &changed);
  FuncProfInfo *GetFuncData();

  FreqType SumEdgesCount(const MapleVector<BBUseEdge*> &edges) const;
  BBUseInfo *GetBBUseInfo(const BB &bb) const;
  BBUseInfo *GetOrCreateBBUseInfo(const BB &bb) ;
  MeFunction *func;
  MemPool *mp;
  bool succCalcuAllEdgeFreq = false;
  std::unordered_map<const BB*, BBUseInfo*> bbProfileInfo;
};

MAPLE_FUNC_PHASE_DECLARE(MEProfUse, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PROFILEUSE_H
