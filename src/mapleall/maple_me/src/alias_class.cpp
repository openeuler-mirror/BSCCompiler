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
#include "alias_class.h"
#include "mpl_logging.h"
#include "opcode_info.h"
#include "ssa_mir_nodes.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "ipa_side_effect.h"

namespace {
using namespace maple;

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

inline bool IsNullOrDummySymbolOst(const OriginalSt *ost) {
  if ((ost == nullptr) || (ost && ost->IsSymbolOst() && (ost->GetMIRSymbol()->GetName() == "__nads_dummysym__"))) {
    return true;
  }
  return false;
}

inline bool OriginalStIsAuto(const OriginalSt *ost) {
  if (!ost->IsSymbolOst()) {
    return false;
  }
  MIRSymbol *sym = ost->GetMIRSymbol();
  return sym->GetStorageClass() == kScAuto || sym->GetStorageClass() == kScFormal;
}

// if epxr is type convertion, get the expr before convertion
// expr op like cvt/retype/floor/round/ceil/trunc is type convertion
inline BaseNode *RemoveTypeConversionIfExist(BaseNode *expr) {
  while (kOpcodeInfo.IsTypeCvt(expr->GetOpCode())) {
    expr = expr->Opnd(0);
  }
  return expr;
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

const FuncDesc &AliasClass::GetFuncDescFromCallStmt(const CallNode &callstmt) const {
  MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callstmt.GetPUIdx());
  return callee->GetFuncDesc();
}

// ost is a restrict pointer
static bool IsRestrictPointer(const OriginalSt *ost) {
  if (!IsAddress(ost->GetType()->GetPrimType())) {
    return false;
  }

  if (ost->IsSymbolOst() && ost->GetIndirectLev() == 0) {
    auto *symbol = ost->GetMIRSymbol();
    auto fieldId = ost->GetFieldID();
    if (fieldId != 0) {
      auto type = symbol->GetType();
      CHECK_FATAL(type->IsStructType(), "must be struct type");
      bool restrictField = static_cast<MIRStructType*>(type)->IsFieldRestrict(fieldId);
      return restrictField;
    } else {
      bool restrictPointer = symbol->GetAttr(ATTR_restrict);
      return restrictPointer;
    }
  }
  return false;
}

bool AliasClass::CallHasNoPrivateDefEffect(StmtNode *stmt) const {
  if (calleeHasSideEffect) {
    return true;
  }
  CallNode *callstmt = dynamic_cast<CallNode *>(stmt);
  if (callstmt == nullptr) {
    return true;
  }
  return CallHasNoSideEffectOrPrivateDefEffect(*callstmt, FUNCATTR_noprivate_defeffect);
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

  if ((ost.IsFormal() && !IsRestrictPointer(&ost) && ost.GetIndirectLev() >= 0) || ost.GetIndirectLev() > 0) {
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

OffsetType AliasClass::OffsetInBitOfArrayElement(const ArrayNode *arrayNode) {
  ASSERT(arrayNode->GetOpCode() == OP_array, "must be arrayNode");
  bool arrayAddrIsConst = arrayNode->Opnd(0)->GetOpCode() == OP_addrof;
  if (!arrayAddrIsConst) {
    return OffsetType::InvalidOffset();
  }

  std::vector<int64> arrayIndexVector(arrayNode->NumOpnds() - 1, 0);
  for (uint32 opndId = 1; opndId < arrayNode->NumOpnds(); ++opndId) {
    auto *opnd = arrayNode->Opnd(opndId);
    if (!opnd->IsConstval()) {
      return OffsetType::InvalidOffset();
    }
    auto mirConst = static_cast<ConstvalNode*>(opnd)->GetConstVal();
    ASSERT(mirConst->GetKind() == kConstInt, "array index must be integer");
    auto index = static_cast<MIRIntConst*>(mirConst)->GetValue();
    arrayIndexVector[opndId - 1] = index;
  }

  auto *ptrTypeOfArrayType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayNode->GetTyIdx());
  ASSERT(ptrTypeOfArrayType->IsMIRPtrType(), "must be pointer type");
  auto *mirType = static_cast<MIRPtrType*>(ptrTypeOfArrayType)->GetPointedType();
  constexpr uint32 kUpperBoundaryOfElemNumOfIndexSensitiveArray = 100;
  switch (mirType->GetKind()) {
    case kTypeArray: {
      auto arrayType = static_cast<MIRArrayType *>(mirType);
      if (arrayType->ElemNumber() > kUpperBoundaryOfElemNumOfIndexSensitiveArray) {
        return OffsetType::InvalidOffset();
      }
      return OffsetType(arrayType->GetBitOffsetFromArrayAddress(arrayIndexVector));
    }
    case kTypeFArray:
    case kTypeJArray: {
      ASSERT(arrayIndexVector.size() == 1, "FArray/JArray has single index");
      return OffsetType(static_cast<MIRFarrayType*>(mirType)->GetBitOffsetFromArrayAddress(arrayIndexVector.front()));
    }
    default: {
      CHECK_FATAL(false, "unsupported array type");
      return OffsetType::InvalidOffset();
    }
  }
}

// tyIdx is pointer type of memory type, fld is the field of memory type,
// offset is the offset of base.
//     |-----offset------|---|---|-----|
// prevLevOst          tyIdx fld
OriginalSt *AliasClass::FindOrCreateExtraLevOst(SSATab *ssaTab, OriginalSt *prevLevOst, const TyIdx &tyIdx,
    FieldID fld, OffsetType offset) {
  return ssaTab->GetOriginalStTable().FindOrCreateExtraLevOriginalSt(prevLevOst, tyIdx, fld, offset);
}

// return next level of baseAddress. Argument tyIdx specifies the pointer of the memory accessed.
// fieldId represent offset from type of this memory.
// Example: iread <*type> fld (base) or iassign <*type> fld (base):
//   tyIdx is TyIdx of <*type>(notice not <type>), fieldId is fld
AliasElem *AliasClass::FindOrCreateExtraLevAliasElem(BaseNode &baseAddress, const TyIdx &tyIdx,
                                                     FieldID fieldId, bool typeHasBeenCasted) {
  auto *baseAddr = RemoveTypeConversionIfExist(&baseAddress);
  AliasInfo aliasInfoOfBaseAddress = CreateAliasElemsExpr(*baseAddr);
  if (aliasInfoOfBaseAddress.ae == nullptr) {
    return FindOrCreateDummyNADSAe();
  }
  auto *baseOst = aliasInfoOfBaseAddress.ae->GetOst();
  SetTypeUnsafeForBaseTypeCvt(tyIdx, baseOst);
  if (mirModule.IsCModule() && IsNullOrDummySymbolOst(baseOst)) {
    return FindOrCreateDummyNADSAe();
  }
  // calculate the offset of extraLevOst (i.e. offset from baseAddr)
  OffsetType offset = typeHasBeenCasted ? OffsetType(kOffsetUnknown) : aliasInfoOfBaseAddress.offset;
  // If base has a valid baseFld, check type of this baseFld. If it is the same as tyIdx,
  // update tyIdx as base memory type (not fieldType now), update fieldId by merging fieldId and baseFld
  TyIdx newTyIdx = tyIdx;
  FieldID baseFld = aliasInfoOfBaseAddress.fieldID;
  MIRType *baseType = baseOst->GetType();
  if (!aliasInfoOfBaseAddress.offset.IsInvalid() && baseFld != 0 && baseType->IsMIRPtrType()) {
    MIRType *baseMemType = static_cast<MIRPtrType*>(baseType)->GetPointedType();
    if (baseMemType->IsStructType() && static_cast<int32>(baseMemType->NumberOfFieldIDs()) >= baseFld) {
      MIRType *fieldType = static_cast<MIRStructType*>(baseMemType)->GetFieldType(baseFld);
      MIRType *memType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
      ASSERT(memType->IsMIRPtrType(), "tyIdx is TyIdx of iread/iassign, must be pointer type!");
      // Get memory type from base, and find it the same as iread/iassign
      if (fieldType->GetTypeIndex() == static_cast<MIRPtrType*>(memType)->GetPointedTyIdx()) {
        fieldId += baseFld;
        newTyIdx = baseType->GetTypeIndex();
        // We use base type instead of field type, the offset should be set zero if it is valid.
        offset = offset.IsInvalid() ? offset : OffsetType(0);
      }
    }
  }

  auto newOst = FindOrCreateExtraLevOst(&ssaTab, baseOst, newTyIdx, fieldId, offset);

  CHECK_FATAL(newOst != nullptr, "null ptr check");
  if (newOst->GetIndex() == osym2Elem.size()) {
    osym2Elem.push_back(nullptr);
    ssaTab.GetVersionStTable().CreateZeroVersionSt(newOst);
  }
  return FindOrCreateAliasElem(*newOst);
}

AliasElem &AliasClass::FindOrCreateAliasElemOfAddrofOSt(OriginalSt &oSt) {
  OriginalSt *zeroFieldIDOst = ssaTab.FindOrCreateSymbolOriginalSt(*oSt.GetMIRSymbol(), oSt.GetPuIdx(), 0);
  if (zeroFieldIDOst->GetIndex() == osym2Elem.size()) {
    osym2Elem.push_back(nullptr);
    ssaTab.GetVersionStTable().CreateZeroVersionSt(zeroFieldIDOst);
  }
  (void)FindOrCreateAliasElem(*zeroFieldIDOst);

  OriginalSt *addrofOst = ssaTab.FindOrCreateAddrofSymbolOriginalSt(zeroFieldIDOst);
  // if ost's preLevelNode has not been set before, we should also add ost to addrofOst's nextLevelNodes
  // Otherwise, we should skip here to avoid repetition in nextLevelNodes.
  if (oSt.GetPrevLevelOst() == nullptr) {
    oSt.SetPrevLevelOst(addrofOst);
    addrofOst->AddNextLevelOst(&oSt);
  } else {
    ASSERT(oSt.GetPrevLevelOst() == addrofOst, "prev-level-ost inconsistent");
  }
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
      int64 offsetVal =
          (addrof.GetFieldID() == 0) ? 0 : oSt.GetMIRSymbol()->GetType()->GetBitOffsetFromBaseAddr(oSt.GetFieldID());
      return AliasInfo(ae, addrof.GetFieldID(), OffsetType(offsetVal));
    }
    case OP_dread: {
      OriginalSt *ost = static_cast<AddrofSSANode&>(expr).GetSSAVar()->GetOst();
      if (ost->GetFieldID() != 0 && ost->GetPrevLevelOst() == nullptr) {
        (void) FindOrCreateAliasElemOfAddrofOSt(*ost);
      }

      AliasElem *ae = FindOrCreateAliasElem(*ost);
      return AliasInfo(ae, 0, OffsetType(0));
    }
    case OP_regread: {
      OriginalSt &oSt = *static_cast<RegreadSSANode&>(expr).GetSSAVar()->GetOst();
      return (oSt.IsSpecialPreg()) ? AliasInfo() : AliasInfo(FindOrCreateAliasElem(oSt), 0);
    }
    case OP_iread: {
      auto &iread = static_cast<IreadSSANode&>(expr);
      MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx());
      CHECK_FATAL(mirType->GetKind() == kTypePointer, "CreateAliasElemsExpr: ptr type expected in iread");
      auto *typeOfField = static_cast<MIRPtrType *>(mirType)->GetPointedType();
      if (iread.GetFieldID() > 0) {
        typeOfField = static_cast<MIRStructType *>(typeOfField)->GetFieldType(iread.GetFieldID());
      }
      bool typeHasBeenCasted = IreadedMemInconsistentWithPointedType(iread.GetPrimType(), typeOfField->GetPrimType());
      return AliasInfo(FindOrCreateExtraLevAliasElem(
          *iread.Opnd(0), iread.GetTyIdx(), iread.GetFieldID(), typeHasBeenCasted), 0, OffsetType(0));
    }
    case OP_iaddrof: {
      auto &iread = static_cast<IreadNode&>(expr);
      const auto &aliasInfo = CreateAliasElemsExpr(*iread.Opnd(0));
      OffsetType offset = aliasInfo.offset;
      if (iread.GetFieldID() != 0) {
        auto mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx());
        auto pointeeType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
        OffsetType offsetOfField(pointeeType->GetBitOffsetFromBaseAddr(iread.GetFieldID()));
        offset = offset + offsetOfField;
      }
      return AliasInfo(aliasInfo.ae, iread.GetFieldID(), offset);
    }
    case OP_add:
    case OP_sub: {
      const auto &aliasInfo = CreateAliasElemsExpr(*expr.Opnd(0));
      (void)CreateAliasElemsExpr(*expr.Opnd(1));

      if (aliasInfo.offset.IsInvalid()) {
        return AliasInfo(aliasInfo.ae, 0, OffsetType::InvalidOffset());
      }

      auto *opnd = expr.Opnd(1);
      if (!opnd->IsConstval() || !IsAddress(expr.GetPrimType())) {
        return AliasInfo(aliasInfo.ae, 0, OffsetType::InvalidOffset());
      }
      auto mirConst = static_cast<ConstvalNode*>(opnd)->GetConstVal();
      CHECK_FATAL(mirConst->GetKind() == kConstInt, "array index must be integer");
      int64 constVal = static_cast<MIRIntConst*>(mirConst)->GetValue();
      if (expr.GetOpCode() == OP_sub) {
        constVal = -constVal;
      } else {
        ASSERT(expr.GetOpCode() == OP_add, "Wrong operation!");
      }
      OffsetType newOffset = aliasInfo.offset + static_cast<uint64>(constVal) * static_cast<uint64>(bitsPerByte);
      return AliasInfo(aliasInfo.ae, 0, newOffset);
    }
    case OP_array: {
      for (size_t i = 1; i < expr.NumOpnds(); ++i) {
        (void)CreateAliasElemsExpr(*expr.Opnd(i));
      }

      const auto &aliasInfo = CreateAliasElemsExpr(*expr.Opnd(0));
      OffsetType offset = OffsetInBitOfArrayElement(static_cast<const ArrayNode*>(&expr));
      OffsetType newOffset = offset + aliasInfo.offset;
      return AliasInfo(aliasInfo.ae, 0, newOffset);
    }
    case OP_cvt:
    case OP_retype: {
      return CreateAliasElemsExpr(*expr.Opnd(0));
    }
    case OP_select: {
      (void)CreateAliasElemsExpr(*expr.Opnd(0));
      AliasInfo ainfo = CreateAliasElemsExpr(*expr.Opnd(1));
      AliasInfo ainfo2 = CreateAliasElemsExpr(*expr.Opnd(2));
      if (!OpCanFormAddress(expr.Opnd(1)->GetOpCode()) && !OpCanFormAddress(expr.Opnd(2)->GetOpCode())) {
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
      IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrn.GetIntrinsic()];
      if (intrinDesc->IsVectorOp()) {
        SetPtrOpndsNextLevNADS(0, static_cast<unsigned int>(intrn.NumOpnds()), intrn.GetNopnd(),
                               false);
        return AliasInfo();
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

// given a struct/union assignment, regard it as if the fields that appear
// in the code are also assigned
void AliasClass::ApplyUnionForFieldsInCopiedAgg() {
  for (const auto &ostPair : aggsToUnion) {
    OriginalSt *lhsost = ostPair.first;
    OriginalSt *rhsost = ostPair.second;
    auto *preLevOfLHSOst = lhsost->GetPrevLevelOst();
    if (preLevOfLHSOst == nullptr) {
      preLevOfLHSOst = FindOrCreateAliasElemOfAddrofOSt(*lhsost).GetOst();
    }
    auto *preLevOfRHSOst = rhsost->GetPrevLevelOst();
    if (preLevOfRHSOst == nullptr) {
      preLevOfRHSOst = FindOrCreateAliasElemOfAddrofOSt(*rhsost).GetOst();
    }

    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsost->GetTyIdx());
    MIRStructType *mirStructType = static_cast<MIRStructType *>(mirType);
    FieldID numFieldIDs = static_cast<FieldID>(mirStructType->NumberOfFieldIDs());
    for (FieldID fieldID = 1; fieldID <= numFieldIDs; fieldID++) {
      MIRType *fieldType = mirStructType->GetFieldType(fieldID);
      if (!IsPotentialAddress(fieldType->GetPrimType(), &mirModule)) {
        continue;
      }
      OffsetType offset(mirStructType->GetBitOffsetFromBaseAddr(fieldID));

      const auto &nextLevOstsOfLHS = preLevOfLHSOst->GetNextLevelOsts();
      auto fieldOstLHS =
          ssaTab.GetOriginalStTable().FindExtraLevOriginalSt(nextLevOstsOfLHS, fieldType, fieldID, offset);

      const auto &nextLevOstsOfRHS = preLevOfRHSOst->GetNextLevelOsts();
      auto fieldOstRHS =
          ssaTab.GetOriginalStTable().FindExtraLevOriginalSt(nextLevOstsOfRHS, fieldType, fieldID, offset);

      if (fieldOstLHS == nullptr && fieldOstRHS == nullptr) {
        continue;
      }

      if (fieldOstLHS == nullptr) {
        auto ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(lhsost->GetTyIdx());
        fieldOstLHS = ssaTab.GetOriginalStTable().FindOrCreateExtraLevOriginalSt(
            preLevOfLHSOst, ptrType->GetTypeIndex(), fieldID, offset);
      }

      if (fieldOstRHS == nullptr) {
        auto ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(rhsost->GetTyIdx());
        fieldOstRHS = ssaTab.GetOriginalStTable().FindOrCreateExtraLevOriginalSt(
            preLevOfRHSOst, ptrType->GetTypeIndex(), fieldID, offset);
      }

      if (fieldOstLHS->GetIndex() == osym2Elem.size()) {
        osym2Elem.push_back(nullptr);
        ssaTab.GetVersionStTable().CreateZeroVersionSt(fieldOstLHS);
      }
      AliasElem *aeOfLHSField = FindOrCreateAliasElem(*fieldOstLHS);

      if (fieldOstRHS->GetIndex() == osym2Elem.size()) {
        osym2Elem.push_back(nullptr);
        ssaTab.GetVersionStTable().CreateZeroVersionSt(fieldOstRHS);
      }
      AliasElem *aeOfRHSField = FindOrCreateAliasElem(*fieldOstRHS);

      unionFind.Union(aeOfLHSField->id, aeOfRHSField->id);
    }
  }
}

// There are three cases that cause ptr escape from its original definition:
// 1. pointer formal parameters for function definition; (has been dealed in UnionForNADS)
// 2. pointer actual parameters for function call; (has been dealed in call stmts)
// 3. pointer assigned to other variable whose alias will never be analyzed. (dealed here)
//    - integer <- ptr or vice versa
//    - agg <- ptr or vice versa
//
// return true if ptr escapes
bool AliasClass::SetNextLevNADSForEscapePtr(AliasElem &lhsAe, BaseNode &rhs) {
  TyIdx lhsTyIdx = lhsAe.GetOriginalSt().GetTyIdx();
  PrimType lhsPtyp = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx)->GetPrimType();
  BaseNode *realRhs = RemoveTypeConversionIfExist(&rhs);
  PrimType rhsPtyp = realRhs->GetPrimType();
  if (lhsPtyp != rhsPtyp) {
    if ((IsPotentialAddress(lhsPtyp, &mirModule) && !IsPotentialAddress(rhsPtyp, &mirModule)) ||
        (!IsPotentialAddress(lhsPtyp, &mirModule) && IsPotentialAddress(rhsPtyp, &mirModule))) {
      lhsAe.SetNextLevNotAllDefsSeen(true);
      AliasInfo realRhsAinfo = CreateAliasElemsExpr(*realRhs);
      if (realRhsAinfo.ae != nullptr) {
        realRhsAinfo.ae->SetNextLevNotAllDefsSeen(true);
      }
      return true;
    } else if (IsPotentialAddress(lhsPtyp, &mirModule) &&
               realRhs->GetOpCode() == OP_constval &&
               static_cast<ConstvalNode*>(realRhs)->GetConstVal()->IsZero()) {
      // special case, pointer initial : ptr <- 0
      // In some cases for some language like C, pointers may be initialized as null pointer at first,
      // and assigned a new value in other program site. If we do not handle this case, all pointers
      // with 0 intial value will be set nextLevNADS and their nextLev will be unionized after this step.
      // The result is right, but the set of nextLevNADS will be very large, leading to a great amount of
      // aliasing pointers and the imprecision of the alias analyse result.
      // Hence we do nothing but return true here to skip setting pointer nextLevNADS.
      return true;
    }
  }
  return false;
}

void AliasClass::ApplyUnionForDassignCopy(AliasElem &lhsAe, AliasElem *rhsAe, BaseNode &rhs) {
  if (SetNextLevNADSForEscapePtr(lhsAe, rhs)) {
    return;
  }
  if (rhsAe == nullptr) {
    lhsAe.SetNextLevNotAllDefsSeen(true);
    return;
  }
  if (rhsAe->GetOriginalSt().GetIndirectLev() < 0) {
    for (auto *ost : rhsAe->GetOriginalSt().GetNextLevelOsts()) {
      osym2Elem[ost->GetIndex()]->SetNextLevNotAllDefsSeen(true);
    }
  }
  if (mirModule.IsCModule()) {
    auto *lhsOst = lhsAe.GetOst();
    TyIdx lhsTyIdx = lhsOst->GetTyIdx();
    auto *rhsOst = rhsAe->GetOst();
    TyIdx rhsTyIdx = rhsOst->GetTyIdx();
    MIRType *rhsType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(rhsTyIdx);
    if (lhsTyIdx == rhsTyIdx &&
        (rhsType->GetKind() == kTypeStruct || rhsType->GetKind() == kTypeUnion)) {
      if (lhsOst->GetIndex() < rhsOst->GetIndex()) {
        aggsToUnion[lhsOst] = rhsOst;
      } else {
        aggsToUnion[rhsOst] = lhsOst;
      }
    }
  }

  if (rhsAe->GetOriginalSt().GetIndirectLev() > 0 || rhsAe->IsNotAllDefsSeen()) {
    lhsAe.SetNextLevNotAllDefsSeen(true);
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

    auto &ost = aliasElem->GetOriginalSt();
    auto *prevLevOst = ost.GetPrevLevelOst();
    if (ost.GetOffset().IsInvalid() && prevLevOst != nullptr) {
      for (auto *mayValueAliasOst : prevLevOst->GetNextLevelOsts()) {
        if (!IsPotentialAddress(mayValueAliasOst->GetType()->GetPrimType(), &mirModule)) {
          continue;
        }
        auto *ae = FindOrCreateAliasElem(*mayValueAliasOst);
        ae->SetNextLevNotAllDefsSeen(true);
        unionFind.Union(aliasElem->GetClassID(), ae->GetClassID());
      }
    }
  }
  if (opnd.GetOpCode() == OP_cvt) {
    SetPtrOpndNextLevNADS(*opnd.Opnd(0), aliasElem, hasNoPrivateDefEffect);
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

void AliasClass::SetAggPtrFieldsNextLevNADS(const OriginalSt &ost) {
  MIRTypeKind typeKind = ost.GetType()->GetKind();
  if (typeKind == kTypeStruct || typeKind == kTypeUnion || typeKind == kTypeStructIncomplete) {
    auto *structType = static_cast<MIRStructType*>(ost.GetType());
    OriginalSt *prevOst = ost.GetPrevLevelOst();
    // if prevOst exist, all ptr nextLev ost should be set NextLevNADS
    if (prevOst != nullptr) {
      for (auto *nextOst : prevOst->GetNextLevelOsts()) {
        AliasElem *nextAe = FindOrCreateAliasElem(*nextOst);
        if (nextAe->IsNextLevNotAllDefsSeen()) {
          continue; // has been set before
        }
        // pointer arithmetic may cause ptr escape from no address type
        // e.g.
        // Examples: struct Node {
        // Examples:  int val;
        // Examples:  Node *next;
        // Examples: } node;
        // Examples: *((int*)&node + 1) <- ptr :
        // LHS is next, and ptr will escape, but LHS's primtype is agg, not an address type.
        //
        // Hence we should not skip for this case
        if (!IsPotentialAddress(nextOst->GetType()->GetPrimType(), &mirModule)) {
          MIRType *prevType = static_cast<MIRPtrType*>(prevOst->GetType())->GetPointedType();
          int64 bitOffset = prevType->GetBitOffsetFromBaseAddr(nextOst->GetFieldID());
          if (nextOst->GetOffset().val == bitOffset) {
            continue;
          }
        }
        nextAe->SetNextLevNotAllDefsSeen(true);
      }
      return;
    }
    // if prevOst not exist, all ptr field should be set NextLevNADS
    FieldID numFieldIDs = static_cast<FieldID>(structType->NumberOfFieldIDs());
    for (FieldID fieldID = 1; fieldID <= numFieldIDs; fieldID++) {
      MIRType *fieldType = structType->GetFieldType(fieldID);
      if (!IsPotentialAddress(fieldType->GetPrimType(), &mirModule)) {
        continue;
      }
      int64 bitOffset = structType->GetBitOffsetFromBaseAddr(fieldID);
      OffsetType offset(bitOffset);
      MapleUnorderedMap<SymbolFieldPair, OStIdx, HashSymbolFieldPair> &mirSt2Ost =
          ssaTab.GetOriginalStTable().mirSt2Ost;
      auto fldOstIt = mirSt2Ost.find(
          SymbolFieldPair(ost.GetMIRSymbol()->GetStIdx(), fieldID, fieldType->GetTypeIndex(), offset));
      if (fldOstIt != mirSt2Ost.end()) {
        OriginalSt *fldOst = ssaTab.GetOriginalStFromID(fldOstIt->second);
        AliasElem *fldAe = FindOrCreateAliasElem(*fldOst);
        fldAe->SetNextLevNotAllDefsSeen(true);
      }
    }
  }
}

void AliasClass::SetAggOpndPtrFieldsNextLevNADS(MapleVector<BaseNode*> &opnds) {
  for (size_t i = 0; i < opnds.size(); ++i) {
    BaseNode *opnd = opnds[i];
    if (opnd->GetPrimType() != PTY_agg) {
      continue;
    }
    AliasInfo aInfo = CreateAliasElemsExpr(*opnd);
    if (aInfo.ae == nullptr ||
        (aInfo.ae->IsNextLevNotAllDefsSeen() && aInfo.ae->GetOriginalSt().GetIndirectLev() > 0)) {
      continue;
    }
    SetAggPtrFieldsNextLevNADS(aInfo.ae->GetOriginalSt());
  }
}

void AliasClass::SetPtrFieldsOfAggNextLevNADS(const BaseNode *opnd, const AliasElem *aliasElem) {
  if (opnd->GetPrimType() != PTY_agg) {
    return;
  }
  if (aliasElem == nullptr ||
      (aliasElem->IsNextLevNotAllDefsSeen() && aliasElem->GetOriginalSt().GetIndirectLev() > 0)) {
    return;
  }
  SetAggPtrFieldsNextLevNADS(aliasElem->GetOriginalSt());
}

// based on ost1's extra level ost's, ensure corresponding ones exist for ost2
void AliasClass::CreateMirroringAliasElems(const OriginalSt *ost1, OriginalSt *ost2) {
  if (ost1->IsSameSymOrPreg(ost2)) {
    return;
  }
  auto &nextLevelNodes = ost1->GetNextLevelOsts();
  auto it = nextLevelNodes.begin();
  for (; it != nextLevelNodes.end(); ++it) {
    OriginalSt *nextLevelOst1 = *it;
    AliasElem *ae1 = FindOrCreateAliasElem(*nextLevelOst1);
    OriginalSt *nextLevelOst2 = ssaTab.FindOrCreateExtraLevOriginalSt(
        ost2, ost2->GetTyIdx(), nextLevelOst1->GetFieldID());
    if (nextLevelOst2->GetIndex() == osym2Elem.size()) {
      osym2Elem.push_back(nullptr);
      ssaTab.GetVersionStTable().CreateZeroVersionSt(nextLevelOst2);
    }
    AliasElem *ae2 = FindOrCreateAliasElem(*nextLevelOst2);
    unionFind.Union(ae1->GetClassID(), ae2->GetClassID());
    CreateMirroringAliasElems(nextLevelOst1, nextLevelOst2); // recursive call
  }
}

// Iteratively propagate type unsafe info to next level.
void AliasClass::PropagateTypeUnsafeVertically(const OriginalSt &ost) {
  for (auto *nextLevOst : ost.GetNextLevelOsts()) {
    TypeBasedAliasAnalysis::SetOstTypeUnsafe(*nextLevOst);
    PropagateTypeUnsafeVertically(*nextLevOst);
  }
}
// Propagate type unsafe info as soon as assign set has been created.
// If any Ost in assign set is type unsafe, propagate type-unsafe info
// among all elements in this assign set and next level osts.
void AliasClass::PropagateTypeUnsafe() {
  if (!MeOption::tbaa || MeOption::optLevel >= 3) {
    return;
  }
  TypeBasedAliasAnalysis::GetOstTypeUnsafe().resize(osym2Elem.size(), false);
  for (auto *ae : id2Elem) {
    auto *assSet = ae->GetAssignSet();
    if (assSet == nullptr) {
      OriginalSt *ost = ae->GetOst();
      if (ost->GetType()->IsUnsafeType() || TypeBasedAliasAnalysis::IsOstTypeUnsafe(*ost)) {
        // Vertical propagate : to next level.
        PropagateTypeUnsafeVertically(*ost);
      }
      continue;
    }
    bool unsafe = false;
    // if any element in assign set is typeUnsafe, all elements will be set unsafe
    for (auto elemID : *assSet) {
      OriginalSt &elemOst = id2Elem[elemID]->GetOriginalSt();
      if (elemOst.GetType()->IsUnsafeType() || TypeBasedAliasAnalysis::IsOstTypeUnsafe(elemOst)) {
        unsafe = true;
        break;
      }
    }
    if (unsafe) {
      for (auto elemID : *assSet) {
        OriginalSt &elemOst = id2Elem[elemID]->GetOriginalSt();
        // Horizontal propagate : in assignSet
        TypeBasedAliasAnalysis::SetOstTypeUnsafe(elemOst);
        // Vertical propagate : to next level.
        PropagateTypeUnsafeVertically(elemOst);
      }
    }
  }
}

// if ae->ost is an address of an union type (or its field), we set ost type unsafe.
// example: union {int x; float y;} u; u.x and u.y must alias, although their type is incompatible
void AliasClass::SetTypeUnsafeForAddrofUnion(const AliasElem *ae) const {
  if (!MeOption::tbaa || MeOption::optLevel >= 3) {
    return;
  }
  if (ae == nullptr) {
    return;
  }
  const OriginalSt &ost = ae->GetOriginalSt();
  // if ost is an address of an union type (or its field), we set it type unsafe.
  if (ost.GetIndirectLev() == -1) {
    MIRType *rhsType = ost.GetType();
    ASSERT(rhsType->IsMIRPtrType(), "Ost with -1 indirect level must have pointer type!");
    MIRType *pointedType = static_cast<MIRPtrType*>(rhsType)->GetPointedType();
    if (pointedType->GetKind() == kTypeUnion) {
      TypeBasedAliasAnalysis::SetOstTypeUnsafe(ost);
    }
  }
}

// For x <- y, if type of x is different from y, type conversion occurs, and we should set them type unsafe.
void AliasClass::SetTypeUnsafeForTypeConversion(const AliasElem *lhsAe, const AliasElem *rhsAe) const {
  if (!MeOption::tbaa || MeOption::optLevel >= 3) {
    return;
  }
  if (lhsAe == nullptr || rhsAe == nullptr) {
    return;
  }
  // if lhs and rhs have different type, set both of them type unsafe
  const OriginalSt &lhsOst = lhsAe->GetOriginalSt();
  const OriginalSt &rhsOst = rhsAe->GetOriginalSt();
  MIRType *lhsType = lhsOst.GetType();
  MIRType *rhsType = rhsOst.GetType();
  if (lhsType != rhsType && (lhsType->IsMIRPtrType() || rhsType->IsMIRPtrType())) {
    TypeBasedAliasAnalysis::SetOstTypeUnsafe(lhsOst);
    TypeBasedAliasAnalysis::SetOstTypeUnsafe(rhsOst);
  }
}

// Example: iread <*type> (base) or iassign <*type> (base):
// If base type is different from accessedType(i.e. <*type>), base type is re-interpreted,
// and implicit type conversion occurs. We should set them type unsafe.
void AliasClass::SetTypeUnsafeForBaseTypeCvt(const TyIdx &accessTyIdx, const OriginalSt *baseOst) const {
  if (!MeOption::tbaa || MeOption::optLevel >= 3) {
    return;
  }
  if (baseOst->GetTyIdx() != accessTyIdx) {
    // base type is different from accessed type, implicit type conversion occurs. Set base ost unsafe
    TypeBasedAliasAnalysis::SetOstTypeUnsafe(*baseOst);
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
      if (ost->GetFieldID() != 0) {
        (void)FindOrCreateAliasElemOfAddrofOSt(*ost);
      }
      AliasElem *lhsAe = FindOrCreateAliasElem(*ost);
      ASSERT_NOT_NULL(lhsAe);
      ApplyUnionForDassignCopy(*lhsAe, rhsAinfo.ae, *stmt.Opnd(0));
      SetTypeUnsafeForAddrofUnion(rhsAinfo.ae);
      SetTypeUnsafeForTypeConversion(lhsAe, rhsAinfo.ae);
      return;
    }
    case OP_iassign: {
      auto &iassignNode = static_cast<IassignNode&>(stmt);
      AliasInfo rhsAinfo = CreateAliasElemsExpr(*iassignNode.Opnd(1));
      AliasElem *lhsAe =
          FindOrCreateExtraLevAliasElem(*iassignNode.Opnd(0), iassignNode.GetTyIdx(), iassignNode.GetFieldID(), false);
      if (lhsAe != nullptr) {
        ApplyUnionForDassignCopy(*lhsAe, rhsAinfo.ae, *iassignNode.Opnd(1));
      }
      SetTypeUnsafeForAddrofUnion(rhsAinfo.ae);
      SetTypeUnsafeForTypeConversion(lhsAe, rhsAinfo.ae);
      return;
    }
    case OP_throw: {
      AliasInfo ainfo = CreateAliasElemsExpr(*stmt.Opnd(0));
      SetPtrOpndNextLevNADS(*stmt.Opnd(0), ainfo.ae, false);
      return;
    }
    case OP_call:
    case OP_callassigned: {
      const FuncDesc &desc = GetFuncDescFromCallStmt(static_cast<CallNode&>(stmt));
      bool hasnoprivatedefeffect = CallHasNoPrivateDefEffect(&stmt);
      for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
        const AliasInfo &ainfo = CreateAliasElemsExpr(*stmt.Opnd(i));
        // no need to solve args that are not used.
        if (desc.IsArgUnused(i)) {
          continue;
        }
        SetPtrOpndNextLevNADS(*stmt.Opnd(i), ainfo.ae, hasnoprivatedefeffect);
        SetPtrFieldsOfAggNextLevNADS(stmt.Opnd(i), ainfo.ae);
      }
      break;
    }
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned: {
      const FuncDesc &desc = GetFuncDescFromCallStmt(static_cast<CallNode&>(stmt));
      bool hasnoprivatedefeffect = CallHasNoPrivateDefEffect(&stmt);
      for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
        const AliasInfo &ainfo = CreateAliasElemsExpr(*stmt.Opnd(i));
        if (ainfo.ae == nullptr) {
          continue;
        }
        if (i == 0) {
          continue;
        }
        // no need to solve args that are not used.
        if (desc.IsArgUnused(i)) {
          continue;
        }
        if (hasnoprivatedefeffect && ainfo.ae->GetOriginalSt().IsPrivate()) {
          continue;
        }
        if (!IsPotentialAddress(stmt.Opnd(i)->GetPrimType(), &mirModule)) {
          continue;
        }
        if (stmt.Opnd(i)->GetOpCode() == OP_addrof && IsReadOnlyOst(ainfo.ae->GetOriginalSt())) {
          continue;
        }
        ainfo.ae->SetNextLevNotAllDefsSeen(true);
      }
      break;
    }
    case OP_asm:
    case OP_icall:
    case OP_icallassigned: {
      for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
        const AliasInfo &ainfo = CreateAliasElemsExpr(*stmt.Opnd(i));
        if (stmt.GetOpCode() != OP_asm && i == 0) {
          continue;
        }
        SetPtrOpndNextLevNADS(*stmt.Opnd(i), ainfo.ae, false);
        SetPtrFieldsOfAggNextLevNADS(stmt.Opnd(i), ainfo.ae);
      }
      break;
    }
    case OP_intrinsiccall:
    case OP_intrinsiccallassigned: {
      bool opndsNextLevNADS = static_cast<IntrinsiccallNode&>(stmt).GetIntrinsic() == INTRN_JAVA_POLYMORPHIC_CALL;
      for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
        const AliasInfo &ainfo = CreateAliasElemsExpr(*stmt.Opnd(i));
        if (opndsNextLevNADS) {
          SetPtrOpndNextLevNADS(*stmt.Opnd(i), ainfo.ae, false);
        }
      }
      break;
    }
    default: {
      for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        (void)CreateAliasElemsExpr(*stmt.Opnd(i));
      }
      break;
    }
  }
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    if (stmt.GetOpCode() == OP_callassigned) {
      auto &callStmt = static_cast<CallNode&>(stmt);
      auto *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt.GetPUIdx());
      if (mirFunc != nullptr && mirFunc->GetFuncDesc().IsReturnNoAlias()) {
        MapleVector<MustDefNode> &mustDefs = ssaTab.GetStmtsSSAPart().GetMustDefNodesOf(callStmt);
        for (auto &mustDef : mustDefs) {
          (void)FindOrCreateAliasElem(*mustDef.GetResult()->GetOst());
        }
        return;
      }
    }
    SetNotAllDefsSeenForMustDefs(stmt);
  }
}

void AliasClass::UnionAddrofOstOfUnionFields() {
  // map stIdx of union to aliasElem of addrofOst of union fields
  std::map<StIdx, AliasElem*> sym2AddrofOstAE;
  for (auto *aliasElem : id2Elem) {
    auto &ost = aliasElem->GetOriginalSt();
    // indirect level of addrof ost eq -1
    if (ost.GetIndirectLev() != -1) {
      continue;
    }
    ASSERT(ost.IsSymbolOst(), "only symbol has address");
    if (ost.GetMIRSymbol()->GetType()->GetKind() != kTypeUnion) {
      continue;
    }
    StIdx stIdx = ost.GetMIRSymbol()->GetStIdx();
    auto it = sym2AddrofOstAE.find(stIdx);
    if (it != sym2AddrofOstAE.end()) {
      unionFind.Union(it->second->id, aliasElem->id);
    } else {
      sym2AddrofOstAE[stIdx] = aliasElem;
    }
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
      id2Elem[id]->assignSet = id2Elem[rootID]->assignSet;
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

void AliasClass::GetValueAliasSetOfOst(OriginalSt *ost, std::set<OriginalSt*> &result) {
  if (ost == nullptr || ost->GetIndex() >= osym2Elem.size()) {
    return;
  }
  AliasElem *rootAe = nullptr;
  auto *ae = osym2Elem[ost->GetIndex()];
  if (ae->GetAssignSet() != nullptr) {
    rootAe = ae;
  }
  if (rootAe == nullptr) {
    for (auto *aliasElem : id2Elem) {
      if (unionFind.Root(aliasElem->GetClassID()) != aliasElem->GetClassID()) {
        continue;
      }
      if (aliasElem->GetAssignSet() == nullptr) {
        continue;
      }
      if (aliasElem->GetAssignSet()->find(ae->GetClassID()) != aliasElem->GetAssignSet()->end()) {
        rootAe = aliasElem;
        break;
      }
    }
  }
  if (rootAe == nullptr) {
    result.insert(ost);
  } else {
    for (auto elemID : *(rootAe->GetAssignSet())) {
      result.insert(id2Elem[elemID]->GetOst());
    }
  }
}

void AliasClass::UnionAllPointedTos() {
  std::vector<AliasElem*> pointedTos;
  for (auto *aliasElem : id2Elem) {
    // OriginalSt is pointed to by another OriginalSt
    if (aliasElem->GetOriginalSt().GetPrevLevelOst() != nullptr) {
      aliasElem->SetNotAllDefsSeen(true);
      pointedTos.push_back(aliasElem);
    }
  }
  for (size_t i = 1; i < pointedTos.size(); ++i) {
    unionFind.Union(pointedTos[0]->GetClassID(), pointedTos[i]->GetClassID());
  }
}

void AliasClass::UnionNextLevelOfAliasOst(std::set<AliasElem *> &aesToUnionNextLev) {
  while (!aesToUnionNextLev.empty()) {
    auto tmpSet = aesToUnionNextLev;
    aesToUnionNextLev.clear();
    for (auto *aliasElem : tmpSet) {
      auto aeId = aliasElem->GetClassID();
      if (unionFind.Root(aeId) != aeId) {
        continue;
      }

      std::set<OriginalSt *> mayAliasOsts;
      for (uint32 id = 0; id < id2Elem.size(); ++id) {
        if (unionFind.Root(id) != aeId) {
          continue;
        }
        auto *ost = id2Elem[id]->GetOst();
        auto &nextLevOsts = ost->GetNextLevelOsts();
        mayAliasOsts.insert(nextLevOsts.begin(), nextLevOsts.end());

        auto *assignSet = id2Elem[id]->GetAssignSet();
        if (assignSet == nullptr) {
          continue;
        }
        for (auto valAliasId : *assignSet) {
          if (valAliasId == id) {
            continue;
          }
          auto &nextLevelOsts = id2Elem[valAliasId]->GetOst()->GetNextLevelOsts();
          mayAliasOsts.insert(nextLevelOsts.begin(), nextLevelOsts.end());
        }
      }

      if (mayAliasOsts.empty()) {
        continue;
      }

      auto it = mayAliasOsts.begin();
      auto *ostA = *it;
      if (ostA->IsFinal()) {
        for (; it != mayAliasOsts.end(); ++it) {
          ostA = *it;
          if (!ostA->IsFinal()) {
            break;
          }
        }
      }
      auto idA = FindAliasElem(*ostA)->GetClassID();

      for (; it != mayAliasOsts.end(); ++it) {
        auto *ostB = *it;
        if (ostB->IsFinal()) {
          continue;
        }
        auto idB = FindAliasElem(*ostB)->GetClassID();
        if (unionFind.Root(idA) != unionFind.Root(idB)) {
          unionFind.Union(idA, idB);
          aesToUnionNextLev.insert(id2Elem[unionFind.Root(idA)]);
        }
      }
    }
  }
}

// process the union among the pointed's of assignsets
void AliasClass::ApplyUnionForPointedTos() {
  // first, process nextLevNotAllDefsSeen for alias elems until no change
  bool change = true;
  while (change) {
    change = false;
    for (AliasElem *aliaselem : id2Elem) {
      if (aliaselem->IsNextLevNotAllDefsSeen() || aliaselem->IsNotAllDefsSeen()) {
        const auto &nextLevelNodes = aliaselem->GetOriginalSt().GetNextLevelOsts();
        auto ostit = nextLevelNodes.begin();
        for (; ostit != nextLevelNodes.end(); ++ostit) {
          AliasElem *indae = FindAliasElem(**ostit);
          if (!indae->GetOriginalSt().IsFinal() && !indae->IsNotAllDefsSeen()) {
            indae->SetNotAllDefsSeen(true);
            indae->SetNextLevNotAllDefsSeen(true);
            change = true;
          }
        }
      }
    }
  }
  // only for c language
  // base ptr may add/sub an offset to point to any field
  if (mirModule.IsCModule()) {
    ApplyUnionForStorageOverlaps();
  }
  // Union agg itself with all its fields because when the agg is modified,
  // all its fields may be modified too.
  if (!mirModule.IsJavaModule()) {
    UnionForAggAndFields();
  }

  std::vector<bool> aliasElemProcessed(id2Elem.size(), false);
  MapleSet<uint> tempset(std::less<uint>(), acAlloc.Adapter());
  for (AliasElem *aliaselem : id2Elem) {
    if (aliasElemProcessed[aliaselem->GetClassID()]) {
      continue;
    }

    if (aliaselem->GetAssignSet() == nullptr) {
      // next level Alias Elements of aliaselem may alias each other. union aliasing elements in following
      auto &nextLevelNodes = aliaselem->GetOriginalSt().GetNextLevelOsts();
      std::set<AliasElem *> aesToUnionNextLev;
      for (auto ostAit = nextLevelNodes.begin(); ostAit != nextLevelNodes.end(); ++ostAit) {
        for (auto ostBit = ostAit; ostBit != nextLevelNodes.end(); ++ostBit) {
          if (!MayAliasBasicAA(*ostAit, *ostBit)) {
            continue;
          }
          uint32 idA = FindAliasElem(**ostAit)->GetClassID();
          uint32 idB = FindAliasElem(**ostBit)->GetClassID();
          if (unionFind.Root(idA) != unionFind.Root(idB)) {
            unionFind.Union(idA, idB);
            aesToUnionNextLev.insert(id2Elem[unionFind.Root(idA)]);
          }
        }
      }
      // union next-level-osts of aliased osts
      UnionNextLevelOfAliasOst(aesToUnionNextLev);
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
      aliasElemProcessed[*setit] = true;
    }
    if (hasNextLevNotAllDefsSeen) {
      // make all pointedto's in this assignSet notAllDefsSeen
      for (auto setit = aliaselem->GetAssignSet()->begin(); setit != aliaselem->GetAssignSet()->end();
           ++setit) {
        AliasElem *ae0 = id2Elem[*setit];
        MapleVector<OriginalSt *> &nextLevelNodes = ae0->GetOriginalSt().GetNextLevelOsts();
        auto ostit = nextLevelNodes.begin();
        for (; ostit != nextLevelNodes.end(); ++ostit) {
          AliasElem *indae = FindAliasElem(**ostit);
          if (!indae->GetOriginalSt().IsFinal()) {
            indae->SetNotAllDefsSeen(true);
          }
        }
      }
      continue;
    }

    // apply union among the assignSet elements
    std::set<AliasElem *> aesToUnionNextLev;
    tempset = *(aliaselem->GetAssignSet());
    while (tempset.size() > 1) { // At least two elements in the same assignSet so that we can connect their next level
      // pick one alias element
      MapleSet<uint>::iterator pickit = tempset.begin();
      AliasElem *ae1 = id2Elem[*pickit];
      (void)tempset.erase(pickit);
      auto &nextLevelNodes1 = ae1->GetOriginalSt().GetNextLevelOsts();
      for (MapleSet<uint>::iterator setit = tempset.begin(); setit != tempset.end(); ++setit) {
        AliasElem *ae2 = id2Elem[*setit];
        auto &nextLevelNodes2 = ae2->GetOriginalSt().GetNextLevelOsts();
        // union the next level of elements in the same assignSet
        for (auto ost1it = nextLevelNodes1.begin(); ost1it != nextLevelNodes1.end(); ++ost1it) {
          for (auto ost2it = nextLevelNodes2.begin(); ost2it != nextLevelNodes2.end(); ++ost2it) {
            if (!MayAliasBasicAA(*ost1it, *ost2it)) {
              continue;
            }
            uint32 idA = FindAliasElem(**ost1it)->GetClassID();
            uint32 idB = FindAliasElem(**ost2it)->GetClassID();
            if (unionFind.Root(idA) != unionFind.Root(idB)) {
              unionFind.Union(idA, idB);
              aesToUnionNextLev.insert(id2Elem[unionFind.Root(idA)]);
            }
          }
        }
      }
    }

    // union next-level-osts of aliased osts
    UnionNextLevelOfAliasOst(aesToUnionNextLev);
  }
}

void AliasClass::CollectRootIDOfNextLevelNodes(const OriginalSt &ost,
                                               std::set<unsigned int> &rootIDOfNADSs) {
  for (OriginalSt *nextLevelNode : ost.GetNextLevelOsts()) {
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
    std::set<AliasElem *> aesToUnionNextLev;
    for (size_t elemIdB : rootIDOfNADSs) {
      if (unionFind.Root(elemIdA) != unionFind.Root(elemIdB)) {
        unionFind.Union(elemIdA, elemIdB);
        aesToUnionNextLev.insert(id2Elem[unionFind.Root(elemIdA)]);
      }
    }
    UnionNextLevelOfAliasOst(aesToUnionNextLev);
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
  std::set<AliasElem *> aesToUnionNextLev;
  for (AliasElem *ae : notAllDefsSeenAes) {
    auto idA = notAllDefsSeenAe->GetClassID();
    auto idB = ae->GetClassID();
    if (unionFind.Root(idA) != unionFind.Root(idB)) {
      unionFind.Union(idA, idB);
      aesToUnionNextLev.insert(id2Elem[unionFind.Root(idA)]);
    }
  }
  UnionNextLevelOfAliasOst(aesToUnionNextLev);

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
    bool hasNotAllDefsSeen = ae->IsNotAllDefsSeen();
    if (!hasNotAllDefsSeen) {
      // see if any at level 1 has notAllDefsSeen set
      for (OriginalSt *nextLevelNode : ost->GetNextLevelOsts()) {
        ae = osym2Elem[nextLevelNode->GetIndex()];
        if (ae != nullptr && ae->IsNotAllDefsSeen()) {
          hasNotAllDefsSeen = true;
          break;
        }
      }
    }
    if (hasNotAllDefsSeen) {
      // set to true for all members at this level
      for (OriginalSt *nextLevelNode : ost->GetNextLevelOsts()) {
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
  std::set<AliasElem *> aesToUnionNextLev;
  for (; it != nextLevOsts->end(); ++it) {
    OriginalSt *ostB = *it;
    AliasElem *aeB = FindAliasElem(*ostB);
    auto idA = aeA->GetClassID();
    auto idB = aeB->GetClassID();
    if (unionFind.Root(idA) != unionFind.Root(idB)) {
      unionFind.Union(idA, idB);
      aesToUnionNextLev.insert(id2Elem[unionFind.Root(idA)]);
    }
  }
  // union next-level-osts of aliased osts
  UnionNextLevelOfAliasOst(aesToUnionNextLev);
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

    // work on the next indirect level
    auto &nextLevOsts = ost->GetNextLevelOsts();

    MIRType *pointedType = static_cast<MIRPtrType *>(mirType)->GetPointedType();
    if (pointedType->GetKind() == kTypeUnion) {
      // union all fields of union
      UnionAllNodes(&nextLevOsts);
      continue;
    }

    for (auto *nextLevOst : nextLevOsts) {
      // offset of nextLevOst is indeteminate, it may alias all other next-level-osts
      if (nextLevOst->GetOffset().IsInvalid()) {
        UnionAllNodes(&nextLevOsts);
        break;
      }

      MIRType *nextLevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(nextLevOst->GetTyIdx());
      if (nextLevType->HasFields()) {
        // union all fields if one next-level-ost has fields
        UnionAllNodes(&nextLevOsts);
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
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(ostA.GetPrevLevelOst()->GetTyIdx());
  MIRType *mirTypeB =
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(ostB.GetPrevLevelOst()->GetTyIdx());
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
    OriginalSt &ostA = id2Elem[idA]->GetOriginalSt();
    for (unsigned int idB : newGroups) {
      OriginalSt &ostB = id2Elem[idB]->GetOriginalSt();
      if (AliasAccordingToType(ostA.GetPrevLevelOst()->GetTyIdx(), ostB.GetPrevLevelOst()->GetTyIdx()) &&
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
    if (aliasElem->IsNotAllDefsSeen()) {
      if (aliasElem->GetClassID() == unionFind.Root(aliasElem->GetClassID())) {
        notAllDefsSeenClassSetRoots.push_back(aliasElem);
      }
      nadsOsts.insert(aliasElem->GetOst());
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

// if ostA alias with virtual-var ostB, base pointer of ostB must negatively offset from address of ostA
// if ostA is global, negative-offset access is not permitted
bool AliasResultInNegtiveOffset(const OriginalSt *ostA, const OriginalSt *ostB) {
  if (!(ostA->GetIndirectLev() == 0 && ostA->IsSymbolOst() && ostA->GetMIRSymbol()->IsGlobal())) {
    return false;
  }
  if (ostB->GetIndirectLev() <= 0) {
    return false;
  }

  OffsetType offsetA = ostA->GetOffset();
  OffsetType offsetB = ostB->GetOffset();
  auto typeA = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ostA->GetTyIdx());
  if (!offsetA.IsInvalid() && !offsetB.IsInvalid()) {
    int32 bitSizeA = static_cast<int32>(GetTypeBitSize(typeA));
    return (offsetA + bitSizeA) < offsetB;
  }
  return false;
}

bool AliasClass::MayAliasBasicAA(const OriginalSt *ostA, const OriginalSt *ostB) {
  if (ostA == ostB) {
    return true;
  }

  // final var  has no alias relation
  if (ostA->IsFinal() || ostB->IsFinal()) {
    return false;
  }

  auto indirectLevA = ostA->GetIndirectLev();
  auto indirectLevB = ostB->GetIndirectLev();
  // address of var has no alias relation
  if (indirectLevA < 0 || indirectLevB < 0) {
    return false;
  }
  // alias analysis based on type info
  if (!TypeBasedAliasAnalysis::MayAlias(ostA, ostB)) {
    return false;
  }
  // for global variable, negative-offset access is not permitted
  if (indirectLevA == 0 && ostA->IsSymbolOst() && ostA->GetMIRSymbol()->IsGlobal() && indirectLevB > 0) {
    return !AliasResultInNegtiveOffset(ostA, ostB);
  }
  if (indirectLevB == 0 && ostB->IsSymbolOst() && ostB->GetMIRSymbol()->IsGlobal() && indirectLevA > 0) {
    return !AliasResultInNegtiveOffset(ostB, ostA);
  }

  // flow-insensitive basicAA cannot analysis alias relation of virtual-var.
  // Pointer arithmetic may changed value of pointer, and result in aggPtr->fldA alias with aggPtr->fldB.
  if (indirectLevA > 0 || indirectLevB > 0) {
    return true;
  }

  // indirectLevA == 0 && indirectLevB == 0
  // preg has no alias
  if (ostA->IsPregOst() || ostB->IsPregOst()) {
    return false;
  }

  // different zero-level-var not alias each other
  if (ostA->GetMIRSymbol() != ostB->GetMIRSymbol()) {
    return false;
  }
  const OffsetType &offsetA = ostA->GetOffset();
  const OffsetType &offsetB = ostB->GetOffset();
  // alias analysis based on offset.
  if (!offsetA.IsInvalid() && !offsetB.IsInvalid()) {
    // return if memory of ostA and ostB overlap
    int32 bitSizeA = static_cast<int32>(GetTypeBitSize(ostA->GetType()));
    int32 bitSizeB = static_cast<int32>(GetTypeBitSize(ostB->GetType()));
    return IsMemoryOverlap(offsetA, bitSizeA, offsetB, bitSizeB);
  }
  return true;
}

bool AliasClass::MayAlias(const OriginalSt *ostA, const OriginalSt *ostB) const {
  if (!MayAliasBasicAA(ostA, ostB)) {
    return false;
  }

  if (ostA->GetIndex() >= osym2Elem.size()) {
    return true;
  }
  if (ostB->GetIndex() >= osym2Elem.size()) {
    return true;
  }

  auto aliasElemA = osym2Elem[ostA->GetIndex()];
  auto aliasSet = aliasElemA->GetClassSet();
  if (aliasSet == nullptr) {
    return false;
  }
  auto aliasElemB = osym2Elem[ostB->GetIndex()];
  return (aliasSet->find(aliasElemB->GetClassID()) != aliasSet->end());
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

// collect the mayUses caused by defined final field.
void AliasClass::CollectMayUseFromDefinedFinalField(std::set<OriginalSt*> &mayUseOsts) {
  for (AliasElem *aliasElem : id2Elem) {
    ASSERT(aliasElem != nullptr, "null ptr check");
    auto &ost = aliasElem->GetOriginalSt();
    if (!ost.IsFinal()) {
      continue;
    }

    auto *prevLevelOst = ost.GetPrevLevelOst();
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
void AliasClass::InsertMayUseNode(std::set<OriginalSt*> &mayUseOsts, AccessSSANodes *ssaPart) {
  for (OriginalSt *ost : mayUseOsts) {
    ssaPart->InsertMayUseNode(MayUseNode(
        ssaTab.GetVersionStTable().GetVersionStVectorItem(ost->GetZeroVersionIndex())));
  }
}

void AliasClass::CollectMayUseFromFormals(std::set<OriginalSt*> &mayUseOsts) {
  auto *curFunc = mirModule.CurFunction();
  std::set<OriginalSt*> restrictFormalOsts;
  for (size_t formalId = 0; formalId < curFunc->GetFormalCount(); ++formalId) {
    auto *formal = curFunc->GetFormal(formalId);
    if (formal->GetAttr(ATTR_restrict)) {
      (void)restrictFormalOsts.insert(ssaTab.FindOrCreateSymbolOriginalSt(*formal, curFunc->GetPuidx(), 0));
    }
  }
  // non-restrict formals are set next-level-not-all-defines-seen,
  // the may-used virtual vars are collected in nadsOsts, not need collect again
  if (restrictFormalOsts.empty()) {
    return;
  }

  for (auto *aliasElem : id2Elem) {
    if (aliasElem == nullptr || aliasElem->IsNextLevNotAllDefsSeen()) {
      continue;
    }

    bool valueAliasRestrictFormal = false;
    auto *ost = aliasElem->GetOst();
    if (restrictFormalOsts.find(ost) != restrictFormalOsts.end()) {
      valueAliasRestrictFormal = true; // ostValueAliasWithFormal value alias itself
      // virtual osts with indirectLev = 2 are set not-all-defines-seen and collected in nadsOsts.
      // only next-level osts(indirectLev = 1) of formals are collected in the following:
      for (auto *nextLevelOst : ost->GetNextLevelOsts()) {
        mayUseOsts.insert(nextLevelOst);
      }
    }

    auto *assignSet = aliasElem->GetAssignSet();
    if (assignSet == nullptr) {
      continue;
    }

    if (!valueAliasRestrictFormal) {
      for (auto *formalOst : restrictFormalOsts) {
        // ostValueAliasWithFormal has not been used in function, no need to collect its next-level-osts
        if (formalOst->GetIndex() >= osym2Elem.size()) {
          continue;
        }
        auto aeId = osym2Elem[formalOst->GetIndex()]->GetClassID();
        if (assignSet->find(aeId) != assignSet->end()) {
          valueAliasRestrictFormal = true;
          break;
        }
      }
    }

    if (!valueAliasRestrictFormal) {
      continue;
    }

    for (auto id : *assignSet) {
      auto *valueAliasedOst = id2Elem[id]->GetOst();
      for (auto *nextLevelOst : valueAliasedOst->GetNextLevelOsts()) {
        mayUseOsts.insert(nextLevelOst);
      }
    }
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
  mayUseOsts.insert(nadsOsts.begin(), nadsOsts.end());
  // 2. collect mayUses caused by globals_affected_by_call.
  CollectMayUseFromGlobalsAffectedByCalls(mayUseOsts);
  CollectMayUseFromFormals(mayUseOsts);
  // 3. collect mayUses caused by defined final field for constructor
  if (mirModule.CurFunction()->IsConstructor()) {
    CollectMayUseFromDefinedFinalField(mayUseOsts);
  }
  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  InsertMayUseNode(mayUseOsts, ssaPart);
}

// collect next_level_nodes of the ost of ReturnOpnd into mayUseOsts
void AliasClass::CollectPtsToOfReturnOpnd(const OriginalSt &ost, std::set<OriginalSt*> &mayUseOsts) {
  for (OriginalSt *nextLevelOst : ost.GetNextLevelOsts()) {
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
        auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
        InsertMayUseNode(mayUseOsts, ssaPart);
      }
    }
  }
}

void AliasClass::InsertMayUseAll(const StmtNode &stmt) {
  TypeOfMayUseList &mayUseNodes = ssaTab.GetStmtsSSAPart().GetMayUseNodesOf(stmt);
  for (AliasElem *aliasElem : id2Elem) {
    if (aliasElem->GetOriginalSt().GetIndirectLev() >= 0 && !aliasElem->GetOriginalSt().IsPregOst()) {
      MayUseNode mayUse = MayUseNode(
          ssaTab.GetVersionStTable().GetVersionStVectorItem(aliasElem->GetOriginalSt().GetZeroVersionIndex()));
      (void)mayUseNodes.emplace(mayUse.GetOpnd()->GetOrigIdx(), mayUse);
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
      if (!MayAliasBasicAA(ostOfLhsAe, ostOfAliasAe)) {
        continue;
      }
      (void)mayDefOsts.insert(ostOfAliasAe);
    }
  }
}

void AliasClass::InsertMayDefNode(std::set<OriginalSt*> &mayDefOsts, AccessSSANodes *ssaPart,
                                  StmtNode &stmt, BBId bbid) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    ssaPart->InsertMayDefNode(MayDefNode(
        ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
    ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
  }
}

void AliasClass::InsertMayDefDassign(StmtNode &stmt, BBId bbid) {
  std::set<OriginalSt*> mayDefOsts;
  CollectMayDefForDassign(stmt, mayDefOsts);
  InsertMayDefNode(mayDefOsts, ssaTab.GetStmtsSSAPart().SSAPartOf(stmt), stmt, bbid);
}

bool AliasClass::IsEquivalentField(TyIdx tyIdxA, FieldID fldA, TyIdx tyIdxB, FieldID fldB) const {
  if (mirModule.IsJavaModule() && tyIdxA != tyIdxB) {
    (void)klassHierarchy->UpdateFieldID(tyIdxA, tyIdxB, fldA);
  }
  return fldA == fldB;
}

bool AliasClass::IsAliasInfoEquivalentToExpr(const AliasInfo &ai, const BaseNode *expr) {
  if (ai.ae == nullptr || ai.offset.IsInvalid()) {
    return false;
  }
  switch (expr->GetOpCode()) {
    case OP_addrof:
    case OP_dread:
    case OP_regread: {
      return true;
    }
    case OP_iread: {
      BaseNode *base = expr->Opnd(0);
      return IsAliasInfoEquivalentToExpr(CreateAliasElemsExpr(*base), base);
    }
    default: {
      return false;
    }
  }
}

void AliasClass::CollectMayDefForIassign(StmtNode &stmt, std::set<OriginalSt*> &mayDefOsts) {
  auto &iassignNode = static_cast<IassignNode&>(stmt);
  AliasElem *lhsAe =
      FindOrCreateExtraLevAliasElem(*iassignNode.Opnd(0), iassignNode.GetTyIdx(), iassignNode.GetFieldID(), false);

  // lhsAe does not alias with any aliasElem
  if (lhsAe->GetClassSet() == nullptr) {
    (void)mayDefOsts.insert(&lhsAe->GetOriginalSt());
    return;
  }
  OriginalSt *ostOfLhsAe = &lhsAe->GetOriginalSt();
  TyIdx pointedTyIdx = ostOfLhsAe->GetTyIdx();
  BaseNode *rhs = iassignNode.GetRHS();
  // we will try to filter rhs's alias element when rhs is iread expr
  AliasInfo ai = CreateAliasElemsExpr(*rhs);
  OriginalSt *rhsOst = nullptr;
  if (rhs->GetOpCode() == OP_iread && IsAliasInfoEquivalentToExpr(ai, rhs)) {
    rhsOst = ai.ae->GetOst();
  }
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
      if (!MayAliasBasicAA(ostOfLhsAe, &ostOfAliasAE)) {
        continue;
      }
      if (TypeBasedAliasAnalysis::FilterAliasElemOfRHSForIassign(&ostOfAliasAE, ostOfLhsAe, rhsOst)) {
        continue;
      }
    }
    (void)mayDefOsts.insert(&ostOfAliasAE);
  }
}

void AliasClass::InsertMayDefNodeExcludeFinalOst(std::set<OriginalSt*> &mayDefOsts,
                                                 AccessSSANodes *ssapPart, StmtNode &stmt,
                                                 BBId bbid) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    if (!mayDefOst->IsFinal()) {
      ssapPart->InsertMayDefNode(MayDefNode(
          ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
      ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
    }
  }
}

void AliasClass::InsertMayDefIassign(StmtNode &stmt, BBId bbid) {
  std::set<OriginalSt*> mayDefOsts;
  CollectMayDefForIassign(stmt, mayDefOsts);
  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  if (mayDefOsts.size() == 1) {
    InsertMayDefNode(mayDefOsts, ssaPart, stmt, bbid);
  } else {
    InsertMayDefNodeExcludeFinalOst(mayDefOsts, ssaPart, stmt, bbid);
  }

  TypeOfMayDefList &mayDefNodes = ssaTab.GetStmtsSSAPart().GetMayDefNodesOf(stmt);
  // go thru inserted MayDefNode to add the base info
  TypeOfMayDefList::iterator it = mayDefNodes.begin();
  for (; it != mayDefNodes.end(); ++it) {
    MayDefNode &mayDef = it->second;
    OriginalSt *ost = mayDef.GetResult()->GetOst();
    if (ost->GetIndirectLev() == 1) {
      mayDef.base = ssaTab.GetVersionStTable().GetZeroVersionSt(ost->GetPrevLevelOst());
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
        for (OriginalSt *nextLevelOst : oSt->GetNextLevelOsts()) {
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
      theSSAPart->InsertMayUseNode(MayUseNode(vst0));
      theSSAPart->InsertMayDefNode(MayDefNode(vst0, &stmt));
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
      if (elemID != lhsAe->GetClassID()) {
        (void)mayDefOsts.insert(&aliasElem->GetOriginalSt());
      }
    }
  }
}

void AliasClass::CollectMayUseForNextLevel(const OriginalSt *ost, std::set<OriginalSt*> &mayUseOsts,
                                           const StmtNode &stmt, bool isFirstOpnd) {
  for (OriginalSt *nextLevelOst : ost->GetNextLevelOsts()) {
    AliasElem *indAe = FindAliasElem(*nextLevelOst);
    if (indAe->IsNotAllDefsSeen()) {
      return;
    }

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
      if (!indAe->IsNextLevNotAllDefsSeen()) {
        CollectMayUseForNextLevel(&indAe->GetOriginalSt(), mayUseOsts, stmt, false);
      }
    } else {
      for (unsigned int elemID : *(indAe->GetClassSet())) {
        OriginalSt *ost1 = &id2Elem[elemID]->GetOriginalSt();
        auto retPair = mayUseOsts.insert(ost1);
        if (retPair.second && !indAe->IsNextLevNotAllDefsSeen()) {
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
    expr = RemoveTypeConversionIfExist(expr);
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

void AliasClass::CollectMayDefUseForCallOpnd(const StmtNode &stmt,
                                             std::set<OriginalSt*> &mayDefOsts,
                                             std::set<OriginalSt*> &mayUseOsts) {
  size_t opndId = kOpcodeInfo.IsICall(stmt.GetOpCode()) ? 1 : 0;
  const FuncDesc *desc = nullptr;
  if (stmt.GetOpCode() == OP_call || stmt.GetOpCode() == OP_callassigned) {
    desc = &GetFuncDescFromCallStmt(static_cast<const CallNode&>(stmt));
  }
  for (; opndId < stmt.NumOpnds(); ++opndId) {
    if (desc != nullptr && (desc->IsArgUnused(opndId) || desc->IsArgReadSelfOnly(opndId))) {
      continue;
    }
    BaseNode *expr = stmt.Opnd(opndId);
    expr = RemoveTypeConversionIfExist(expr);
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

    if (desc != nullptr && desc->IsArgReadMemoryOnly(opndId)) {
      CollectMayUseForNextLevel(&aInfo.ae->GetOriginalSt(), mayUseOsts, stmt, opndId == 0);
      continue;
    }

    if (desc != nullptr && desc->IsArgWriteMemoryOnly(opndId)) {
      CollectMayUseForNextLevel(&aInfo.ae->GetOriginalSt(), mayDefOsts, stmt, opndId == 0);
      continue;
    }
    CollectMayUseForNextLevel(&aInfo.ae->GetOriginalSt(), mayDefOsts, stmt, opndId == 0);
    CollectMayUseForNextLevel(&aInfo.ae->GetOriginalSt(), mayUseOsts, stmt, opndId == 0);
  }
}

void AliasClass::InsertMayDefNodeForCall(std::set<OriginalSt*> &mayDefOsts, AccessSSANodes *ssaPart,
                                         StmtNode &stmt, BBId bbid,
                                         bool hasNoPrivateDefEffect) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    if (!hasNoPrivateDefEffect || !mayDefOst->IsPrivate()) {
      ssaPart->InsertMayDefNode(MayDefNode(
          ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
      ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
    }
  }
}

// Insert mayDefs and mayUses for the callees.
// Four kinds of mayDefs and mayUses are inserted, which are caused by callee
// opnds, not_all_def_seen_ae, globalsAffectedByCalls, and mustDefs.
void AliasClass::InsertMayDefUseCall(StmtNode &stmt, BBId bbid, bool isDirectCall) {
  bool hasSideEffect = true;
  bool hasNoPrivateDefEffect = false;
  const FuncDesc *desc = nullptr;
  if (isDirectCall) {
    desc = &GetFuncDescFromCallStmt(static_cast<CallNode&>(stmt));
    hasSideEffect = !desc->IsPure() && !desc->IsConst();
  }
  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  std::set<OriginalSt*> mayDefOstsA;
  std::set<OriginalSt*> mayUseOstsA;
  // 1. collect mayDefs and mayUses caused by callee-opnds
  CollectMayDefUseForCallOpnd(stmt, mayDefOstsA, mayUseOstsA);
  // 2. collect mayDefs and mayUses caused by not_all_def_seen_ae
  mayUseOstsA.insert(nadsOsts.begin(), nadsOsts.end());
  if (hasSideEffect) {
    mayDefOstsA.insert(nadsOsts.begin(), nadsOsts.end());
  }
  // insert mayuse node caused by opnd and not_all_def_seen_ae.
  InsertMayUseNode(mayUseOstsA, ssaPart);
  // insert maydef node caused by opnd and not_all_def_seen_ae.
  InsertMayDefNodeForCall(mayDefOstsA, ssaPart, stmt, bbid, hasNoPrivateDefEffect);
  // 3. insert mayDefs and mayUses caused by globalsAffectedByCalls
  std::set<OriginalSt*> mayDefUseOstsB;
  CollectMayUseFromGlobalsAffectedByCalls(mayDefUseOstsB);
  if (desc == nullptr || !desc->IsConst()) {
    InsertMayUseNode(mayDefUseOstsB, ssaPart);
  }
  // insert may def node, if the callee has side-effect.
  if (hasSideEffect) {
    InsertMayDefNodeExcludeFinalOst(mayDefUseOstsB, ssaPart, stmt, bbid);
  }
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    // 4. insert mayDefs caused by the mustDefs
    std::set<OriginalSt*> mayDefOstsC;
    CollectMayDefForMustDefs(stmt, mayDefOstsC);
    InsertMayDefNodeExcludeFinalOst(mayDefOstsC, ssaPart, stmt, bbid);
  }
}

void AliasClass::InsertMayUseNodeExcludeFinalOst(const std::set<OriginalSt*> &mayUseOsts,
                                                 AccessSSANodes *ssaPart) {
  for (OriginalSt *mayUseOst : mayUseOsts) {
    if (!mayUseOst->IsFinal()) {
      ssaPart->InsertMayUseNode(MayUseNode(
          ssaTab.GetVersionStTable().GetVersionStVectorItem(mayUseOst->GetZeroVersionIndex())));
    }
  }
}

// Insert mayDefs and mayUses for intrinsiccall.
// Four kinds of mayDefs and mayUses are inserted, which are caused by callee
// opnds, not_all_def_seen_ae, globalsAffectedByCalls, and mustDefs.
void AliasClass::InsertMayDefUseIntrncall(StmtNode &stmt, BBId bbid) {
  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  auto &intrinNode = static_cast<IntrinsiccallNode&>(stmt);
  IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrinNode.GetIntrinsic()];
  std::set<OriginalSt*> mayDefUseOsts;
  // 1. collect mayDefs and mayUses caused by not_all_defs_seen_ae
  if (!mirModule.IsCModule()) {
    for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
      InsertMayUseExpr(*stmt.Opnd(i));
    }
  } else {
    CollectMayUseForCallOpnd(stmt, mayDefUseOsts);
  }
  // 2. collect mayDefs and mayUses caused by not_all_defs_seen_ae
  mayDefUseOsts.insert(nadsOsts.begin(), nadsOsts.end());
  // 3. collect mayDefs and mayUses caused by globalsAffectedByCalls
  CollectMayUseFromGlobalsAffectedByCalls(mayDefUseOsts);
  InsertMayUseNodeExcludeFinalOst(mayDefUseOsts, ssaPart);
  if (!intrinDesc->HasNoSideEffect() || calleeHasSideEffect) {
    InsertMayDefNodeExcludeFinalOst(mayDefUseOsts, ssaPart, stmt, bbid);
  }
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    // 4. insert maydefs caused by the mustdefs
    std::set<OriginalSt*> mayDefOsts;
    CollectMayDefForMustDefs(stmt, mayDefOsts);
    InsertMayDefNodeExcludeFinalOst(mayDefOsts, ssaPart, stmt, bbid);
  }
}

void AliasClass::InsertMayDefUseClinitCheck(IntrinsiccallNode &stmt, BBId bbid) {
  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  for (OStIdx ostIdx : globalsMayAffectedByClinitCheck) {
    AliasElem *aliasElem = osym2Elem[ostIdx];
    OriginalSt &ostOfAE = aliasElem->GetOriginalSt();
    ssaPart->InsertMayDefNode(MayDefNode(
        ssaTab.GetVersionStTable().GetVersionStVectorItem(ostOfAE.GetZeroVersionIndex()), &stmt));
    ssaTab.AddDefBB4Ost(ostOfAE.GetIndex(), bbid);
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
      InsertMayDefUseCall(stmt, bbID, true);
      return;
    }
    case OP_asm:
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
      InsertMayDefUseCall(stmt, bbID, false);
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
