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

inline bool IsPotentialAddress(PrimType primType) {
  return IsAddress(primType) || IsPrimitiveDynType(primType);
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

// result = first - second => elements in first and not in second
template <typename FirstOstPtrSetType, typename SecondOstPtrSetType, typename ResOstPtrSetType>
inline void OstPtrSetSub(const FirstOstPtrSetType &first, const SecondOstPtrSetType &second, ResOstPtrSetType &result) {
  if (second.empty()) {
    result.insert(first.begin(), first.end());
  } else {
    std::set_difference(first.begin(), first.end(), second.begin(), second.end(),
                        std::inserter(result, result.end()), OriginalSt::OriginalStPtrComparator());
  }
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
void AliasClass::RecordAliasAnalysisInfo(const VersionSt &vst) {
  if (IsNextLevNotAllDefsSeen(vst.GetIndex())) {
    return;
  }
  auto &ost = *vst.GetOst();
  if (IsNotAllDefsSeen(ost.GetIndex())) {
    return;
  }
  if (ost.IsSymbolOst() && ost.GetIndirectLev() >= 0) {
    const MIRSymbol *sym = ost.GetMIRSymbol();
    if (sym->IsGlobal() && (mirModule.IsCModule() || (!sym->HasAddrOfValues() && !sym->GetIsTmp()))) {
      (void)globalsMayAffectedByClinitCheck.insert(ost.GetIndex());
      if (!sym->IsReflectionClassInfo()) {
        if (!ost.IsFinal() || InConstructorLikeFunc()) {
          (void)globalsAffectedByCalls.insert(&ost);
          if (mirModule.IsCModule()) {
            SetNotAllDefsSeen(ost.GetIndex());
          }
        }
        SetNextLevNotAllDefsSeen(vst.GetIndex());
      }
    } else if (mirModule.IsCModule() &&
               (sym->GetStorageClass() == kScPstatic || sym->GetStorageClass() == kScFstatic)) {
      (void)globalsAffectedByCalls.insert(&ost);
      SetNotAllDefsSeen(ost.GetIndex());
      SetNextLevNotAllDefsSeen(vst.GetIndex());
    }
  }

  if ((ost.IsFormal() && !IsRestrictPointer(&ost) && ost.GetIndirectLev() >= 0) || ost.GetIndirectLev() > 0) {
    SetNextLevNotAllDefsSeen(vst.GetIndex());
  }
  if (ost.GetIndirectLev() > 1 || (IsNextLevNotAllDefsSeen(ost.GetPointerVstIdx()) && !ost.IsFinal())) {
    SetNotAllDefsSeen(ost.GetIndex());
  }
  unionFind.NewMember(vst.GetIndex());
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
    int64 index = static_cast<MIRIntConst*>(mirConst)->GetExtValue();
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
OriginalSt *AliasClass::FindOrCreateExtraLevOst(SSATab *ssaTable, const VersionSt *pointerVst, const TyIdx &tyIdx,
                                                FieldID fld, OffsetType offset) {
  auto nextLevOst = ssaTable->GetOriginalStTable().FindOrCreateExtraLevOriginalSt(pointerVst, tyIdx, fld, offset);
  ssaTable->GetVersionStTable().CreateZeroVersionSt(nextLevOst);
  return nextLevOst;
}

static void UpdateFieldIdAndPtrType(const MIRType &baseType, FieldID baseFldId, OffsetType &offset,
                                    TyIdx &memPtrTyIdx, FieldID &fld) {
  MIRType *memPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(memPtrTyIdx);
  ASSERT(memPtrType->IsMIRPtrType(), "tyIdx is TyIdx of iread/iassign, must be pointer type!");
  auto *memType = static_cast<MIRPtrType*>(memPtrType)->GetPointedType();
  if (fld != 0) {
    CHECK_FATAL(memType->IsStructType(), "must be struct type");
    // to be conservative, return for out-of-bound fld
    if (static_cast<int32>(memType->NumberOfFieldIDs()) < fld) {
      return;
    }
    offset += static_cast<MIRStructType*>(memType)->GetBitOffsetFromBaseAddr(fld);
  }

  if (offset.IsInvalid()) {
    return;
  }
  if (!baseType.IsMIRPtrType()) {
    return;
  }
  if (baseFldId != 0) {
    return;
  }
  MIRType *baseMemType = static_cast<const MIRPtrType&>(baseType).GetPointedType();
  if (!baseMemType->IsStructType() ||
      static_cast<int32>(baseMemType->NumberOfFieldIDs()) < baseFldId) {
    return;
  }
  auto *structType = static_cast<MIRStructType*>(baseMemType);
  MIRType *fieldType = structType->GetFieldType(baseFldId);
  if (fieldType->GetTypeIndex() != memType->GetTypeIndex()) {
    return;
  }

  auto newFldId = baseFldId + fld;
  auto offsetOfNewFld = structType->GetBitOffsetFromBaseAddr(newFldId);
  if (offset.val != offsetOfNewFld) {
    return;
  }

  memPtrTyIdx = baseType.GetTypeIndex();
  fld = newFldId;
}

// return next level of baseAddress. Argument tyIdx specifies the pointer of the memory accessed.
// fieldId represent offset from type of this memory.
// Example: iread <*type> fld (base) or iassign <*type> fld (base):
//   tyIdx is TyIdx of <*type>(notice not <type>), fieldId is fld
VersionSt *AliasClass::FindOrCreateVstOfExtraLevOst(BaseNode &expr, const TyIdx &tyIdx,
                                                    FieldID fieldId, bool typeHasBeenCasted) {
  auto *baseAddr = RemoveTypeConversionIfExist(&expr);
  AliasInfo aliasInfoOfBaseAddress = CreateAliasInfoExpr(*baseAddr);
  if (aliasInfoOfBaseAddress.vst == nullptr) {
    return FindOrCreateDummyNADSVst();
  }
  auto *vstOfBaseAddress = aliasInfoOfBaseAddress.vst;
  auto *ostOfBaseAddress = vstOfBaseAddress->GetOst();
  if (mirModule.IsCModule() && IsNullOrDummySymbolOst(ostOfBaseAddress)) {
    return FindOrCreateDummyNADSVst();
  }
  // calculate the offset of extraLevOst (i.e. offset from baseAddr)
  OffsetType offset = typeHasBeenCasted ? OffsetType(kOffsetUnknown) : aliasInfoOfBaseAddress.offset;
  // If base has a valid baseFld, check type of this baseFld. If it is the same as tyIdx,
  // update tyIdx as base memory type (not fieldType now), update fieldId by merging fieldId and baseFld
  TyIdx newTyIdx = tyIdx;
  MIRType *baseType = ostOfBaseAddress->GetType();
  FieldID baseFld = aliasInfoOfBaseAddress.fieldID;
  UpdateFieldIdAndPtrType(*baseType, baseFld, offset, newTyIdx, fieldId);

  auto *nextLevOst = FindOrCreateExtraLevOst(&ssaTab, vstOfBaseAddress, newTyIdx, fieldId, offset);
  ASSERT(nextLevOst != nullptr, "failed in creating next-level-ost");
  auto *zeroVersionOfNextLevOst = ssaTab.GetVerSt(nextLevOst->GetZeroVersionIndex());
  RecordAliasAnalysisInfo(*zeroVersionOfNextLevOst);
  return zeroVersionOfNextLevOst;
}

VersionSt &AliasClass::FindOrCreateVstOfAddrofOSt(OriginalSt &oSt) {
  CHECK_FATAL(oSt.GetIndirectLev() == 0, "ost must be at level 0");
  auto vstOfAddrofOst = ssaTab.GetVerSt(oSt.GetPointerVstIdx());
  if (vstOfAddrofOst != nullptr) {
    return *vstOfAddrofOst;
  }

  OriginalSt *zeroFieldIDOst = &oSt;
  if (oSt.GetFieldID() != 0) {
    zeroFieldIDOst = ssaTab.FindOrCreateSymbolOriginalSt(*oSt.GetMIRSymbol(), oSt.GetPuIdx(), 0);
    auto *zeroVersionOfZeroFieldIDOst = ssaTab.GetVersionStTable().GetOrCreateZeroVersionSt(*zeroFieldIDOst);
    RecordAliasAnalysisInfo(*zeroVersionOfZeroFieldIDOst);
  }

  OriginalSt *addrofOst = ssaTab.FindOrCreateAddrofSymbolOriginalSt(zeroFieldIDOst);
  auto *zeroVersionVst = ssaTab.GetVerSt(addrofOst->GetZeroVersionIndex());

  if (oSt.GetFieldID() != 0) {
    oSt.SetPointerVst(zeroVersionVst);
    ssaTab.GetOriginalStTable().AddNextLevelOstOfVst(zeroVersionVst, &oSt);
  }

  RecordAliasAnalysisInfo(*zeroVersionVst);
  return *zeroVersionVst;
}

AliasInfo AliasClass::CreateAliasInfoExpr(BaseNode &expr) {
  switch (expr.GetOpCode()) {
    case OP_addrof: {
      AddrofSSANode &addrof = static_cast<AddrofSSANode &>(expr);
      VersionSt *vst = addrof.GetSSAVar();
      OriginalSt &oSt = *vst->GetOst();
      oSt.SetAddressTaken();
      RecordAliasAnalysisInfo(*vst);
      auto vstOfAddrof = &FindOrCreateVstOfAddrofOSt(oSt);
      int64 offsetVal =
          (addrof.GetFieldID() == 0) ? 0 : oSt.GetMIRSymbol()->GetType()->GetBitOffsetFromBaseAddr(oSt.GetFieldID());
      return AliasInfo(vstOfAddrof, addrof.GetFieldID(), OffsetType(offsetVal));
    }
    case OP_dread: {
      VersionSt *vst = static_cast<AddrofSSANode&>(expr).GetSSAVar();
      OriginalSt *ost = vst->GetOst();
      if (ost->GetFieldID() != 0 && ost->GetPrevLevelOst() == nullptr) {
        (void) FindOrCreateVstOfAddrofOSt(*ost);
      }

      RecordAliasAnalysisInfo(*vst);
      return AliasInfo(vst, 0, OffsetType(0));
    }
    case OP_regread: {
      VersionSt *vst = static_cast<RegreadSSANode&>(expr).GetSSAVar();
      OriginalSt &oSt = *vst->GetOst();
      return (oSt.IsSpecialPreg()) ? AliasInfo() : AliasInfo(vst, 0);
    }
    case OP_iread: {
      auto &iread = static_cast<IreadSSANode&>(expr);
      MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx());
      CHECK_FATAL(mirType->GetKind() == kTypePointer, "CreateAliasInfoExpr: ptr type expected in iread");
      auto *typeOfField = static_cast<MIRPtrType *>(mirType)->GetPointedType();
      if (iread.GetFieldID() > 0) {
        typeOfField = static_cast<MIRStructType *>(typeOfField)->GetFieldType(iread.GetFieldID());
      }
      bool typeHasBeenCasted = IreadedMemInconsistentWithPointedType(iread.GetPrimType(), typeOfField->GetPrimType());
      return AliasInfo(
          FindOrCreateVstOfExtraLevOst(*iread.Opnd(0), iread.GetTyIdx(), iread.GetFieldID(), typeHasBeenCasted),
          0, OffsetType(0));
    }
    case OP_iaddrof: {
      auto &iread = static_cast<IreadNode&>(expr);
      const auto &aliasInfo = CreateAliasInfoExpr(*iread.Opnd(0));
      OffsetType offset = aliasInfo.offset;
      if (iread.GetFieldID() != 0) {
        auto mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx());
        auto pointeeType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
        OffsetType offsetOfField(pointeeType->GetBitOffsetFromBaseAddr(iread.GetFieldID()));
        offset = offset + offsetOfField;
      }
      return AliasInfo(aliasInfo.vst, iread.GetFieldID(), offset);
    }
    case OP_add:
    case OP_sub: {
      const auto &aliasInfo = CreateAliasInfoExpr(*expr.Opnd(0));
      (void) CreateAliasInfoExpr(*expr.Opnd(1));

      if (aliasInfo.offset.IsInvalid()) {
        return AliasInfo(aliasInfo.vst, 0, OffsetType::InvalidOffset());
      }

      auto *opnd = expr.Opnd(1);
      if (!opnd->IsConstval() || !IsAddress(expr.GetPrimType())) {
        return AliasInfo(aliasInfo.vst, 0, OffsetType::InvalidOffset());
      }
      auto mirConst = static_cast<ConstvalNode*>(opnd)->GetConstVal();
      CHECK_FATAL(mirConst->GetKind() == kConstInt, "array index must be integer");
      int64 constVal = static_cast<MIRIntConst*>(mirConst)->GetExtValue();
      if (expr.GetOpCode() == OP_sub) {
        constVal = -constVal;
      } else {
        ASSERT(expr.GetOpCode() == OP_add, "Wrong operation!");
      }
      OffsetType newOffset = aliasInfo.offset + static_cast<uint64>(constVal) * static_cast<uint64>(bitsPerByte);
      return AliasInfo(aliasInfo.vst, 0, newOffset);
    }
    case OP_array: {
      for (size_t i = 1; i < expr.NumOpnds(); ++i) {
        (void) CreateAliasInfoExpr(*expr.Opnd(i));
      }

      const auto &aliasInfo = CreateAliasInfoExpr(*expr.Opnd(0));
      OffsetType offset = OffsetInBitOfArrayElement(static_cast<const ArrayNode*>(&expr));
      OffsetType newOffset = offset + aliasInfo.offset;
      return AliasInfo(aliasInfo.vst, 0, newOffset);
    }
    case OP_cvt:
    case OP_retype: {
      return CreateAliasInfoExpr(*expr.Opnd(0));
    }
    case OP_select: {
      (void) CreateAliasInfoExpr(*expr.Opnd(0));
      AliasInfo ainfo = CreateAliasInfoExpr(*expr.Opnd(1));
      AliasInfo ainfo2 = CreateAliasInfoExpr(*expr.Opnd(2));
      if (!OpCanFormAddress(expr.Opnd(1)->GetOpCode()) && !OpCanFormAddress(expr.Opnd(2)->GetOpCode())) {
        break;
      }
      if (ainfo.vst == nullptr) {
        return ainfo2;
      }
      if (ainfo2.vst == nullptr) {
        return ainfo;
      }
      ApplyUnionForDassignCopy(*ainfo.vst, ainfo2.vst, *expr.Opnd(2));
      return ainfo;
    }
    case OP_intrinsicop: {
      auto &intrn = static_cast<IntrinsicopNode&>(expr);
      if (intrn.GetIntrinsic() == INTRN_MPL_READ_OVTABLE_ENTRY ||
          (intrn.GetIntrinsic() == INTRN_JAVA_MERGE && intrn.NumOpnds() == 1 &&
           intrn.GetNopndAt(0)->GetOpCode() == OP_dread)) {
        return CreateAliasInfoExpr(*intrn.GetNopndAt(0));
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
        (void) CreateAliasInfoExpr(*expr.Opnd(i));
      }
  }
  return AliasInfo();
}

// when a mustDef is a pointer, set its pointees' notAllDefsSeen flag to true
void AliasClass::SetNotAllDefsSeenForMustDefs(const StmtNode &callas) {
  MapleVector<MustDefNode> &mustDefs = ssaTab.GetStmtsSSAPart().GetMustDefNodesOf(callas);
  for (auto &mustDef : mustDefs) {
    RecordAliasAnalysisInfo(*mustDef.GetResult());
    SetNextLevNotAllDefsSeen(mustDef.GetResult()->GetIndex());
  }
}

// given a struct/union assignment, regard it as if the fields that appear
// in the code are also assigned
void AliasClass::ApplyUnionForFieldsInCopiedAgg() {
  for (const auto &ostPair : aggsToUnion) {
    OriginalSt *lhsost = ostPair.first;
    OriginalSt *rhsost = ostPair.second;
    auto *preLevOfLHSOst = ssaTab.GetVerSt(lhsost->GetPointerVstIdx());
    if (preLevOfLHSOst == nullptr) {
      preLevOfLHSOst = &FindOrCreateVstOfAddrofOSt(*lhsost);
    }
    auto *preLevOfRHSOst = ssaTab.GetVerSt(rhsost->GetPointerVstIdx());
    if (preLevOfRHSOst == nullptr) {
      preLevOfRHSOst = &FindOrCreateVstOfAddrofOSt(*rhsost);
    }

    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsost->GetTyIdx());
    MIRStructType *mirStructType = static_cast<MIRStructType *>(mirType);
    FieldID numFieldIDs = static_cast<FieldID>(mirStructType->NumberOfFieldIDs());
    auto tyIdxOfPrevLevOst = preLevOfLHSOst->GetOst()->GetTyIdx();
    for (FieldID fieldID = 1; fieldID <= numFieldIDs; fieldID++) {
      MIRType *fieldType = mirStructType->GetFieldType(fieldID);
      if (!IsPotentialAddress(fieldType->GetPrimType())) {
        continue;
      }
      OffsetType offset(mirStructType->GetBitOffsetFromBaseAddr(fieldID));

      auto fieldOstLHS = ssaTab.GetOriginalStTable().FindExtraLevOriginalSt(
          preLevOfLHSOst, tyIdxOfPrevLevOst, fieldType, fieldID, offset);
      auto fieldOstRHS = ssaTab.GetOriginalStTable().FindExtraLevOriginalSt(
          preLevOfRHSOst, tyIdxOfPrevLevOst, fieldType, fieldID, offset);
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

      auto *zeroVersionStOfFieldOstLHS = ssaTab.GetVersionStTable().GetOrCreateZeroVersionSt(*fieldOstLHS);
      RecordAliasAnalysisInfo(*zeroVersionStOfFieldOstLHS);

      auto *zeroVersionStOfFieldOstRHS = ssaTab.GetVersionStTable().GetOrCreateZeroVersionSt(*fieldOstRHS);
      RecordAliasAnalysisInfo(*zeroVersionStOfFieldOstRHS);

      CHECK_FATAL(fieldOstLHS, "fieldOstLHS is nullptr!");
      CHECK_FATAL(fieldOstRHS, "fieldOstRHS is nullptr!");
      unionFind.Union(fieldOstLHS->GetZeroVersionIndex(), fieldOstRHS->GetZeroVersionIndex());
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
bool AliasClass::SetNextLevNADSForEscapePtr(const VersionSt &lhsVst, BaseNode &rhs) {
  TyIdx lhsTyIdx = lhsVst.GetOst()->GetTyIdx();
  PrimType lhsPtyp = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx)->GetPrimType();
  BaseNode *realRhs = RemoveTypeConversionIfExist(&rhs);
  PrimType rhsPtyp = realRhs->GetPrimType();
  if (lhsPtyp != rhsPtyp) {
    if ((IsPotentialAddress(lhsPtyp) && !IsPotentialAddress(rhsPtyp)) ||
        (!IsPotentialAddress(lhsPtyp) && IsPotentialAddress(rhsPtyp))) {
      SetNextLevNotAllDefsSeen(lhsVst.GetIndex());
      AliasInfo realRhsAinfo = CreateAliasInfoExpr(*realRhs);
      if (realRhsAinfo.vst != nullptr) {
        SetNextLevNotAllDefsSeen(realRhsAinfo.vst->GetIndex());
      }
      return true;
    } else if (IsPotentialAddress(lhsPtyp) &&
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

void AliasClass::ApplyUnionForDassignCopy(VersionSt &lhsVst, VersionSt *rhsVst, BaseNode &rhs) {
  if (SetNextLevNADSForEscapePtr(lhsVst, rhs)) {
    return;
  }
  if (rhsVst == nullptr) {
    SetNextLevNotAllDefsSeen(lhsVst.GetIndex());
    return;
  }

  auto *rhsOst = rhsVst->GetOst();
  if (rhsOst->GetIndirectLev() < 0) {
    for (auto *ost : *ssaTab.GetOriginalStTable().GetNextLevelOstsOfVst(rhsVst)) {
      SetNextLevNotAllDefsSeen(ost->GetZeroVersionIndex());
    }
  }

  if (mirModule.IsCModule()) {
    auto *lhsOst = lhsVst.GetOst();
    TyIdx lhsTyIdx = lhsOst->GetTyIdx();
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

  if (rhsOst->GetIndirectLev() > 0 || IsNotAllDefsSeen(rhsOst->GetIndex())) {
    SetNextLevNotAllDefsSeen(lhsVst.GetIndex());
    return;
  }
  PrimType rhsPtyp = rhsOst->GetType()->GetPrimType();
  if (!(IsPrimitiveInteger(rhsPtyp) && GetPrimTypeSize(rhsPtyp) == GetPrimTypeSize(PTY_ptr)) ||
      kOpcodeInfo.NotPure(rhs.GetOpCode()) ||
      HasMallocOpnd(&rhs) ||
      (rhs.GetOpCode() == OP_addrof && IsReadOnlyOst(*rhsOst))) {
    return;
  }
  unionFind.Union(lhsVst.GetIndex(), rhsVst->GetIndex());
}

void AliasClass::SetPtrOpndNextLevNADS(const BaseNode &opnd, VersionSt *vst, bool hasNoPrivateDefEffect) {
  if (vst == nullptr) {
    return;
  }

  auto *ost = vst->GetOst();
  if (IsPotentialAddress(opnd.GetPrimType()) &&
      !(hasNoPrivateDefEffect && ost->IsPrivate()) &&
      !(opnd.GetOpCode() == OP_addrof && IsReadOnlyOst(*ost))) {
    SetNextLevNotAllDefsSeen(vst->GetIndex());
    auto *prevLevVst = ssaTab.GetVerSt(ost->GetPointerVstIdx());
    if (ost->GetOffset().IsInvalid() && prevLevVst != nullptr) {
      auto *siblingOsts = ssaTab.GetOriginalStTable().GetNextLevelOstsOfVst(prevLevVst);
      for (auto *mayValueAliasOst : *siblingOsts) {
        if (!IsPotentialAddress(mayValueAliasOst->GetType()->GetPrimType())) {
          continue;
        }
        SetNextLevNotAllDefsSeen(mayValueAliasOst->GetZeroVersionIndex());
        unionFind.Union(vst->GetIndex(), mayValueAliasOst->GetZeroVersionIndex());
      }
    }
  }
  if (opnd.GetOpCode() == OP_cvt) {
    SetPtrOpndNextLevNADS(*opnd.Opnd(0), vst, hasNoPrivateDefEffect);
  }
}

// Set aliasElem of the pointer-type opnds of a call as next_level_not_all_defines_seen
void AliasClass::SetPtrOpndsNextLevNADS(unsigned int start, unsigned int end,
                                        MapleVector<BaseNode*> &opnds,
                                        bool hasNoPrivateDefEffect) {
  for (size_t i = start; i < end; ++i) {
    BaseNode *opnd = opnds[i];
    AliasInfo ainfo = CreateAliasInfoExpr(*opnd);
    SetPtrOpndNextLevNADS(*opnd, ainfo.vst, hasNoPrivateDefEffect);
  }
}

void AliasClass::SetAggPtrFieldsNextLevNADS(const OriginalSt &ost) {
  MIRTypeKind typeKind = ost.GetType()->GetKind();
  if (typeKind == kTypeStruct || typeKind == kTypeUnion || typeKind == kTypeStructIncomplete) {
    auto *structType = static_cast<MIRStructType*>(ost.GetType());
    auto *prevVst = ssaTab.GetVerSt(ost.GetPointerVstIdx());
    // if prevOst exist, all ptr nextLev ost should be set NextLevNADS
    if (prevVst != nullptr) {
      auto *nextLevelOsts = ssaTab.GetOriginalStTable().GetNextLevelOstsOfVst(prevVst);
      for (auto *nextOst : *nextLevelOsts) {
        if (IsNextLevNotAllDefsSeen(nextOst->GetZeroVersionIndex())) {
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
        if (!IsPotentialAddress(nextOst->GetType()->GetPrimType())) {
          MIRType *prevType = static_cast<MIRPtrType*>(prevVst->GetOst()->GetType())->GetPointedType();
          int64 bitOffset = prevType->GetBitOffsetFromBaseAddr(nextOst->GetFieldID());
          if (nextOst->GetOffset().val == bitOffset) {
            continue;
          }
        }
        SetNextLevNotAllDefsSeen(nextOst->GetZeroVersionIndex());
      }
      return;
    }
    // if prevOst not exist, all ptr field should be set NextLevNADS
    FieldID numFieldIDs = static_cast<FieldID>(structType->NumberOfFieldIDs());
    for (FieldID fieldID = 1; fieldID <= numFieldIDs; fieldID++) {
      MIRType *fieldType = structType->GetFieldType(fieldID);
      if (!IsPotentialAddress(fieldType->GetPrimType())) {
        continue;
      }
      int64 bitOffset = structType->GetBitOffsetFromBaseAddr(fieldID);
      OffsetType offset(bitOffset);
      OriginalSt *fldOst = ssaTab.GetOriginalStTable().FindSymbolOriginalSt(
          *ost.GetMIRSymbol(), fieldID, fieldType->GetTypeIndex(), offset);
      if (fldOst != nullptr) {
        SetNextLevNotAllDefsSeen(fldOst->GetZeroVersionIndex());
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
    AliasInfo aInfo = CreateAliasInfoExpr(*opnd);
    if (aInfo.vst == nullptr ||
        (IsNextLevNotAllDefsSeen(aInfo.vst->GetIndex()) &&
         aInfo.vst->GetOst()->GetIndirectLev() > 0)) {
      continue;
    }
    SetAggPtrFieldsNextLevNADS(*aInfo.vst->GetOst());
  }
}

void AliasClass::SetPtrFieldsOfAggNextLevNADS(const BaseNode *opnd, const VersionSt *vst) {
  if (opnd->GetPrimType() != PTY_agg) {
    return;
  }
  if (vst == nullptr ||
      (IsNextLevNotAllDefsSeen(vst->GetIndex()) &&
       vst->GetOst()->GetIndirectLev() > 0)) {
    return;
  }
  SetAggPtrFieldsNextLevNADS(*vst->GetOst());
}

// Iteratively propagate type unsafe info to next level.
void AliasClass::PropagateTypeUnsafeVertically(const VersionSt &vst) const {
  auto *nextLevelOsts = ssaTab.GetOriginalStTable().GetNextLevelOstsOfVst(&vst);
  if (nextLevelOsts == nullptr) {
    return;
  }
  for (auto *nextLevOst : *nextLevelOsts) {
    auto *zeroVersionSt = ssaTab.GetVersionStTable().GetZeroVersionSt(nextLevOst);
    TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(*zeroVersionSt);
    PropagateTypeUnsafeVertically(*zeroVersionSt);
  }
}

bool AliasClass::IsGlobalOstTypeUnsafe(const OriginalSt &ost) const {
  if (ost.IsLocal()) {
    return false;
  }
  const MIRSymbol *sym = ost.GetMIRSymbol();
  if (sym == nullptr) {
    return false;
  }
  const GSymbolTable &gSymbolTable = GlobalTables::GetGsymTable();
  static std::vector<bool> unsafeGSym(gSymbolTable.GetSymbolTableSize(), false); // index is SyIdx
  static bool hasInited = false; // if unsafeGSym is initialed before cannot be checked by unsafeGSym.size
  if (!hasInited) {
    for (size_t i = 0; i < gSymbolTable.GetSymbolTableSize(); ++i) {
      const MIRSymbol *gSym = gSymbolTable.GetSymbol(i);
      if (gSym == nullptr) {
        continue;
      }
      // example : typeA *gSym = (typeA*)initVal (assume initVal with typeB*)
      if (gSym->IsConst()) {
        MIRConst *initValue = gSym->GetKonst();
        // set gSym type unsafe if type conversion occurs
        if (&initValue->GetType() != gSym->GetType() &&
            (initValue->GetType().IsMIRPtrType() || gSym->GetType()->IsMIRPtrType())) {
          unsafeGSym[gSym->GetStIndex()] = true;
        }
      }
    }
    hasInited = true;
  }
  return unsafeGSym[sym->GetStIndex()];
}
// Propagate type unsafe info as soon as assign set has been created.
// If any Ost in assign set is type unsafe, propagate type-unsafe info
// among all elements in this assign set and next level osts.
void AliasClass::PropagateTypeUnsafe() {
  if (!mirModule.IsCModule() || !MeOption::tbaa) {
    return;
  }

  TypeBasedAliasAnalysis::GetPtrTypeUnsafe().resize(ssaTab.GetVersionStTableSize(), false);
  for (auto *vst : ssaTab.GetVersionStTable()) {
    auto *assSet = GetAssignSet(*vst);
    if (assSet == nullptr) {
      auto *ost = vst->GetOst();
      if (ost->GetType()->IsUnsafeType() || TypeBasedAliasAnalysis::IsValueTypeUnsafe(*vst) ||
          IsGlobalOstTypeUnsafe(*ost)) {
        TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(*vst);
        // Vertical propagate : to next level.
        PropagateTypeUnsafeVertically(*vst);
      }
      continue;
    }
    bool unsafe = false;
    // if any element in assign set is typeUnsafe, all elements will be set unsafe
    for (auto valueAliasVstIdx : *assSet) {
      VersionSt *valueAliasVst = ssaTab.GetVerSt(valueAliasVstIdx);
      OriginalSt &elemOst = *valueAliasVst->GetOst();
      if (elemOst.GetType()->IsUnsafeType() || TypeBasedAliasAnalysis::IsValueTypeUnsafe(*valueAliasVst) ||
          IsGlobalOstTypeUnsafe(elemOst)) {
        unsafe = true;
        break;
      }
    }
    if (unsafe) {
      for (auto valueAliasVstIdx : *assSet) {
        VersionSt *valueAliasVst = ssaTab.GetVerSt(valueAliasVstIdx);
        // Horizontal propagate : in assignSet
        TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(*valueAliasVst);
        // Vertical propagate : to next level.
        PropagateTypeUnsafeVertically(*valueAliasVst);
      }
    }
  }
}

// type may be potential address type like u64/ptr etc.
bool AliasClass::IsAddrTypeConsistent(MIRType *typeA, MIRType *typeB) const {
  if (typeA == nullptr || typeB == nullptr) {
    return false;
  }
  if (typeA == typeB) {
    return true;
  }
  if (!typeA->IsMIRPtrType() || !typeB->IsMIRPtrType()) {
    return false;
  }
  // <* [N] elemType> and <* elemType> are consistent
  MIRType *pointedTypeA = static_cast<MIRPtrType *>(typeA)->GetPointedType();
  MIRType *pointedTypeB = static_cast<MIRPtrType *>(typeB)->GetPointedType();
  if (pointedTypeA->IsMIRArrayType()) {
    pointedTypeA = static_cast<MIRArrayType *>(pointedTypeA)->GetElemType();
  }
  if (pointedTypeB->IsMIRArrayType()) {
    pointedTypeB = static_cast<MIRArrayType *>(pointedTypeB)->GetElemType();
  }
  return pointedTypeA == pointedTypeB;
}

// Type of AliasInfo may be inconsistent with its corresponding expr.
// The root cause is AliasInfo of addrof/iaddrof has a type as pointer
// to base, instead of its field type.
MIRType *AliasClass::GetAliasInfoRealType(const AliasInfo &ai, const BaseNode &expr) {
  switch (expr.GetOpCode()) {
    case OP_addrof: {
      // type of addrof's vst is a pointer type to agg, instead of its field type.
      if (ai.fieldID == 0) {
        return ai.vst->GetOst()->GetType();
      }
      // if field is not 0, return pointer type of field type
      auto *aggType = static_cast<MIRPtrType *>(ai.vst->GetOst()->GetType())->GetPointedType();
      MIRType *fieldType = static_cast<MIRStructType *>(aggType)->GetFieldType(ai.fieldID);
      return GlobalTables::GetTypeTable().GetOrCreatePointerType(*fieldType);
    }
    case OP_iaddrof: {
      MIRType *memType = static_cast<const IreadNode&>(expr).GetType();
      return GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
    }
    case OP_dread:
    case OP_regread:
    case OP_iread: {
      return ai.vst->GetOst()->GetType();
    }
    case OP_add:
    case OP_sub:
    case OP_array:
    case OP_cvt:
    case OP_retype: {
      return GetAliasInfoRealType(CreateAliasInfoExpr(*expr.Opnd(0)), *expr.Opnd(0));
    }
    case OP_select: {
      AliasInfo ai1 = CreateAliasInfoExpr(*expr.Opnd(1));
      if (ai1.vst != nullptr) {
        return GetAliasInfoRealType(ai1, *expr.Opnd(1));
      }
      return GetAliasInfoRealType(CreateAliasInfoExpr(*expr.Opnd(2)), *expr.Opnd(2));
    }
    default: {
      return nullptr;
    }
  }
}

// if ae->ost is an address of an union type (or its field), we set ost type unsafe.
// example: union {int x; float y;} u; u.x and u.y must alias, although their type is incompatible
void AliasClass::SetTypeUnsafeForAddrofUnion(const VersionSt *vst) const {
  if (!mirModule.IsCModule() || !MeOption::tbaa) {
    return;
  }
  if (vst == nullptr) {
    return;
  }
  const OriginalSt &ost = *vst->GetOst();
  // if ost is an address of an union type (or its field), we set it type unsafe.
  if (ost.GetIndirectLev() == -1) {
    MIRType *rhsType = ost.GetType();
    ASSERT(rhsType->IsMIRPtrType(), "Ost with -1 indirect level must have pointer type!");
    MIRType *pointedType = static_cast<MIRPtrType*>(rhsType)->GetPointedType();
    if (pointedType->GetKind() == kTypeUnion) {
      TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(*vst);
    }
  }
}

// For x <- y, if type of x is different from y, type conversion occurs, and we should set them type unsafe.
void AliasClass::SetTypeUnsafeForTypeConversion(const VersionSt *lhsVst, BaseNode *rhsExpr) {
  if (!mirModule.IsCModule() || !MeOption::tbaa) {
    return;
  }
  AliasInfo rhsAinfo = CreateAliasInfoExpr(*rhsExpr);
  const VersionSt *rhsVst = rhsAinfo.vst;
  if (lhsVst == nullptr || rhsVst == nullptr) {
    return;
  }
  // if lhs and rhs have different type, set both of them type unsafe
  const OriginalSt &lhsOst = *lhsVst->GetOst();
  MIRType *lhsType = lhsOst.GetType();
  MIRType *rhsType = GetAliasInfoRealType(rhsAinfo, *rhsExpr);
  if ((lhsType->IsMIRPtrType() || rhsType->IsMIRPtrType()) &&
      !IsAddrTypeConsistent(lhsType, rhsType)) {
    TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(*lhsVst);
    TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(*rhsVst);
  }
}

void AliasClass::ApplyUnionForIntrinsicCall(const IntrinsiccallNode &intrinsicCall) {
  auto intrinsicId = intrinsicCall.GetIntrinsic();
  bool opndsNextLevNADS = (intrinsicId == INTRN_JAVA_POLYMORPHIC_CALL);
  for (uint32 i = 0; i < intrinsicCall.NumOpnds(); ++i) {
    auto opnd = intrinsicCall.Opnd(i);
    const AliasInfo &ainfo = CreateAliasInfoExpr(*opnd);
    if (opndsNextLevNADS) {
      SetPtrOpndNextLevNADS(*opnd, ainfo.vst, false);
    }

    // intrinsic call memset/memcpy return value of first opnd.
    // create copy relation between first opnd and the returned value.
    if (i == 0 && intrinsicCall.GetOpCode() == OP_intrinsiccallassigned &&
        (intrinsicId == INTRN_C_memset || intrinsicId == INTRN_C_memcpy)) {
      auto &mustDefs = ssaTab.GetStmtsSSAPart().GetMustDefNodesOf(intrinsicCall);
      if (mustDefs.empty()) {
        continue;
      }
      CHECK_FATAL(mustDefs.size() == 1, "multi-assign-part for C_memset/memcpy not supported");
      auto *assignedPtr = mustDefs.front().GetResult();
      if (ainfo.vst == nullptr) {
        continue;
      }
      ApplyUnionForDassignCopy(*ainfo.vst, assignedPtr, *opnd);
    }
  }
}

void AliasClass::ApplyUnionForCopies(StmtNode &stmt) {
  switch (stmt.GetOpCode()) {
    case OP_maydassign:
    case OP_dassign:
    case OP_regassign: {
      // RHS
      ASSERT_NOT_NULL(stmt.Opnd(0));
      AliasInfo rhsAinfo = CreateAliasInfoExpr(*stmt.Opnd(0));
      // LHS
      auto *lhsVst = ssaTab.GetStmtsSSAPart().GetAssignedVarOf(stmt);
      OriginalSt *lhsOst = lhsVst->GetOst();
      if (lhsOst->GetFieldID() != 0) {
        (void) FindOrCreateVstOfAddrofOSt(*lhsOst);
      }
      RecordAliasAnalysisInfo(*lhsVst);
      ApplyUnionForDassignCopy(*lhsVst, rhsAinfo.vst, *stmt.Opnd(0));
      SetTypeUnsafeForAddrofUnion(rhsAinfo.vst);
      return;
    }
    case OP_iassign: {
      auto &iassignNode = static_cast<IassignNode&>(stmt);
      AliasInfo rhsAinfo = CreateAliasInfoExpr(*iassignNode.Opnd(1));
      auto *lhsVst =
        FindOrCreateVstOfExtraLevOst(*iassignNode.Opnd(0), iassignNode.GetTyIdx(), iassignNode.GetFieldID(), false);
      if (lhsVst != nullptr) {
        ApplyUnionForDassignCopy(*lhsVst, rhsAinfo.vst, *iassignNode.Opnd(1));
      }
      SetTypeUnsafeForAddrofUnion(rhsAinfo.vst);
      if (iassignNode.IsExpandedFromArrayOfCharFunc()) {
        TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(lhsVst->GetOst()->GetPointerVstIdx());
      }
      return;
    }
    case OP_throw: {
      AliasInfo ainfo = CreateAliasInfoExpr(*stmt.Opnd(0));
      SetPtrOpndNextLevNADS(*stmt.Opnd(0), ainfo.vst, false);
      return;
    }
    case OP_call:
    case OP_callassigned: {
      const FuncDesc &desc = GetFuncDescFromCallStmt(static_cast<CallNode&>(stmt));
      bool hasnoprivatedefeffect = CallHasNoPrivateDefEffect(&stmt);
      for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
        const AliasInfo &ainfo = CreateAliasInfoExpr(*stmt.Opnd(i));
        // no need to solve args that are not used or readSelfOnly.
        if (desc.IsArgUnused(i)) {
          continue;
        }
        if (desc.IsReturnNoAlias() && desc.IsArgReadSelfOnly(i)) {
          continue;
        }
        if (desc.IsReturnNoAlias() && desc.IsArgReadMemoryOnly(i) &&
            ainfo.vst != nullptr && ssaTab.GetNextLevelOsts(*ainfo.vst) != nullptr) {
          // Arg reads memory, we should set mayUse(*arg) here.
          // If it has next level, memory alias of its nextLev will be inserted to MayUse later.
          // If it has no next level, no elements will be inserted thru this arg.
          continue;
        }
        SetPtrOpndNextLevNADS(*stmt.Opnd(i), ainfo.vst, hasnoprivatedefeffect);
        SetPtrFieldsOfAggNextLevNADS(stmt.Opnd(i), ainfo.vst);
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
        const AliasInfo &ainfo = CreateAliasInfoExpr(*stmt.Opnd(i));
        if (ainfo.vst == nullptr) {
          continue;
        }
        if (i == 0) {
          continue;
        }
        // no need to solve args that are not used.
        if (desc.IsArgUnused(i)) {
          continue;
        }
        if (hasnoprivatedefeffect && ainfo.vst->GetOst()->IsPrivate()) {
          continue;
        }
        if (!IsPotentialAddress(stmt.Opnd(i)->GetPrimType())) {
          continue;
        }
        if (stmt.Opnd(i)->GetOpCode() == OP_addrof && IsReadOnlyOst(*ainfo.vst->GetOst())) {
          continue;
        }
        SetNextLevNotAllDefsSeen(ainfo.vst->GetIndex());
      }
      break;
    }
    case OP_asm:
    case OP_icall:
    case OP_icallassigned:
    case OP_icallproto:
    case OP_icallprotoassigned: {
      for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
        const AliasInfo &ainfo = CreateAliasInfoExpr(*stmt.Opnd(i));
        if (stmt.GetOpCode() != OP_asm && i == 0) {
          continue;
        }
        SetPtrOpndNextLevNADS(*stmt.Opnd(i), ainfo.vst, false);
        SetPtrFieldsOfAggNextLevNADS(stmt.Opnd(i), ainfo.vst);
      }
      break;
    }
    case OP_intrinsiccall:
    case OP_intrinsiccallassigned: {
      ApplyUnionForIntrinsicCall(static_cast<IntrinsiccallNode&>(stmt));
      break;
    }
    default: {
      for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        (void) CreateAliasInfoExpr(*stmt.Opnd(i));
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
          RecordAliasAnalysisInfo(*mustDef.GetResult());
        }
        return;
      }
    }
    SetNotAllDefsSeenForMustDefs(stmt);
  }
}

void AliasClass::ApplyUnionForPhi(const PhiNode &phi) {
  auto lhs = phi.GetResult();
  RecordAliasAnalysisInfo(*lhs);
  auto &opnds = phi.GetPhiOpnds();
  for (auto *opnd : opnds) {
    RecordAliasAnalysisInfo(*opnd);
    unionFind.Union(lhs->GetIndex(), opnd->GetIndex());
  }
}

void AliasClass::CreateAssignSets() {
  // iterate through all the alias elems
  for (auto *vst : ssaTab.GetVersionStTable()) {
    auto vstIdx = vst->GetIndex();
    if (!unionFind.Find(vstIdx)) {
      // vst is not an alias analysis target
      continue;
    }

    auto vstIdxOfRoot = unionFind.Root(vstIdx);
    if (unionFind.GetElementsNumber(vstIdxOfRoot) > 1) {
      auto maxVstIdx = (vstIdx > vstIdxOfRoot) ? vstIdx : vstIdxOfRoot;
      if (maxVstIdx >= assignSetOfVst.size()) {
        constexpr uint32 bufferSize = 10;
        uint32 incNum = maxVstIdx + bufferSize - assignSetOfVst.size();
        assignSetOfVst.insert(assignSetOfVst.end(), incNum, nullptr);
      }
      if (assignSetOfVst[vstIdxOfRoot] == nullptr) {
        assignSetOfVst[vstIdxOfRoot] = acMemPool.New<AliasSet>(acAlloc.Adapter());
      }
      assignSetOfVst[vstIdx] = assignSetOfVst[vstIdxOfRoot];
      assignSetOfVst[vstIdxOfRoot]->insert(vstIdx);
    }
  }
}

void AliasClass::DumpAssignSets() {
  LogInfo::MapleLogger() << "/////// assign sets ///////\n";
  for (auto *vst : ssaTab.GetVersionStTable()) {
    auto vstIdx = vst->GetIndex();
    if (!unionFind.Find(vstIdx)) {
      // VersionSt is not an alias analysis target
      continue;
    }
    if (unionFind.Root(vstIdx) != vstIdx) {
      // VersionSt is no root of value-alias-set
      continue;
    }

    auto *assignSet = GetAssignSet(*vst);
    if (assignSet == nullptr) {
      LogInfo::MapleLogger() << "Alone: ";
      vst->Dump();
      LogInfo::MapleLogger() << '\n';
    } else {
      LogInfo::MapleLogger() << "Members of assign set " << vstIdx << ": ";
      for (auto valueAliasVst : *assignSet) {
        ssaTab.GetVerSt(valueAliasVst)->Dump();
        LogInfo::MapleLogger() << ", ";
      }
      LogInfo::MapleLogger() << '\n';
    }
  }
}

void AliasClass::GetValueAliasSetOfVst(size_t vstIdx, std::set<size_t> &result) {
  if (vstIdx >= assignSetOfVst.size() || assignSetOfVst[vstIdx] == nullptr) {
    return;
  }
  auto *assignSet = assignSetOfVst[vstIdx];
  if (assignSet == nullptr) {
    result.insert(vstIdx);
  } else {
    result.insert(assignSet->begin(), assignSet->end());
  }
}

void AliasClass::UnionAllPointedTos() {
  std::vector<OStIdx> pointedTos;
  for (auto *ost : ssaTab.GetOriginalStTable()) {
    // OriginalSt is pointed to by another OriginalSt
    if (ost->GetPrevLevelOst() != nullptr) {
      SetNotAllDefsSeen(ost->GetIndex());
      pointedTos.push_back(ost->GetIndex());
    }
  }
  for (size_t i = 1; i < pointedTos.size(); ++i) {
    unionFind.Union(pointedTos[0], pointedTos[i]);
  }
}

void AliasClass::UnionNextLevelOfAliasOst(OstPtrSet &ostsToUnionNextLev) {
  while (!ostsToUnionNextLev.empty()) {
    auto tmpSet = ostsToUnionNextLev;
    ostsToUnionNextLev.clear();
    for (auto *ost : tmpSet) {
      auto ostIdx = ost->GetIndex();
      if (unionFind.Root(ostIdx) != ostIdx) {
        continue;
      }

      std::set<OriginalSt *> mayAliasOsts;
      for (auto *vst : ssaTab.GetVersionStTable()) {
        if (!unionFind.Find(vst->GetOrigIdx()) ||
            unionFind.Root(vst->GetOrigIdx()) != ostIdx) {
          continue;
        }

        const auto &nextLevelOstCollector = [&](size_t vstIdx) -> void {
          auto *nextLevOsts = ssaTab.GetNextLevelOsts(vstIdx);
          if (nextLevOsts != nullptr) {
            mayAliasOsts.insert(nextLevOsts->begin(), nextLevOsts->end());
          }
        };

        auto *assignSet = GetAssignSet(*vst);
        if (assignSet == nullptr) {
          nextLevelOstCollector(vst->GetIndex());
          continue;
        } else {
          for (auto assignedVstIdx : *assignSet) {
            nextLevelOstCollector(assignedVstIdx);
          }
        }
      }

      if (mayAliasOsts.empty()) {
        continue;
      }

      auto it = mayAliasOsts.begin();
      // find the first non-final ost
      while (it != mayAliasOsts.end() && (*it)->IsFinal()) {
        ++it;
      }
      if (it == mayAliasOsts.end()) {
        continue;
      }

      auto ostIdxA = (*it)->GetIndex();
      ++it;
      for (; it != mayAliasOsts.end(); ++it) {
        auto *ostB = *it;
        if (ostB->IsFinal()) {
          continue;
        }
        auto ostIdxB = ostB->GetIndex();
        if (unionFind.Root(ostIdxA) != unionFind.Root(ostIdxB)) {
          unionFind.Union(ostIdxA, ostIdxB);

          auto *rootOst = ssaTab.GetOriginalStFromID(OStIdx(unionFind.Root(ostIdxA)));
          ostsToUnionNextLev.insert(rootOst);
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
    for (auto vst: ssaTab.GetVersionStTable()) {
      auto vstIdx = vst->GetIndex();
      if (!unionFind.Find(vstIdx)) {
        continue;
      }
      if (IsNextLevNotAllDefsSeen(vstIdx) || IsNotAllDefsSeen(vst->GetOrigIdx())) {
        const auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vstIdx);
        if (nextLevelOsts == nullptr) {
          continue;
        }
        for (auto *ost : *nextLevelOsts) {
          if (!ost->IsFinal() && !IsNotAllDefsSeen(ost->GetIndex())) {
            SetNotAllDefsSeen(ost->GetIndex());
            SetNextLevNotAllDefsSeen(ost->GetZeroVersionIndex());
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

  std::vector<bool> vstProcessed(ssaTab.GetVersionStTableSize(), false);
  for (auto vst : ssaTab.GetVersionStTable()) {
    auto vstIdx = vst->GetIndex();
    if (vstProcessed[vstIdx]) {
      continue;
    }

    auto *assignSet = GetAssignSet(*vst);
    if (assignSet == nullptr) {
      // next level Alias Elements of aliaselem may alias each other. union aliasing elements in following
      OstPtrSet ostsToUnionNextLev;
      auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vstIdx);
      if (nextLevelOsts == nullptr) {
        continue;
      }

      const auto &endIt = nextLevelOsts->end();
      for (auto ostItA = nextLevelOsts->begin(); ostItA != endIt; ++ostItA) {
        OStIdx ostIdxA = (*ostItA)->GetIndex();
        auto ostItB = ostItA;
        ++ostItB;
        for (; ostItB != endIt; ++ostItB) {
          if (!MayAliasBasicAA(*ostItA, *ostItB)) {
            continue;
          }
          OStIdx ostIdxB = (*ostItB)->GetIndex();
          if (unionFind.Root(ostIdxA) != unionFind.Root(ostIdxB)) {
            unionFind.Union(ostIdxA, ostIdxB);

            auto *rootOst = ssaTab.GetOriginalStFromID(OStIdx(unionFind.Root(ostIdxA)));
            ostsToUnionNextLev.insert(rootOst);
          }
        }
      }
      // union next-level-osts of aliased osts
      UnionNextLevelOfAliasOst(ostsToUnionNextLev);
      continue;
    }

    // iterate through all the alias elems to check if any has indirectLev > 0
    // or if any has nextLevNotAllDefsSeen being true
    bool hasNextLevNotAllDefsSeen = false;
    for (auto vstIdxB : *assignSet) {
      auto *ost = ssaTab.GetVerSt(vstIdxB)->GetOst();
      if (ost->GetIndirectLev() > 0 || IsNotAllDefsSeen(ost->GetIndex()) || IsNextLevNotAllDefsSeen(vstIdxB)) {
        hasNextLevNotAllDefsSeen = true;
        break;
      }
      vstProcessed[vstIdxB] = true;
    }
    if (hasNextLevNotAllDefsSeen) {
      // make all pointedto's in this assignSet notAllDefsSeen
      for (auto vstIdxB : *assignSet) {
        auto *nextLevelNodes = ssaTab.GetNextLevelOsts(vstIdxB);
        if (nextLevelNodes == nullptr) {
          continue;
        }
        for (auto *ost : *nextLevelNodes) {
          if (!ost->IsFinal()) {
            SetNotAllDefsSeen(ost->GetIndex());
          }
        }
      }
      continue;
    }

    // apply union among the assignSet elements
    OstPtrSet ostsToUnionNextLev;
    for (auto itA = assignSet->begin(); itA != assignSet->end(); ++itA) {
      auto *nextLevelNodesA = ssaTab.GetNextLevelOsts(*itA);
      if (nextLevelNodesA == nullptr) {
        continue;
      }

      auto itB = itA;
      ++itB;
      for (; itB != assignSet->end(); ++itB) {
        auto *nextLevelNodesB = ssaTab.GetNextLevelOsts(*itB);
        if (nextLevelNodesB == nullptr) {
          continue;
        }

        // union the next level of elements in the same assignSet
        for (auto ostA : *nextLevelNodesA) {
          for (auto ostB : *nextLevelNodesB) {
            if (!MayAliasBasicAA(ostA, ostB)) {
              continue;
            }
            uint32 rootA = unionFind.Root(ostA->GetIndex());
            uint32 rootB = unionFind.Root(ostB->GetIndex());
            if (rootA != rootB) {
              unionFind.Union(ostA->GetIndex(), ostB->GetIndex());

              auto newRootIdA = unionFind.Root(ostA->GetIndex());
              auto *rootOstA = ssaTab.GetOriginalStFromID(OStIdx(newRootIdA));
              ostsToUnionNextLev.insert(rootOstA);
            }
          }
        }
      }
    }
    // union next-level-osts of aliased osts
    UnionNextLevelOfAliasOst(ostsToUnionNextLev);
  }
}

void AliasClass::CollectRootIDOfNextLevelNodes(const MapleVector<OriginalSt*> *nextLevelOsts,
                                               std::set<unsigned int> &rootIDOfNADSs) {
  if (nextLevelOsts == nullptr) {
    return;
  }
  for (OriginalSt *nextLevelOst : *nextLevelOsts) {
    if (!nextLevelOst->IsFinal()) {
      (void)rootIDOfNADSs.insert(unionFind.Root(nextLevelOst->GetIndex()));
    }
  }
}

void AliasClass::UnionForNotAllDefsSeen() {
  std::set<unsigned int> rootOstIDOfNADSs;
  for (auto *vst : ssaTab.GetVersionStTable()) {
    auto vstIdx = vst->GetIndex();
    auto *assignSet = GetAssignSet(*vst);
    if (assignSet == nullptr) {
      if (IsNotAllDefsSeen(vst->GetOst()->GetIndex()) || IsNextLevNotAllDefsSeen(vstIdx)) {
        auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vstIdx);
        CollectRootIDOfNextLevelNodes(nextLevelOsts, rootOstIDOfNADSs);
      }
      continue;
    }
    for (size_t vstIdxA : *assignSet) {
      auto ostIdxA = ssaTab.GetVerSt(vstIdxA)->GetOrigIdx();
      if (IsNotAllDefsSeen(ostIdxA) || IsNextLevNotAllDefsSeen(vstIdxA)) {
        for (size_t vstIdxB : *assignSet) {
          auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vstIdxB);
          CollectRootIDOfNextLevelNodes(nextLevelOsts, rootOstIDOfNADSs);
        }
        break;
      }
    }
  }
  if (!rootOstIDOfNADSs.empty()) {
    unsigned int idA = *(rootOstIDOfNADSs.begin());
    rootOstIDOfNADSs.erase(rootOstIDOfNADSs.begin());
    OstPtrSet ostsToUnionNextLev;
    for (size_t idB : rootOstIDOfNADSs) {
      if (unionFind.Root(idA) != unionFind.Root(idB)) {
        unionFind.Union(idA, idB);

        auto *rootOst = ssaTab.GetOriginalStFromID(OStIdx(unionFind.Root(idA)));
        ostsToUnionNextLev.insert(rootOst);
      }
    }
    UnionNextLevelOfAliasOst(ostsToUnionNextLev);
    for (auto *ost : ssaTab.GetOriginalStTable()) {
      auto ostIdx = ost->GetIndex();
      if (unionFind.Find(ostIdx) && unionFind.Root(ostIdx) == unionFind.Root(idA)) {
        SetNotAllDefsSeen(ostIdx);
      }
    }
  }
}

void AliasClass::UnionForNotAllDefsSeenCLang() {
  std::vector<OStIdx> notAllDefsSeenOsts;
  for (auto *ost : ssaTab.GetOriginalStTable()) {
    if (IsNotAllDefsSeen(ost->GetIndex())) {
      notAllDefsSeenOsts.push_back(ost->GetIndex());
    }
  }

  if (notAllDefsSeenOsts.empty()) {
    return;
  }

  // nadsOst is the first notAllDefsSeen ost.
  // Union nadsOst with the other notAllDefsSeen osts.
  auto idA = notAllDefsSeenOsts[0];
  (void)notAllDefsSeenOsts.erase(notAllDefsSeenOsts.begin());
  OstPtrSet ostsToUnionNextLev;
  for (const auto &idB : notAllDefsSeenOsts) {
    if (unionFind.Root(idA) != unionFind.Root(idB)) {
      unionFind.Union(idA, idB);

      auto *rootOst = ssaTab.GetOriginalStFromID(OStIdx(unionFind.Root(idA)));
      ostsToUnionNextLev.insert(rootOst);
    }
  }
  UnionNextLevelOfAliasOst(ostsToUnionNextLev);
  uint rootIdOfNotAllDefsSeenAe = unionFind.Root(idA);

  for (auto *ost : ssaTab.GetOriginalStTable()) {
    auto ostIdx = ost->GetIndex();
    if (unionFind.Find(ostIdx) && unionFind.Root(ostIdx) == rootIdOfNotAllDefsSeenAe) {
      SetNotAllDefsSeen(ostIdx);
    }
  }

  // iterate through originalStTable; if the symbol (at level 0) is
  // notAllDefsSeen, then set the same for all its level 1 members; then
  // for each level 1 member of each symbol, if any is set notAllDefsSeen,
  // set all members at that level to same;
  for (VersionSt *vst : ssaTab.GetVersionStTable()) {
    if (!vst->GetOst()->IsSymbolOst()) {
      continue;
    }
    if (!unionFind.Find(vst->GetIndex())) {
      // vst is not alias analysis target
      continue;
    }

    auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vst->GetIndex());
    if (nextLevelOsts == nullptr) {
      continue;
    }

    bool nextLevelNotAllDefsSeen = IsNotAllDefsSeen(vst->GetOrigIdx());
    if (!nextLevelNotAllDefsSeen) {
      // see if any at level 1 has notAllDefsSeen set
      for (OriginalSt *nextLevelOst : *nextLevelOsts) {
        if (IsNotAllDefsSeen(nextLevelOst->GetIndex())) {
          nextLevelNotAllDefsSeen = true;
          break;
        }
      }
    }
    if (nextLevelNotAllDefsSeen) {
      // set to true for all members at this level
      for (OriginalSt *nextLevelOst : *nextLevelOsts) {
        SetNotAllDefsSeen(nextLevelOst->GetIndex());
        unionFind.Union(idA, nextLevelOst->GetIndex());
      }
    }
  }
}

void AliasClass::UnionForAggAndFields() {
  // key: index of MIRSymbol; value: OStIdx.
  std::map<uint32, std::set<OStIdx>> symbol2Osts;
  // collect alias elements with same MIRSymbol, and the ost is zero level.
  for (OriginalSt *ost : ssaTab.GetOriginalStTable()) {
    if (ost->GetIndirectLev() == 0 && ost->IsSymbolOst()) {
      (void)symbol2Osts[ost->GetMIRSymbol()->GetStIdx().FullIdx()].insert(ost->GetIndex());
    }
  }

  // union alias elements of Agg(fieldID == 0) and fields(fieldID > 0).
  for (auto &sym2Ost : symbol2Osts) {
    auto &ostsWithSameSymbol = sym2Ost.second;
    if (ostsWithSameSymbol.size() <= 1) {
      continue;
    }
    for (const auto idA : ostsWithSameSymbol) {
      auto *ostA = ssaTab.GetOriginalStFromID(idA);
      CHECK_FATAL(ostA, "ostA is nullptr!");
      if (ostA->GetFieldID() == 0) {
        (void)ostsWithSameSymbol.erase(idA);
        for (const auto &idB : ostsWithSameSymbol) {
          unionFind.Union(idA, idB);
        }
        break;
      }
    }
  }
}

// fabricate the imaginary not_all_def_seen AliasElem
VersionSt *AliasClass::FindOrCreateDummyNADSVst() {
  MIRSymbol *dummySym = mirModule.GetMIRBuilder()->GetOrCreateSymbol(TyIdx(PTY_i32), "__nads_dummysym__", kStVar,
                                                                     kScGlobal, nullptr, kScopeGlobal, false);
  ASSERT_NOT_NULL(dummySym);
  dummySym->SetIsTmp(true);
  dummySym->SetIsDeleted();
  OriginalSt *dummyOst = ssaTab.GetOriginalStTable().FindOrCreateSymbolOriginalSt(*dummySym, 0, 0);
  auto zeroVersionOfDummyOst = ssaTab.GetVersionStTable().GetOrCreateZeroVersionSt(*dummyOst);
  SetNotAllDefsSeen(dummyOst->GetIndex());
  dummyOst->SetOffset(OffsetType::InvalidOffset());
  if (MeOption::tbaa) {
    TypeBasedAliasAnalysis::SetVstValueTypeUnsafe(*zeroVersionOfDummyOst);
  }
  unionFind.NewMember(zeroVersionOfDummyOst->GetIndex());
  return zeroVersionOfDummyOst;
}

void AliasClass::UnionAllNodes(MapleVector<OriginalSt *> *nextLevOsts) {
  if (nextLevOsts == nullptr) {
    return;
  }
  if (nextLevOsts->size() < 2) {
    return;
  }

  auto it = nextLevOsts->begin();
  OriginalSt *ostA = *it;
  ++it;
  OstPtrSet ostsToUnionNextLev;
  for (; it != nextLevOsts->end(); ++it) {
    OriginalSt *ostB = *it;
    if (unionFind.Root(ostA->GetIndex()) != unionFind.Root(ostB->GetIndex())) {
      unionFind.Union(ostA->GetIndex(), ostB->GetIndex());

      auto *rootOst = ssaTab.GetOriginalStFromID(OStIdx(unionFind.Root(ostA->GetIndex())));
      ostsToUnionNextLev.insert(rootOst);
    }
  }
  // union next-level-osts of aliased osts
  UnionNextLevelOfAliasOst(ostsToUnionNextLev);
}

// This is applicable only for C language.  For each ost that is a struct,
// union all fields within the same struct
void AliasClass::ApplyUnionForStorageOverlaps() {
  // iterate through all the alias elems
  for (VersionSt *vst : ssaTab.GetVersionStTable()) {
    MIRType *mirType = vst->GetOst()->GetType();
    if (mirType->GetKind() != kTypePointer) {
      continue;
    }

    // work on the next indirect level
    auto *nextLevOsts = ssaTab.GetNextLevelOsts(vst->GetIndex());
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
      // offset of nextLevOst is indeteminate, it may alias all other next-level-osts
      if (nextLevOst->GetOffset().IsInvalid()) {
        UnionAllNodes(nextLevOsts);
        break;
      }

      MIRType *nextLevType = nextLevOst->GetType();
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
void AliasClass::CollectAliasGroups(std::map<unsigned, std::set<unsigned>> &aliasGroups) {
  // key is the root id. The set contains ids of aes that alias with the root.
  for (auto *ost : ssaTab.GetOriginalStTable()) {
    auto ostId = ost->GetIndex();
    unsigned int rootID = unionFind.Root(ostId);
    if (ostId == rootID) {
      continue;
    }

    if (aliasGroups.find(rootID) == aliasGroups.end()) {
      std::set<unsigned int> idsAliasWithRoot;
      (void)aliasGroups.insert(make_pair(rootID, idsAliasWithRoot));
    }
    (void)aliasGroups[rootID].insert(ostId);
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

bool AliasClass::AliasAccordingToFieldID(const OriginalSt &ostA, const OriginalSt &ostB) const {
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

void AliasClass::ProcessIdsAliasWithRoot(const std::set<uint32> &idsAliasWithRoot,
                                         std::vector<uint32> &newGroups) {
  for (uint32 ostIdxA : idsAliasWithRoot) {
    bool unioned = false;
    OriginalSt *ostA = ssaTab.GetOriginalStFromID(OStIdx(ostIdxA));
    for (uint32 ostIdxB : newGroups) {
      OriginalSt *ostB = ssaTab.GetOriginalStFromID(OStIdx(ostIdxB));
      CHECK_FATAL(ostB, "ostB is nullptr");
      CHECK_FATAL(ostB->GetPrevLevelOst(), "ostB PrevLevelOst is nullptr");
      if (AliasAccordingToType(ostA->GetPrevLevelOst()->GetTyIdx(), ostB->GetPrevLevelOst()->GetTyIdx()) &&
          AliasAccordingToFieldID(*ostA, *ostB)) {
        unionFind.Union(ostIdxA, ostIdxB);
        unioned = true;
        break;
      }
    }
    if (!unioned) {
      newGroups.push_back(ostIdxA);
    }
  }
}

void AliasClass::ReconstructAliasGroups() {
  // map the root id to the set contains the aliasElem-id that alias with the root.
  std::map<uint32, std::set<uint32>> aliasGroups;
  CollectAliasGroups(aliasGroups);
  unionFind.Reinit();
  // kv.first is the root id. kv.second is the id the alias with the root.
  for (const auto &oneGroup : aliasGroups) {
    std::vector<uint32> newGroups;  // contains one id of each new alias group.
    uint32 rootId = oneGroup.first;
    const std::set<uint32> &idsAliasWithRoot = oneGroup.second;
    newGroups.push_back(rootId);
    ProcessIdsAliasWithRoot(idsAliasWithRoot, newGroups);
  }
}

void AliasClass::CollectNotAllDefsSeenAes() {
  for (auto *ost : ssaTab.GetOriginalStTable()) {
    if (IsNotAllDefsSeen(ost->GetIndex())) {
      nadsOsts.insert(ost);
    }
  }
}

void AliasClass::CreateClassSets() {
  // iterate through all the alias elems
  for (OriginalSt *ost : ssaTab.GetOriginalStTable()) {
    auto ostIdx = ost->GetIndex();
    if (!unionFind.Find(ostIdx)) {
      continue;
    }
    unsigned int rootID = unionFind.Root(ostIdx);
    if (unionFind.GetElementsNumber(rootID) > 1) {
      auto *aliasSet = GetAliasSet(OStIdx(rootID));
      if (aliasSet == nullptr) {
        aliasSet = acMemPool.New<AliasSet>(acAlloc.Adapter());
        SetAliasSet(OStIdx(rootID), aliasSet);
      }
      SetAliasSet(ost->GetIndex(), aliasSet);
      (void)aliasSet->insert(ost->GetIndex());
    }
  }
  CollectNotAllDefsSeenAes();
#if DEBUG
  for (OriginalSt *ost : ssaTab.GetOriginalStTable()) {
    if (unionFind.Find(ost->GetIndex()) &&
        unionFind.Root(ost->GetIndex()) == ost->GetIndex() &&
        GetAliasSet(ost->GetIndex()) != nullptr) {
      CHECK_FATAL(GetAliasSet(ost->GetIndex())->size() == unionFind.GetElementsNumber(ost->GetIndex()),
                  "AliasClass::CreateClassSets: wrong result");
    }
  }
#endif
}

void AliasElem::Dump() const {
  ost.Dump();
  LogInfo::MapleLogger() << "id" << id << ((notAllDefsSeen) ? "? " : " ");
}

void AliasClass::DumpClassSets(bool onlyDumpRoot) {
  LogInfo::MapleLogger() << "/////// class sets ///////\n";
  for (auto *ost : ssaTab.GetOriginalStTable()) {
    auto ostIdx = ost->GetIndex();
    if (onlyDumpRoot) {
      if (!unionFind.Find(ostIdx)) {
        continue;
      }
      if (unionFind.Root(ostIdx) != ostIdx) {
        continue;
      }
    }

    auto *aliasSet = GetAliasSet(*ost);
    if (aliasSet == nullptr || aliasSet->size() == 1) {
      LogInfo::MapleLogger() << "Alone: ";
      ost->Dump();
      if (IsNotAllDefsSeen(ostIdx)) {
        LogInfo::MapleLogger() << "?";
      }
      LogInfo::MapleLogger() << '\n';
    } else {
      LogInfo::MapleLogger() << "Members of alias class " << ostIdx << ": ";
      for (auto aliasedOstIdx : *aliasSet) {
        CHECK_FATAL(ssaTab.GetOriginalStFromID(OStIdx(aliasedOstIdx)), "orig st is nullptr!");
        ssaTab.GetOriginalStFromID(OStIdx(aliasedOstIdx))->Dump();
        if (IsNotAllDefsSeen(OStIdx(aliasedOstIdx))) {
          LogInfo::MapleLogger() << "?";
        }
        LogInfo::MapleLogger() << ", ";
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

  if (ostA->GetIndex() >= aliasSetOfOst.size()) {
    return true;
  }
  if (ostB->GetIndex() >= aliasSetOfOst.size()) {
    return true;
  }

  auto *aliasSet = GetAliasSet(*ostA);
  if (aliasSet == nullptr) {
    return false;
  }
  return (aliasSet->find(ostB->GetIndex()) != aliasSet->end());
}

// here starts pass 2 code
void AliasClass::InsertMayUseExpr(BaseNode &expr) {
  for (size_t i = 0; i < expr.NumOpnds(); ++i) {
    InsertMayUseExpr(*expr.Opnd(i));
  }
  if (expr.GetOpCode() != OP_iread) {
    return;
  }
  AliasInfo rhsAinfo = CreateAliasInfoExpr(expr);
  if (rhsAinfo.vst == nullptr) {
    rhsAinfo.vst = FindOrCreateDummyNADSVst();
  }
  auto &ireadNode = static_cast<IreadSSANode&>(expr);
  ireadNode.SetSSAVar(*rhsAinfo.vst);
  ASSERT(ireadNode.GetSSAVar() != nullptr, "AliasClass::InsertMayUseExpr(): iread cannot have empty mayuse");
}

// collect the mayUses caused by globalsAffectedByCalls.
void AliasClass::CollectMayUseFromGlobalsAffectedByCalls(OstPtrSet &mayUseOsts) {
  mayUseOsts.insert(globalsAffectedByCalls.begin(), globalsAffectedByCalls.end());
}

// collect the mayUses caused by defined final field.
void AliasClass::CollectMayUseFromDefinedFinalField(OstPtrSet &mayUseOsts) {
  for (auto *ost : ssaTab.GetOriginalStTable()) {
    if (!ost->IsFinal()) {
      continue;
    }

    auto *prevLevelOst = ost->GetPrevLevelOst();
    if (prevLevelOst == nullptr) {
      continue;
    }

    TyIdx tyIdx = prevLevelOst->GetTyIdx();
    auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    ASSERT(ptrType->IsMIRPtrType(), "Type of pointer must be MIRPtrType!");
    tyIdx = static_cast<MIRPtrType*>(ptrType)->GetPointedTyIdx();
    if (tyIdx == mirModule.CurFunction()->GetClassTyIdx()) {
      (void)mayUseOsts.insert(ost);
    }
  }
}

// insert the ost of mayUseOsts into mayUseNodes
void AliasClass::InsertMayUseNode(OstPtrSet &mayUseOsts, AccessSSANodes *ssaPart) {
  for (OriginalSt *ost : mayUseOsts) {
    ssaPart->InsertMayUseNode(MayUseNode(
        ssaTab.GetVersionStTable().GetVersionStVectorItem(ost->GetZeroVersionIndex())));
  }
}

void AliasClass::CollectMayUseFromFormals(OstPtrSet &mayUseOsts, bool needToBeRestrict) {
  auto *curFunc = mirModule.CurFunction();
  for (size_t formalId = 0; formalId < curFunc->GetFormalCount(); ++formalId) {
    auto *formal = curFunc->GetFormal(formalId);
    if (!needToBeRestrict || formal->GetAttr(ATTR_restrict)) {
      auto *restrictFormalOst = ssaTab.GetOriginalStTable().FindSymbolOriginalSt(*formal);
      if (restrictFormalOst == nullptr) {
        continue;
      }

      auto zeroVersionIdxOfFormal = restrictFormalOst->GetZeroVersionIndex();
      auto zeroVersionSt = ssaTab.GetVerSt(zeroVersionIdxOfFormal);
      if (zeroVersionSt == nullptr) {
        continue;
      }

      const auto &collectNextLevelOsts = [&](size_t vstIdx) {
        auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vstIdx);
        if (nextLevelOsts == nullptr) {
          return;
        }
        for (auto *nextLevelOst : *nextLevelOsts) {
          mayUseOsts.insert(nextLevelOst);
        }
      };
      auto *assignSet = GetAssignSet(*zeroVersionSt);
      if (assignSet == nullptr) {
        collectNextLevelOsts(zeroVersionIdxOfFormal);
      } else {
        for (auto valueAliasVstIdx : *assignSet) {
          collectNextLevelOsts(valueAliasVstIdx);
        }
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
  OstPtrSet mayUseOsts;
  // 1. collect mayUses caused by not_all_def_seen_ae.
  mayUseOsts.insert(nadsOsts.begin(), nadsOsts.end());
  // 2. collect mayUses caused by globals_affected_by_call.
  CollectMayUseFromGlobalsAffectedByCalls(mayUseOsts);
  CollectMayUseFromFormals(mayUseOsts, true);
  // 3. collect mayUses caused by defined final field for constructor
  if (mirModule.CurFunction()->IsConstructor()) {
    CollectMayUseFromDefinedFinalField(mayUseOsts);
  }
  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  InsertMayUseNode(mayUseOsts, ssaPart);
}

// collect next_level_nodes of the ost of ReturnOpnd into mayUseOsts
void AliasClass::CollectPtsToOfReturnOpnd(const VersionSt &vst, OstPtrSet &mayUseOsts) {
  auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vst);
  if (nextLevelOsts == nullptr) {
    return;
  }
  for (OriginalSt *nextLevelOst : *nextLevelOsts) {
    if (nextLevelOst->IsFinal()) {
      continue;
    }
    auto *aliasSet = GetAliasSet(*nextLevelOst);
    if (aliasSet == nullptr) {
      (void)mayUseOsts.insert(nextLevelOst);
      continue;
    } else {
      for (auto ostIdx : *aliasSet) {
        auto *aliasedOst = ssaTab.GetOriginalStFromID(OStIdx(ostIdx));
        (void)mayUseOsts.insert(aliasedOst);
      }
    }
  }
}

// insert mayuses at a return stmt caused by its return operand being a pointer
void AliasClass::InsertReturnOpndMayUse(const StmtNode &stmt) {
  if (stmt.GetOpCode() == OP_return && stmt.NumOpnds() != 0) {
    // insert mayuses for the return operand's next level
    BaseNode *retValue = stmt.Opnd(0);
    auto *vst = CreateAliasInfoExpr(*retValue).vst;
    if (IsPotentialAddress(retValue->GetPrimType()) && vst != nullptr) {
      if (retValue->GetOpCode() == OP_addrof && IsReadOnlyOst(*vst->GetOst())) {
        return;
      }
      if (!IsNextLevNotAllDefsSeen(vst->GetIndex())) {
        OstPtrSet mayUseOsts;
        auto *assignSet = GetAssignSet(*vst);
        if (assignSet == nullptr) {
          CollectPtsToOfReturnOpnd(*vst, mayUseOsts);
        } else {
          for (size_t valueAliasVstIdx : *assignSet) {
            CollectPtsToOfReturnOpnd(*ssaTab.GetVerSt(valueAliasVstIdx), mayUseOsts);
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
  for (auto *ost : ssaTab.GetOriginalStTable()) {
    if (ost->GetIndirectLev() >= 0 && !ost->IsPregOst()) {
      MayUseNode mayUse = MayUseNode(
          ssaTab.GetVersionStTable().GetVersionStVectorItem(ost->GetZeroVersionIndex()));
      (void)mayUseNodes.emplace(ost->GetIndex(), mayUse);
    }
  }
}

void AliasClass::CollectMayDefForDassign(const StmtNode &stmt, OstPtrSet &mayDefOsts) {
  OriginalSt *ostOfLhs = ssaTab.GetStmtsSSAPart().GetAssignedVarOf(stmt)->GetOst();
  auto *aliasSet = GetAliasSet(*ostOfLhs);
  if (aliasSet == nullptr) {
    return;
  }

  FieldID fldIDA = ostOfLhs->GetFieldID();
  for (uint aliasOstIdx : *aliasSet) {
    if (aliasOstIdx == ostOfLhs->GetIndex()) {
      continue;
    }
    OriginalSt *ostOfAliasAe = ssaTab.GetOriginalStFromID(OStIdx(aliasOstIdx));
    FieldID fldIDB = ostOfAliasAe->GetFieldID();
    if (!mirModule.IsCModule()) {
      if (ostOfAliasAe->GetTyIdx() != ostOfLhs->GetTyIdx()) {
        continue;
      }
      if (fldIDA == fldIDB || fldIDA == 0 || fldIDB == 0) {
        (void)mayDefOsts.insert(ostOfAliasAe);
      }
    } else {
      if (!MayAliasBasicAA(ostOfLhs, ostOfAliasAe)) {
        continue;
      }
      (void)mayDefOsts.insert(ostOfAliasAe);
    }
  }
}

void AliasClass::InsertMayDefNode(OstPtrSet &mayDefOsts, AccessSSANodes *ssaPart,
                                  StmtNode &stmt, BBId bbid) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    ssaPart->InsertMayDefNode(MayDefNode(
        ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
    ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
  }
}

void AliasClass::InsertMayDefDassign(StmtNode &stmt, BBId bbid) {
  OstPtrSet mayDefOsts;
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
  if (ai.vst == nullptr || ai.offset.IsInvalid()) {
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
      return IsAliasInfoEquivalentToExpr(CreateAliasInfoExpr(*base), base);
    }
    default: {
      return false;
    }
  }
}

void AliasClass::CollectMayDefForIassign(StmtNode &stmt, OstPtrSet &mayDefOsts) {
  auto &iassignNode = static_cast<IassignNode&>(stmt);
  VersionSt *lhsVst =
      FindOrCreateVstOfExtraLevOst(*iassignNode.Opnd(0), iassignNode.GetTyIdx(), iassignNode.GetFieldID(), false);

  OriginalSt *ostOfLhs = lhsVst->GetOst();
  auto *aliasSet = GetAliasSet(*ostOfLhs);

  // lhsAe does not alias with any aliasElem
  if (aliasSet == nullptr) {
    (void)mayDefOsts.insert(ostOfLhs);
    return;
  }

  TyIdx pointedTyIdx = ostOfLhs->GetTyIdx();
  BaseNode *rhs = iassignNode.GetRHS();
  // we will try to filter rhs's alias element when rhs is iread expr
  AliasInfo ai = CreateAliasInfoExpr(*rhs);
  OriginalSt *rhsOst = nullptr;
  if (rhs->GetOpCode() == OP_iread && IsAliasInfoEquivalentToExpr(ai, rhs)) {
    rhsOst = ai.vst->GetOst();
  }
  for (uint32 aliasOstIdx : *aliasSet) {
    OriginalSt *aliasedOst = ssaTab.GetOriginalStFromID(OStIdx(aliasOstIdx));
    if (!mirModule.IsCModule()) {
      if ((aliasedOst->GetIndirectLev() == 0) && OriginalStIsAuto(aliasedOst)) {
        continue;
      }
      if (aliasedOst->GetTyIdx() != pointedTyIdx && pointedTyIdx != 0) {
        continue;
      }
    } else {
      if (!MayAliasBasicAA(ostOfLhs, aliasedOst)) {
        continue;
      }
      if (TypeBasedAliasAnalysis::FilterAliasElemOfRHSForIassign(aliasedOst, ostOfLhs, rhsOst)) {
        continue;
      }
    }
    (void)mayDefOsts.insert(aliasedOst);
  }
}

void AliasClass::InsertMayDefNodeExcludeFinalOst(OstPtrSet &mayDefOsts,
                                                 AccessSSANodes *ssaPart, StmtNode &stmt,
                                                 BBId bbid) {
  for (OriginalSt *mayDefOst : mayDefOsts) {
    if (!mayDefOst->IsFinal()) {
      ssaPart->InsertMayDefNode(MayDefNode(
          ssaTab.GetVersionStTable().GetVersionStVectorItem(mayDefOst->GetZeroVersionIndex()), &stmt));
      ssaTab.AddDefBB4Ost(mayDefOst->GetIndex(), bbid);
    }
  }
}

void AliasClass::InsertMayDefIassign(StmtNode &stmt, BBId bbid) {
  OstPtrSet mayDefOsts;
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
  std::set<uint32> aliasSet;
  // collect the full alias set first
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    BaseNode *addrBase = stmt.Opnd(i);
    if (addrBase->IsSSANode()) {
      VersionSt *vst = static_cast<SSANode*>(addrBase)->GetSSAVar();
      const auto &collectAliasedOsts = [&](const OStIdx &ostIdx) {
        auto *aliasSetOfCurOSt = GetAliasSet(ostIdx);
        if (aliasSetOfCurOSt == nullptr) {
          return;
        }
        for (uint32 aliasOstIdx : *aliasSetOfCurOSt) {
          aliasSet.insert(aliasOstIdx);
        }
      };

      if (addrBase->GetOpCode() == OP_addrof) {
        collectAliasedOsts(vst->GetOrigIdx());
      } else {
        auto *nextLevelOsts = ssaTab.GetNextLevelOsts(*vst);
        if (nextLevelOsts != nullptr) {
          for (OriginalSt *nextLevelOst : *nextLevelOsts) {
            collectAliasedOsts(nextLevelOst->GetIndex());
          }
        }
      }
    } else {
      for (auto *nadsOst : nadsOsts) {
        aliasSet.insert(nadsOst->GetIndex());
      }
    }
  }
  // do the insertion according to aliasSet
  AccessSSANodes *theSSAPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  for (uint32 ostIdx : aliasSet) {
    OriginalSt *aliasOst = ssaTab.GetOriginalStFromID(OStIdx(ostIdx));
    if (!aliasOst->IsFinal()) {
      VersionSt *vst0 = ssaTab.GetVerSt(aliasOst->GetZeroVersionIndex());
      CHECK_FATAL(theSSAPart, "theSSAPart is nullptr!");
      theSSAPart->InsertMayUseNode(MayUseNode(vst0));
      theSSAPart->InsertMayDefNode(MayDefNode(vst0, &stmt));
      ssaTab.AddDefBB4Ost(aliasOst->GetIndex(), bbid);
    }
  }
}

// collect mayDefs caused by mustDefs
void AliasClass::CollectMayDefForMustDefs(const StmtNode &stmt, OstPtrSet &mayDefOsts) {
  MapleVector<MustDefNode> &mustDefs = ssaTab.GetStmtsSSAPart().GetMustDefNodesOf(stmt);
  for (MustDefNode &mustDef : mustDefs) {
    VersionSt *vst = mustDef.GetResult();
    OriginalSt *ost = vst->GetOst();
    if (IsNotAllDefsSeen(ost->GetIndex())) {
      continue;
    }
    auto *aliasSet = GetAliasSet(*ost);
    if (aliasSet == nullptr) {
      continue;
    }
    for (auto aliasOstIdx : *aliasSet) {
      if (aliasOstIdx == ost->GetIndex()) {
        continue;
      }
      auto *aliasOst = ssaTab.GetOriginalStFromID(OStIdx(aliasOstIdx));
      (void)mayDefOsts.insert(aliasOst);
    }
  }
}

void AliasClass::CollectMayUseForNextLevel(const VersionSt &vst, OstPtrSet &mayUseOsts,
                                           const StmtNode &stmt, bool isFirstOpnd) {
  auto *nextLevelOsts = ssaTab.GetNextLevelOsts(vst);
  if (nextLevelOsts == nullptr) {
    return;
  }
  for (OriginalSt *nextLevelOst : *nextLevelOsts) {
    if (nextLevelOst->IsFinal()) {
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

    auto zeroVersionIdx = nextLevelOst->GetZeroVersionIndex();
    auto zeroVersionSt = ssaTab.GetVerSt(zeroVersionIdx);
    auto *aliasSet = GetAliasSet(*nextLevelOst);
    if (aliasSet == nullptr) {
      (void)mayUseOsts.insert(nextLevelOst);
      CollectMayUseForNextLevel(*zeroVersionSt, mayUseOsts, stmt, false);
    } else {
      for (uint32 aliasOstIdx : *aliasSet) {
        OriginalSt *aliasOst = ssaTab.GetOriginalStFromID(OStIdx(aliasOstIdx));
        CHECK_FATAL(aliasOst->GetVersionsIndices().size() == 1, "ost must only has zero version");
        auto retPair = mayUseOsts.insert(aliasOst);
        if (retPair.second) {
          CollectMayUseForNextLevel(*zeroVersionSt, mayUseOsts, stmt, false);
        }
      }
    }
  }
}

void AliasClass::CollectMayDefUseForIthOpnd(const VersionSt &vstOfIthOpnd, OstPtrSet &mayUseOsts,
                                            const StmtNode &stmt, bool isFirstOpnd) {
  CollectMayUseForNextLevel(vstOfIthOpnd, mayUseOsts, stmt, isFirstOpnd);

  if (vstOfIthOpnd.GetIndex() >= assignSetOfVst.size()) {
    return;
  }
  auto *valueAliasVsts = assignSetOfVst[vstOfIthOpnd.GetIndex()];
  if (valueAliasVsts == nullptr) {
    return;
  }

  // collect may use/def from value-alias-vsts of opnd
  for (auto vstIdx : *valueAliasVsts) {
    if (vstIdx == vstOfIthOpnd.GetIndex()) {
      continue;
    }

    auto *valueAliasVst = ssaTab.GetVerSt(vstIdx);
    if (valueAliasVst == nullptr) {
      continue;
    }
    CollectMayUseForNextLevel(*valueAliasVst, mayUseOsts, stmt, isFirstOpnd);
  }
}

void AliasClass::CollectMayUseForIntrnCallOpnd(const StmtNode &stmt,
                                               OstPtrSet &mayDefOsts, OstPtrSet &mayUseOsts) {
  auto &intrinNode = static_cast<const IntrinsiccallNode&>(stmt);
  IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrinNode.GetIntrinsic()];
  if (intrinDesc->IsAtomic()) {
    // No matter which memorder is selected or whether the atomic operation defines ost or not,
    // args or globals will be considered as being rewritten by other threads and hence need to be reloaded
    CollectMayUseFromGlobalsAffectedByCalls(mayDefOsts);
    CollectMayUseFromFormals(mayDefOsts, false);
  }
  for (uint32 opndId = 0; opndId < stmt.NumOpnds(); ++opndId) {
    BaseNode *expr = stmt.Opnd(opndId);
    expr = RemoveTypeConversionIfExist(expr);
    AliasInfo aInfo = CreateAliasInfoExpr(*expr);
    auto vst = aInfo.vst;
    if (vst == nullptr) {
      continue;
    }

    if (vst->GetOst()->GetType()->PointsToConstString()) {
      continue;
    }
    OstPtrSet mayDefUseOsts;
    CollectMayDefUseForIthOpnd(*vst, mayDefUseOsts, stmt, opndId == 0);
    bool writeOpnd = intrinDesc->WriteNthOpnd(opndId);
    if (mayDefUseOsts.size() == 0 && writeOpnd) {
      // create next-level ost as it not seen before
      auto nextLevOst = FindOrCreateExtraLevOst(
          &ssaTab, vst, vst->GetOst()->GetTyIdx(), 0, OffsetType(0));
      CHECK_FATAL(nextLevOst != nullptr, "Failed to create next-level ost");
      auto *zeroVersionOfNextLevOst = ssaTab.GetVerSt(nextLevOst->GetZeroVersionIndex());
      RecordAliasAnalysisInfo(*zeroVersionOfNextLevOst);
      // add this into nads
      (void)nadsOsts.insert(nextLevOst);
      (void)mayDefUseOsts.insert(nextLevOst);
    }
    if (writeOpnd) {
      mayDefOsts.insert(mayDefUseOsts.begin(), mayDefUseOsts.end());
    }
    if (intrinDesc->ReadNthOpnd(opndId)) {
      mayUseOsts.insert(mayDefUseOsts.begin(), mayDefUseOsts.end());
    }
  }
}

void AliasClass::CollectMayDefUseForCallOpnd(const StmtNode &stmt,
                                             OstPtrSet &mayDefOsts, OstPtrSet &mayUseOsts,
                                             OstPtrSet &mustNotDefOsts, OstPtrSet &mustNotUseOsts) {
  size_t opndId = kOpcodeInfo.IsICall(stmt.GetOpCode()) ? 1 : 0;
  const FuncDesc *desc = nullptr;
  if (stmt.GetOpCode() == OP_call || stmt.GetOpCode() == OP_callassigned) {
    desc = &GetFuncDescFromCallStmt(static_cast<const CallNode&>(stmt));
  }
  for (; opndId < stmt.NumOpnds(); ++opndId) {
    BaseNode *expr = stmt.Opnd(opndId);
    expr = RemoveTypeConversionIfExist(expr);
    if (!IsPotentialAddress(expr->GetPrimType())) {
      continue;
    }

    AliasInfo aInfo = CreateAliasInfoExpr(*expr);
    if (aInfo.vst == nullptr) {
      continue;
    }

    if (aInfo.vst->GetOst()->GetType()->PointsToConstString()) {
      ssaTab.CollectIterNextLevel(aInfo.vst->GetIndex(), mustNotDefOsts);
      continue;
    }

    if (desc != nullptr && (desc->IsArgUnused(opndId) || desc->IsArgReadSelfOnly(opndId))) {
      // configured desc guarantees iterNextLevel of opnd are never modified through any way(like global or other opnds)
      if (desc->IsConfiged()) {
        OstPtrSet result;
        ssaTab.CollectIterNextLevel(aInfo.vst->GetIndex(), result);
        mustNotDefOsts.insert(result.begin(), result.end());
        mustNotUseOsts.insert(result.begin(), result.end());
      }
      continue;
    }

    if (desc != nullptr && desc->IsArgReadMemoryOnly(opndId)) {
      CollectMayDefUseForIthOpnd(*aInfo.vst, mayUseOsts, stmt, opndId == 0);
      if (desc->IsConfiged()) {
        ssaTab.CollectIterNextLevel(aInfo.vst->GetIndex(), mustNotDefOsts);
      }
      continue;
    }

    if (desc != nullptr && desc->IsArgWriteMemoryOnly(opndId)) {
      CollectMayDefUseForIthOpnd(*aInfo.vst, mayDefOsts, stmt, opndId == 0);
      continue;
    }
    CollectMayDefUseForIthOpnd(*aInfo.vst, mayDefOsts, stmt, opndId == 0);
    CollectMayDefUseForIthOpnd(*aInfo.vst, mayUseOsts, stmt, opndId == 0);
  }
}

void AliasClass::InsertMayDefNodeForCall(OstPtrSet &mayDefOsts, AccessSSANodes *ssaPart,
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
  OstPtrSet mayDefOstsA;
  OstPtrSet mayUseOstsA;
  OstPtrSet mustNotDefOsts;
  OstPtrSet mustNotUseOsts;
  // 1. collect mayDefs and mayUses caused by callee-opnds
  CollectMayDefUseForCallOpnd(stmt, mayDefOstsA, mayUseOstsA, mustNotDefOsts, mustNotUseOsts);
  // 2. collect mayDefs and mayUses caused by not_all_def_seen_ae
  OstPtrSetSub(nadsOsts, mustNotUseOsts, mayUseOstsA);
  if (hasSideEffect) {
    OstPtrSetSub(nadsOsts, mustNotDefOsts, mayDefOstsA);
  }
  // insert mayuse node caused by opnd and not_all_def_seen_ae.
  InsertMayUseNode(mayUseOstsA, ssaPart);
  // insert maydef node caused by opnd and not_all_def_seen_ae.
  InsertMayDefNodeForCall(mayDefOstsA, ssaPart, stmt, bbid, hasNoPrivateDefEffect);

  // 3. insert mayDefs and mayUses caused by globalsAffectedByCalls
  OstPtrSet mayDefUseOfGOsts;
  CollectMayUseFromGlobalsAffectedByCalls(mayDefUseOfGOsts);
  if (desc == nullptr || !desc->IsConst()) {
    InsertMayUseNode(mayDefUseOfGOsts, ssaPart);
  }
  // insert may def node, if the callee has side-effect.
  if (hasSideEffect) {
    InsertMayDefNodeExcludeFinalOst(mayDefUseOfGOsts, ssaPart, stmt, bbid);
  }
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    // 4. insert mayDefs caused by the mustDefs
    OstPtrSet mayDefOstsC;
    CollectMayDefForMustDefs(stmt, mayDefOstsC);
    InsertMayDefNodeExcludeFinalOst(mayDefOstsC, ssaPart, stmt, bbid);
  }
}

void AliasClass::InsertMayUseNodeExcludeFinalOst(const OstPtrSet &mayUseOsts,
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
  OstPtrSet mayUseOsts;
  OstPtrSet mayDefOsts;
  // 1. collect mayDefs and mayUses caused by opnds
  if (!mirModule.IsCModule()) {
    for (uint32 i = 0; i < stmt.NumOpnds(); ++i) {
      InsertMayUseExpr(*stmt.Opnd(i));
    }
    CollectMayUseFromGlobalsAffectedByCalls(mayUseOsts);
    auto &intrinNode = static_cast<const IntrinsiccallNode&>(stmt);
    IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrinNode.GetIntrinsic()];
    if (!intrinDesc->HasNoSideEffect()) {
      CollectMayUseFromGlobalsAffectedByCalls(mayDefOsts);
    }
  } else {
    CollectMayUseForIntrnCallOpnd(stmt, mayDefOsts, mayUseOsts);
  }
  InsertMayUseNodeExcludeFinalOst(mayUseOsts, ssaPart);
  InsertMayDefNodeExcludeFinalOst(mayDefOsts, ssaPart, stmt, bbid);

  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    // 4. insert maydefs caused by the mustdefs
    OstPtrSet mayDefOstsFromMustdefs;
    CollectMayDefForMustDefs(stmt, mayDefOstsFromMustdefs);
    InsertMayDefNodeExcludeFinalOst(mayDefOstsFromMustdefs, ssaPart, stmt, bbid);
  }
}

void AliasClass::InsertMayDefUseClinitCheck(IntrinsiccallNode &stmt, BBId bbid) {
  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  for (const OStIdx &ostIdx : globalsMayAffectedByClinitCheck) {
    OriginalSt *ost = ssaTab.GetOriginalStFromID(OStIdx(ostIdx));
    CHECK_FATAL(ost, "ost is nullptr!");
    ssaPart->InsertMayDefNode(
        MayDefNode(ssaTab.GetVersionStTable().GetVersionStVectorItem(ost->GetZeroVersionIndex()), &stmt));
    ssaTab.AddDefBB4Ost(ostIdx, bbid);
  }
}

void AliasClass::InsertMayDefUseAsm(StmtNode &stmt, const BBId bbID) {
  AsmNode &asmStmt = static_cast<AsmNode&>(stmt);

  /* process listClobber */
  for (size_t i = 0; i < asmStmt.clobberList.size(); ++i) {
    const std::string &str = GlobalTables::GetUStrTable().GetStringFromStrIdx(asmStmt.clobberList[i]);
    if (str[0] == 'm') {
      InsertMayDefUseCall(stmt, bbID, false);
      return;
    }
  }

  auto *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  OstPtrSet mayDefOsts;
  OstPtrSet mayUseOsts;
  /* process input constraint */
  for (size_t i = 0; i < asmStmt.numOpnds; ++i) {
    BaseNode *expr = asmStmt.Opnd(i);
    expr = RemoveTypeConversionIfExist(expr);
    if (!IsPotentialAddress(expr->GetPrimType())) {
      continue;
    }

    AliasInfo aInfo = CreateAliasInfoExpr(*expr);
    if (aInfo.vst == nullptr) {
      continue;
    }

    if (aInfo.vst->GetOst()->GetType()->PointsToConstString()) {
      continue;
    }
    CollectMayDefUseForIthOpnd(*aInfo.vst, mayUseOsts, stmt, false);

    const std::string &str = GlobalTables::GetUStrTable().GetStringFromStrIdx(asmStmt.inputConstraints[i]);
    if (str[0] == '+') {
      CollectMayUseForNextLevel(*aInfo.vst, mayDefOsts, stmt, false);
    }
  }

  /* process output -- outputs are handled in mustdefs of the stmt. */
  CollectMayDefForMustDefs(stmt, mayDefOsts);

  InsertMayUseNode(mayUseOsts, ssaPart);
  InsertMayDefNode(mayDefOsts, ssaPart, stmt, bbID);
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
    case OP_asm: {
      InsertMayDefUseAsm(stmt, bbID);
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
    case OP_icallprotoassigned:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_icall:
    case OP_icallproto: {
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
