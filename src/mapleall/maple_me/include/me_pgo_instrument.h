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
#ifndef MAPLE_ME_INCLUDE_PGOINSTRUMENT_H
#define MAPLE_ME_INCLUDE_PGOINSTRUMENT_H

#include "bb.h"
#include "me_cfg_mst.h"
#include "me_cfg.h"
#include "itab_util.h"

namespace maple {
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

 private:
  BB *srcBB;
  BB *destBB;
  uint64 weight;
  bool inMST;
  bool isCritical;
  bool isFake;
};

template <class Edge>
class PGOInstrument {
 public:
  PGOInstrument(MeFunction &func, MemPool &mp, bool dump) : dump(dump), mst(func, mp, dump), func(&func), mp(&mp) {}

  void FindInstrumentEdges() {
    mst.ComputeMST();
  }

  ~PGOInstrument() {
    ClearBBGroupInfo();
  }

  const std::vector<Edge*> &GetAllEdges() {
    return mst.GetAllEdges();
  }

  uint32 GetAllBBs() {
    return mst.GetAllBBs();
  }

  void GetInstrumentBBs(std::vector<BB*> &bbs) const {
    std::vector<Edge*> instrumentEdges;
    mst.GetInstrumentEdges(instrumentEdges);
    std::unordered_set<BB*> visitedBBs;

    for (auto &edge : instrumentEdges) {
      BB *src = edge->GetSrcBB();
      BB *dest = edge->GetDestBB();
      /* instrument the srcbb if it has a single succ */
      if (src->GetSucc().size() <= 1) {
        if (src == func->GetCfg()->GetCommonEntryBB()) {
          bbs.push_back(dest);
        } else {
          bbs.push_back(src);
        }
      } else if (!edge->IsCritical()) {
        bbs.push_back(dest);
      } else {
        if (func->GetMIRModule().IsCModule()) {
          if (src->GetKind() == kBBIgoto || src->GetKind() == kBBGoto) {
            if (visitedBBs.find(dest) == visitedBBs.end()) {
              // In this case, we have to instrument it anyway
              bbs.push_back(dest);
              (void)visitedBBs.insert(dest);
            }
          } else {
            func->GetCfg()->DumpToFile("profGenErrorForCModule", false);
            CHECK_FATAL(false, "Unexpected case %d -> %d", src->UintID(), dest->UintID());
          }
        } else {
          func->GetCfg()->DumpToFile("profGenError", false);
          CHECK_FATAL(false, "impossible critial edge %d -> %d", src->UintID(), dest->UintID());
        }
      }
    }
  }

  void DumpEdgeInfo() const {
    mst.DumpEdgesInfo();
  }

  uint64 ComputeFuncHash() {
    uint64 allEdgeSize = mst.GetAllEdgesSize();
    std::ostringstream ss;
    auto eIt = func->GetCfg()->valid_end();
    // compute func CFG hash,used to verify function IR change
    for (auto bIt = func->GetCfg()->valid_begin(); bIt != eIt; ++bIt) {
      auto *bb = *bIt;
      if (bIt == func->GetCfg()->common_exit()) {
        continue;
      }
      for (auto *succBB : bb->GetSucc()) {
        ss << succBB->GetBBId() << " ";
      }
    }
    uint32 hashCode = DJBHash(ss.str().c_str());
    return (allEdgeSize << kEdgeSizeInfoShift) | hashCode;
  }

  uint64 ComputeLinenoHash() const {
    const std::string& fileName = func->GetMIRModule().GetFileName();
    uint64 fileNameHash = DJBHash(fileName.c_str());
    std::string lineNo = std::to_string(func->GetMirFunc()->GetSrcPosition().LineNum());
    uint64 linenoHash = (fileNameHash << 32) | DJBHash(lineNo.c_str());
    return linenoHash;
  }

  void ClearBBGroupInfo() const {
    auto eIt = func->GetCfg()->valid_end();
    for (auto bIt = func->GetCfg()->valid_begin(); bIt != eIt; ++bIt) {
      auto *bb = *bIt;
      bb->ClearGroup();
    }
  }

 protected:
  bool dump;
 private:
  static constexpr uint32 kEdgeSizeInfoShift = 32;
  CFGMST<Edge> mst;
  MeFunction *func;
  MemPool *mp;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PGOINSTRUMENT_H
