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
#ifndef MAPLE_PGO_INCLUDE_INSTRUMENT_H
#define MAPLE_PGO_INCLUDE_INSTRUMENT_H

#include "types_def.h"
#include "cfg_mst.h"
#include "mir_function.h"

namespace maple {
MIRSymbol *GetOrCreateProfSymForFunc(MIRFunction &func, uint32 elemCnt);

template<typename BB>
class BBEdge {
 public:
  BBEdge(BB *src, BB *dest, uint64 w = 1, bool isCritical = false, bool isFake = false)
      : srcBB(src), destBB(dest), weight(w), inMST(false), isCritical(isCritical), isFake(isFake) {}

  ~BBEdge() = default;

  BB *GetSrcBB() const {
    return srcBB;
  }

  BB *GetDestBB() const {
    return destBB;
  }

  uint64 GetWeight() const {
    return weight;
  }

  void SetWeight(uint64 w) {
    weight = w;
  }

  bool IsCritical() const {
    return isCritical;
  }

  bool IsFake() const {
    return isFake;
  }

  bool IsInMST() const {
    return inMST;
  }

  void SetInMST() {
    inMST = true;
  }

  int32 GetCondition() const {
    return condition;
  }

  void SetCondition(int32 cond) {
    condition = cond;
  }

  bool IsBackEdge() const {
    return isBackEdge;
  }

  void SetIsBackEdge() {
    isBackEdge = true;
  }

 private:
  BB *srcBB;
  BB *destBB;
  uint64 weight;
  bool inMST;
  bool isCritical;
  bool isFake;
  int32 condition = -1;
  bool isBackEdge = false;
};

template<typename BB>
class BBUseEdge : public BBEdge<BB> {
 public:
  BBUseEdge(BB *src, BB *dest, uint64 w = 1, bool isCritical = false, bool isFake = false)
      : BBEdge<BB>(src, dest, w, isCritical, isFake) {}
  virtual ~BBUseEdge() = default;
  void SetCount(uint64 value) {
    countValue = value;
    valid = true;
  }

  uint64 GetCount() const {
    return countValue;
  }

  bool GetStatus() const {
    return valid;
  }
 private:
  bool valid = false;
  uint64 countValue = 0;
};

template<class IRBB, class Edge>
class PGOInstrumentTemplate {
 public:
  explicit PGOInstrumentTemplate(MemPool &mp) : mst(mp) {}

  void GetInstrumentBBs(std::vector<IRBB*> &bbs, IRBB *commonEnty) const;
  void PrepareInstrumentInfo(IRBB *commonEntry, IRBB* commmonExit) {
    mst.ComputeMST(commonEntry, commmonExit);
  }
  const std::vector<Edge*> &GetAllEdges() {
    return mst.GetAllEdges();
  }
 private:
  CFGMST<Edge, IRBB> mst;
};

template<typename BB>
class BBUseInfo {
 public:
  explicit BBUseInfo(MemPool &tmpPool)
      : innerAlloc(&tmpPool), inEdges(innerAlloc.Adapter()), outEdges(innerAlloc.Adapter()) {}
  virtual ~BBUseInfo() = default;
  void SetCount(uint64 value) {
    countValue = value;
    valid = true;
  }
  uint64 GetCount() const {
    return countValue;
  }

  bool GetStatus() const {
    return valid;
  }

  void AddOutEdge(BBUseEdge<BB> *e) {
    outEdges.push_back(e);
    if (!e->GetStatus()) {
      unknownOutEdges++;
    }
  }

  void AddInEdge(BBUseEdge<BB> *e) {
    inEdges.push_back(e);
    if (!e->GetStatus()) {
      unknownInEdges++;
    }
  }

  const MapleVector<BBUseEdge<BB>*> &GetInEdges() const {
    return inEdges;
  }

  MapleVector<BBUseEdge<BB>*> &GetInEdges() {
    return inEdges;
  }

  size_t GetInEdgeSize() const {
    return inEdges.size();
  }

  const MapleVector<BBUseEdge<BB>*> &GetOutEdges() const {
    return outEdges;
  }

  MapleVector<BBUseEdge<BB>*> &GetOutEdges() {
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

  BBUseEdge<BB> *GetOnlyUnknownOutEdges();

  uint32 GetUnknownInEdges() const {
    return unknownInEdges;
  }

  BBUseEdge<BB> *GetOnlyUnknownInEdges();

 private:
  bool valid = false;
  uint64 countValue = 0;
  uint32 unknownInEdges = 0;
  uint32 unknownOutEdges = 0;
  MapleAllocator innerAlloc;
  MapleVector<BBUseEdge<BB>*> inEdges;
  MapleVector<BBUseEdge<BB>*> outEdges;
};
} /* namespace maple */
#endif // MAPLE_PGO_INCLUDE_INSTRUMENT_H
