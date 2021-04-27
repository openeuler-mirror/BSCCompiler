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

const std::set<std::string> kNoSideEffectWhiteList {
  "Landroid_2Fos_2FParcel_24ReadWriteHelper_3B_7CwriteString_7C_28Landroid_2Fos_2FParcel_3BLjava_2Flang_2FString_3B_29V",
};

inline bool IsReadOnlyOst(const OriginalSt &ost) {
  return ost.GetMIRSymbol()->HasAddrOfValues();
}

inline bool IsPotentialAddress(PrimType primType, const MIRModule *mirModule) {
  return IsAddress(primType) || IsPrimitiveDynType(primType) ||
         (primType == PTY_u64 && mirModule->IsCModule());
}

// return true if this expression opcode can result in a valid address
static bool OpCanFormAddress(Opcode op) {
  switch (op) {
    case OP_dread:
    case OP_regread:
    case OP_iread:
    case OP_ireadoff:
    case OP_ireadfpoff:
    case OP_ireadpcoff:
    case OP_addrof:
    case OP_addroffunc:
    case OP_addroflabel:
    case OP_addroffpc:
    case OP_iaddrof:
    case OP_constval:
    case OP_conststr:
    case OP_conststr16:
    case OP_alloca:
    case OP_malloc:
    case OP_add:
    case OP_sub:
    case OP_select:
    case OP_array:
    case OP_intrinsicop:
      return true;
    default: ;
  }
  return false;
}

inline bool IsNullOrDummySymbolOst(OriginalSt *ost) {
  if ((ost == nullptr) || (ost && ost->IsSymbolOst() && (ost->GetMIRSymbol()->GetName() == "__nads_dummysym__"))) {
    return true;
  }
  return false;
}

inline bool OriginalStIsAuto(OriginalSt *ost) {
  if (!ost->IsSymbolOst()) {
    return false;
  }
  MIRSymbol *sym = ost->GetMIRSymbol();
  return sym->GetStorageClass() == kScAuto || sym->GetStorageClass() == kScFormal;
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
  if (!hasAttr && attrKind == FUNCATTR_nosideeffect) {
    const std::string &funcName = callee->GetName();
    hasAttr = kNoSideEffectWhiteList.find(funcName) != kNoSideEffectWhiteList.end();
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
    if (sym->IsGlobal() && (mirModule.IsCModule() || (!sym->HasAddrOfValues() && !sym->GetIsTmp()))) {
      (void)globalsMayAffectedByClinitCheck.insert(ostIdx);
      if (!sym->IsReflectionClassInfo()) {
        if (!ost.IsFinal() || InConstructorLikeFunc()) {
          (void)globalsAffectedByCalls.insert(aliasElem->GetClassID());
          if (mirModule.IsCModule()) {
            aliasElem->SetNotAllDefsSeen(true);
          }
        }
        aliasElem->SetNextLevNotAllDefsSeen(true);
      }
    } else if (mirModule.IsCModule() &&
               (sym->GetStorageClass() == kScPstatic || sym->GetStorageClass() == kScFstatic)) {
      (void)globalsAffectedByCalls.insert(aliasElem->GetClassID());
      aliasElem->SetNotAllDefsSeen(true);
      aliasElem->SetNextLevNotAllDefsSeen(true);
    }
  }
  if (aliasElem->GetOriginalSt().IsFormal() || ost.GetIndirectLev() > 0) {
    aliasElem->SetNextLevNotAllDefsSeen(true);
  }
  if (ost.GetIndirectLev() > 1) {
    aliasElem->SetNotAllDefsSeen(true);
  }
  id2Elem.push_back(aliasElem);
  osym2Elem[ostIdx] = aliasElem;
  unionFind.NewMember();
  return aliasElem;
}

AliasElem *AliasClass::FindOrCreateExtraLevAliasElem(BaseNode &expr, TyIdx tyIdx, FieldID fieldId) {
  AliasInfo ainfo = CreateAliasElemsExpr(kOpcodeInfo.IsTypeCvt(expr.GetOpCode()) ? *expr.Opnd(0) : expr);
  if (!mirModule.IsCModule()) {
    if (ainfo.ae == nullptr) {
      return nullptr;
    }
  } else if (ainfo.ae == nullptr ||
             (fieldId && ainfo.ae->GetOriginalSt().GetIndirectLev() != -1 && ainfo.ae->GetOriginalSt().GetTyIdx() != tyIdx)) {
    return FindOrCreateDummyNADSAe();
  }
  OriginalSt *newOst = nullptr;
  if (mirModule.IsCModule() && ainfo.ae->GetOriginalSt().GetTyIdx() != tyIdx) {
    newOst = GetAliasAnalysisTable()->FindOrCreateExtraLevOriginalSt(
        ainfo.ae->GetOriginalSt(), ainfo.ae->GetOriginalSt().GetTyIdx(), 0);
  } else {
    newOst = GetAliasAnalysisTable()->FindOrCreateExtraLevOriginalSt(
        ainfo.ae->GetOriginalSt(), ainfo.fieldID ? ainfo.ae->GetOriginalSt().GetTyIdx() : tyIdx,
        fieldId + ainfo.fieldID);
  }
  CHECK_FATAL(newOst != nullptr, "null ptr check");
  if (newOst->GetIndex() == osym2Elem.size()) {
    osym2Elem.push_back(nullptr);
    ssaTab.GetVersionStTable().CreateZeroVersionSt(newOst);
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

AliasInfo AliasClass::CreateAliasElemsExpr(BaseNode &expr) {
  switch (expr.GetOpCode()) {
    case OP_addrof: {
      AddrofSSANode &addrof = static_cast<AddrofSSANode &>(expr);
      OriginalSt &oSt = *addrof.GetSSAVar()->GetOst();
      oSt.SetAddressTaken();
      FindOrCreateAliasElem(oSt);
      AliasElem *ae = &FindOrCreateAliasElemOfAddrofOSt(oSt);
      return AliasInfo(ae, addrof.GetFieldID());
    }
    case OP_dread: {
      OriginalSt &ost = *static_cast<AddrofSSANode&>(expr).GetSSAVar()->GetOst();
      AliasElem *ae = FindOrCreateAliasElem(ost);
      return AliasInfo(ae, 0);
    }
    case OP_regread: {
      OriginalSt &oSt = *static_cast<RegreadSSANode&>(expr).GetSSAVar()->GetOst();
      return (oSt.IsSpecialPreg()) ? AliasInfo() : AliasInfo(FindOrCreateAliasElem(oSt), 0);
    }
    case OP_iread: {
      auto &iread = static_cast<IreadSSANode&>(expr);
      return AliasInfo(
          FindOrCreateExtraLevAliasElem(utils::ToRef(iread.Opnd(0)), iread.GetTyIdx(), iread.GetFieldID()), 0);
    }
    case OP_iaddrof: {
      auto &iread = static_cast<IreadNode&>(expr);
      AliasElem *aliasElem = FindOrCreateExtraLevAliasElem(*iread.Opnd(0), iread.GetTyIdx(), 0);
      return AliasInfo(&FindOrCreateAliasElemOfAddrofOSt(aliasElem->GetOriginalSt()), iread.GetFieldID());
    }
    case OP_add:
    case OP_sub:
    case OP_array:
    case OP_retype: {
      for (size_t i = 1; i < expr.NumOpnds(); ++i) {
        (void)CreateAliasElemsExpr(*expr.Opnd(i));
      }
      return CreateAliasElemsExpr(*expr.Opnd(0));
    }
    case OP_select: {
      (void)CreateAliasElemsExpr(*expr.Opnd(0));
      AliasInfo ainfo = CreateAliasElemsExpr(*expr.Opnd(1));
      AliasInfo ainfo2 = CreateAliasElemsExpr(*expr.Opnd(2));
      if (!OpCanFormAddress(expr.Opnd(1)->GetOpCode()) || !OpCanFormAddress(expr.Opnd(2)->GetOpCode())) {
        break;
      }
      if (ainfo.ae == nullptr) {
        return ainfo2;
      }
      if (ainfo2.ae == nullptr) {
        return ainfo;
      }
      ApplyUnionForDassignCopy(*ainfo.ae, ainfo2.ae, *expr.Opnd(2));
      return ainfo;
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
        (void)CreateAliasElemsExpr(*expr.Opnd(i));
      }
  }
  return AliasInfo();
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
  if (!IsPotentialAddress(rhs.GetPrimType(), &mirModule) ||
      kOpcodeInfo.NotPure(rhs.GetOpCode()) ||
      HasMallocOpnd(&rhs) ||
      (rhs.GetOpCode() == OP_addrof && IsReadOnlyOst(rhsAe->GetOriginalSt()))) {
    return;
  }
  unionFind.Union(lhsAe.GetClassID(), rhsAe->GetClassID());
}

void AliasClass::SetPtrOpndNextLevNADS(const BaseNode &opnd, AliasElem *aliasElem, bool hasNoPrivateDefEffect) {
  if (IsPotentialAddress(opnd.GetPrimType(), &mirModule) && aliasElem != nullptr &&
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
    AliasInfo ainfo = CreateAliasElemsExpr(*opnd);
    SetPtrOpndNextLevNADS(*opnd, ainfo.ae, hasNoPrivateDefEffect);
  }
}

// based on ost1's extra level ost's, ensure corresponding ones exist for ost2
void AliasClass::CreateMirroringAliasElems(const OriginalSt *ost1, OriginalSt *ost2) {
  if (ost1->IsSameSymOrPreg(ost2)) {
    return;
  }
  MapleVector<OriginalSt *> *nextLevelNodes = GetAliasAnalysisTable()->GetNextLevelNodes(*ost1);
  MapleVector<OriginalSt *>::iterator it = nextLevelNodes->begin();
  for (; it != nextLevelNodes->end(); ++it) {
    OriginalSt *nextLevelOst1 = *it;
    AliasElem *ae1 = FindOrCreateAliasElem(*nextLevelOst1);
    OriginalSt *nextLevelOst2 = GetAliasAnalysisTable()->FindOrCreateExtraLevOriginalSt(
        *ost2, ost2->GetTyIdx(), nextLevelOst1->GetFieldID());
    if (nextLevelOst2->GetIndex() == osym2Elem.size()) {
      osym2Elem.push_back(nullptr);
      ssaTab.GetVersionStTable().CreateZeroVersionSt(nextLevelOst2);
    }
    AliasElem *ae2 = FindOrCreateAliasElem(*nextLevelOst2);
    unionFind.Union(ae1->GetClassID(), ae2->GetClassID());
    CreateMirroringAliasElems(nextLevelOst1, nextLevelOst2); // recursive call
  }
}

void AliasClass::ApplyUnionForCopies(StmtNode &stmt) {
  switch (stmt.GetOpCode()) {
    case OP_maydassign:
    case OP_dassign:
    case OP_regassign: {
      // RHS
      ASSERT_NOT_NULL(stmt.Opnd(0));
      AliasInfo rhsAinfo = CreateAliasElemsExpr(*stmt.Opnd(0));
      // LHS
      OriginalSt *ost = ssaTab.GetStmtsSSAPart().GetAssignedVarOf(stmt)->GetOst();
      AliasElem *lhsAe = FindOrCreateAliasElem(*ost);
      ASSERT_NOT_NULL(lhsAe);
      ApplyUnionForDassignCopy(*lhsAe, rhsAinfo.ae, *stmt.Opnd(0));
      // at p = x, if the next level of either side exists, create other
      // side's next level
      if (mirModule.IsCModule() && rhsAinfo.ae &&
          (lhsAe->GetOriginalSt().GetTyIdx() == rhsAinfo.ae->GetOriginalSt().GetTyIdx())) {
        CreateMirroringAliasElems(&rhsAinfo.ae->GetOriginalSt(), &lhsAe->GetOriginalSt());
        CreateMirroringAliasElems(&lhsAe->GetOriginalSt(), &rhsAinfo.ae->GetOriginalSt());
      }
      return;
    }
    case OP_iassign: {
      auto &iassignNode = static_cast<IassignNode&>(stmt);
      AliasInfo rhsAinfo = CreateAliasElemsExpr(*iassignNode.Opnd(1));
      AliasElem *lhsAliasElem = FindOrCreateExtraLevAliasElem(*iassignNode.Opnd(0), iassignNode.GetTyIdx(),
          iassignNode.GetFieldID());
      if (lhsAliasElem != nullptr) {
        ApplyUnionForDassignCopy(*lhsAliasElem, rhsAinfo.ae, *iassignNode.Opnd(1));
      }
      return;
    }
    case OP_throw: {
      AliasInfo ainfo = CreateAliasElemsExpr(*stmt.Opnd(0));
      SetPtrOpndNextLevNADS(*stmt.Opnd(0), ainfo.ae, false);
      return;
    }
    case OP_call:
    case OP_callassigned: {
      auto &call = static_cast<CallNode&>(stmt);
      ASSERT(call.GetPUIdx() < GlobalTables::GetFunctionTable().GetFuncTable().size(),
             "index out of range in AliasClass::ApplyUnionForCopies");
      if (mirModule.IsCModule() || CallHasSideEffect(call)) {
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
      }
      break;
    }
    default: ;
  }
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    CreateAliasElemsExpr(*stmt.Opnd(i));
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
// process the union among the pointed's of assignsets
void AliasClass::ApplyUnionForPointedTos() {
  // first, process nextLevNotAllDefsSeen for alias elems
  for (AliasElem *aliaselem : id2Elem) {
    if (aliaselem->IsNextLevNotAllDefsSeen()) {
      MapleVector<OriginalSt *> *nextLevelNodes = GetAliasAnalysisTable()->GetNextLevelNodes(aliaselem->GetOriginalSt());
      MapleVector<OriginalSt *>::iterator ostit = nextLevelNodes->begin();
      for (; ostit != nextLevelNodes->end(); ++ostit) {
        AliasElem *indae = FindAliasElem(**ostit);
        if (!indae->GetOriginalSt().IsFinal() && !indae->IsNotAllDefsSeen()) {
          indae->SetNotAllDefsSeen(true);
          indae->SetNextLevNotAllDefsSeen(true);
        }
      }
    }
  }
  // do one more time to ensure proper propagation
  for (AliasElem *aliaselem : id2Elem) {
    if (aliaselem->IsNextLevNotAllDefsSeen()) {
      MapleVector<OriginalSt *> *nextLevelNodes = GetAliasAnalysisTable()->GetNextLevelNodes(aliaselem->GetOriginalSt());
      MapleVector<OriginalSt *>::iterator ostit = nextLevelNodes->begin();
      for (; ostit != nextLevelNodes->end(); ++ostit) {
        AliasElem *indae = FindAliasElem(**ostit);
        if (!indae->GetOriginalSt().IsFinal() && !indae->IsNotAllDefsSeen()) {
          indae->SetNotAllDefsSeen(true);
          indae->SetNextLevNotAllDefsSeen(true);
        }
      }
    }
  }

  MapleSet<uint> tempset(std::less<uint>(), acAlloc.Adapter());
  for (AliasElem *aliaselem : id2Elem) {
    if (aliaselem->GetAssignSet() == nullptr) {
      continue;
    }

    // iterate through all the alias elems to check if any has indirectLev > 0
    // or if any has nextLevNotAllDefsSeen being true
    bool hasNextLevNotAllDefsSeen = false;
    for (auto setit = aliaselem->GetAssignSet()->begin(); setit != aliaselem->GetAssignSet()->end();
         ++setit) {
      AliasElem *ae0 = id2Elem[*setit];
      if (ae0->GetOriginalSt().GetIndirectLev() > 0 || ae0->IsNotAllDefsSeen() || ae0->IsNextLevNotAllDefsSeen()) {
        hasNextLevNotAllDefsSeen = true;
        break;
      }
    }
    if (hasNextLevNotAllDefsSeen) {
      // make all pointedto's in this assignSet notAllDefsSeen
      for (auto setit = aliaselem->GetAssignSet()->begin(); setit != aliaselem->GetAssignSet()->end();
           ++setit) {
        AliasElem *ae0 = id2Elem[*setit];
        MapleVector<OriginalSt *> *nextLevelNodes = GetAliasAnalysisTable()->GetNextLevelNodes(ae0->GetOriginalSt());
        MapleVector<OriginalSt *>::iterator ostit = nextLevelNodes->begin();
        for (; ostit != nextLevelNodes->end(); ++ostit) {
          AliasElem *indae = FindAliasElem(**ostit);
          if (!indae->GetOriginalSt().IsFinal()) {
            indae->SetNotAllDefsSeen(true);
          }
        }
      }
      continue;
    }

    // apply union among the assignSet elements
    tempset = *(aliaselem->GetAssignSet());
    do {
      // pick one alias element
      MapleSet<uint>::iterator pickit = tempset.begin();
      if (pickit == tempset.end()) {
        break;  // done processing all elements in assignSet
      }
      AliasElem *ae1 = id2Elem[*pickit];
      (void)tempset.erase(pickit);
      for (MapleSet<uint>::iterator setit = tempset.begin(); setit != tempset.end(); ++setit) {
        AliasElem *ae2 = id2Elem[*setit];
        MapleVector<OriginalSt *> *nextLevelNodes1 = GetAliasAnalysisTable()->GetNextLevelNodes(ae1->GetOriginalSt());
        MapleVector<OriginalSt *>::iterator ost1it = nextLevelNodes1->begin();
        for (; ost1it != nextLevelNodes1->end(); ++ost1it) {
          MapleVector<OriginalSt *> *nextLevelNodes2 = GetAliasAnalysisTable()->GetNextLevelNodes(ae2->GetOriginalSt());
          MapleVector<OriginalSt *>::iterator ost2it = nextLevelNodes2->begin();
          for (; ost2it != nextLevelNodes2->end(); ++ost2it) {
            bool hasFieldid0 = (*ost1it)->GetFieldID() == 0 || (*ost2it)->GetFieldID() == 0;
            if (((*ost1it)->GetFieldID() != (*ost2it)->GetFieldID()) && !hasFieldid0) {
              continue;
            }
            if (((*ost1it)->IsFinal() || (*ost2it)->IsFinal())) {
              continue;
            }
            AliasElem *indae1 = FindAliasElem(**ost1it);
            AliasElem *indae2 = FindAliasElem(**ost2it);
            unionFind.Union(indae1->GetClassID(), indae2->GetClassID());
          }
        }
      }
    } while (true);
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

void AliasClass::UnionForNotAllDefsSeenCLang() {
  std::vector<AliasElem *> notAllDefsSeenAes;
  for (AliasElem *ae : id2Elem) {
    if (ae->IsNotAllDefsSeen()) {
      notAllDefsSeenAes.push_back(ae);
    }
  }

  if (notAllDefsSeenAes.empty()) {
    return;
  }

  // notAllDefsSeenAe is the first notAllDefsSeen AliasElem.
  // Union notAllDefsSeenAe with the other notAllDefsSeen aes.
  AliasElem *notAllDefsSeenAe = notAllDefsSeenAes[0];
  (void)notAllDefsSeenAes.erase(notAllDefsSeenAes.begin());
  for (AliasElem *ae : notAllDefsSeenAes) {
    unionFind.Union(notAllDefsSeenAe->GetClassID(), ae->GetClassID());
  }

  uint rootIdOfNotAllDefsSeenAe = unionFind.Root(notAllDefsSeenAe->GetClassID());
  for (AliasElem *ae : id2Elem) {
    if (unionFind.Root(ae->GetClassID()) == rootIdOfNotAllDefsSeenAe) {
      ae->SetNotAllDefsSeen(true);
    }
  }

  // iterate through originalStTable; if the symbol (at level 0) is
  // notAllDefsSeen, then set the same for all its level 1 members; then
  // for each level 1 member of each symbol, if any is set notAllDefsSeen,
  // set all members at that level to same;
  for (uint32 i = 1; i < ssaTab.GetOriginalStTableSize(); i++) {
    OriginalSt *ost = ssaTab.GetOriginalStTable().GetOriginalStFromID(OStIdx(i));
    if (!ost->IsSymbolOst()) {
      continue;
    }
    AliasElem *ae = osym2Elem[ost->GetIndex()];
    if (ae == nullptr) {
      continue;
    }
    OriginalSt *ostOfAe = &(ae->GetOriginalSt());
    bool hasNotAllDefsSeen = ae->IsNotAllDefsSeen() || ostOfAe->GetMIRSymbol()->GetStorageClass() == kScFormal;
    if (!hasNotAllDefsSeen) {
      // see if any at level 1 has notAllDefsSeen set
      for (OriginalSt *nextLevelNode : *GetAliasAnalysisTable()->GetNextLevelNodes(*ost)) {
        ae = osym2Elem[nextLevelNode->GetIndex()];
        if (ae != nullptr && ae->IsNotAllDefsSeen()) {
          hasNotAllDefsSeen = true;
          break;
        }
      }
    }
    if (hasNotAllDefsSeen) {
      // set to true for all members at this level
      for (OriginalSt *nextLevelNode : *GetAliasAnalysisTable()->GetNextLevelNodes(*ost)) {
        AliasElem *nextLevAE = osym2Elem[nextLevelNode->GetIndex()];
        if (nextLevAE != nullptr) {
          nextLevAE->SetNotAllDefsSeen(true);
          unionFind.Union(notAllDefsSeenAe->GetClassID(), nextLevAE->GetClassID());
        }
      }
    }
  }
}

void AliasClass::UnionForAggAndFields() {
  // key: index of MIRSymbol; value: id of alias element.
  std::map<uint32, std::set<uint32>> symbol2AEs;

  // collect alias elements with same MIRSymbol, and the ost is zero level.
  for (auto *aliasElem : id2Elem) {
    OriginalSt &ost = aliasElem->GetOriginalSt();
    if (ost.GetIndirectLev() == 0 && ost.IsSymbolOst()) {
      (void)symbol2AEs[ost.GetMIRSymbol()->GetStIdx().FullIdx()].insert(aliasElem->GetClassID());
    }
  }

  // union alias elements of Agg(fieldID == 0) and fields(fieldID > 0).
  for (auto &sym2AE : symbol2AEs) {
    auto &aesWithSameSymbol = sym2AE.second;
    for (auto idA : aesWithSameSymbol) {
      if (id2Elem[idA]->GetOriginalSt().GetFieldID() == 0 && aesWithSameSymbol.size() > 1) {
        (void)aesWithSameSymbol.erase(idA);
        for (auto idB : aesWithSameSymbol) {
          unionFind.Union(idA, idB);
        }
        break;
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
  OriginalSt *dummyOst = ssaTab.GetOriginalStTable().FindOrCreateSymbolOriginalSt(*dummySym, 0, 0);
  ssaTab.GetVersionStTable().CreateZeroVersionSt(dummyOst);
  if (osym2Elem.size() > dummyOst->GetIndex() && osym2Elem[dummyOst->GetIndex()] != nullptr) {
    return osym2Elem[dummyOst->GetIndex()];
  } else {
    AliasElem *dummyAe = acMemPool.New<AliasElem>(id2Elem.size(), *dummyOst);
    dummyAe->SetNotAllDefsSeen(true);
    id2Elem.push_back(dummyAe);
    osym2Elem.push_back(dummyAe);
    unionFind.NewMember();
    return dummyAe;
  }
}

void AliasClass::UnionAllNodes(MapleVector<OriginalSt *> *nextLevOsts) {
  if (nextLevOsts->size() < 2) {
    return;
  }

  auto it = nextLevOsts->begin();
  OriginalSt *ostA = *it;
  AliasElem *aeA = FindAliasElem(*ostA);
  ++it;
  for (; it != nextLevOsts->end(); ++it) {
    OriginalSt *ostB = *it;
    AliasElem *aeB = FindAliasElem(*ostB);
    unionFind.Union(aeA->GetClassID(), aeB->GetClassID());
  }
}

// This is applicable only for C language.  For each ost that is a struct,
// union all fields within the same struct
void AliasClass::ApplyUnionForStorageOverlaps() {
  // iterate through all the alias elems
  for (AliasElem *ae : id2Elem) {
    OriginalSt *ost = &ae->GetOriginalSt();
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx());
    if (mirType->GetKind() != kTypePointer) {
      continue;
    }
    auto nextLevOsts = GetAliasAnalysisTable()->GetNextLevelNodes(*ost);
    if (nextLevOsts == nullptr) {
      continue;
    }

    MIRType *pointedType = static_cast<MIRPtrType *>(mirType)->GetPointedType();
    if (pointedType->GetKind() == kTypeUnion) {
      // union all fields of union
      UnionAllNodes(nextLevOsts);
      continue;
    }

    for (auto *nextLevOst : *nextLevOsts) {
      MIRType *nextLevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(nextLevOst->GetTyIdx());
      if (nextLevType->HasFields()) {
        // union all fields if one next-level-ost has fields
        UnionAllNodes(nextLevOsts);
        break;
      }
    }
  }
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
  AliasInfo rhsAinfo = CreateAliasElemsExpr(expr);
  if (rhsAinfo.ae == nullptr) {
    rhsAinfo.ae = FindOrCreateDummyNADSAe();
  }
  auto &ireadNode = static_cast<IreadSSANode&>(expr);
  ireadNode.SetSSAVar(*ssaTab.GetVersionStTable().GetZeroVersionSt(&rhsAinfo.ae->GetOriginalSt()));
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
      if (mirModule.IsCModule() || !IsNullOrDummySymbolOst(&notAllDefsSeenAE->GetOriginalSt())) {
        (void)mayUseOsts.insert(&notAllDefsSeenAE->GetOriginalSt());
      }
    } else {
      for (unsigned int elemID : *(notAllDefsSeenAE->GetClassSet())) {
        AliasElem *aliasElem = id2Elem[elemID];
        if (!mirModule.IsCModule() && aliasElem->GetOriginalSt().GetIndirectLev() == 0 &&
            OriginalStIsAuto(&aliasElem->GetOriginalSt())) {
          continue;
        }
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
        MayUseNode(ssaTab.GetVersionStTable().GetVersionStVectorItem(ost->GetZeroVersionIndex())));
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
    AliasInfo aInfo = CreateAliasElemsExpr(*retValue);
    if (IsPotentialAddress(retValue->GetPrimType(), &mirModule) && aInfo.ae != nullptr) {
      if (retValue->GetOpCode() == OP_addrof && IsReadOnlyOst(aInfo.ae->GetOriginalSt())) {
        return;
      }
      if (!aInfo.ae->IsNextLevNotAllDefsSeen()) {
        std::set<OriginalSt*> mayUseOsts;
        if (aInfo.ae->GetAssignSet() == nullptr) {
          CollectPtsToOfReturnOpnd(aInfo.ae->GetOriginalSt(), mayUseOsts);
        } else {
          for (unsigned int elemID : *(aInfo.ae->GetAssignSet())) {
            CollectPtsToOfReturnOpnd(id2Elem[elemID]->GetOriginalSt(), mayUseOsts);
          }
        }
        // insert mayUses
        TypeOfMayUseList &mayUseNodes = ssaTab.GetStmtsSSAPart().GetMayUseNodesOf(stmt);
        InsertMayUseNode(mayUseOsts, mayUseNodes);
      }
    }
  }
}

void AliasClass::InsertMayUseAll(const StmtNode &stmt) {
  TypeOfMayUseList &mayUseNodes = ssaTab.GetStmtsSSAPart().GetMayUseNodesOf(stmt);
  for (AliasElem *aliasElem : id2Elem) {
    if (aliasElem->GetOriginalSt().GetIndirectLev() >= 0 && !aliasElem->GetOriginalSt().IsPregOst()) {
      mayUseNodes.emplace_back(MayUseNode(
          ssaTab.GetVersionStTable().GetVersionStVectorItem(aliasElem->GetOriginalSt().GetZeroVersionIndex())));
    }
  }
}

void AliasClass::CollectMayDefForDassign(const StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts) {
  AliasElem *lhsAe = osym2Elem.at(ssaTab.GetStmtsSSAPart().GetAssignedVarOf(stmt)->GetOrigIdx());
  if (lhsAe->GetClassSet() == nullptr) {
    return;
  }

  OriginalSt *ostOfLhsAe = &lhsAe->GetOriginalSt();
  FieldID fldIDA = ostOfLhsAe->GetFieldID();
  MIRType *lhsAeType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ostOfLhsAe->GetTyIdx());
  MIRType *lhsSymType = ostOfLhsAe->GetMIRSymbol()->GetType();
  for (uint elemId : *(lhsAe->GetClassSet())) {
    if (elemId == lhsAe->GetClassID()) {
      continue;
    }
    OriginalSt *ostOfAliasAe = &id2Elem[elemId]->GetOriginalSt();
    FieldID fldIDB = ostOfAliasAe->GetFieldID();
    if (!mirModule.IsCModule()) {
      if (ostOfAliasAe->GetTyIdx() != ostOfLhsAe->GetTyIdx()) {
        continue;
      }
      if (fldIDA == fldIDB || fldIDA == 0 || fldIDB == 0) {
        (void)mayDefOsts.insert(ostOfAliasAe);
      }
    } else {
      if (ostOfAliasAe->IsSameSymOrPreg(ostOfLhsAe) &&
          fldIDA != fldIDB &&
          lhsSymType->GetKind() != kTypeUnion) {
        MIRType *aliasAeType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ostOfAliasAe->GetTyIdx());
        if (!lhsAeType->HasFields() && !aliasAeType->HasFields()) {
          continue;
        }
        if (lhsAeType->HasFields()) {
          if ((fldIDB < fldIDA) ||
              (fldIDB > (fldIDA + static_cast<int32>(lhsAeType->NumberOfFieldIDs())))) {
            if (!aliasAeType->HasFields()) {
              continue;
            }
          }
        }
        if (aliasAeType->HasFields()) {
          if (fldIDA < fldIDB ||
              fldIDA > (fldIDB + static_cast<int32>(aliasAeType->NumberOfFieldIDs()))) {
            if (!lhsAeType->HasFields()) {
              continue;
            }
          }
        }
      }
      (void)mayDefOsts.insert(ostOfAliasAe);
    }
  }
}

void AliasClass::InsertMayDefNode(std::set<OriginalSt*> &mayDefOsts, TypeOfMayDefList &mayDefNodes,
                                  StmtNode &stmt, BBId bbid) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    mayDefNodes.emplace_back(
        MayDefNode(ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
    ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
  }
}

void AliasClass::InsertMayDefDassign(StmtNode &stmt, BBId bbid) {
  std::set<OriginalSt*> mayDefOsts;
  CollectMayDefForDassign(stmt, mayDefOsts);
  TypeOfMayDefList &mayDefNodes = ssaTab.GetStmtsSSAPart().GetMayDefNodesOf(stmt);
  InsertMayDefNode(mayDefOsts, mayDefNodes, stmt, bbid);
}

bool AliasClass::IsEquivalentField(TyIdx tyIdxA, FieldID fldA, TyIdx tyIdxB, FieldID fldB) const {
  if (mirModule.IsJavaModule() && tyIdxA != tyIdxB) {
    (void)klassHierarchy->UpdateFieldID(tyIdxA, tyIdxB, fldA);
  }
  return fldA == fldB;
}

void AliasClass::CollectMayDefForIassign(StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts) {
  auto &iassignNode = static_cast<IassignNode&>(stmt);
  AliasInfo baseAinfo = CreateAliasElemsExpr(*iassignNode.Opnd(0));
  AliasElem *lhsAe = nullptr;
  if (baseAinfo.ae != nullptr &&
      (!mirModule.IsCModule() ||
       iassignNode.GetFieldID() == 0 ||
       baseAinfo.ae->GetOriginalSt().GetIndirectLev() == -1 ||
       baseAinfo.ae->GetOriginalSt().GetTyIdx() == iassignNode.GetTyIdx())) {
    // get the next-level-ost that will be assigned to
    FieldID fldOfIass = iassignNode.GetFieldID() + baseAinfo.fieldID;
    if (mirModule.IsCModule() &&
        baseAinfo.ae->GetOriginalSt().GetTyIdx() != iassignNode.GetTyIdx()) {
      fldOfIass = 0;
    }
    OriginalSt *lhsOst = nullptr;
    TyIdx tyIdxOfIass = iassignNode.GetTyIdx();
    OriginalSt &ostOfBaseExpr = baseAinfo.ae->GetOriginalSt();
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
  OriginalSt *ostOfLhsAe = &lhsAe->GetOriginalSt();
  TyIdx pointedTyIdx = ostOfLhsAe->GetTyIdx();
  MIRType *lhsAeType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointedTyIdx);
  for (unsigned int elemID : *(lhsAe->GetClassSet())) {
    AliasElem *aliasElem = id2Elem[elemID];
    OriginalSt &ostOfAliasAE = aliasElem->GetOriginalSt();
    if (!mirModule.IsCModule()) {
      if ((ostOfAliasAE.GetIndirectLev() == 0) && OriginalStIsAuto(&ostOfAliasAE)) {
        continue;
      }
      if (ostOfAliasAE.GetTyIdx() != pointedTyIdx && pointedTyIdx != 0) {
        continue;
      }
    } else {
      if (ostOfAliasAE.IsSameSymOrPreg(ostOfLhsAe) &&
          ostOfAliasAE.GetFieldID() != ostOfLhsAe->GetFieldID() &&
          ostOfAliasAE.GetFieldID() != 0 && ostOfLhsAe->GetFieldID() != 0) {
        MIRType *aliasAeType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ostOfAliasAE.GetTyIdx());
        if (!lhsAeType->HasFields() && !aliasAeType->HasFields()) {
          continue;
        }
        if (lhsAeType->HasFields()) {
          if ((ostOfAliasAE.GetFieldID() < ostOfLhsAe->GetFieldID()) ||
              (ostOfAliasAE.GetFieldID() >
               (ostOfLhsAe->GetFieldID() + static_cast<int32>(lhsAeType->NumberOfFieldIDs())))) {
            continue;
          }
        } else {
          if ((ostOfLhsAe->GetFieldID() < ostOfAliasAE.GetFieldID()) ||
              (ostOfLhsAe->GetFieldID() >
               (ostOfAliasAE.GetFieldID() + static_cast<int32>(aliasAeType->NumberOfFieldIDs())))) {
            continue;
          }
        }
      }
    }
    (void)mayDefOsts.insert(&ostOfAliasAE);
  }
}

void AliasClass::InsertMayDefNodeExcludeFinalOst(std::set<OriginalSt*> &mayDefOsts,
                                                 TypeOfMayDefList &mayDefNodes, StmtNode &stmt,
                                                 BBId bbid) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    if (!mayDefOst->IsFinal()) {
      mayDefNodes.emplace_back(
          MayDefNode(ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
      ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
    }
  }
}

void AliasClass::InsertMayDefIassign(StmtNode &stmt, BBId bbid) {
  std::set<OriginalSt*> mayDefOsts;
  CollectMayDefForIassign(stmt, mayDefOsts);
  TypeOfMayDefList &mayDefNodes = ssaTab.GetStmtsSSAPart().GetMayDefNodesOf(stmt);
  if (mayDefOsts.size() == 1) {
    InsertMayDefNode(mayDefOsts, mayDefNodes, stmt, bbid);
  } else {
    InsertMayDefNodeExcludeFinalOst(mayDefOsts, mayDefNodes, stmt, bbid);
  }
  ASSERT(!mayDefNodes.empty(), "AliasClass::InsertMayUseIassign(): iassign cannot have empty maydef");
  // go thru inserted MayDefNode to add the base info
  TypeOfMayDefList::iterator it = mayDefNodes.begin();
  for (; it != mayDefNodes.end(); ++it) {
    MayDefNode &mayDef = *it;
    OriginalSt *ost = mayDef.GetResult()->GetOst();
    if (ost->GetIndirectLev() == 1) {
      mayDef.base = ssaTab.GetVersionStTable().GetZeroVersionSt(GetAliasAnalysisTable()->GetPrevLevelNode(*ost));
    }
  }
}

void AliasClass::InsertMayDefUseSyncOps(StmtNode &stmt, BBId bbid) {
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
      VersionSt *vst0 = ssaTab.GetVersionStTable().GetVersionStVectorItem(ostOfAliasAE.GetZeroVersionIndex());
      theSSAPart->GetMayUseNodes().emplace_back(MayUseNode(vst0));
      theSSAPart->GetMayDefNodes().emplace_back(MayDefNode(vst0, &stmt));
      ssaTab.AddDefBB4Ost(ostOfAliasAE.GetIndex(), bbid);
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
      bool isNotAllDefsSeen = false;
      for (AliasElem *notAllDefsSeenAe : notAllDefsSeenClassSetRoots) {
        if (notAllDefsSeenAe->GetClassSet() == nullptr) {
          if (elemID == notAllDefsSeenAe->GetClassID()) {
            isNotAllDefsSeen = true;
            break;  // inserted already
          }
        } else if (notAllDefsSeenAe->GetClassSet()->find(elemID) != notAllDefsSeenAe->GetClassSet()->end()) {
          isNotAllDefsSeen = true;
          break;  // inserted already
        }
      }
      if (isNotAllDefsSeen) {
        continue;
      }
      AliasElem *aliasElem = id2Elem[elemID];
      if (elemID != lhsAe->GetClassID() &&
          aliasElem->GetOriginalSt().GetTyIdx() == lhsAe->GetOriginalSt().GetMIRSymbol()->GetTyIdx()) {
        (void)mayDefOsts.insert(&aliasElem->GetOriginalSt());
      }
    }
  }
}

void AliasClass::CollectMayUseForNextLevel(OriginalSt *ost, std::set<OriginalSt*> &mayUseOsts, const StmtNode &stmt, bool isFirstOpnd) {
  for (OriginalSt *nextLevelOst : *(GetAliasAnalysisTable()->GetNextLevelNodes(*ost))) {
    AliasElem *indAe = FindAliasElem(*nextLevelOst);

    if (indAe->GetOriginalSt().IsFinal()) {
      // only final fields pointed to by the first opnd(this) are considered.
      if (!isFirstOpnd) {
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
      CollectMayUseForNextLevel(&indAe->GetOriginalSt(), mayUseOsts, stmt, false);
    } else {
      for (unsigned int elemID : *(indAe->GetClassSet())) {
        OriginalSt *ost1 = &id2Elem[elemID]->GetOriginalSt();
        if (mayUseOsts.find(ost1) == mayUseOsts.end()) {
          (void)mayUseOsts.insert(ost1);
          CollectMayUseForNextLevel(ost1, mayUseOsts, stmt, false);
        }
      }
    }
  }
}

void AliasClass::CollectMayUseForCallOpnd(const StmtNode &stmt, std::set<OriginalSt*> &mayUseOsts) {
  size_t opndId = kOpcodeInfo.IsICall(stmt.GetOpCode()) ? 1 : 0;
  for (; opndId < stmt.NumOpnds(); ++opndId) {
    BaseNode *expr = stmt.Opnd(opndId);
    if (!IsPotentialAddress(expr->GetPrimType(), &mirModule)) {
      continue;
    }

    AliasInfo aInfo = CreateAliasElemsExpr(*expr);
    if (aInfo.ae == nullptr || 
        (aInfo.ae->IsNextLevNotAllDefsSeen() && aInfo.ae->GetOriginalSt().GetIndirectLev() > 0)) {
      continue;
    }

    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(aInfo.ae->GetOriginalSt().GetTyIdx())->PointsToConstString()) {
      continue;
    }

    CollectMayUseForNextLevel(&aInfo.ae->GetOriginalSt(), mayUseOsts, stmt, opndId == 0);
  }
}

void AliasClass::InsertMayDefNodeForCall(std::set<OriginalSt*> &mayDefOsts, TypeOfMayDefList &mayDefNodes,
                                         StmtNode &stmt, BBId bbid,
                                         bool hasNoPrivateDefEffect) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    if (!hasNoPrivateDefEffect || !mayDefOst->IsPrivate()) {
      mayDefNodes.emplace_back(
          MayDefNode(ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
      ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
    }
  }
}

// Insert mayDefs and mayUses for the callees.
// Four kinds of mayDefs and mayUses are inserted, which are caused by callee
// opnds, not_all_def_seen_ae, globalsAffectedByCalls, and mustDefs.
void AliasClass::InsertMayDefUseCall(StmtNode &stmt, BBId bbid, bool hasSideEffect, bool hasNoPrivateDefEffect) {
  auto *theSSAPart = static_cast<MayDefMayUsePart*>(ssaTab.GetStmtsSSAPart().SSAPartOf(stmt));
  std::set<OriginalSt*> mayDefUseOstsA;
  // 1. collect mayDefs and mayUses caused by callee-opnds
  CollectMayUseForCallOpnd(stmt, mayDefUseOstsA);
  // 2. collect mayDefs and mayUses caused by not_all_def_seen_ae
  CollectMayUseFromNADS(mayDefUseOstsA);
  InsertMayUseNode(mayDefUseOstsA, theSSAPart->GetMayUseNodes());
  // insert may def node, if the callee has side-effect.
  if (hasSideEffect) {
    InsertMayDefNodeForCall(mayDefUseOstsA, theSSAPart->GetMayDefNodes(), stmt, bbid, hasNoPrivateDefEffect);
  }
  // 3. insert mayDefs and mayUses caused by globalsAffectedByCalls
  std::set<OriginalSt*> mayDefUseOstsB;
  CollectMayUseFromGlobalsAffectedByCalls(mayDefUseOstsB);
  InsertMayUseNode(mayDefUseOstsB, theSSAPart->GetMayUseNodes());
  // insert may def node, if the callee has side-effect.
  if (hasSideEffect) {
    InsertMayDefNodeExcludeFinalOst(mayDefUseOstsB, theSSAPart->GetMayDefNodes(), stmt, bbid);
    if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
      // 4. insert mayDefs caused by the mustDefs
      std::set<OriginalSt*> mayDefOstsC;
      CollectMayDefForMustDefs(stmt, mayDefOstsC);
      InsertMayDefNodeExcludeFinalOst(mayDefOstsC, theSSAPart->GetMayDefNodes(), stmt, bbid);
    }
  }
}

void AliasClass::InsertMayUseNodeExcludeFinalOst(const std::set<OriginalSt*> &mayUseOsts,
                                                 TypeOfMayUseList &mayUseNodes) {
  for (OriginalSt *mayUseOst : mayUseOsts) {
    if (!mayUseOst->IsFinal()) {
      mayUseNodes.emplace_back(
          MayUseNode(ssaTab.GetVersionStTable().GetVersionStVectorItem(mayUseOst->GetZeroVersionIndex())));
    }
  }
}

// Insert mayDefs and mayUses for intrinsiccall.
// Four kinds of mayDefs and mayUses are inserted, which are caused by callee
// opnds, not_all_def_seen_ae, globalsAffectedByCalls, and mustDefs.
void AliasClass::InsertMayDefUseIntrncall(StmtNode &stmt, BBId bbid) {
  auto *theSSAPart = static_cast<MayDefMayUsePart*>(ssaTab.GetStmtsSSAPart().SSAPartOf(stmt));
  auto &intrinNode = static_cast<IntrinsiccallNode&>(stmt);
  IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrinNode.GetIntrinsic()];
  std::set<OriginalSt*> mayDefUseOsts;
  // 1. collect mayDefs and mayUses caused by not_all_defs_seen_ae
  if (!mirModule.IsCModule()) {
    for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
      InsertMayUseExpr(*stmt.Opnd(i));
    }
  }
  // 2. collect mayDefs and mayUses caused by not_all_defs_seen_ae
  CollectMayUseFromNADS(mayDefUseOsts);
  // 3. collect mayDefs and mayUses caused by globalsAffectedByCalls
  CollectMayUseFromGlobalsAffectedByCalls(mayDefUseOsts);
  InsertMayUseNodeExcludeFinalOst(mayDefUseOsts, theSSAPart->GetMayUseNodes());
  if (!intrinDesc->HasNoSideEffect() || calleeHasSideEffect) {
    InsertMayDefNodeExcludeFinalOst(mayDefUseOsts, theSSAPart->GetMayDefNodes(), stmt, bbid);
  }
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    // 4. insert maydefs caused by the mustdefs
    std::set<OriginalSt*> mayDefOsts;
    CollectMayDefForMustDefs(stmt, mayDefOsts);
    InsertMayDefNodeExcludeFinalOst(mayDefOsts, theSSAPart->GetMayDefNodes(), stmt, bbid);
  }
}

void AliasClass::InsertMayDefUseClinitCheck(IntrinsiccallNode &stmt, BBId bbid) {
  TypeOfMayDefList &mayDefNodes = ssaTab.GetStmtsSSAPart().GetMayDefNodesOf(stmt);
  for (OStIdx ostIdx : globalsMayAffectedByClinitCheck) {
    AliasElem *aliasElem = osym2Elem[ostIdx];
    OriginalSt &ostOfAE = aliasElem->GetOriginalSt();
    std::string typeNameOfOst = ostOfAE.GetMIRSymbol()->GetName();
    std::string typeNameOfStmt = GlobalTables::GetTypeTable().GetTypeFromTyIdx(stmt.GetTyIdx())->GetName();
    if (typeNameOfOst.find(typeNameOfStmt) != std::string::npos) {
      mayDefNodes.emplace_back(
          MayDefNode(ssaTab.GetVersionStTable().GetVersionStVectorItem(ostOfAE.GetZeroVersionIndex()), &stmt));
      ssaTab.AddDefBB4Ost(ostOfAE.GetIndex(), bbid);
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
      InsertMayDefUseCall(stmt, bbID, CallHasSideEffect(static_cast<CallNode&>(stmt)),
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
      InsertMayDefUseCall(stmt, bbID, true, false);
      return;
    }
    case OP_intrinsiccallwithtype: {
      auto &intrnNode = static_cast<IntrinsiccallNode&>(stmt);
      if (intrnNode.GetIntrinsic() == INTRN_JAVA_CLINIT_CHECK) {
        InsertMayDefUseClinitCheck(intrnNode, bbID);
      }
      InsertMayDefUseIntrncall(stmt, bbID);
      return;
    }
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      InsertMayDefUseIntrncall(stmt, bbID);
      return;
    }
    case OP_maydassign:
    case OP_dassign: {
      InsertMayDefDassign(stmt, bbID);
      return;
    }
    case OP_iassign: {
      InsertMayDefIassign(stmt, bbID);
      return;
    }
    case OP_syncenter:
    case OP_syncexit: {
      InsertMayDefUseSyncOps(stmt, bbID);
      return;
    }
    default:
      return;
  }
}
}  // namespace maple
