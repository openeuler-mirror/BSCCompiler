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
#ifndef MAPLE_ME_INCLUDE_SSAPRE_H
#define MAPLE_ME_INCLUDE_SSAPRE_H
#include "irmap.h"
#include "occur.h"
#include "securec.h"

namespace maple {
enum PreKind {
  kExprPre,
  kStmtPre,
  kLoadPre,
  kAddrPre
};

class ExprHoist;
class HoistSummary;

class SSAPre {
 public:
  friend class ExprHoist;
  SSAPre(IRMap &hMap, Dominance &currDom, Dominance &currPdom, MemPool &memPool, MemPool &mp2, PreKind kind,
         uint32 limit)
      : irMap(&hMap),
        ssaTab(&hMap.GetSSATab()),
        mirModule(&hMap.GetSSATab().GetModule()),
        dom(&currDom),
        pdom(&currPdom),
        ssaPreMemPool(&memPool),
        ssaPreAllocator(&memPool),
        perCandMemPool(&mp2),
        perCandAllocator(&mp2),
        workList(ssaPreAllocator.Adapter()),
        preKind(kind),
        allOccs(ssaPreAllocator.Adapter()),
        phiOccs(ssaPreAllocator.Adapter()),
        exitOccs(ssaPreAllocator.Adapter()),
        preLimit(limit),
        dfPhiDfns(std::less<uint32>(), ssaPreAllocator.Adapter()),
        varPhiDfns(std::less<uint32>(), ssaPreAllocator.Adapter()),
        rename2Set(std::less<uint32>(), ssaPreAllocator.Adapter()),
        temp2LocalRefVarMap(ssaPreAllocator.Adapter()) {
    preWorkCandHashTable.GetWorkcandHashTable().fill(nullptr);
  }

  virtual ~SSAPre() = default;

  void ApplySSAPRE();
  bool DefVarDominateOcc(const MeExpr *meExpr, const MeOccur &meOcc) const;
  virtual void CollectVarForMeExpr(MeExpr &meExpr, std::vector<MeExpr*> &varVec) const = 0;
  virtual void CollectVarForCand(MeRealOcc &realOcc, std::vector<MeExpr*> &varVec) const = 0;
  const MapleVector<MeRealOcc*> &GetRealOccList() const {
    return workCand->GetRealOccs();
  }

  virtual IassignMeStmt *CopyIassignMeStmt(const IassignMeStmt &iaStmt) const;
  void IncTreeid() {
    // Incremented by 2 for each tree; purpose is to avoid processing a node the third time inside a tree
    curTreeId += 2;
  }

  virtual void DumpWorkList() const;
  virtual void DumpWorkListWrap() const;
  GStrIdx NewTempStrIdx();
  virtual BB *GetBB(BBId id) const = 0;
  virtual PUIdx GetPUIdx() const = 0;
  virtual void SetCurFunction(PUIdx) const {}

  void GetIterDomFrontier(const BB *bb, MapleSet<uint32> *dfset) const {
    for (auto bbid : dom->GetIterDomFrontier(bb->GetID())) {
      (void)dfset->insert(dom->GetDtDfnItem(bbid));
    }
  }

  void SetSpillAtCatch(bool status) {
    spillAtCatch = status;
  }

  bool GetSpillAtCatch() const {
    return spillAtCatch;
  }

  PreWorkCand* GetWorkCand() const {
    return workCand;
  }

  void SetAddedNewLocalRefVars(bool status) {
    addedNewLocalRefVars = status;
  }

  bool GetAddedNewLocalRefVars() const {
    return addedNewLocalRefVars;
  }

  void SetSSAPreDebug(bool status) {
    enableDebug = status;
  }

  bool GetSSAPreDebug() const {
    return enableDebug;
  }

  void SetPlacementRC(bool status) {
    placementRCEnabled = status;
  }

  bool GetPlacementRCOn() const {
    return placementRCEnabled;
  }

  void SetRcLoweringOn(bool status) {
    rcLoweringEnabled = status;
  }

  bool GetRcLoweringOn() const {
    return rcLoweringEnabled;
  }

  void SetRegReadAtReturn(bool status) {
    regReadAtReturn = status;
  }

  bool GetRegReadAtReturn() const {
    return regReadAtReturn;
  }

  bool strengthReduction = false;
  bool doLFTR = false;
  bool doMinCut = false;

 protected:
  // step 6 codemotion methods
  ScalarMeExpr *CreateNewCurTemp(const MeExpr &meExpr);
  VarMeExpr *CreateNewCurLocalRefVar();
  virtual void GenerateSaveRealOcc(MeRealOcc &realOcc) = 0;
  virtual void GenerateReloadRealOcc(MeRealOcc &realOcc) = 0;
  void GenerateSaveInsertedOcc(MeInsertedOcc &insertedOcc);
  void GenerateSavePhiOcc(MePhiOcc &phiOcc);
  void UpdateInsertedPhiOccOpnd();
  virtual OpMeExpr *FormLFTRCompare(MeRealOcc *compOcc, MeExpr *regorvar) {
    (void)compOcc;
    (void)regorvar;
    return nullptr;
  }
  virtual void CodeMotion();
  // step 5 Finalize methods
  bool WillBeAvail(MePhiOcc *phiOcc) const {
    if (!workCand->applyMinCut) {
      return phiOcc->IsWillBeAvail();
    }
    return phiOcc->IsMCWillBeAvail();
  }
  bool OKToInsert(MePhiOpndOcc *phiOpnd);
  virtual void Finalize1();
  void SetSave(MeOccur &defX);
  void SetReplacement(MePhiOcc &occ, MeOccur &repDef);
  virtual void Finalize2();
  // hoist methods
  void HoistExpr();
  void ExprHoistPrepare();
  static void HoistClean();
  // step 4 willbevail methods
  void ComputeCanBeAvail() const;
  void ResetCanBeAvail(MePhiOcc &occ) const;
  void ComputeLater() const;
  void ResetLater(MePhiOcc &occ) const;
  // step 3 downsafety methods
  void ResetDS(MePhiOpndOcc &phiOpnd) const;
  void ComputeDS() const;
  // step 2 renaming methods
  virtual bool AllVarsSameVersion(const MeRealOcc &realOcc1, const MeRealOcc &realOcc2) const {
    return realOcc1.GetMeExpr() == realOcc2.GetMeExpr();
  }

  void Rename1();
  MeExpr *GetReplaceMeExpr(const MeExpr &opnd, const BB &ePhiBB, size_t j) const;
  virtual MeExpr *PhiOpndFromRes(MeRealOcc &realOcc, size_t i) const = 0;
  virtual void Rename2();
  // step 1 phi insertion methods
  void SetVarPhis(MeExpr *meExpr);
  virtual void ComputeVarAndDfPhis() = 0;
  virtual void CreateSortedOccs();
  // phi insertion methods end
  virtual void BuildWorkList() = 0;
  virtual void BuildEntryLHSOcc4Formals() const {}

  virtual void BuildWorkListLHSOcc(MeStmt&, int32) {}

  virtual void BuildWorkListIvarLHSOcc(MeStmt&, int32, bool, MeExpr*) {}

  virtual void BuildWorkListExpr(MeStmt &meStmt, int32 seqStmt, MeExpr &meExpr, bool isRebuilt, MeExpr *tempVar,
                                 bool isRootExpr, bool insertSorted) = 0;
  virtual void BuildWorkListStmt(MeStmt &stmt, uint32 seqStmt, bool isRebuilt, MeExpr *tempVar = nullptr);
  virtual void BuildWorkListBB(BB *bb);
  virtual void ConstructUseOccurMap() {}

  void CreateMembarOcc(MeStmt &meStmt, int seqStmt);
  virtual void CreateMembarOccAtCatch(BB &bb);
  void CreateExitOcc(BB &bb) {
    MeOccur *exitOcc = ssaPreMemPool->New<MeOccur>(kOccExit, 0, bb, nullptr);
    exitOccs.push_back(exitOcc);
  }
  virtual void CreateCompOcc(MeStmt *meStmt, int seqStmt, OpMeExpr *comapre, bool isRebuilt) {
    (void)meStmt;
    (void)seqStmt;
    (void)comapre;
    (void)isRebuilt;
  }

  bool CheckIfAnyLocalOpnd(const MeExpr &meExpr) const;
  void CreateRealOcc(MeStmt &meStmt, int32 seqStmt, MeExpr &meExpr, bool insertSorted, bool isLHS = false);
  virtual bool ScreenPhiBB(BBId bbId) const = 0;
  virtual bool EpreLocalRefVar() const {
    return false;
  }

  virtual void EnterCandsForSSAUpdate(OStIdx, const BB&) {}

  virtual bool IsLoopHeadBB(BBId) const {
    return false;
  }
  virtual RegMeExpr *ResolveAllInjuringDefs(RegMeExpr *regx) const { return regx; }
  virtual MeExpr *ResolveAllInjuringDefs(MeExpr *x) const { return x; }
  virtual void SubstituteOpnd(MeExpr *x, MeExpr *oldopnd, MeExpr *newopnd) {
    (void)x;
    (void)oldopnd;
    (void)newopnd;
  }
  virtual void SRSetNeedRepair(MeOccur *useocc, std::set<MeStmt *> *needRepairInjuringDefs) {
    (void)useocc;
    (void)needRepairInjuringDefs;
  }
  virtual MeExpr *SRRepairInjuries(MeOccur *useocc,
      std::set<MeStmt *> *needRepairInjuringDefs,
      std::set<MeStmt *> *repairedInjuringDefs) {
    (void)useocc;
    (void)needRepairInjuringDefs;
    (void)repairedInjuringDefs;
    return nullptr;
  }

  IRMap *irMap;
  SSATab *ssaTab;
  MIRModule *mirModule;
  Dominance *dom;
  Dominance *pdom;
  MemPool *ssaPreMemPool;
  MapleAllocator ssaPreAllocator;
  MemPool *perCandMemPool;
  MapleAllocator perCandAllocator;
  MapleList<PreWorkCand*> workList;
  PreWorkCand *workCand = nullptr;  // the current PreWorkCand
  ExprHoist *eh = nullptr;
  PreKind preKind;

  uint32 curTreeId = 0;  // based on number of rooted trees processed in collecting
  // PRE work candidates; incremented by 2 for each tree;
  // purpose is to avoid processing a node the third time
  // inside a tree (which is a DAG)
  // the following 3 lists are all maintained in order of dt_preorder
  MapleVector<MeOccur*> allOccs;     // cleared at start of each workcand
  MapleVector<MePhiOcc*> phiOccs;    // cleared at start of each workcand
  MapleVector<MeOccur*> exitOccs;  // this is shared by all workcands
  uint32 preLimit;  // set by command-line option to limit the number of candidates optimized (for debugging purpose)
  // step 1 phi insertion data structures
  // following are set of BBs in terms of their dfn's; index into
  // dominance->pdt_preorder to get their bbid's
  MapleSet<uint32> dfPhiDfns;   // phis inserted due to dominance frontiers
  MapleSet<uint32> varPhiDfns;  // phis inserted due to the var operands
  // step 2 renaming data structures
  uint32 classCount = 0;            // count class created during renaming
  MapleSet<uint32> rename2Set;  // set created by rename1 for use rename2; value
  // is index into workCand->realOccs
  // step 6 codemotion data structures
  ScalarMeExpr *curTemp = nullptr;            // the created temp for current workCand
  VarMeExpr *curLocalRefVar = nullptr;  // the created localrefvar for ref-type iread
  MapleMap<RegMeExpr*, VarMeExpr*> temp2LocalRefVarMap;
  int32 reBuiltOccIndex = -1;  // stores the size of worklist every time when try to add new worklist, update before
  // each code motion
  uint32 strIdxCount = 0;  // ssapre will create a lot of temp variables if using var to store redundances, start from 0
  PreWorkCandHashTable preWorkCandHashTable;

  virtual void DoSSAFRE() {};

  bool enableDebug = false;
  bool rcLoweringEnabled = false;
  bool regReadAtReturn = false;
  bool spillAtCatch = false;
  bool placementRCEnabled = false;
  bool addedNewLocalRefVars = false;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SSAPRE_H
