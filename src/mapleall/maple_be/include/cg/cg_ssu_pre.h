/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_CG_INCLUDE_CGSSUPRE_H
#define MAPLEBE_CG_INCLUDE_CGSSUPRE_H
#include <vector>
#include "mempool.h"
#include "mempool_allocator.h"
#include "cg_dominance.h"

// Use SSUPRE to determine where to insert restores for callee-saved registers.
// The usage interface is DoRestorePlacementOpt().  Class SPreWorkCand is used
// as input/output interface.

namespace maplebe {

typedef uint32 BBId;

// This must have been constructed by the caller of DoRestorePlacementOpt() and
// passed to it as parameter.  The caller of DoRestorePlacementOpt() describes
// the problem via occBBs and saveBBs.  DoRestorePlacementOpt()'s outputs are
// returned to the caller by setting restoreAtEntryBBs and restoreAtExitBBs.
class SPreWorkCand {
 public:
  explicit SPreWorkCand(MapleAllocator *alloc):
     occBBs(alloc->Adapter()), saveBBs(alloc->Adapter()),
     restoreAtEntryBBs(alloc->Adapter()), restoreAtExitBBs(alloc->Adapter())  {}
  // inputs
  MapleSet<BBId> occBBs; // Id's of BBs with appearances of the callee-saved reg
  MapleSet<BBId> saveBBs;  // Id's of BBs with saves of the callee-saved reg
  // outputs
  MapleSet<BBId> restoreAtEntryBBs; // Id's of BBs to insert restores of the register at BB entry
  MapleSet<BBId> restoreAtExitBBs; // Id's of BBs to insert restores of the register at BB exit
};

extern void DoRestorePlacementOpt(CGFunc *f, PostDomAnalysis *pdom, SPreWorkCand *workCand);

enum SOccType {
  kSOccUndef,
  kSOccReal,
  kSOccLambda,
  kSOccLambdaRes,
  kSOccEntry,
  kSOccKill,
};

class SOcc {
 public:
  SOcc(SOccType ty, BB *bb) : occTy(ty), cgbb(bb) {}
  virtual ~SOcc() = default;

  virtual void Dump() const = 0;
  bool IsPostDominate(PostDomAnalysis *pdom, const SOcc *occ) const {
    return pdom->PostDominate(*cgbb, *occ->cgbb);
  }

 public:
  SOccType occTy;
  uint32 classId = 0;
  BB *cgbb;  // the BB it occurs in
  SOcc *use = nullptr;  // points to its single use
};

class SRealOcc : public SOcc {
 public:
  SRealOcc(BB *bb): SOcc(kSOccReal, bb) {}
  virtual ~SRealOcc() = default;

  void Dump() const {
    LogInfo::MapleLogger() << "RealOcc at bb" << cgbb->GetId();
    LogInfo::MapleLogger() << " classId" << classId;
  }

 public:
  bool redundant = true;
};

class SLambdaOcc;

class SLambdaResOcc : public SOcc {
 public:
  explicit SLambdaResOcc(BB *bb): SOcc(kSOccLambdaRes, bb) {}
  virtual ~SLambdaResOcc() = default;

  void Dump() const {
    LogInfo::MapleLogger() << "LambdaResOcc at bb" << cgbb->GetId() << " classId" << classId;
  }

 public:
  SLambdaOcc *useLambdaOcc = nullptr;  // its rhs use
  bool hasRealUse = false;
  bool insertHere = false;
};

class SLambdaOcc : public SOcc {
 public:
  SLambdaOcc(BB *bb, MapleAllocator &alloc)
      : SOcc(kSOccLambda, bb), lambdaRes(alloc.Adapter()) {}
  virtual ~SLambdaOcc() = default;

  bool WillBeAnt() const {
    return isCanBeAnt && !isEarlier;
  }

  void Dump() const {
    LogInfo::MapleLogger() << "LambdaOcc at bb" << cgbb->GetId() << " classId" << classId << " Lambda[";
    for (size_t i = 0; i < lambdaRes.size(); i++) {
      lambdaRes[i]->Dump();
      if (i != lambdaRes.size() - 1) {
        LogInfo::MapleLogger() << ", ";
      }
    }
    LogInfo::MapleLogger() << "]";
  }

 public:
  bool isUpsafe = true;
  bool isCanBeAnt = true;
  bool isEarlier = true;
  MapleVector<SLambdaResOcc*> lambdaRes;
};

class SEntryOcc : public SOcc {
 public:
  explicit SEntryOcc(BB *bb) : SOcc(kSOccEntry, bb) {}
  virtual ~SEntryOcc() = default;

  void Dump() const {
    LogInfo::MapleLogger() << "EntryOcc at bb" << cgbb->GetId();
  }
};

class SKillOcc : public SOcc {
 public:
  explicit SKillOcc(BB *bb) : SOcc(kSOccKill, bb) {}
  virtual ~SKillOcc() = default;

  void Dump() const {
    LogInfo::MapleLogger() << "KillOcc at bb" << cgbb->GetId();
  }
};

class SSUPre {
 public:
  SSUPre(CGFunc *cgfunc, PostDomAnalysis *pd, MemPool *memPool, SPreWorkCand *wkcand, bool enDebug)
      : cgFunc(cgfunc),
        pdom(pd),
        spreMp(memPool),
        spreAllocator(memPool),
        workCand(wkcand),
        realOccDfns(std::less<uint32>(), spreAllocator.Adapter()),
        lambdaDfns(std::less<uint32>(), spreAllocator.Adapter()),
        classCount(0),
        realOccs(spreAllocator.Adapter()),
        allOccs(spreAllocator.Adapter()),
        lambdaOccs(spreAllocator.Adapter()),
        entryOccs(spreAllocator.Adapter()),
        enabledDebug(enDebug) {
    CreateEntryOcc(cgfunc->GetFirstBB());
  }
  ~SSUPre() = default;

  void ApplySSUPre();

 private:
  // step 6 methods
  void CodeMotion();
  // step 5 methods
  void Finalize();
  // step 4 methods
  void ResetCanBeAnt(SLambdaOcc *lambda) const;
  void ComputeCanBeAnt() const;
  void ResetEarlier(SLambdaOcc *lambda) const;
  void ComputeEarlier() const;
  // step 3 methods
  void ResetUpsafe(const SLambdaResOcc *lambdaRes) const;
  void ComputeUpsafe() const;
  // step 2 methods
  void Rename();
  // step 1 methods
  void GetIterPdomFrontier(const BB *bb, MapleSet<uint32> *pdfset) const {
    for (BBId bbid : pdom->GetIpdomFrontier(bb->GetId())) {
      (void)pdfset->insert(pdom->GetPdtDfnItem(bbid));
    }
  }
  void FormLambdas();
  void FormLambdaRes();
  void CreateSortedOccs();
  // step 0 methods
  void CreateEntryOcc(BB *bb) {
    SEntryOcc *entryOcc = spreMp->New<SEntryOcc>(bb);
    entryOccs.push_back(entryOcc);
  }
  void FormReals();

  CGFunc *cgFunc;
  PostDomAnalysis *pdom;
  MemPool *spreMp;
  MapleAllocator spreAllocator;
  SPreWorkCand *workCand;
  // following are set of BBs in terms of their dfn's; index into
  // dominance->pdt_preorder to get their bbid's
  // step 0
  MapleSet<uint32> realOccDfns; // set by FormReals()
  // step 1 lambda insertion data structures:
  MapleSet<uint32> lambdaDfns;  // set by FormLambdas()
  // step 2 renaming
  uint32 classCount;  // for assigning new class id
  // the following 4 lists are all maintained in order of pdt_preorder
  MapleVector<SOcc*> realOccs;          // both real and kill occurrences
  MapleVector<SOcc*> allOccs;
  MapleVector<SLambdaOcc*> lambdaOccs;
  MapleVector<SEntryOcc*> entryOccs;
  bool enabledDebug;
};

};  // namespace maplabe
#endif  // MAPLEBE_CG_INCLUDE_CGSSUPRE_H
