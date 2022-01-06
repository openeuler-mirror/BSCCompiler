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
#include "ssa_tab.h"
#include "union_find.h"

namespace maple {
namespace {
constexpr int64 bitsPerByte = 8;

inline int64 GetTypeBitSize(MIRType *type) {
  if (type->GetKind() == kTypeBitField) {
    return static_cast<MIRBitFieldType*>(type)->GetFieldSize();
  } else {
    return static_cast<int64>(type->GetSize()) * bitsPerByte;
  }
}

inline bool IsMemoryOverlap(OffsetType startA, int64 sizeA, OffsetType startB, int64 sizeB) {
  // A : |---------|
  // B :   |----------|
  return startA < (startB + sizeB) &&
         startB < (startA + sizeA);
}
} // anonymous namespace

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

  OriginalSt *GetOst() {
    return &ost;
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

  void SetClassSet(MapleSet<unsigned int> *newClassSet) {
    classSet = newClassSet;
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
  OffsetType offset; // corresponds to offset of array-element and offset from add/sub

  AliasInfo() : ae(nullptr), fieldID(0), offset(kOffsetUnknown) {}
  AliasInfo(AliasElem *ae0, FieldID fld) : ae(ae0), fieldID(fld), offset(kOffsetUnknown) {}
  AliasInfo(AliasElem *ae0, FieldID fld, OffsetType offset) : ae(ae0), fieldID(fld), offset(offset) {}
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
        aggsToUnion(acAlloc.Adapter()),
        nadsOsts(acAlloc.Adapter()),
        lessThrowAlias(lessThrowAliasParam),
        ignoreIPA(ignoreIPA),
        calleeHasSideEffect(setCalleeHasSideEffect),
        klassHierarchy(kh) {}

  ~AliasClass() override = default;

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

  UnionFind &GetUnionFind() {
    return unionFind;
  }

  void ApplyUnionForCopies(StmtNode &stmt);
  void ApplyUnionForFieldsInCopiedAgg(OriginalSt *lhsOst, OriginalSt *rhsOst);
  void ApplyUnionForFieldsInCopiedAgg();
  void UnionAddrofOstOfUnionFields();
  void CreateAssignSets();
  void DumpAssignSets();
  void GetValueAliasSetOfOst(OriginalSt *ost, std::set<OriginalSt*> &result);
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
  void InsertMayDefUseCall(StmtNode &stmt, BBId bbid, bool isDirectCall);
  void GenericInsertMayDefUse(StmtNode &stmt, BBId bbID);

  static bool MayAliasBasicAA(const OriginalSt *ostA, const OriginalSt *ostB);
  bool MayAlias(const OriginalSt *ostA, const OriginalSt *ostB) const;

  static OffsetType OffsetInBitOfArrayElement(const ArrayNode *arrayNode);
  static OriginalSt *FindOrCreateExtraLevOst(SSATab *ssaTab, OriginalSt *prevLevOst, const TyIdx &tyIdx,
                                             FieldID fld, OffsetType offset);
  const MapleVector<AliasElem*> &Id2AliasElem() {
    return id2Elem;
  }

  MapleAllocator &GetMapleAllocator() {
    return acAlloc;
  }

  static inline bool IreadedMemInconsistentWithPointedType(PrimType ireadedPrimType, PrimType pointedPrimType) {
    if (IsPrimitiveVector(ireadedPrimType) || IsPrimitiveVector(pointedPrimType)) {
      return (GetPrimTypeSize(ireadedPrimType) != GetPrimTypeSize(pointedPrimType));
    }
    return false;
  }

 protected:
  virtual bool InConstructorLikeFunc() const {
    return true;
  }
  MIRModule &mirModule;

 private:
  bool CallHasNoSideEffectOrPrivateDefEffect(const CallNode &stmt, FuncAttrKind attrKind) const;
  const FuncDesc &GetFuncDescFromCallStmt(const CallNode &stmt) const;
  bool CallHasNoPrivateDefEffect(StmtNode *stmt) const;
  AliasElem *FindOrCreateAliasElem(OriginalSt &ost);
  AliasElem *FindOrCreateExtraLevAliasElem(BaseNode &expr, const TyIdx &tyIdx, FieldID fieldId, bool typeHasBeenCasted);
  AliasInfo CreateAliasElemsExpr(BaseNode &expr);
  void SetNotAllDefsSeenForMustDefs(const StmtNode &callas);
  void SetPtrOpndNextLevNADS(const BaseNode &opnd, AliasElem *aliasElem, bool hasNoPrivateDefEffect);
  void SetPtrOpndsNextLevNADS(unsigned int start, unsigned int end, MapleVector<BaseNode*> &opnds,
                              bool hasNoPrivateDefEffect);
  void SetAggPtrFieldsNextLevNADS(const OriginalSt &ost);
  void SetPtrFieldsOfAggNextLevNADS(const BaseNode *opnd, const AliasElem *aliasElem);
  void SetAggOpndPtrFieldsNextLevNADS(MapleVector<BaseNode*> &opnds);
  void ApplyUnionForFieldsInAggCopy(const OriginalSt *lhsost, const OriginalSt *rhsost);
  void ApplyUnionForDassignCopy(AliasElem &lhsAe, AliasElem *rhsAe, BaseNode &rhs);
  bool SetNextLevNADSForEscapePtr(AliasElem &lhsAe, BaseNode &rhs);
  void CreateMirroringAliasElems(const OriginalSt *ost1, OriginalSt *ost2);
  void UnionNextLevelOfAliasOst(std::set<AliasElem *> &aesToUnionNextLev);
  AliasElem *FindOrCreateDummyNADSAe();
  AliasElem &FindOrCreateAliasElemOfAddrofOSt(OriginalSt &oSt);
  void CollectMayDefForMustDefs(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts);
  void CollectMayUseForNextLevel(const OriginalSt *ost, std::set<OriginalSt*> &mayUseOsts,
                                 const StmtNode &stmt, bool isFirstOpnd);
  void CollectMayUseForCallOpnd(const StmtNode &stmt, std::set<OriginalSt*> &mayUseOsts);
  void CollectMayDefUseForCallOpnd(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts,
                                   std::set<OriginalSt*> &mayUseOsts);
  void InsertMayDefNodeForCall(std::set<OriginalSt*> &mayDefOsts, AccessSSANodes *ssaPart,
                               StmtNode &stmt, BBId bbid, bool hasNoPrivateDefEffect);
  void InsertMayUseExpr(BaseNode &expr);
  void CollectMayUseFromFormals(std::set<OriginalSt*> &mayUseOsts);
  void CollectMayUseFromGlobalsAffectedByCalls(std::set<OriginalSt*> &mayUseOsts);
  void CollectMayUseFromDefinedFinalField(std::set<OriginalSt*> &mayUseOsts);
  void InsertMayUseNode(std::set<OriginalSt*> &mayUseOsts, AccessSSANodes *ssaPart);
  void InsertMayUseReturn(const StmtNode &stmt);
  void CollectPtsToOfReturnOpnd(const OriginalSt &ost, std::set<OriginalSt*> &mayUseOsts);
  void InsertReturnOpndMayUse(const StmtNode &stmt);
  void InsertMayUseAll(const StmtNode &stmt);
  void CollectMayDefForDassign(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts);
  void InsertMayDefNode(std::set<OriginalSt*> &mayDefOsts, AccessSSANodes *ssaPart, StmtNode &stmt, BBId bbid);
  void InsertMayDefDassign(StmtNode &stmt, BBId bbid);
  bool IsEquivalentField(TyIdx tyIdxA, FieldID fldA, TyIdx tyIdxB, FieldID fldB) const;
  bool IsAliasInfoEquivalentToExpr(const AliasInfo &ai, const BaseNode *expr);
  void CollectMayDefForIassign(StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts);
  void InsertMayDefNodeExcludeFinalOst(std::set<OriginalSt*> &mayDefOsts, AccessSSANodes *ssaPart,
                                       StmtNode &stmt, BBId bbid);
  void InsertMayDefIassign(StmtNode &stmt, BBId bbid);
  void InsertMayDefUseSyncOps(StmtNode &stmt, BBId bbid);
  void InsertMayUseNodeExcludeFinalOst(const std::set<OriginalSt*> &mayUseOsts, AccessSSANodes *ssaPart);
  void InsertMayDefUseIntrncall(StmtNode &stmt, BBId bbid);
  void InsertMayDefUseClinitCheck(IntrinsiccallNode &stmt, BBId bbid);
  virtual BB *GetBB(BBId id) = 0;
  void ProcessIdsAliasWithRoot(const std::set<unsigned int> &idsAliasWithRoot, std::vector<unsigned int> &newGroups);
  int GetOffset(const Klass &super, const Klass &base) const;
  void UnionAllNodes(MapleVector<OriginalSt *> *nextLevOsts);

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
  MapleMap<OriginalSt*, OriginalSt*> aggsToUnion; // aggs are copied, their fields should be unioned
  MapleSet<OriginalSt *> nadsOsts;
  bool lessThrowAlias;
  bool ignoreIPA;        // whether to ignore information provided by IPA
  bool calleeHasSideEffect;
  KlassHierarchy *klassHierarchy;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ALIAS_CLASS_H
