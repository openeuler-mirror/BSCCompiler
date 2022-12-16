/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "orig_symbol.h"
#include "mir_type.h"
#include "ver_symbol.h"
#include "class_hierarchy.h"

namespace maple {
bool OriginalSt::Equal(const OriginalSt &ost) const {
  if (IsSymbolOst()) {
    return symOrPreg.mirSt == ost.symOrPreg.mirSt &&
           fieldID == ost.GetFieldID() &&
           GetIndirectLev() == ost.GetIndirectLev();
  }
  if (IsPregOst()) {
    return symOrPreg.pregIdx == ost.symOrPreg.pregIdx && GetIndirectLev() == ost.GetIndirectLev();
  }
  return false;
}

// current ost can not be define by MayDef, or used by mayUse
bool OriginalSt::IsTopLevelOst() const {
  // 1. pregs, exclude special-pregs, are top-level.
  if (IsPregOst()) {
    if (IsSpecialPreg()) {
      return false;
    }
    return true;
  }
  // 2. address taken symbols, may be defined via dereference, are not top-level.
  if (addressTaken) {
    return false;
  }
  // 3. global symbols, can be defined by callees, are not top-level.
  if (!isLocal) {
    return false;
  }
  // 4. local static symbols, can be defined by recursive callees, are not top-level.
  if (GetMIRSymbol()->IsPUStatic()) {
    return false;
  }
  return true;
}

void OriginalSt::SetPointerVst(const VersionSt *vst) {
  ASSERT(pointerVstIdx == 0 || pointerVstIdx == vst->GetIndex(), "wrong pointee");
  pointerVstIdx = vst->GetIndex();
  prevLevOst = vst->GetOst();
  SetPointerTyIdx(vst->GetOst()->GetTyIdx());
}

void OriginalSt::Dump() const {
  if (indirectLev > 0) {
    GetPrevLevelOst()->Dump();
    LogInfo::MapleLogger() << "(vstIdx:" << pointerVstIdx << ")";
    LogInfo::MapleLogger() << "->" << "{" << fieldID << "/" << offset.val << "}" << "[idx:" << GetIndex() <<"]";
    if (IsFinal()) {
      LogInfo::MapleLogger() << "F";
    }
    if (IsPrivate()) {
      LogInfo::MapleLogger() << "P";
    }
    return;
  }

  if (IsSymbolOst()) {
    LogInfo::MapleLogger() << (symOrPreg.mirSt->IsGlobal() ? "$" : "%") << symOrPreg.mirSt->GetName();
    LogInfo::MapleLogger() << "{" << "offset:" << (offset.IsInvalid() ? "UND" : std::to_string(offset.val)) << "}";
    if (fieldID != 0) {
      LogInfo::MapleLogger() << "{" << fieldID << "}";
    }
    LogInfo::MapleLogger() << "<" << static_cast<int32>(GetIndirectLev()) << ">";
    if (IsFinal()) {
      LogInfo::MapleLogger() << "F";
    }
    if (IsPrivate()) {
      LogInfo::MapleLogger() << "P";
    }
  } else if (IsPregOst()) {
    LogInfo::MapleLogger() << "%" << GetMIRPreg()->GetPregNo();
    LogInfo::MapleLogger() << "<" << static_cast<int32>(indirectLev) << ">";
  }
  LogInfo::MapleLogger() << "[idx:" << GetIndex() <<"]";
}

OriginalStTable::OriginalStTable(MemPool &memPool, MIRModule &mod)
    : alloc(&memPool),
      mirModule(mod),
      originalStVector({ nullptr }, alloc.Adapter()),
      mirSt2Ost(alloc.Adapter()),
      addrofSt2Ost(alloc.Adapter()),
      preg2Ost(alloc.Adapter()),
      pType2Ost(std::less<TyIdx>(), alloc.Adapter()),
      malloc2Ost(alloc.Adapter()),
      thisField2Ost(std::less<uint32>(), alloc.Adapter()),
      nextLevelOstsOfVst(alloc.Adapter()) {}

void OriginalStTable::Dump() {
  mirModule.GetOut() << "==========original st table===========\n";
  for (size_t i = 1; i < Size(); ++i) {
    const OriginalSt *oriSt = GetOriginalStFromID(OStIdx(i));
    if (oriSt == nullptr) {
      continue;
    }
    oriSt->Dump();
  }
  mirModule.GetOut() << "\n=======end original st table===========\n";
}

OriginalSt *OriginalStTable::FindOrCreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx, FieldID fld) {
  OffsetType offset(mirSt.GetType()->GetBitOffsetFromBaseAddr(fld));
  auto ostType = mirSt.GetType();
  if (fld != 0) {
    CHECK_FATAL(ostType->IsStructType(), "must be structure type");
    ostType = static_cast<MIRStructType*>(ostType)->GetFieldType(fld);
  }
  auto it = mirSt2Ost.find(SymbolFieldPair(mirSt.GetStIdx(), fld, ostType->GetTypeIndex(), offset));
  if (it == mirSt2Ost.end()) {
    // create a new OriginalSt
    return CreateSymbolOriginalSt(mirSt, puIdx, fld);
  }
  CHECK_FATAL(it->second < originalStVector.size(),
              "index out of range in OriginalStTable::FindOrCreateSymbolOriginalSt");
  return originalStVector[it->second];
}

OriginalSt *OriginalStTable::CreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx, FieldID fld, const TyIdx &tyIdx,
                                                    const OffsetType &offset) {
  auto *ost = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), mirSt, puIdx, fld, alloc);
  ost->SetOffset(offset);
  ost->SetIsFinal(mirSt.IsFinal());
  ost->SetIsPrivate(mirSt.IsPrivate());
  ost->SetTyIdx(tyIdx);
  originalStVector.push_back(ost);
  mirSt2Ost[SymbolFieldPair(mirSt.GetStIdx(), fld, tyIdx, offset)] = ost->GetIndex();
  return ost;
}

std::pair<OriginalSt*, bool> OriginalStTable::FindOrCreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx,
                                                                           FieldID fld, const TyIdx &tyIdx,
                                                                           const OffsetType &offset) {
  auto it = mirSt2Ost.find(SymbolFieldPair(mirSt.GetStIdx(), fld, tyIdx, offset));
  if (it == mirSt2Ost.end()) {
    // create a new OriginalSt
    auto *newOst = CreateSymbolOriginalSt(mirSt, puIdx, fld, tyIdx, offset);
    return std::make_pair(newOst, true);
  }
  CHECK_FATAL(it->second < originalStVector.size(),
              "index out of range in OriginalStTable::FindOrCreateSymbolOriginalSt");
  return std::make_pair(originalStVector[it->second], false);
}

OriginalSt *OriginalStTable::FindOrCreatePregOriginalSt(PregIdx pregIdx, PUIdx puIdx) {
  auto it = preg2Ost.find(pregIdx);
  return (it == preg2Ost.end()) ? CreatePregOriginalSt(pregIdx, puIdx)
                                : originalStVector.at(it->second);
}

OriginalSt *OriginalStTable::CreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx pidx, FieldID fld) {
  auto *ost = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), mirSt, pidx, fld, alloc);
  if (fld == 0) {
    ost->SetTyIdx(mirSt.GetTyIdx());
    ost->SetIsFinal(mirSt.IsFinal());
    ost->SetIsPrivate(mirSt.IsPrivate());
  } else {
    auto *structType = static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(mirSt.GetTyIdx()));
    ASSERT(structType, "CreateSymbolOriginalSt: non-zero fieldID for non-structure");
    ost->SetTyIdx(structType->GetFieldTyIdx(fld));
    FieldAttrs fattrs = structType->GetFieldAttrs(fld);
    ost->SetIsFinal(fattrs.GetAttr(FLDATTR_final) && !mirModule.CurFunction()->IsConstructor());
    ost->SetIsPrivate(fattrs.GetAttr(FLDATTR_private));
  }
  OffsetType offset(mirSt.GetType()->GetBitOffsetFromBaseAddr(fld));
  ost->SetOffset(offset);
  originalStVector.push_back(ost);
  mirSt2Ost[SymbolFieldPair(mirSt.GetStIdx(), fld, ost->GetTyIdx(), offset)] = ost->GetIndex();
  return ost;
}

OriginalSt *OriginalStTable::CreatePregOriginalSt(PregIdx pregIdx, PUIdx puIdx) {
  auto *ost = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), pregIdx, puIdx, alloc);
  if (pregIdx < 0) {
    ost->SetTyIdx(TyIdx(PTY_unknown));
  } else {
    ost->SetTyIdx(GlobalTables::GetTypeTable().GetPrimType(ost->GetMIRPreg()->GetPrimType())->GetTypeIndex());
  }
  originalStVector.push_back(ost);
  preg2Ost[pregIdx] = ost->GetIndex();
  return ost;
}

OriginalSt *OriginalStTable::FindSymbolOriginalSt(const MIRSymbol &mirSt, FieldID fld, const TyIdx &tyIdx,
                                                  const OffsetType &offset) {
  const auto it = mirSt2Ost.find(SymbolFieldPair(mirSt.GetStIdx(), fld, tyIdx, offset));
  if (it == mirSt2Ost.end()) {
    return nullptr;
  }
  return GetOriginalStFromID(it->second);
}

OriginalSt *OriginalStTable::FindSymbolOriginalSt(const MIRSymbol &mirSt) {
  return FindSymbolOriginalSt(mirSt, 0, mirSt.GetTyIdx(), OffsetType(0));
}

OriginalSt *OriginalStTable::FindOrCreateAddrofSymbolOriginalSt(OriginalSt *ost) {
  if (ost->GetPrevLevelOst() != nullptr) {
    return ost->GetPrevLevelOst();
  }
  OriginalSt *prevLevelOst = nullptr;
  auto it = addrofSt2Ost.find(ost->GetMIRSymbol()->GetStIdx());
  if (it != addrofSt2Ost.end()) {
    prevLevelOst = originalStVector[it->second];
  } else {
    // create a new node
    prevLevelOst = alloc.GetMemPool()->New<OriginalSt>(
        originalStVector.size(), *ost->GetMIRSymbol(), ost->GetPuIdx(), 0, alloc);
    originalStVector.push_back(prevLevelOst);
    prevLevelOst->SetIndirectLev(-1);
    MIRPtrType pointType(ost->GetMIRSymbol()->GetTyIdx(), PTY_ptr);

    TyIdx newTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&pointType);
    prevLevelOst->SetTyIdx(newTyIdx);
    prevLevelOst->SetFieldID(0);
    addrofSt2Ost[ost->GetMIRSymbol()->GetStIdx()] = prevLevelOst->GetIndex();
  }
  return prevLevelOst;
}

MIRType *OriginalStTable::GetTypeFromBaseAddressAndFieldId(TyIdx tyIdx, FieldID fieldId, bool isFieldArrayType) const {
  MIRType *voidType = GlobalTables::GetTypeTable().GetVoid();
  if (tyIdx == 0u) {
    return voidType;
  }
  // use the tyIdx info from the instruction
  const MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (mirType->GetKind() != kTypePointer) {
    return voidType;
  }

  auto *pointedType = static_cast<const MIRPtrType*>(mirType)->GetPointedType();
  if (fieldId == 0) {
    return pointedType;
  }

  if (static_cast<uint32>(fieldId) > pointedType->NumberOfFieldIDs()) {
    return voidType;
  }

  CHECK_FATAL(pointedType->IsStructType(), "must be struct type");
  auto *fieldType = static_cast<MIRStructType*>(pointedType)->GetFieldType(fieldId);
  if (!isFieldArrayType && GetElemType(*fieldType)) {
    return GetElemType(*fieldType);
  }
  return fieldType;
}

OriginalSt *OriginalStTable::FindOrCreateExtraLevOriginalSt(
    const VersionSt *vst, TyIdx tyIdx, FieldID fieldId, const OffsetType &offset, bool isFieldArrayType) {
  if (!vst->GetOst()->IsSymbolOst() && !vst->GetOst()->IsPregOst()) {
    return nullptr;
  }

  auto *ost = vst->GetOst();
  tyIdx = (tyIdx == 0u) ? ost->GetTyIdx() : tyIdx;
  MIRType *typeOfExtraLevOst = GetTypeFromBaseAddressAndFieldId(tyIdx, fieldId, isFieldArrayType);

  FieldID fieldIDInOst = fieldId;
  OriginalSt *nextLevOst = FindExtraLevOriginalSt(vst, tyIdx, typeOfExtraLevOst, fieldIDInOst, offset);
  if (nextLevOst != nullptr) {
    return nextLevOst;
  }

  // create a new node
  if (ost->IsSymbolOst()) {
    nextLevOst = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), *ost->GetMIRSymbol(),
        ost->GetPuIdx(), fieldIDInOst, alloc);
  } else {
    nextLevOst = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), ost->GetPregIdx(),
        ost->GetPuIdx(), alloc);
    nextLevOst->SetFieldID(fieldIDInOst);
  }
  originalStVector.push_back(nextLevOst);
  CHECK_FATAL(ost->GetIndirectLev() < INT8_MAX, "boundary check");
  nextLevOst->SetIndirectLev(ost->GetIndirectLev() + 1);
  nextLevOst->SetPointerVst(vst);
  nextLevOst->SetOffset(offset);
  nextLevOst->SetPointerTyIdx(tyIdx);
  nextLevOst->SetAddressTaken(true);
  tyIdx = (tyIdx == 0u) ? ost->GetTyIdx() : tyIdx;
  if (tyIdx != 0u) {
    // use the tyIdx info from the instruction
    const MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (mirType->GetKind() == kTypePointer) {
      const auto *ptType = static_cast<const MIRPtrType*>(mirType);
      TyIdxFieldAttrPair fieldPair = ptType->GetPointedTyIdxFldAttrPairWithFieldID(fieldId);
      nextLevOst->SetIsFinal(fieldPair.second.GetAttr(FLDATTR_final));
      nextLevOst->SetIsPrivate(fieldPair.second.GetAttr(FLDATTR_private));
    }
    nextLevOst->SetTyIdx(typeOfExtraLevOst->GetTypeIndex());
  }
  ASSERT(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
  if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx())->PointsToConstString()) {
    nextLevOst->SetIsFinal(true);
  }
  AddNextLevelOstOfVst(vst, nextLevOst);
  return nextLevOst;
}

OriginalSt *OriginalStTable::FindExtraLevOriginalSt(const MapleVector<OriginalSt*> &nextLevelOsts,
                                                    const TyIdx &tyIdxOfPtr, const MIRType *type,
                                                    FieldID fld, const OffsetType &offset) const {
  for (OriginalSt *nextLevelOst : nextLevelOsts) {
    if (tyIdxOfPtr != nextLevelOst->GetPointerTyIdx()) {
      continue;
    }
    if (nextLevelOst->GetType() != type) {
      continue;
    }
    if (nextLevelOst->GetOffset() != offset) {
      continue;
    }
    if (nextLevelOst->GetFieldID() == fld || fld == 0) {
      return nextLevelOst;
    }
  }
  return nullptr;
}

void OriginalStTable::AddNextLevelOstOfVst(size_t vstIdx, OriginalSt *ost) {
  if (vstIdx >= nextLevelOstsOfVst.size()) {
    size_t bufferSize = 10;
    size_t incNum = vstIdx - nextLevelOstsOfVst.size() + bufferSize;
    (void)nextLevelOstsOfVst.insert(nextLevelOstsOfVst.cend(), incNum, nullptr);
  }
  if (nextLevelOstsOfVst[vstIdx] == nullptr) {
    nextLevelOstsOfVst[vstIdx] = alloc.New<MapleVector<OriginalSt*>>(alloc.Adapter());
  }
  nextLevelOstsOfVst[vstIdx]->push_back(ost);
}

void OriginalStTable::AddNextLevelOstOfVst(const VersionSt *vst, OriginalSt *ost) {
  AddNextLevelOstOfVst(vst->GetIndex(), ost);
}

MapleVector<OriginalSt*> *OriginalStTable::GetNextLevelOstsOfVst(size_t vstIdx) const {
  if (vstIdx >= nextLevelOstsOfVst.size()) {
    return nullptr;
  }
  return nextLevelOstsOfVst[vstIdx];
}

MapleVector<OriginalSt*> *OriginalStTable::GetNextLevelOstsOfVst(const VersionSt *vst) const {
  return GetNextLevelOstsOfVst(vst->GetIndex());
}

OriginalSt *OriginalStTable::FindExtraLevOriginalSt(const VersionSt *vst, const TyIdx &tyIdxOfPtr,
                                                    const MIRType *typeOfOst, FieldID fld,
                                                    const OffsetType &offset) const {
  auto *nextLevelOsts = GetNextLevelOstsOfVst(vst);
  if (nextLevelOsts == nullptr) {
    return nullptr;
  }

  return FindExtraLevOriginalSt(*nextLevelOsts, tyIdxOfPtr, typeOfOst, fld, offset);
}
}  // namespace maple
