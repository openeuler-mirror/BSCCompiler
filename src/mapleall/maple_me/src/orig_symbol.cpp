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

void OriginalSt::Dump() const {
  if (IsSymbolOst()) {
    LogInfo::MapleLogger() << (symOrPreg.mirSt->IsGlobal() ? "$" : "%") << symOrPreg.mirSt->GetName();
    if (!offset.IsInvalid()) {
      LogInfo::MapleLogger() << "{" << "offset:" << offset.val << "}";
    }
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
      thisField2Ost(std::less<uint32>(), alloc.Adapter()) {}

void OriginalStTable::Dump() {
  mirModule.GetOut() << "==========original st table===========\n";
  for (size_t i = 1; i < Size(); ++i) {
    const OriginalSt *verst = GetOriginalStFromID(OStIdx(i));
    verst->Dump();
  }
  mirModule.GetOut() << "\n=======end original st table===========\n";
}

OriginalSt *OriginalStTable::FindOrCreateSymbolOriginalSt(MIRSymbol &mirst, PUIdx pidx, FieldID fld) {
  OffsetType offset(mirst.GetType()->GetBitOffsetFromBaseAddr(fld));
  auto ostType = mirst.GetType();
  if (fld != 0) {
    CHECK_FATAL(ostType->IsStructType(), "must be structure type");
    ostType = static_cast<MIRStructType*>(ostType)->GetFieldType(fld);
  }
  auto it = mirSt2Ost.find(SymbolFieldPair(mirst.GetStIdx(), fld, ostType->GetTypeIndex(), offset));
  if (it == mirSt2Ost.end()) {
    // create a new OriginalSt
    return CreateSymbolOriginalSt(mirst, pidx, fld);
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

std::pair<OriginalSt*, bool> OriginalStTable::FindOrCreateSymbolOriginalSt(MIRSymbol &mirst, PUIdx pidx,
    FieldID fld, const TyIdx &tyIdx, const OffsetType &offset) {
  auto it = mirSt2Ost.find(SymbolFieldPair(mirst.GetStIdx(), fld, tyIdx, offset));
  if (it == mirSt2Ost.end()) {
    // create a new OriginalSt
    auto *newOst = CreateSymbolOriginalSt(mirst, pidx, fld, tyIdx, offset);
    return std::make_pair(newOst, true);
  }
  CHECK_FATAL(it->second < originalStVector.size(),
              "index out of range in OriginalStTable::FindOrCreateSymbolOriginalSt");
  return std::make_pair(originalStVector[it->second], false);
}

OriginalSt *OriginalStTable::FindOrCreatePregOriginalSt(PregIdx regidx, PUIdx pidx) {
  auto it = preg2Ost.find(regidx);
  return (it == preg2Ost.end()) ? CreatePregOriginalSt(regidx, pidx)
                                : originalStVector.at(it->second);
}

OriginalSt *OriginalStTable::CreateSymbolOriginalSt(MIRSymbol &mirst, PUIdx pidx, FieldID fld) {
  auto *ost = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), mirst, pidx, fld, alloc);
  if (fld == 0) {
    ost->SetTyIdx(mirst.GetTyIdx());
    ost->SetIsFinal(mirst.IsFinal());
    ost->SetIsPrivate(mirst.IsPrivate());
  } else {
    auto *structType = static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(mirst.GetTyIdx()));
    ASSERT(structType, "CreateSymbolOriginalSt: non-zero fieldID for non-structure");
    ost->SetTyIdx(structType->GetFieldTyIdx(fld));
    FieldAttrs fattrs = structType->GetFieldAttrs(fld);
    ost->SetIsFinal(fattrs.GetAttr(FLDATTR_final) && !mirModule.CurFunction()->IsConstructor());
    ost->SetIsPrivate(fattrs.GetAttr(FLDATTR_private));
  }
  OffsetType offset(mirst.GetType()->GetBitOffsetFromBaseAddr(fld));
  ost->SetOffset(offset);
  originalStVector.push_back(ost);
  mirSt2Ost[SymbolFieldPair(mirst.GetStIdx(), fld, ost->GetTyIdx(), offset)] = ost->GetIndex();
  return ost;
}

OriginalSt *OriginalStTable::CreatePregOriginalSt(PregIdx regidx, PUIdx pidx) {
  auto *ost = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), regidx, pidx, alloc);
  if (regidx < 0) {
    ost->SetTyIdx(TyIdx(PTY_unknown));
  } else {
    ost->SetTyIdx(GlobalTables::GetTypeTable().GetPrimType(ost->GetMIRPreg()->GetPrimType())->GetTypeIndex());
  }
  originalStVector.push_back(ost);
  preg2Ost[regidx] = ost->GetIndex();
  return ost;
}

OriginalSt *OriginalStTable::FindSymbolOriginalSt(MIRSymbol &mirst) {
  auto it = mirSt2Ost.find(SymbolFieldPair(mirst.GetStIdx(), 0, mirst.GetTyIdx(), OffsetType(0)));
  if (it == mirSt2Ost.end()) {
    return nullptr;
  }
  CHECK_FATAL(it->second < originalStVector.size(), "index out of range in OriginalStTable::FindSymbolOriginalSt");
  return originalStVector[it->second];
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
  ost->SetPrevLevelOst(prevLevelOst);
  prevLevelOst->AddNextLevelOst(ost);
  return prevLevelOst;
}

OriginalSt *OriginalStTable::FindOrCreateExtraLevSymOrRegOriginalSt(OriginalSt *ost, TyIdx tyIdx,
    FieldID fld, const OffsetType &offset, const KlassHierarchy *klassHierarchy) {
  MIRType *typeOfExtraLevOst = GlobalTables::GetTypeTable().GetVoid();
  OffsetType offsetOfNextLevOst(kOffsetUnknown);
  tyIdx = (tyIdx == 0u) ? ost->GetTyIdx() : tyIdx;
  if (tyIdx != 0u) {
    // use the tyIdx info from the instruction
    const MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (mirType->GetKind() == kTypePointer) {
      const auto *ptType = static_cast<const MIRPtrType*>(mirType);
      auto *pointedType = ptType->GetPointedType();
      if (fld == 0) {
        offsetOfNextLevOst = offset;
        typeOfExtraLevOst = pointedType;
      } else if (fld <= pointedType->NumberOfFieldIDs()) {
        CHECK_FATAL(pointedType->IsStructType(), "must be struct type");
        typeOfExtraLevOst = static_cast<MIRStructType*>(pointedType)->GetFieldType(fld);
        offsetOfNextLevOst = offset + pointedType->GetBitOffsetFromBaseAddr(fld);
      }
    }
  }

  TyIdx ptyIdxOfOst = ost->GetTyIdx();
  FieldID fldIDInOst = fld;
  if (ptyIdxOfOst != tyIdx) {
    if (klassHierarchy != nullptr) {
      (void)klassHierarchy->UpdateFieldID(tyIdx, ptyIdxOfOst, fldIDInOst);
    } else {
      fldIDInOst = 0;
    }
  }
  OriginalSt *nextLevOst =
      FindExtraLevOriginalSt(ost->GetNextLevelOsts(), typeOfExtraLevOst, fldIDInOst, offsetOfNextLevOst);
  if (nextLevOst != nullptr) {
    return nextLevOst;
  }

  // create a new node
  if (ost->IsSymbolOst()) {
    nextLevOst = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), *ost->GetMIRSymbol(),
        ost->GetPuIdx(), fldIDInOst, alloc);
  } else {
    nextLevOst = alloc.GetMemPool()->New<OriginalSt>(originalStVector.size(), ost->GetPregIdx(),
        ost->GetPuIdx(), alloc);
    nextLevOst->SetFieldID(fldIDInOst);
  }
  originalStVector.push_back(nextLevOst);
  CHECK_FATAL(ost->GetIndirectLev() < INT8_MAX, "boundary check");
  nextLevOst->SetIndirectLev(ost->GetIndirectLev() + 1);
  nextLevOst->SetPrevLevelOst(ost);
  nextLevOst->SetOffset(offsetOfNextLevOst);
  tyIdx = (tyIdx == 0u) ? ost->GetTyIdx() : tyIdx;
  if (tyIdx != 0u) {
    // use the tyIdx info from the instruction
    const MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (mirType->GetKind() == kTypePointer) {
      const auto *ptType = static_cast<const MIRPtrType*>(mirType);
      TyIdxFieldAttrPair fieldPair = ptType->GetPointedTyIdxFldAttrPairWithFieldID(fld);
      nextLevOst->SetIsFinal(fieldPair.second.GetAttr(FLDATTR_final));
      nextLevOst->SetIsPrivate(fieldPair.second.GetAttr(FLDATTR_private));
    }
    nextLevOst->SetTyIdx(typeOfExtraLevOst->GetTypeIndex());
  }
  ASSERT(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
  if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx())->PointsToConstString()) {
    nextLevOst->SetIsFinal(true);
  }
  ost->AddNextLevelOst(nextLevOst);
  return nextLevOst;
}

OriginalSt *OriginalStTable::FindOrCreateExtraLevOriginalSt(OriginalSt *ost, TyIdx ptyIdx, FieldID fld,
                                                            const OffsetType &offset) {
  if (ost->IsSymbolOst() || ost->IsPregOst()) {
    return FindOrCreateExtraLevSymOrRegOriginalSt(ost, ptyIdx, fld, offset);
  }
  return nullptr;
}

OriginalSt *OriginalStTable::FindExtraLevOriginalSt(const MapleVector<OriginalSt*> &nextLevelOsts, MIRType *type,
                                                    FieldID fld, const OffsetType &offset) {
  for (OriginalSt *nextLevelOst : nextLevelOsts) {
    if (nextLevelOst->GetOffset() == offset && nextLevelOst->GetType() == type) {
      if (nextLevelOst->GetFieldID() == fld || fld == 0) {
        return nextLevelOst;
      }
    }
  }
  return nullptr;
}
}  // namespace maple
