/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ALIAS_CLASS_H
#define MAPLE_ME_INCLUDE_ALIAS_CLASS_H
#include "mempool.h"
#include "mempool_allocator.h"
#include "phase.h"
#include "ssa_tab.h"
#include "union_find.h"
#include "class_hierarchy.h"
#include "alias_analysis_table.h"

namespace maple {
class AliasElem {
  friend class AliasClass;
 public:
  AliasElem(uint32 i, OriginalSt &origst)
      : id(i),
        ost(origst) {}

  ~AliasElem() = default;

  void Dump() const;

  uint32 GetClassID() const {
    return id;
  }

  OriginalSt &GetOriginalSt() {
    return ost;
  }
  const OriginalSt &GetOriginalSt() const {
    return ost;
  }

  bool IsNotAllDefsSeen() const {
    return notAllDefsSeen;
  }
  void SetNotAllDefsSeen(bool allDefsSeen) {
    notAllDefsSeen = allDefsSeen;
  }

  bool IsNextLevNotAllDefsSeen() const {
    return nextLevNotAllDefsSeen;
  }
  void SetNextLevNotAllDefsSeen(bool allDefsSeen) {
    nextLevNotAllDefsSeen = allDefsSeen;
  }

  const MapleSet<unsigned int> *GetClassSet() const {
    return classSet;
  }
  void AddClassToSet(unsigned int id) {
    classSet->emplace(id);
  }

  const MapleSet<unsigned int> *GetAssignSet() const {
    return assignSet;
  }
  void AddAssignToSet(unsigned int id) {
    assignSet->emplace(id);
  }

 private:
  uint32 id;  // the original alias class id, before any union; start from 0
  OriginalSt &ost;
  bool notAllDefsSeen = false;         // applied to current level; unused for lev -1
  bool nextLevNotAllDefsSeen = false;  // remember that next level's elements need to be made notAllDefsSeen
  MapleSet<unsigned int> *classSet = nullptr;    // points to the set of members of its class; nullptr for
                                                 // single-member classes
  MapleSet<unsigned int> *assignSet = nullptr;   // points to the set of members that have assignments among themselves
};

// this is only for use as return value of CreateAliasElemsExpr()
class AliasInfo {
 public:
  AliasElem *ae;
  FieldID fieldID; // corresponds to fieldID in OP-addrof/OP_iaddrof

  AliasInfo() : ae(nullptr), fieldID(0) {}
  AliasInfo(AliasElem *ae0, FieldID fld) : ae(ae0), fieldID(fld) {}
  ~AliasInfo() {}
};

class AliasClass : public AnalysisResult {
 public:
  AliasClass(MemPool &memPool, MIRModule &mod, SSATab &ssaTabParam, bool lessThrowAliasParam, bool ignoreIPA,
             bool setCalleeHasSideEffect = false, KlassHierarchy *kh = nullptr)
      : AnalysisResult(&memPool),
        mirModule(mod),
        acMemPool(memPool),
        acAlloc(&memPool),
        ssaTab(ssaTabParam),
        unionFind(memPool),
        osym2Elem(ssaTabParam.GetOriginalStTableSize(), nullptr, acAlloc.Adapter()),
        id2Elem(acAlloc.Adapter()),
        notAllDefsSeenClassSetRoots(acAlloc.Adapter()),
        globalsAffectedByCalls(std::less<unsigned int>(), acAlloc.Adapter()),
        globalsMayAffectedByClinitCheck(acAlloc.Adapter()),
        lessThrowAlias(lessThrowAliasParam),
        ignoreIPA(ignoreIPA),
        calleeHasSideEffect(setCalleeHasSideEffect),
        klassHierarchy(kh) {}

  ~AliasClass() override = default;

  AliasAnalysisTable *GetAliasAnalysisTable() {
    if (aliasAnalysisTable == nullptr) {
      aliasAnalysisTable = acMemPool.New<AliasAnalysisTable>(ssaTab, acAlloc, *klassHierarchy);
    }
    return aliasAnalysisTable;
  }

  const AliasElem *FindAliasElem(const OriginalSt &ost) const {
    return osym2Elem.at(ost.GetIndex());
  }
  AliasElem *FindAliasElem(const OriginalSt &ost) {
    return const_cast<AliasElem*>(const_cast<const AliasClass*>(this)->FindAliasElem(ost));
  }

  size_t GetAliasElemCount() const {
    return osym2Elem.size();
  }

  const AliasElem *FindID2Elem(size_t id) const {
    return id2Elem.at(id);
  }
  AliasElem *FindID2Elem(size_t id) {
    return id2Elem.at(id);
  }

  bool IsCreatedByElimRC(const OriginalSt &ost) const {
    return ost.GetIndex() >= osym2Elem.size();
  }

  void ReinitUnionFind() {
    unionFind.Reinit();
  }

  void ApplyUnionForCopies(StmtNode &stmt);
  void CreateAssignSets();
  void DumpAssignSets();
  void UnionAllPointedTos();
  void ApplyUnionForPointedTos();
  void CollectRootIDOfNextLevelNodes(const OriginalSt &ost, std::set<unsigned int> &rootIDOfNADSs);
  void UnionForNotAllDefsSeen();
  void UnionForNotAllDefsSeenCLang();
  void ApplyUnionForStorageOverlaps();
  void UnionForAggAndFields();
  void CollectAliasGroups(std::map<unsigned int, std::set<unsigned int>> &aliasGroups);
  bool AliasAccordingToType(TyIdx tyIdxA, TyIdx tyIdxB);
  bool AliasAccordingToFieldID(const OriginalSt &ostA, const OriginalSt &ostB);
  void ReconstructAliasGroups();
  void CollectNotAllDefsSeenAes();
  void CreateClassSets();
  void DumpClassSets();
  void InsertMayDefUseCall(StmtNode &stmt, BBId bbid, bool hasSideEffect, bool hasNoPrivateDefEffect);
  void GenericInsertMayDefUse(StmtNode &stmt, BBId bbID);

 protected:
  virtual bool InConstructorLikeFunc() const {
    return true;
  }
  MIRModule &mirModule;

 private:
  bool CallHasNoSideEffectOrPrivateDefEffect(const CallNode &stmt, FuncAttrKind attrKind) const;
  bool CallHasSideEffect(const CallNode &stmt) const;
  bool CallHasNoPrivateDefEffect(const CallNode &stmt) const;
  AliasElem *FindOrCreateAliasElem(OriginalSt &ost);
  AliasElem *FindOrCreateExtraLevAliasElem(BaseNode &expr, TyIdx tyIdx, FieldID fieldId);
  AliasInfo CreateAliasElemsExpr(BaseNode &expr);
  void SetNotAllDefsSeenForMustDefs(const StmtNode &callas);
  void SetPtrOpndNextLevNADS(const BaseNode &opnd, AliasElem *aliasElem, bool hasNoPrivateDefEffect);
  void SetPtrOpndsNextLevNADS(unsigned int start, unsigned int end, MapleVector<BaseNode*> &opnds,
                              bool hasNoPrivateDefEffect);
  void ApplyUnionForDassignCopy(const AliasElem &lhsAe, const AliasElem *rhsAe, const BaseNode &rhs);
  void CreateMirroringAliasElems(OriginalSt *ost1, OriginalSt *ost2);
  AliasElem *FindOrCreateDummyNADSAe();
  bool IsPointedTo(OriginalSt &oSt);
  AliasElem &FindOrCreateAliasElemOfAddrofOSt(OriginalSt &oSt);
  void CollectMayDefForMustDefs(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts);
  void CollectMayUseForCallOpnd(const StmtNode &stmt, std::set<OriginalSt*> &mayUseOsts);
  void InsertMayDefNodeForCall(std::set<OriginalSt*> &mayDefOsts, TypeOfMayDefList &mayDefNodes,
                               StmtNode &stmt, BBId bbid, bool hasNoPrivateDefEffect);
  void InsertMayUseExpr(BaseNode &expr);
  void CollectMayUseFromGlobalsAffectedByCalls(std::set<OriginalSt*> &mayUseOsts);
  void CollectMayUseFromNADS(std::set<OriginalSt*> &mayUseOsts);
  void CollectMayUseFromDefinedFinalField(std::set<OriginalSt*> &mayUseOsts);
  void InsertMayUseNode(std::set<OriginalSt*> &mayUseOsts, TypeOfMayUseList &mayUseNodes);
  void InsertMayUseReturn(const StmtNode &stmt);
  void CollectPtsToOfReturnOpnd(const OriginalSt &ost, std::set<OriginalSt*> &mayUseOsts);
  void InsertReturnOpndMayUse(const StmtNode &stmt);
  void InsertMayUseAll(const StmtNode &stmt);
  void CollectMayDefForDassign(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts);
  void InsertMayDefNode(std::set<OriginalSt*> &mayDefOsts, TypeOfMayDefList &mayDefNodes, StmtNode &stmt, BBId bbid);
  void InsertMayDefDassign(StmtNode &stmt, BBId bbid);
  bool IsEquivalentField(TyIdx tyIdxA, FieldID fldA, TyIdx tyIdxB, FieldID fldB) const;
  void CollectMayDefForIassign(StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts);
  void InsertMayDefNodeExcludeFinalOst(std::set<OriginalSt*> &mayDefOsts, TypeOfMayDefList &mayDefNodes,
                                       StmtNode &stmt, BBId bbid);
  void InsertMayDefIassign(StmtNode &stmt, BBId bbid);
  void InsertMayDefUseSyncOps(StmtNode &stmt, BBId bbid);
  void InsertMayUseNodeExcludeFinalOst(const std::set<OriginalSt*> &mayUseOsts, TypeOfMayUseList &mayUseNodes);
  void InsertMayDefUseIntrncall(StmtNode &stmt, BBId bbid);
  void InsertMayDefUseClinitCheck(IntrinsiccallNode &stmt, BBId bbid);
  virtual BB *GetBB(BBId id) = 0;
  void ProcessIdsAliasWithRoot(const std::set<unsigned int> &idsAliasWithRoot, std::vector<unsigned int> &newGroups);
  int GetOffset(const Klass &super, const Klass &base) const;

  MemPool &acMemPool;
  MapleAllocator acAlloc;
  SSATab &ssaTab;
  UnionFind unionFind;
  MapleVector<AliasElem*> osym2Elem;                    // index is OStIdx
  MapleVector<AliasElem*> id2Elem;                      // index is the id
  MapleVector<AliasElem*> notAllDefsSeenClassSetRoots;  // root of the not_all_defs_seen class sets
  MapleSet<unsigned int> globalsAffectedByCalls;                // set of class ids of globals
  // aliased at calls; needed only when wholeProgramScope is true
  MapleSet<OStIdx> globalsMayAffectedByClinitCheck;
  bool lessThrowAlias;
  bool ignoreIPA;        // whether to ignore information provided by IPA
  bool calleeHasSideEffect;
  KlassHierarchy *klassHierarchy;
  AliasAnalysisTable *aliasAnalysisTable = nullptr;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ALIAS_CLASS_H
