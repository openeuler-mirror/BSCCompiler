/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
class Klass;
class KlassHierarchy;
constexpr int64 kBitsPerByte = 8;

inline bool IsMemoryOverlap(OffsetType startA, int64 sizeA, OffsetType startB, int64 sizeB) {
  // A : |---------|
  // B :   |----------|
  if (startA.IsInvalid() || startB.IsInvalid()) {
    return true; // process conservatively
  }
  return startA < (startB + sizeB) &&
         startB < (startA + sizeA);
}

class OstHash {
 public:
  const uint32 operator() (const OriginalSt *ost) const {
    return ost->GetIndex().GetIdx();
  }
};

// this is only for use as return value of CreateAliasInfoExpr()
class AliasInfo {
 public:
  VersionSt *vst;
  FieldID fieldID; // corresponds to fieldID in OP-addrof/OP_iaddrof
  OffsetType offset; // corresponds to offset of array-element and offset from add/sub

  AliasInfo() : vst(nullptr), fieldID(0), offset(kOffsetUnknown) {}
  AliasInfo(VersionSt *vst0, FieldID fld) : vst(vst0), fieldID(fld), offset(kOffsetUnknown) {}
  AliasInfo(VersionSt *vst0, FieldID fld, const OffsetType &offset) : vst(vst0), fieldID(fld), offset(offset) {}
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
        unionFind(memPool, ssaTabParam.GetVersionStTableSize()),
        globalsAffectedByCalls(acAlloc.Adapter()),
        globalsMayAffectedByClinitCheck(acAlloc.Adapter()),
        aggsToUnion(acAlloc.Adapter()),
        nadsOsts(acAlloc.Adapter()),
        ostWithUndefinedOffsets(acAlloc.Adapter()),
        assignSetOfVst(acAlloc.Adapter()),
        aliasSetOfOst(acAlloc.Adapter()),
        addrofVstNextLevNotAllDefsSeen(acAlloc.Adapter()),
        vstNextLevNotAllDefsSeen(acAlloc.Adapter()),
        ostNotAllDefsSeen(acAlloc.Adapter()),
        lessThrowAlias(lessThrowAliasParam),
        ignoreIPA(ignoreIPA),
        calleeHasSideEffect(setCalleeHasSideEffect),
        klassHierarchy(kh) {}

  ~AliasClass() override = default;

  using AssignSet = MapleSet<unsigned>; // value is VersionSt index
  using VstIdx2AssignSet = MapleVector<AssignSet*>; // index is VersionSt index
  using AliasSet = MapleSet<unsigned>; // value is OStIdx
  using OstIdx2AliasSet = MapleVector<AliasSet*>; // index is OStIdx
  using AliasAttrVec = MapleVector<bool>; // index is OStIdx/VersionSt index

  void SetAnalyzedOstNum(size_t num) {
    analyzedOstNum = num;
  }

  bool OstAnalyzed(const OStIdx &ostIdx) const {
    return ostIdx < analyzedOstNum;
  }

  void ReinitUnionFind() {
    unionFind.Reinit();
  }

  const UnionFind &GetUnionFind() const {
    return unionFind;
  }

  void ApplyUnionForPhi(const PhiNode &phi);
  void ApplyUnionForIntrinsicCall(const IntrinsiccallNode &intrinsicCall);
  void ApplyUnionForCopies(StmtNode &stmt);
  void ApplyUnionForDirectAssign(const StmtNode &stmt);
  void ApplyUnionForIndirectAssign(const StmtNode &stmt);
  void ApplyUnionForCommonDirectCalls(StmtNode &stmt);
  void ApplyUnionForJavaSpecialCalls(StmtNode &stmt);
  void ApplyUnionForCallAssigned(const StmtNode &stmt);
  void ApplyUnionForFieldsInCopiedAgg();
  void ApplyUnionForElementsInCopiedArray();
  bool IsGlobalOstTypeUnsafe(const OriginalSt &ost) const;
  void PropagateTypeUnsafe();
  void PropagateTypeUnsafeVertically(const VersionSt &vst) const;
  MIRType *GetAliasInfoRealType(const AliasInfo &ai, const BaseNode &expr);
  bool IsAddrTypeConsistent(const MIRType *typeA, const MIRType *typeB) const;
  void SetTypeUnsafeForAddrofUnion(const VersionSt *vst) const;
  void SetTypeUnsafeForTypeConversion(const VersionSt *lhsVst, BaseNode *rhsExpr);
  void CreateAssignSets();
  void DumpAssignSets();
  void GetValueAliasSetOfVst(size_t vstIdx, std::set<size_t> &result);
  void UnionAllPointedTos();
  void ApplyUnionForPointedTos();
  void CollectRootIDOfNextLevelNodes(const MapleVector<OriginalSt*> *nextLevelOsts,
                                     std::set<unsigned int> &rootIDOfNADSs);
  void UnionForNotAllDefsSeen();
  void UnionForNotAllDefsSeenCLang();
  void ApplyUnionForStorageOverlaps();
  void UnionForAggAndFields();
  void CollectAliasGroups(std::map<unsigned int, std::set<unsigned int>> &aliasGroups);
  bool AliasAccordingToType(TyIdx tyIdxA, TyIdx tyIdxB);
  bool AliasAccordingToFieldID(const OriginalSt &ostA, const OriginalSt &ostB) const;
  void ReconstructAliasGroups();
  void CollectNotAllDefsSeenAes();
  void CreateClassSets();
  void DumpClassSets(bool onlyDumpRoot = true);
  void InsertMayDefUseCall(StmtNode &stmt, BBId bbid, bool isDirectCall);
  void GenericInsertMayDefUse(StmtNode &stmt, BBId bbID);

  static bool MayAliasBasicAA(const OriginalSt *ostA, const OriginalSt *ostB);
  bool MayAlias(const OriginalSt *ostA, const OriginalSt *ostB) const;

  static OffsetType OffsetInBitOfArrayElement(const ArrayNode *arrayNode);

  MapleAllocator &GetMapleAllocator() {
    return acAlloc;
  }

  static inline bool IreadedMemInconsistentWithPointedType(PrimType ireadedPrimType, PrimType pointedPrimType) {
    if (IsPrimitiveVector(ireadedPrimType) || IsPrimitiveVector(pointedPrimType)) {
      return (GetPrimTypeSize(ireadedPrimType) != GetPrimTypeSize(pointedPrimType));
    }
    return false;
  }

  AliasSet *GetAliasSet(const OStIdx &ostIdx) const {
    if (ostIdx >= aliasSetOfOst.size()) {
      return nullptr;
    }
    return aliasSetOfOst[ostIdx];
  }

  AliasSet *GetAliasSet(const OriginalSt &ost) const {
    return GetAliasSet(ost.GetIndex());
  }

 protected:
  virtual bool InConstructorLikeFunc() const {
    return true;
  }

  void SetAliasSet(const OStIdx &ostIdx, AliasSet *aliasSet) {
    if (aliasSetOfOst.size() <= ostIdx) {
      constexpr size_t bufferSize = 10;
      size_t incNum = ostIdx - aliasSetOfOst.size() + bufferSize;
      (void)aliasSetOfOst.insert(aliasSetOfOst.end(), incNum, nullptr);
    }
    aliasSetOfOst[ostIdx] = aliasSet;
  }

  MIRModule &mirModule;

 private:
  bool CallHasNoSideEffectOrPrivateDefEffect(const CallNode &stmt, FuncAttrKind attrKind) const;
  const FuncDesc &GetFuncDescFromCallStmt(const CallNode &callstmt) const;
  bool CallHasNoPrivateDefEffect(StmtNode *stmt) const;
  void RecordAliasAnalysisInfo(const VersionSt &vst);
  VersionSt *FindOrCreateVstOfExtraLevOst(
      BaseNode &expr, const TyIdx &tyIdx, FieldID fieldId, bool typeHasBeenCasted, bool isNextLevelArrayType);
  AliasInfo CreateAliasInfoExpr(BaseNode &expr);
  void SetNotAllDefsSeenForMustDefs(const StmtNode &callas);
  void SetPtrOpndNextLevNADS(const BaseNode &opnd, VersionSt *vst, bool hasNoPrivateDefEffect);
  void SetPtrOpndsNextLevNADS(unsigned int start, unsigned int end, MapleVector<BaseNode*> &opnds,
                              bool hasNoPrivateDefEffect);
  void SetAggPtrFieldsNextLevNADS(const VersionSt &vst);
  void SetPtrFieldsOfAggNextLevNADS(const BaseNode *opnd, const VersionSt *vst);
  void SetAggOpndPtrFieldsNextLevNADS(MapleVector<BaseNode*> &opnds);
  void ApplyUnionForDassignCopy(const VersionSt &lhsVst, VersionSt *rhsVst, BaseNode &rhs);
  bool EfficientUnionTarget(const OriginalSt &ost, const BaseNode &expr);
  bool SetNextLevNADSForEscapePtr(const VersionSt &lhsVst, BaseNode &rhs);
  void UnionNextLevelOfAliasOst(OstPtrSet &ostsToUnionNextLev);
  VersionSt *FindOrCreateDummyNADSVst();
  VersionSt &FindOrCreateVstOfAddrofOSt(OriginalSt &oSt);
  void CollectMayDefForMustDefs(const StmtNode &stmt, OstPtrSet &mayDefOsts);
  void CollectMayUseForNextLevel(const VersionSt &vst, OstPtrSet &mayUseOsts,
                                 const StmtNode &stmt, bool isFirstOpnd);
  void CollectMayUseForIntrnCallOpnd(const StmtNode &stmt, OstPtrSet &mayDefOsts, OstPtrSet &mayUseOsts);
  void CollectMayDefUseForIthOpnd(const VersionSt &vstOfIthOpnd, OstPtrSet &mayUseOsts,
                                  const StmtNode &stmt, bool isFirstOpnd);
  void CollectMayDefUseForCallOpnd(const StmtNode &stmt,
                                   OstPtrSet &mayDefOsts, OstPtrSet &mayUseOsts,
                                   OstPtrSet &mustNotDefOsts, OstPtrSet &mustNotUseOsts);
  void InsertMayDefNodeForCall(OstPtrSet &mayDefOsts, AccessSSANodes *ssaPart,
                               StmtNode &stmt, BBId bbid, bool hasNoPrivateDefEffect);
  void InsertMayUseExpr(BaseNode &expr);
  void CollectMayUseFromFormals(OstPtrSet &mayUseOsts, bool needToBeRestrict);
  void CollectMayUseFromGlobalsAffectedByCalls(OstPtrSet &mayUseOsts);
  void CollectMayUseFromDefinedFinalField(OstPtrSet &mayUseOsts);
  void InsertMayUseNode(OstPtrSet &mayUseOsts, AccessSSANodes *ssaPart);
  void InsertMayUseReturn(const StmtNode &stmt);
  void CollectPtsToOfReturnOpnd(const VersionSt &vst, OstPtrSet &mayUseOsts);
  void InsertReturnOpndMayUse(const StmtNode &stmt);
  void InsertMayUseAll(const StmtNode &stmt);
  void CollectMayDefForDassign(const StmtNode &stmt, OstPtrSet &mayDefOsts);
  void InsertMayDefNode(OstPtrSet &mayDefOsts, AccessSSANodes *ssaPart, StmtNode &stmt, BBId bbid);
  void InsertMayDefDassign(StmtNode &stmt, BBId bbid);
  bool IsEquivalentField(const TyIdx &tyIdxA, FieldID fieldA, const TyIdx &tyIdxB, FieldID fieldB) const;
  bool IsAliasInfoEquivalentToExpr(const AliasInfo &ai, const BaseNode *expr);
  void CollectMayDefForIassign(StmtNode &stmt, OstPtrSet &mayDefOsts);
  void InsertMayDefNodeExcludeFinalOst(OstPtrSet &mayDefOsts, AccessSSANodes *ssaPart,
                                       StmtNode &stmt, BBId bbid);
  void InsertMayDefIassign(StmtNode &stmt, BBId bbid);
  void InsertMayDefUseSyncOps(StmtNode &stmt, BBId bbid);
  void InsertMayUseNodeExcludeFinalOst(const OstPtrSet &mayUseOsts, AccessSSANodes *ssaPart);
  void InsertMayDefUseIntrncall(StmtNode &stmt, BBId bbid);
  void InsertMayDefUseClinitCheck(IntrinsiccallNode &stmt, BBId bbid);
  void InsertMayDefUseAsm(StmtNode &stmt, const BBId bbID);
  virtual BB *GetBB(BBId id) = 0;
  void ProcessIdsAliasWithRoot(const std::set<unsigned int> &idsAliasWithRoot, std::vector<unsigned int> &newGroups);
  int GetOffset(const Klass &super, const Klass &base) const;
  void UnionAllNodes(MapleVector<OriginalSt *> *nextLevOsts);

  AssignSet *GetAssignSet(size_t vstIdx) {
    if (vstIdx >= assignSetOfVst.size()) {
      return nullptr;
    }
    return assignSetOfVst[vstIdx];
  }

  AssignSet *GetAssignSet(const VersionSt &vst) {
    return GetAssignSet(vst.GetIndex());
  }

  bool IsAddrofVstNextLevNotAllDefSeen(size_t vstIdx) {
    if (vstIdx >= addrofVstNextLevNotAllDefsSeen.size()) {
      return false;
    }
    return addrofVstNextLevNotAllDefsSeen[vstIdx];
  }

  void SetAddrofVstNextLevNotAllDefsSeen(size_t vstIdx) {
    if (vstIdx >= addrofVstNextLevNotAllDefsSeen.size()) {
      size_t bufferSize = 5;
      size_t incNum = vstIdx + bufferSize - addrofVstNextLevNotAllDefsSeen.size();
      addrofVstNextLevNotAllDefsSeen.insert(addrofVstNextLevNotAllDefsSeen.end(), incNum, false);
    }
    addrofVstNextLevNotAllDefsSeen[vstIdx] = true;
  }

  bool HasNextLevelNotAllDefSeen(size_t vstIdx) {
    auto *ost = ssaTab.GetVerSt(vstIdx)->GetOst();
    return ost->GetIndirectLev() > 0 || IsNotAllDefsSeen(ost->GetIndex()) || IsNextLevNotAllDefsSeen(vstIdx);
  }

  bool IsNextLevNotAllDefsSeen(size_t vstIdx) {
    if (vstIdx >= vstNextLevNotAllDefsSeen.size()) {
      return false;
    }
    return vstNextLevNotAllDefsSeen[vstIdx];
  }

  void SetNextLevNotAllDefsSeen(size_t vstIdx) {
    if (vstIdx >= vstNextLevNotAllDefsSeen.size()) {
      size_t bufferSize = 5;
      size_t incNum = vstIdx + bufferSize - vstNextLevNotAllDefsSeen.size();
      vstNextLevNotAllDefsSeen.insert(vstNextLevNotAllDefsSeen.end(), incNum, false);
    }
    vstNextLevNotAllDefsSeen[vstIdx] = true;
  }

  bool IsNotAllDefsSeen(const OStIdx &ostIdx) {
    if (ostIdx >= ostNotAllDefsSeen.size()) {
      return false;
    }
    return ostNotAllDefsSeen[ostIdx];
  }

  void SetNotAllDefsSeen(const OStIdx &ostIdx) {
    if (ostIdx >= ostNotAllDefsSeen.size()) {
      size_t bufferSize = 5;
      size_t incNum = ostIdx + bufferSize - ostNotAllDefsSeen.size();
      ostNotAllDefsSeen.insert(ostNotAllDefsSeen.end(), incNum, false);
    }
    ostNotAllDefsSeen[ostIdx] = true;
  }

  MemPool &acMemPool;
  MapleAllocator acAlloc;
  SSATab &ssaTab;
  UnionFind unionFind;
  // set of class ids of globals
  MapleSet<OriginalSt*, OriginalSt::OriginalStPtrComparator> globalsAffectedByCalls;
  // aliased at calls; needed only when wholeProgramScope is true
  MapleSet<OStIdx> globalsMayAffectedByClinitCheck;
  // aggs are copied, their fields should be unioned
  MapleUnorderedMultiMap<OriginalSt*, OriginalSt*, OstHash> aggsToUnion;
  MapleSet<OriginalSt*, OriginalSt::OriginalStPtrComparator> nadsOsts;
  MapleVector<OriginalSt*> ostWithUndefinedOffsets;
  VstIdx2AssignSet assignSetOfVst;
  OstIdx2AliasSet aliasSetOfOst;
  AliasAttrVec addrofVstNextLevNotAllDefsSeen;
  AliasAttrVec vstNextLevNotAllDefsSeen;
  AliasAttrVec ostNotAllDefsSeen;

  bool lessThrowAlias;
  bool ignoreIPA;        // whether to ignore information provided by IPA
  bool calleeHasSideEffect;
  KlassHierarchy *klassHierarchy;
  size_t analyzedOstNum = 0;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ALIAS_CLASS_H
