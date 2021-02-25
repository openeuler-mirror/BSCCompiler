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
#include "alias_class.h"
#include "mpl_logging.h"
#include "opcode_info.h"
#include "ssa_mir_nodes.h"
#include "mir_function.h"
#include "mir_builder.h"

namespace {
using namespace maple;


inline bool IsReadOnlyOst(const OriginalSt &ost) {
  return ost.GetMIRSymbol()->HasAddrOfValues();
}

inline bool IsPotentialAddress(PrimType pType) {
  return IsAddress(pType) || IsPrimitiveDynType(pType);
}
}  // namespace

namespace maple {
bool AliasClass::CallHasNoSideEffectOrPrivateDefEffect(const CallNode &stmt, FuncAttrKind attrKind) const {
  ASSERT(attrKind == FUNCATTR_nosideeffect || attrKind == FUNCATTR_noprivate_defeffect, "Not supportted attrKind");
  MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(stmt.GetPUIdx());
  if (callee == nullptr) {
    return false;
  }
  bool hasAttr = false;
  if (callee->GetFuncAttrs().GetAttr(attrKind)) {
    hasAttr = true;
  } else if (!ignoreIPA) {
    hasAttr = (attrKind == FUNCATTR_nosideeffect) ? (callee->IsNoDefEffect() && callee->IsNoDefArgEffect()) :
                                                    callee->IsNoPrivateDefEffect();
  }
  return hasAttr;
}

bool AliasClass::CallHasSideEffect(const CallNode &stmt) const {
  return calleeHasSideEffect ? true : !CallHasNoSideEffectOrPrivateDefEffect(stmt, FUNCATTR_nosideeffect);
}

bool AliasClass::CallHasNoPrivateDefEffect(const CallNode &stmt) const {
  return calleeHasSideEffect ? false : CallHasNoSideEffectOrPrivateDefEffect(stmt, FUNCATTR_noprivate_defeffect);
}

// here starts pass 1 code
AliasElem *AliasClass::FindOrCreateAliasElem(OriginalSt &ost) {
  OStIdx ostIdx = ost.GetIndex();
  CHECK_FATAL(ostIdx > 0u, "Invalid ost index");
  CHECK_FATAL(ostIdx < osym2Elem.size(), "Index out of range");
  AliasElem *aliasElem = osym2Elem[ostIdx];
  if (aliasElem != nullptr) {
    return aliasElem;
  }
  aliasElem = acMemPool.New<AliasElem>(id2Elem.size(), ost);
  if (ost.IsSymbolOst() && ost.GetIndirectLev() >= 0) {
    const MIRSymbol *sym = ost.GetMIRSymbol();
    if (sym->IsGlobal() && !sym->HasAddrOfValues() && !sym->GetIsTmp()) {
      (void)globalsMayAffectedByClinitCheck.insert(ostIdx);
      if (!sym->IsReflectionClassInfo()) {
        if (!ost.IsFinal() || InConstructorLikeFunc()) {
          (void)globalsAffectedByCalls.insert(aliasElem->GetClassID());
        }
        aliasElem->SetNextLevNotAllDefsSeen(true);
      }
    }
  }
  if (aliasElem->GetOriginalSt().IsFormal() || ost.GetIndirectLev() > 0) {
    aliasElem->SetNextLevNotAllDefsSeen(true);
  }
  id2Elem.push_back(aliasElem);
  osym2Elem[ostIdx] = aliasElem;
  unionFind.NewMember();
  return aliasElem;
}

AliasElem *AliasClass::FindOrCreateExtraLevAliasElem(BaseNode &expr, TyIdx tyIdx, FieldID fieldId) {
  AliasElem *aliasElem = CreateAliasElemsExpr(kOpcodeInfo.IsTypeCvt(expr.GetOpCode()) ? *expr.Opnd(0) : expr);
  if (aliasElem == nullptr) {
    return nullptr;
  }
  OriginalSt *newOst = GetAliasAnalysisTable()->FindOrCreateExtraLevOriginalSt(aliasElem->GetOriginalSt(),
      tyIdx, fieldId);
  CHECK_FATAL(newOst != nullptr, "null ptr check");
  if (newOst->GetIndex() == osym2Elem.size()) {
    osym2Elem.push_back(nullptr);
    ssaTab.GetVersionStTable().CreateVersionSt(newOst, kInitVersion);
  }
  return FindOrCreateAliasElem(*newOst);
}

AliasElem &AliasClass::FindOrCreateAliasElemOfAddrofOSt(OriginalSt &oSt) {
  OriginalSt *addrofOst = GetAliasAnalysisTable()->FindOrCreateAddrofSymbolOriginalSt(oSt);
  if (addrofOst->GetIndex() == osym2Elem.size()) {
    osym2Elem.push_back(nullptr);
  }
  return *FindOrCreateAliasElem(*addrofOst);
}

AliasElem *AliasClass::CreateAliasElemsExpr(BaseNode &expr) {
  switch (expr.GetOpCode()) {
    case OP_addrof: {
      OriginalSt &oSt = *static_cast<AddrofSSANode&>(expr).GetSSAVar()->GetOst();
      oSt.SetAddressTaken();
      FindOrCreateAliasElem(oSt);
      return &FindOrCreateAliasElemOfAddrofOSt(oSt);
    }
    case OP_dread: {
      OriginalSt &ost = *static_cast<AddrofSSANode&>(expr).GetSSAVar()->GetOst();
      return FindOrCreateAliasElem(ost);
    }
    case OP_regread: {
      OriginalSt &oSt = *static_cast<RegreadSSANode&>(expr).GetSSAVar()->GetOst();
      return (oSt.IsSpecialPreg()) ? nullptr : FindOrCreateAliasElem(oSt);
    }
    case OP_iread: {
      auto &iread = static_cast<IreadSSANode&>(expr);
      return FindOrCreateExtraLevAliasElem(utils::ToRef(iread.Opnd(0)), iread.GetTyIdx(), iread.GetFieldID());
    }
    case OP_iaddrof: {
      auto &iread = static_cast<IreadNode&>(expr);
      AliasElem *aliasElem = FindOrCreateExtraLevAliasElem(*iread.Opnd(0), iread.GetTyIdx(), iread.GetFieldID());
      return &FindOrCreateAliasElemOfAddrofOSt(aliasElem->GetOriginalSt());
    }
    case OP_add:
    case OP_sub:
    case OP_array:
    case OP_retype: {
      for (size_t i = 1; i < expr.NumOpnds(); ++i) {
        CreateAliasElemsExpr(*expr.Opnd(i));
      }
      return CreateAliasElemsExpr(*expr.Opnd(0));
    }
    case OP_intrinsicop: {
      auto &intrn = static_cast<IntrinsicopNode&>(expr);
      if (intrn.GetIntrinsic() == INTRN_MPL_READ_OVTABLE_ENTRY ||
          (intrn.GetIntrinsic() == INTRN_JAVA_MERGE && intrn.NumOpnds() == 1 &&
           intrn.GetNopndAt(0)->GetOpCode() == OP_dread)) {
        return CreateAliasElemsExpr(*intrn.GetNopndAt(0));
      }
      // fall-through
    }
    [[clang::fallthrough]];
    default:
      for (size_t i = 0; i < expr.NumOpnds(); ++i) {
        CreateAliasElemsExpr(*expr.Opnd(i));
      }
  }
  return nullptr;
}

// when a mustDef is a pointer, set its pointees' notAllDefsSeen flag to true
void AliasClass::SetNotAllDefsSeenForMustDefs(const StmtNode &callas) {
  MapleVector<MustDefNode> &mustDefs = ssaTab.GetStmtsSSAPart().GetMustDefNodesOf(callas);
  for (auto &mustDef : mustDefs) {
    AliasElem *aliasElem = FindOrCreateAliasElem(*mustDef.GetResult()->GetOst());
    aliasElem->SetNextLevNotAllDefsSeen(true);
  }
}

void AliasClass::ApplyUnionForDassignCopy(const AliasElem &lhsAe, const AliasElem *rhsAe, const BaseNode &rhs) {
  if (rhsAe == nullptr || rhsAe->GetOriginalSt().GetIndirectLev() > 0 || rhsAe->IsNotAllDefsSeen()) {
    AliasElem *aliasElem = FindAliasElem(lhsAe.GetOriginalSt());
    aliasElem->SetNextLevNotAllDefsSeen(true);
    return;
  }
  if (!IsPotentialAddress(rhs.GetPrimType()) || kOpcodeInfo.NotPure(rhs.GetOpCode()) ||
      (rhs.GetOpCode() == OP_addrof && IsReadOnlyOst(rhsAe->GetOriginalSt()))) {
    return;
  }
  unionFind.Union(lhsAe.GetClassID(), rhsAe->GetClassID());
}

void AliasClass::SetPtrOpndNextLevNADS(const BaseNode &opnd, AliasElem *aliasElem, bool hasNoPrivateDefEffect) {
  if (IsPotentialAddress(opnd.GetPrimType()) && aliasElem != nullptr &&
      !(hasNoPrivateDefEffect && aliasElem->GetOriginalSt().IsPrivate()) &&
      !(opnd.GetOpCode() == OP_addrof && IsReadOnlyOst(aliasElem->GetOriginalSt()))) {
    aliasElem->SetNextLevNotAllDefsSeen(true);
  }
}

// Set aliasElem of the pointer-type opnds of a call as next_level_not_all_defines_seen
void AliasClass::SetPtrOpndsNextLevNADS(unsigned int start, unsigned int end,
                                        MapleVector<BaseNode*> &opnds,
                                        bool hasNoPrivateDefEffect) {
  for (size_t i = start; i < end; ++i) {
    BaseNode *opnd = opnds[i];
    SetPtrOpndNextLevNADS(*opnd, CreateAliasElemsExpr(*opnd), hasNoPrivateDefEffect);
  }
}

void AliasClass::ApplyUnionForCopies(StmtNode &stmt) {
  switch (stmt.GetOpCode()) {
    case OP_maydassign:
    case OP_dassign:
    case OP_regassign: {
      // RHS
      ASSERT_NOT_NULL(stmt.Opnd(0));
      AliasElem *rhsAe = CreateAliasElemsExpr(*stmt.Opnd(0));
      // LHS
      OriginalSt *ost = ssaTab.GetStmtsSSAPart().GetAssignedVarOf(stmt)->GetOst();
      AliasElem *lhsAe = FindOrCreateAliasElem(*ost);
      ASSERT_NOT_NULL(lhsAe);
      ApplyUnionForDassignCopy(*lhsAe, rhsAe, *stmt.Opnd(0));
      return;
    }
    case OP_iassign: {
      auto &iassignNode = static_cast<IassignNode&>(stmt);
      AliasElem *rhsAliasElem = CreateAliasElemsExpr(*iassignNode.Opnd(1));
      AliasElem *lhsAliasElem = FindOrCreateExtraLevAliasElem(*iassignNode.Opnd(0), iassignNode.GetTyIdx(),
          iassignNode.GetFieldID());
      if (lhsAliasElem != nullptr) {
        ApplyUnionForDassignCopy(*lhsAliasElem, rhsAliasElem, *iassignNode.Opnd(1));
      }
      return;
    }
    case OP_throw: {
      SetPtrOpndNextLevNADS(*stmt.Opnd(0), CreateAliasElemsExpr(*stmt.Opnd(0)), false);
      return;
    }
    case OP_call:
    case OP_callassigned: {
      auto &call = static_cast<CallNode&>(stmt);
      ASSERT(call.GetPUIdx() < GlobalTables::GetFunctionTable().GetFuncTable().size(),
             "index out of range in AliasClass::ApplyUnionForCopies");
      if (CallHasSideEffect(call)) {
        SetPtrOpndsNextLevNADS(0, static_cast<unsigned int>(call.NumOpnds()), call.GetNopnd(),
                               CallHasNoPrivateDefEffect(call));
      }
      break;
    }
    case OP_virtualcall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_virtualcallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned: {
      auto &call = static_cast<NaryStmtNode&>(stmt);
      SetPtrOpndsNextLevNADS(0, static_cast<unsigned int>(call.NumOpnds()), call.GetNopnd(), false);
      break;
    }
    case OP_icall:
    case OP_icallassigned:
    case OP_virtualicall:
    case OP_interfaceicall:
    case OP_virtualicallassigned:
    case OP_interfaceicallassigned: {
      auto &call = static_cast<NaryStmtNode&>(stmt);
      SetPtrOpndsNextLevNADS(1, static_cast<unsigned int>(call.NumOpnds()), call.GetNopnd(), false);
      break;
    }
    case OP_intrinsiccall:
    case OP_intrinsiccallassigned: {
      auto &intrnNode = static_cast<IntrinsiccallNode&>(stmt);
      if (intrnNode.GetIntrinsic() == INTRN_JAVA_POLYMORPHIC_CALL) {
        SetPtrOpndsNextLevNADS(0, static_cast<unsigned int>(intrnNode.NumOpnds()), intrnNode.GetNopnd(), false);
        break;
      }
      //  fallthrough;
    }
    [[clang::fallthrough]];
    default:
      for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        CreateAliasElemsExpr(*stmt.Opnd(i));
      }
  }
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    SetNotAllDefsSeenForMustDefs(stmt);
  }
}

void AliasClass::CreateAssignSets() {
  // iterate through all the alias elems
  for (auto *aliasElem : id2Elem) {
    unsigned int id = aliasElem->GetClassID();
    unsigned int rootID = unionFind.Root(id);
    if (unionFind.GetElementsNumber(rootID) > 1) {
      // only root id's have assignset
      if (id2Elem[rootID]->GetAssignSet() == nullptr) {
        id2Elem[rootID]->assignSet = acMemPool.New<MapleSet<unsigned int>>(acAlloc.Adapter());
      }
      id2Elem[rootID]->AddAssignToSet(id);
    }
  }
}

void AliasClass::DumpAssignSets() {
  LogInfo::MapleLogger() << "/////// assign sets ///////\n";
  for (auto *aliasElem : id2Elem) {
    if (unionFind.Root(aliasElem->GetClassID()) != aliasElem->GetClassID()) {
      continue;
    }

    if (aliasElem->GetAssignSet() == nullptr) {
      LogInfo::MapleLogger() << "Alone: ";
      aliasElem->Dump();
      LogInfo::MapleLogger() << '\n';
    } else {
      LogInfo::MapleLogger() << "Members of assign set " << aliasElem->GetClassID() << ": ";
      for (unsigned int elemID : *(aliasElem->GetAssignSet())) {
        id2Elem[elemID]->Dump();
      }
      LogInfo::MapleLogger() << '\n';
    }
  }
}

bool AliasClass::IsPointedTo(OriginalSt &oSt) {
  return GetAliasAnalysisTable()->GetPrevLevelNode(oSt) != nullptr;
}

void AliasClass::UnionAllPointedTos() {
  std::vector<AliasElem*> pointedTos;
  for (auto *aliasElem : id2Elem) {
    if (IsPointedTo(aliasElem->GetOriginalSt())) {
      aliasElem->SetNotAllDefsSeen(true);
      pointedTos.push_back(aliasElem);
    }
  }
  for (size_t i = 1; i < pointedTos.size(); ++i) {
    unionFind.Union(pointedTos[0]->GetClassID(), pointedTos[i]->GetClassID());
  }
}

void AliasClass::UpdateNextLevelNodes(std::vector<OriginalSt*> &nextLevelOsts, const AliasElem &aliasElem) {
  for (size_t elemID : *(aliasElem.GetAssignSet())) {
    for (OriginalSt *nextLevelNode : *(GetAliasAnalysisTable()->GetNextLevelNodes(id2Elem[elemID]->GetOriginalSt()))) {
      nextLevelOsts.push_back(nextLevelNode);
    }
  }
}

void AliasClass::UnionNodes(std::vector<OriginalSt*> &nextLevelOsts) {
  for (size_t i = 0; i < nextLevelOsts.size(); ++i) {
    OriginalSt *ost1 = nextLevelOsts[i];
    for (size_t j = i + 1; j < nextLevelOsts.size(); ++j) {
      OriginalSt *ost2 = nextLevelOsts[j];
      if ((ost1->GetFieldID() == 0 || ost2->GetFieldID() == 0 || ost1->GetFieldID() == ost2->GetFieldID()) &&
          !(ost1->IsFinal() || ost2->IsFinal())) {
        unionFind.Union(FindAliasElem(*ost1)->GetClassID(), FindAliasElem(*ost2)->GetClassID());
        break;
      }
    }
  }
}

// process the union among the pointed's of assignsets
void AliasClass::ApplyUnionForPointedTos() {
  for (auto *aliasElem : id2Elem) {
    if (aliasElem->GetAssignSet() == nullptr) {
      continue;
    }
    // apply union among the assignSet elements
    std::vector<OriginalSt*> nextLevelOsts;
    UpdateNextLevelNodes(nextLevelOsts, *aliasElem);
    UnionNodes(nextLevelOsts);
  }
}

void AliasClass::CollectRootIDOfNextLevelNodes(const OriginalSt &ost,
                                               std::set<unsigned int> &rootIDOfNADSs) {
  for (OriginalSt *nextLevelNode : *(GetAliasAnalysisTable()->GetNextLevelNodes(ost))) {
    if (!nextLevelNode->IsFinal()) {
      uint32 id = FindAliasElem(*nextLevelNode)->GetClassID();
      (void)rootIDOfNADSs.insert(unionFind.Root(id));
    }
  }
}

void AliasClass::UnionForNotAllDefsSeen() {
  std::set<unsigned int> rootIDOfNADSs;
  for (auto *aliasElem : id2Elem) {
    if (aliasElem->GetAssignSet() == nullptr) {
      if (aliasElem->IsNotAllDefsSeen() || aliasElem->IsNextLevNotAllDefsSeen()) {
        CollectRootIDOfNextLevelNodes(aliasElem->GetOriginalSt(), rootIDOfNADSs);
      }
      continue;
    }
    for (size_t elemIdA : *(aliasElem->GetAssignSet())) {
      AliasElem *aliasElemA = id2Elem[elemIdA];
      if (aliasElemA->IsNotAllDefsSeen() || aliasElemA->IsNextLevNotAllDefsSeen()) {
        for (unsigned int elemIdB : *(aliasElem->GetAssignSet())) {
          CollectRootIDOfNextLevelNodes(id2Elem[elemIdB]->GetOriginalSt(), rootIDOfNADSs);
        }
        break;
      }
    }
  }
  if (!rootIDOfNADSs.empty()) {
    unsigned int elemIdA = *(rootIDOfNADSs.begin());
    rootIDOfNADSs.erase(rootIDOfNADSs.begin());
    for (size_t elemIdB : rootIDOfNADSs) {
      unionFind.Union(elemIdA, elemIdB);
    }
    for (auto *aliasElem : id2Elem) {
      if (unionFind.Root(aliasElem->GetClassID()) == unionFind.Root(elemIdA)) {
        aliasElem->SetNotAllDefsSeen(true);
      }
    }
  }
}


// fabricate the imaginary not_all_def_seen AliasElem
AliasElem *AliasClass::FindOrCreateDummyNADSAe() {
  MIRSymbol *dummySym = mirModule.GetMIRBuilder()->GetOrCreateSymbol(TyIdx(PTY_i32), "__nads_dummysym__", kStVar,
                                                                     kScGlobal, nullptr, kScopeGlobal, false);
  ASSERT_NOT_NULL(dummySym);
  dummySym->SetIsTmp(true);
  dummySym->SetIsDeleted();
  OriginalSt *dummyOst = ssaTab.GetOriginalStTable().CreateSymbolOriginalSt(*dummySym, 0, 0);
  ssaTab.GetVersionStTable().FindOrCreateVersionSt(dummyOst, kInitVersion);
  if (osym2Elem.size() == dummyOst->GetIndex()) {
    AliasElem *dummyAe = acMemPool.New<AliasElem>(osym2Elem.size(), *dummyOst);
    dummyAe->SetNotAllDefsSeen(true);
    id2Elem.push_back(dummyAe);
    osym2Elem.push_back(dummyAe);
    unionFind.NewMember();
  }
  return osym2Elem[dummyOst->GetIndex()];
}

// TBAA
// Collect the alias groups. Each alias group is a map that maps the rootId to the ids aliasing with the root.
void AliasClass::CollectAliasGroups(std::map<unsigned int, std::set<unsigned int>> &aliasGroups) {
  // key is the root id. The set contains ids of aes that alias with the root.
  for (AliasElem *aliasElem : id2Elem) {
    unsigned int id = aliasElem->GetClassID();
    unsigned int rootID = unionFind.Root(id);
    if (id == rootID) {
      continue;
    }

    if (aliasGroups.find(rootID) == aliasGroups.end()) {
      std::set<unsigned int> idsAliasWithRoot;
      (void)aliasGroups.insert(make_pair(rootID, idsAliasWithRoot));
    }
    (void)aliasGroups[rootID].insert(id);
  }
}

bool AliasClass::AliasAccordingToType(TyIdx tyIdxA, TyIdx tyIdxB) {
  MIRType *mirTypeA = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdxA);
  MIRType *mirTypeB = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdxB);
  if (mirTypeA == mirTypeB || mirTypeA == nullptr || mirTypeB == nullptr) {
    return true;
  }
  if (mirTypeA->GetKind() != mirTypeB->GetKind()) {
    return false;
  }
  switch (mirTypeA->GetKind()) {
    case kTypeScalar: {
      return (mirTypeA->GetPrimType() == mirTypeB->GetPrimType());
    }
    case kTypeClass: {
      Klass *klassA = klassHierarchy->GetKlassFromTyIdx(mirTypeA->GetTypeIndex());
      CHECK_NULL_FATAL(klassA);
      Klass *klassB = klassHierarchy->GetKlassFromTyIdx(mirTypeB->GetTypeIndex());
      CHECK_NULL_FATAL(klassB);
      return (klassA == klassB || klassA->GetKlassName() == namemangler::kJavaLangObjectStr ||
              klassB->GetKlassName() == namemangler::kJavaLangObjectStr ||
              klassHierarchy->IsSuperKlass(klassA, klassB) || klassHierarchy->IsSuperKlass(klassB, klassA));
    }
    case kTypePointer: {
      auto *pointedTypeA = (static_cast<MIRPtrType*>(mirTypeA))->GetPointedType();
      auto *pointedTypeB = (static_cast<MIRPtrType*>(mirTypeB))->GetPointedType();
      return AliasAccordingToType(pointedTypeA->GetTypeIndex(), pointedTypeB->GetTypeIndex());
    }
    case kTypeJArray: {
      auto *mirJarrayTypeA = static_cast<MIRJarrayType*>(mirTypeA);
      auto *mirJarrayTypeB = static_cast<MIRJarrayType*>(mirTypeB);
      return AliasAccordingToType(mirJarrayTypeA->GetElemTyIdx(), mirJarrayTypeB->GetElemTyIdx());
    }
    default:
      return true;
  }
}

int AliasClass::GetOffset(const Klass &super, const Klass &base) const {
  int offset = 0;
  const Klass *superPtr = &super;
  const Klass *basePtr = &base;
  while (basePtr != superPtr) {
    basePtr = basePtr->GetSuperKlass();
    ASSERT_NOT_NULL(basePtr);
    ++offset;
  }
  return offset;
}

bool AliasClass::AliasAccordingToFieldID(const OriginalSt &ostA, const OriginalSt &ostB) {
  if (ostA.GetFieldID() == 0 || ostB.GetFieldID() == 0) {
    return true;
  }
  MIRType *mirTypeA =
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetAliasAnalysisTable()->GetPrevLevelNode(ostA)->GetTyIdx());
  MIRType *mirTypeB =
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetAliasAnalysisTable()->GetPrevLevelNode(ostB)->GetTyIdx());
  TyIdx idxA = mirTypeA->GetTypeIndex();
  TyIdx idxB = mirTypeB->GetTypeIndex();
  FieldID fldA = ostA.GetFieldID();
  if (idxA != idxB) {
    if (!(klassHierarchy->UpdateFieldID(idxA, idxB, fldA))) {
      return false;
    }
  }
  return fldA == ostB.GetFieldID();
}

void AliasClass::ProcessIdsAliasWithRoot(const std::set<unsigned int> &idsAliasWithRoot,
                                         std::vector<unsigned int> &newGroups) {
  for (unsigned int idA : idsAliasWithRoot) {
    bool unioned = false;
    for (unsigned int idB : newGroups) {
      OriginalSt &ostA = id2Elem[idA]->GetOriginalSt();
      OriginalSt &ostB = id2Elem[idB]->GetOriginalSt();
      if (AliasAccordingToType(GetAliasAnalysisTable()->GetPrevLevelNode(ostA)->GetTyIdx(),
                               GetAliasAnalysisTable()->GetPrevLevelNode(ostB)->GetTyIdx()) &&
          AliasAccordingToFieldID(ostA, ostB)) {
        unionFind.Union(idA, idB);
        unioned = true;
        break;
      }
    }
    if (!unioned) {
      newGroups.push_back(idA);
    }
  }
}

void AliasClass::ReconstructAliasGroups() {
  // map the root id to the set contains the aliasElem-id that alias with the root.
  std::map<unsigned int, std::set<unsigned int>> aliasGroups;
  CollectAliasGroups(aliasGroups);
  unionFind.Reinit();
  // kv.first is the root id. kv.second is the id the alias with the root.
  for (auto oneGroup : aliasGroups) {
    std::vector<unsigned int> newGroups;  // contains one id of each new alias group.
    unsigned int rootId = oneGroup.first;
    std::set<unsigned int> idsAliasWithRoot = oneGroup.second;
    newGroups.push_back(rootId);
    ProcessIdsAliasWithRoot(idsAliasWithRoot, newGroups);
  }
}

void AliasClass::CollectNotAllDefsSeenAes() {
  for (AliasElem *aliasElem : id2Elem) {
    if (aliasElem->IsNotAllDefsSeen() && aliasElem->GetClassID() == unionFind.Root(aliasElem->GetClassID())) {
      notAllDefsSeenClassSetRoots.push_back(aliasElem);
    }
  }
}

void AliasClass::CreateClassSets() {
  // iterate through all the alias elems
  for (AliasElem *aliasElem : id2Elem) {
    unsigned int id = aliasElem->GetClassID();
    unsigned int rootID = unionFind.Root(id);
    if (unionFind.GetElementsNumber(rootID) > 1) {
      if (id2Elem[rootID]->GetClassSet() == nullptr) {
        id2Elem[rootID]->classSet = acMemPool.New<MapleSet<unsigned int>>(acAlloc.Adapter());
      }
      aliasElem->classSet = id2Elem[rootID]->classSet;
      aliasElem->AddClassToSet(id);
    }
  }
  CollectNotAllDefsSeenAes();
#if DEBUG
  for (AliasElem *aliasElem : id2Elem) {
    if (aliasElem->GetClassSet() != nullptr && aliasElem->IsNotAllDefsSeen() == false &&
        unionFind.Root(aliasElem->GetClassID()) == aliasElem->GetClassID()) {
      ASSERT(aliasElem->GetClassSet()->size() == unionFind.GetElementsNumber(aliasElem->GetClassID()),
             "AliasClass::CreateClassSets: wrong result");
    }
  }
#endif
}

void AliasElem::Dump() const {
  ost.Dump();
  LogInfo::MapleLogger() << "id" << id << ((notAllDefsSeen) ? "? " : " ");
}

void AliasClass::DumpClassSets() {
  LogInfo::MapleLogger() << "/////// class sets ///////\n";
  for (AliasElem *aliaselem : id2Elem) {
    if (unionFind.Root(aliaselem->GetClassID()) != aliaselem->GetClassID()) {
      continue;
    }

    if (aliaselem->GetClassSet() == nullptr) {
      LogInfo::MapleLogger() << "Alone: ";
      aliaselem->Dump();
      LogInfo::MapleLogger() << '\n';
    } else {
      LogInfo::MapleLogger() << "Members of alias class " << aliaselem->GetClassID() << ": ";
      for (unsigned int elemID : *(aliaselem->GetClassSet())) {
        id2Elem[elemID]->Dump();
      }
      LogInfo::MapleLogger() << '\n';
    }
  }
}

// here starts pass 2 code
void AliasClass::InsertMayUseExpr(BaseNode &expr) {
  for (size_t i = 0; i < expr.NumOpnds(); ++i) {
    InsertMayUseExpr(*expr.Opnd(i));
  }
  if (expr.GetOpCode() != OP_iread) {
    return;
  }
  AliasElem *rhsAe = CreateAliasElemsExpr(expr);
  if (rhsAe == nullptr) {
    rhsAe = FindOrCreateDummyNADSAe();
  }
  auto &ireadNode = static_cast<IreadSSANode&>(expr);
  ireadNode.SetSSAVar(*ssaTab.GetVersionStTable().GetVersionStFromID(rhsAe->GetOriginalSt().GetZeroVersionIndex()));
  ASSERT(ireadNode.GetSSAVar() != nullptr, "AliasClass::InsertMayUseExpr(): iread cannot have empty mayuse");
}

// collect the mayUses caused by globalsAffectedByCalls.
void AliasClass::CollectMayUseFromGlobalsAffectedByCalls(std::set<OriginalSt*> &mayUseOsts) {
  for (unsigned int elemID : globalsAffectedByCalls) {
    (void)mayUseOsts.insert(&id2Elem[elemID]->GetOriginalSt());
  }
}

// collect the mayUses caused by not_all_def_seen_ae(NADS).
void AliasClass::CollectMayUseFromNADS(std::set<OriginalSt*> &mayUseOsts) {
  for (AliasElem *notAllDefsSeenAE : notAllDefsSeenClassSetRoots) {
    if (notAllDefsSeenAE->GetClassSet() == nullptr) {
      // single mayUse
      (void)mayUseOsts.insert(&notAllDefsSeenAE->GetOriginalSt());
    } else {
      for (unsigned int elemID : *(notAllDefsSeenAE->GetClassSet())) {
        AliasElem *aliasElem = id2Elem[elemID];
        (void)mayUseOsts.insert(&aliasElem->GetOriginalSt());
      }
    }
  }
}

// collect the mayUses caused by defined final field.
void AliasClass::CollectMayUseFromDefinedFinalField(std::set<OriginalSt*> &mayUseOsts) {
  for (AliasElem *aliasElem : id2Elem) {
    ASSERT(aliasElem != nullptr, "null ptr check");
    auto &ost = aliasElem->GetOriginalSt();
    if (!ost.IsFinal()) {
      continue;
    }

    auto *prevLevelOst = aliasAnalysisTable->GetPrevLevelNode(ost);
    if (prevLevelOst == nullptr) {
      continue;
    }

    TyIdx tyIdx = prevLevelOst->GetTyIdx();
    auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    ASSERT(ptrType->IsMIRPtrType(), "Type of pointer must be MIRPtrType!");
    tyIdx = static_cast<MIRPtrType*>(ptrType)->GetPointedTyIdx();
    if (tyIdx == mirModule.CurFunction()->GetClassTyIdx()) {
      (void)mayUseOsts.insert(&ost);
    }
  }
}

// insert the ost of mayUseOsts into mayUseNodes
void AliasClass::InsertMayUseNode(std::set<OriginalSt*> &mayUseOsts, TypeOfMayUseList &mayUseNodes) {
  for (OriginalSt *ost : mayUseOsts) {
    mayUseNodes.emplace_back(
        MayUseNode(ssaTab.GetVersionStTable().GetVersionStFromID(ost->GetZeroVersionIndex())));
  }
}

// insert mayUse for Return-statement.
// two kinds of mayUse's are insert into the mayUseNodes:
// 1. mayUses caused by not_all_def_seen_ae;
// 2. mayUses caused by globalsAffectedByCalls;
// 3. collect mayUses caused by defined final field for constructor.
void AliasClass::InsertMayUseReturn(const StmtNode &stmt) {
  std::set<OriginalSt*> mayUseOsts;
  // 1. collect mayUses caused by not_all_def_seen_ae.
  CollectMayUseFromNADS(mayUseOsts);
  // 2. collect mayUses caused by globals_affected_by_call.
  CollectMayUseFromGlobalsAffectedByCalls(mayUseOsts);
  // 3. collect mayUses caused by defined final field for constructor
  if (mirModule.CurFunction()->IsConstructor()) {
    CollectMayUseFromDefinedFinalField(mayUseOsts);
  }
  TypeOfMayUseList &mayUseNodes = ssaTab.GetStmtsSSAPart().GetMayUseNodesOf(stmt);
  InsertMayUseNode(mayUseOsts, mayUseNodes);
}

// collect next_level_nodes of the ost of ReturnOpnd into mayUseOsts
void AliasClass::CollectPtsToOfReturnOpnd(const OriginalSt &ost, std::set<OriginalSt*> &mayUseOsts) {
  for (OriginalSt *nextLevelOst : *(GetAliasAnalysisTable()->GetNextLevelNodes(ost))) {
    AliasElem *aliasElem = FindAliasElem(*nextLevelOst);
    if (!aliasElem->IsNotAllDefsSeen() && !aliasElem->GetOriginalSt().IsFinal()) {
      if (aliasElem->GetClassSet() == nullptr) {
        (void)mayUseOsts.insert(&aliasElem->GetOriginalSt());
      } else {
        for (unsigned int elemID : *(aliasElem->GetClassSet())) {
          (void)mayUseOsts.insert(&id2Elem[elemID]->GetOriginalSt());
        }
      }
    }
  }
}

// insert mayuses at a return stmt caused by its return operand being a pointer
void AliasClass::InsertReturnOpndMayUse(const StmtNode &stmt) {
  if (stmt.GetOpCode() == OP_return && stmt.NumOpnds() != 0) {
    // insert mayuses for the return operand's next level
    BaseNode *retValue = stmt.Opnd(0);
    AliasElem *aliasElem = CreateAliasElemsExpr(*retValue);
    if (IsPotentialAddress(retValue->GetPrimType()) && aliasElem != nullptr && !aliasElem->IsNextLevNotAllDefsSeen() &&
        !(retValue->GetOpCode() == OP_addrof && IsReadOnlyOst(aliasElem->GetOriginalSt()))) {
      std::set<OriginalSt*> mayUseOsts;
      if (aliasElem->GetAssignSet() == nullptr) {
        CollectPtsToOfReturnOpnd(aliasElem->GetOriginalSt(), mayUseOsts);
      } else {
        for (unsigned int elemID : *(aliasElem->GetAssignSet())) {
          CollectPtsToOfReturnOpnd(id2Elem[elemID]->GetOriginalSt(), mayUseOsts);
        }
      }
      // insert mayUses
      TypeOfMayUseList &mayUseNodes = ssaTab.GetStmtsSSAPart().GetMayUseNodesOf(stmt);
      InsertMayUseNode(mayUseOsts, mayUseNodes);
    }
  }
}

void AliasClass::InsertMayUseAll(const StmtNode &stmt) {
  TypeOfMayUseList &mayUseNodes = ssaTab.GetStmtsSSAPart().GetMayUseNodesOf(stmt);
  for (AliasElem *aliasElem : id2Elem) {
    if (aliasElem->GetOriginalSt().GetIndirectLev() >= 0 && !aliasElem->GetOriginalSt().IsPregOst()) {
      mayUseNodes.emplace_back(
          MayUseNode(ssaTab.GetVersionStTable().GetVersionStFromID(aliasElem->GetOriginalSt().GetZeroVersionIndex())));
    }
  }
}

void AliasClass::CollectMayDefForDassign(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts) {
  AliasElem *lhsAe = osym2Elem.at(ssaTab.GetStmtsSSAPart().GetAssignedVarOf(stmt)->GetOrigIdx());
  FieldID fldIDA = lhsAe->GetOriginalSt().GetFieldID();
  ASSERT(lhsAe != nullptr, "aliaselem of lhs should not be null");
  if (lhsAe->GetClassSet() != nullptr) {
    for (unsigned int elemID : *(lhsAe->GetClassSet())) {
      if (elemID != lhsAe->GetClassID()) {
        OriginalSt &ostOfAliasAE = id2Elem[elemID]->GetOriginalSt();
        FieldID fldIDB = ostOfAliasAE.GetFieldID();
        if (fldIDA == fldIDB || fldIDA == 0 || fldIDB == 0) {
          (void)mayDefOsts.insert(&ostOfAliasAE);
        }
      }
    }
  }
}

void AliasClass::InsertMayDefNode(std::set<OriginalSt*> &mayDefOsts, TypeOfMayDefList &mayDefNodes,
                                  StmtNode &stmt) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    mayDefNodes.emplace_back(
        MayDefNode(ssaTab.GetVersionStTable().GetVersionStFromID(mayDefOst->GetZeroVersionIndex()), &stmt));
  }
}

void AliasClass::InsertMayDefDassign(StmtNode &stmt) {
  std::set<OriginalSt*> mayDefOsts;
  CollectMayDefForDassign(stmt, mayDefOsts);
  TypeOfMayDefList &mayDefNodes = ssaTab.GetStmtsSSAPart().GetMayDefNodesOf(stmt);
  InsertMayDefNode(mayDefOsts, mayDefNodes, stmt);
}

bool AliasClass::IsEquivalentField(TyIdx tyIdxA, FieldID fldA, TyIdx tyIdxB, FieldID fldB) const {
  if (tyIdxA != tyIdxB) {
    (void)klassHierarchy->UpdateFieldID(tyIdxA, tyIdxB, fldA);
  }
  return fldA == fldB;
}

void AliasClass::CollectMayDefForIassign(StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts) {
  auto &iassignNode = static_cast<IassignNode&>(stmt);
  AliasElem *baseAe = CreateAliasElemsExpr(*iassignNode.Opnd(0));
  AliasElem *lhsAe = nullptr;
  if (baseAe != nullptr) {
    // get the next-level-ost that will be assigned to
    OriginalSt *lhsOst = nullptr;
    TyIdx tyIdxOfIass = iassignNode.GetTyIdx();
    FieldID fldOfIass = iassignNode.GetFieldID();
    OriginalSt &ostOfBaseExpr = baseAe->GetOriginalSt();
    TyIdx tyIdxOfBaseOSt = ostOfBaseExpr.GetTyIdx();
    for (OriginalSt *nextLevelNode : *(GetAliasAnalysisTable()->GetNextLevelNodes(ostOfBaseExpr))) {
      FieldID fldOfNextLevelOSt = nextLevelNode->GetFieldID();
      if (IsEquivalentField(tyIdxOfIass, fldOfIass, tyIdxOfBaseOSt, fldOfNextLevelOSt)) {
        lhsOst = nextLevelNode;
        break;
      }
    }
    CHECK_FATAL(lhsOst != nullptr, "AliasClass::InsertMayUseExpr: cannot find next level ost");
    lhsAe = osym2Elem[lhsOst->GetIndex()];
  } else {
    lhsAe = FindOrCreateDummyNADSAe();
  }
  // lhsAe does not alias with any aliasElem
  if (lhsAe->GetClassSet() == nullptr) {
    (void)mayDefOsts.insert(&lhsAe->GetOriginalSt());
    return;
  }
  for (unsigned int elemID : *(lhsAe->GetClassSet())) {
    AliasElem *aliasElem = id2Elem[elemID];
    OriginalSt &ostOfAliasAE = aliasElem->GetOriginalSt();
    (void)mayDefOsts.insert(&ostOfAliasAE);
  }
}

void AliasClass::InsertMayDefNodeExcludeFinalOst(std::set<OriginalSt*> &mayDefOsts,
                                                 TypeOfMayDefList &mayDefNodes, StmtNode &stmt) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    if (!mayDefOst->IsFinal()) {
      mayDefNodes.emplace_back(
          MayDefNode(ssaTab.GetVersionStTable().GetVersionStFromID(mayDefOst->GetZeroVersionIndex()), &stmt));
    }
  }
}

void AliasClass::InsertMayDefIassign(StmtNode &stmt) {
  std::set<OriginalSt*> mayDefOsts;
  CollectMayDefForIassign(stmt, mayDefOsts);
  TypeOfMayDefList &mayDefNodes = ssaTab.GetStmtsSSAPart().GetMayDefNodesOf(stmt);
  if (mayDefOsts.size() == 1) {
    InsertMayDefNode(mayDefOsts, mayDefNodes, stmt);
  } else {
    InsertMayDefNodeExcludeFinalOst(mayDefOsts, mayDefNodes, stmt);
  }
  ASSERT(!mayDefNodes.empty(), "AliasClass::InsertMayUseIassign(): iassign cannot have empty maydef");
}

void AliasClass::InsertMayDefUseSyncOps(StmtNode &stmt) {
  std::set<unsigned int> aliasSet;
  // collect the full alias set first
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    BaseNode *addrBase = stmt.Opnd(i);
    if (addrBase->IsSSANode()) {
      OriginalSt *oSt = static_cast<SSANode*>(addrBase)->GetSSAVar()->GetOst();
      if (addrBase->GetOpCode() == OP_addrof) {
        AliasElem *opndAE = osym2Elem[oSt->GetIndex()];
        if (opndAE->GetClassSet() != nullptr) {
          aliasSet.insert(opndAE->GetClassSet()->cbegin(), opndAE->GetClassSet()->cend());
        }
      } else {
        for (OriginalSt *nextLevelOst : *(GetAliasAnalysisTable()->GetNextLevelNodes(*oSt))) {
          AliasElem *opndAE = osym2Elem[nextLevelOst->GetIndex()];
          if (opndAE->GetClassSet() != nullptr) {
            aliasSet.insert(opndAE->GetClassSet()->cbegin(), opndAE->GetClassSet()->cend());
          }
        }
      }
    } else {
      for (AliasElem *notAllDefsSeenAE : notAllDefsSeenClassSetRoots) {
        if (notAllDefsSeenAE->GetClassSet() != nullptr) {
          aliasSet.insert(notAllDefsSeenAE->GetClassSet()->cbegin(), notAllDefsSeenAE->GetClassSet()->cend());
        } else {
          (void)aliasSet.insert(notAllDefsSeenAE->GetClassID());
        }
      }
    }
  }
  // do the insertion according to aliasSet
  AccessSSANodes *theSSAPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  for (unsigned int elemID : aliasSet) {
    AliasElem *aliasElem = id2Elem[elemID];
    OriginalSt &ostOfAliasAE = aliasElem->GetOriginalSt();
    if (!ostOfAliasAE.IsFinal()) {
      VersionSt *vst0 = ssaTab.GetVersionStTable().GetVersionStFromID(ostOfAliasAE.GetZeroVersionIndex());
      theSSAPart->GetMayUseNodes().emplace_back(MayUseNode(vst0));
      theSSAPart->GetMayDefNodes().emplace_back(MayDefNode(vst0, &stmt));
    }
  }
}

// collect mayDefs caused by mustDefs
void AliasClass::CollectMayDefForMustDefs(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts) {
  MapleVector<MustDefNode> &mustDefs = ssaTab.GetStmtsSSAPart().GetMustDefNodesOf(stmt);
  for (MustDefNode &mustDef : mustDefs) {
    VersionSt *vst = mustDef.GetResult();
    OriginalSt *ost = vst->GetOst();
    AliasElem *lhsAe = osym2Elem[ost->GetIndex()];
    if (lhsAe->GetClassSet() == nullptr || lhsAe->IsNotAllDefsSeen()) {
      continue;
    }
    for (unsigned int elemID : *(lhsAe->GetClassSet())) {
      AliasElem *aliasElem = id2Elem[elemID];
      if (elemID != lhsAe->GetClassID() &&
          aliasElem->GetOriginalSt().GetTyIdx() == lhsAe->GetOriginalSt().GetMIRSymbol()->GetTyIdx()) {
        (void)mayDefOsts.insert(&aliasElem->GetOriginalSt());
      }
    }
  }
}

void AliasClass::CollectMayUseForCallOpnd(const StmtNode &stmt, std::set<OriginalSt*> &mayUseOsts) {
  size_t opndId = kOpcodeInfo.IsICall(stmt.GetOpCode()) ? 1 : 0;
  for (; opndId < stmt.NumOpnds(); ++opndId) {
    BaseNode *expr = stmt.Opnd(opndId);
    if (!IsPotentialAddress(expr->GetPrimType())) {
      continue;
    }

    AliasElem *aliasElem = CreateAliasElemsExpr(*expr);
    if (aliasElem == nullptr || aliasElem->IsNextLevNotAllDefsSeen()) {
      continue;
    }

    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(aliasElem->GetOriginalSt().GetTyIdx())->PointsToConstString()) {
      continue;
    }

    for (OriginalSt *nextLevelOst : *(GetAliasAnalysisTable()->GetNextLevelNodes(aliasElem->GetOriginalSt()))) {
      AliasElem *indAe = FindAliasElem(*nextLevelOst);

      if (indAe->GetOriginalSt().IsFinal()) {
        // only final fields pointed to by the first opnd(this) are considered.
        if (opndId != 0) {
          continue;
        }

        auto *callerFunc = mirModule.CurFunction();
        if (!callerFunc->IsConstructor()) {
          continue;
        }

        PUIdx puIdx = static_cast<const CallNode&>(stmt).GetPUIdx();
        auto *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
        if (!calleeFunc->IsConstructor()) {
          continue;
        }

        if (callerFunc->GetBaseClassNameStrIdx() != calleeFunc->GetBaseClassNameStrIdx()) {
          continue;
        }
      }

      if (indAe->GetClassSet() == nullptr) {
        (void)mayUseOsts.insert(&indAe->GetOriginalSt());
      } else {
        for (unsigned int elemID : *(indAe->GetClassSet())) {
          (void)mayUseOsts.insert(&id2Elem[elemID]->GetOriginalSt());
        }
      }
    }
  }
}

void AliasClass::InsertMayDefNodeForCall(std::set<OriginalSt*> &mayDefOsts, TypeOfMayDefList &mayDefNodes,
                                         StmtNode &stmt, bool hasNoPrivateDefEffect) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    if (!hasNoPrivateDefEffect || !mayDefOst->IsPrivate()) {
      mayDefNodes.emplace_back(
          MayDefNode(ssaTab.GetVersionStTable().GetVersionStFromID(mayDefOst->GetZeroVersionIndex()), &stmt));
    }
  }
}

// Insert mayDefs and mayUses for the callees.
// Four kinds of mayDefs and mayUses are inserted, which are caused by callee
// opnds, not_all_def_seen_ae, globalsAffectedByCalls, and mustDefs.
void AliasClass::InsertMayDefUseCall(StmtNode &stmt, bool hasSideEffect, bool hasNoPrivateDefEffect) {
  auto *theSSAPart = static_cast<MayDefMayUsePart*>(ssaTab.GetStmtsSSAPart().SSAPartOf(stmt));
  std::set<OriginalSt*> mayDefUseOstsA;
  // 1. collect mayDefs and mayUses caused by callee-opnds
  CollectMayUseForCallOpnd(stmt, mayDefUseOstsA);
  // 2. collect mayDefs and mayUses caused by not_all_def_seen_ae
  CollectMayUseFromNADS(mayDefUseOstsA);
  InsertMayUseNode(mayDefUseOstsA, theSSAPart->GetMayUseNodes());
  // insert may def node, if the callee has side-effect.
  if (hasSideEffect) {
    InsertMayDefNodeForCall(mayDefUseOstsA, theSSAPart->GetMayDefNodes(), stmt, hasNoPrivateDefEffect);
  }
  // 3. insert mayDefs and mayUses caused by globalsAffectedByCalls
  std::set<OriginalSt*> mayDefUseOstsB;
  CollectMayUseFromGlobalsAffectedByCalls(mayDefUseOstsB);
  InsertMayUseNode(mayDefUseOstsB, theSSAPart->GetMayUseNodes());
  // insert may def node, if the callee has side-effect.
  if (hasSideEffect) {
    InsertMayDefNodeExcludeFinalOst(mayDefUseOstsB, theSSAPart->GetMayDefNodes(), stmt);
    if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
      // 4. insert mayDefs caused by the mustDefs
      std::set<OriginalSt*> mayDefOstsC;
      CollectMayDefForMustDefs(stmt, mayDefOstsC);
      InsertMayDefNodeExcludeFinalOst(mayDefOstsC, theSSAPart->GetMayDefNodes(), stmt);
    }
  }
}

void AliasClass::InsertMayUseNodeExcludeFinalOst(const std::set<OriginalSt*> &mayUseOsts,
                                                 TypeOfMayUseList &mayUseNodes) {
  for (OriginalSt *mayUseOst : mayUseOsts) {
    if (!mayUseOst->IsFinal()) {
      mayUseNodes.emplace_back(
          MayUseNode(ssaTab.GetVersionStTable().GetVersionStFromID(mayUseOst->GetZeroVersionIndex())));
    }
  }
}

// Insert mayDefs and mayUses for intrinsiccall.
// Four kinds of mayDefs and mayUses are inserted, which are caused by callee
// opnds, not_all_def_seen_ae, globalsAffectedByCalls, and mustDefs.
void AliasClass::InsertMayDefUseIntrncall(StmtNode &stmt) {
  auto *theSSAPart = static_cast<MayDefMayUsePart*>(ssaTab.GetStmtsSSAPart().SSAPartOf(stmt));
  auto &intrinNode = static_cast<IntrinsiccallNode&>(stmt);
  IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrinNode.GetIntrinsic()];
  std::set<OriginalSt*> mayDefUseOsts;
  // 1. collect mayDefs and mayUses caused by not_all_defs_seen_ae
  CollectMayUseFromNADS(mayDefUseOsts);
  // 2. collect mayDefs and mayUses caused by globalsAffectedByCalls
  CollectMayUseFromGlobalsAffectedByCalls(mayDefUseOsts);
  InsertMayUseNodeExcludeFinalOst(mayDefUseOsts, theSSAPart->GetMayUseNodes());
  if (!intrinDesc->HasNoSideEffect() || calleeHasSideEffect) {
    InsertMayDefNodeExcludeFinalOst(mayDefUseOsts, theSSAPart->GetMayDefNodes(), stmt);
  }
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    // 3. insert maydefs caused by the mustdefs
    std::set<OriginalSt*> mayDefOsts;
    CollectMayDefForMustDefs(stmt, mayDefOsts);
    InsertMayDefNodeExcludeFinalOst(mayDefOsts, theSSAPart->GetMayDefNodes(), stmt);
  }
}

void AliasClass::InsertMayDefUseClinitCheck(IntrinsiccallNode &stmt) {
  TypeOfMayDefList &mayDefNodes = ssaTab.GetStmtsSSAPart().GetMayDefNodesOf(stmt);
  for (OStIdx ostIdx : globalsMayAffectedByClinitCheck) {
    AliasElem *aliasElem = osym2Elem[ostIdx];
    OriginalSt &ostOfAE = aliasElem->GetOriginalSt();
    std::string typeNameOfOst = ostOfAE.GetMIRSymbol()->GetName();
    std::string typeNameOfStmt = GlobalTables::GetTypeTable().GetTypeFromTyIdx(stmt.GetTyIdx())->GetName();
    if (typeNameOfOst.find(typeNameOfStmt) != std::string::npos) {
      mayDefNodes.emplace_back(
          MayDefNode(ssaTab.GetVersionStTable().GetVersionStFromID(ostOfAE.GetZeroVersionIndex()), &stmt));
    }
  }
}

void AliasClass::GenericInsertMayDefUse(StmtNode &stmt, BBId bbID) {
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    InsertMayUseExpr(*stmt.Opnd(i));
  }
  switch (stmt.GetOpCode()) {
    case OP_return: {
      InsertMayUseReturn(stmt);
      // insert mayuses caused by its return operand being a pointer
      InsertReturnOpndMayUse(stmt);
      return;
    }
    case OP_throw: {
      if (mirModule.GetSrcLang() != kSrcLangJs && lessThrowAlias) {
        ASSERT(GetBB(bbID) != nullptr, "GetBB(bbID) is nullptr in AliasClass::GenericInsertMayDefUse");
        if (!GetBB(bbID)->IsGoto()) {
          InsertMayUseReturn(stmt);
        }
        // if the throw is handled as goto, no alias consequence
      } else {
        InsertMayUseAll(stmt);
      }
      return;
    }
    case OP_gosub:
    case OP_retsub: {
      InsertMayUseAll(stmt);
      return;
    }
    case OP_call:
    case OP_callassigned: {
      InsertMayDefUseCall(stmt, CallHasSideEffect(static_cast<CallNode&>(stmt)),
                          CallHasNoPrivateDefEffect(static_cast<CallNode&>(stmt)));
      return;
    }
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_icallassigned:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_icall: {
      InsertMayDefUseCall(stmt, true, false);
      return;
    }
    case OP_intrinsiccallwithtype: {
      auto &intrnNode = static_cast<IntrinsiccallNode&>(stmt);
      if (intrnNode.GetIntrinsic() == INTRN_JAVA_CLINIT_CHECK) {
        InsertMayDefUseClinitCheck(intrnNode);
      }
      InsertMayDefUseIntrncall(stmt);
      return;
    }
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      InsertMayDefUseIntrncall(stmt);
      return;
    }
    case OP_maydassign:
    case OP_dassign: {
      InsertMayDefDassign(stmt);
      return;
    }
    case OP_iassign: {
      InsertMayDefIassign(stmt);
      return;
    }
    case OP_syncenter:
    case OP_syncexit: {
      InsertMayDefUseSyncOps(stmt);
      return;
    }
    default:
      return;
  }
}
}  // namespace maple
