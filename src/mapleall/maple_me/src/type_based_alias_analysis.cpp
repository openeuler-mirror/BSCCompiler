/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "type_based_alias_analysis.h"
#include "me_option.h"
namespace maple {
MIRType *GetStructTypeOstEmbedded(const OriginalSt *ost) {
  if (ost == nullptr) {
    return nullptr;
  }
  MIRType *aggType = nullptr;
  if (ost->GetPrevLevelOst() != nullptr) {
    MIRType *prevTypeA = ost->GetPrevLevelOst()->GetType();
    ASSERT(prevTypeA->IsMIRPtrType(), "OriginalSt's prev level type must be pointer type!");
    aggType = static_cast<MIRPtrType*>(prevTypeA)->GetPointedType();
  } else if (ost->GetIndirectLev() == 0 && ost->IsSymbolOst()) {
    aggType = ost->GetMIRSymbol()->GetType();
  }
  if (aggType != nullptr && aggType->IsStructType()) {
    return aggType;
  }
  return nullptr;
}

bool TypeBasedAliasAnalysis::MayAlias(const OriginalSt *ostA, const OriginalSt *ostB) {
  if (!MeOption::tbaa) {
    return true; // deal with alias relation conservatively for non-type-safe
  }
  if (ostA == nullptr || ostB == nullptr) {
    return false;
  }
  if (ostA == ostB) {
    return true;
  }
  const OffsetType &offsetA = ostA->GetOffset();
  const OffsetType &offsetB = ostB->GetOffset();
  // Check field alias - If both of ost are fields of the same agg type, check if they overlap
  if (ostA->GetFieldID() != 0 && ostB->GetFieldID() != 0 && !offsetA.IsInvalid() && !offsetB.IsInvalid()) {
    MIRType *aggTypeA = GetStructTypeOstEmbedded(ostA);
    MIRType *aggTypeB = GetStructTypeOstEmbedded(ostB);
    if (aggTypeA == aggTypeB) { // We should check type compatibility here actually
      int32 bitSizeA = static_cast<int32>(GetTypeBitSize(ostA->GetType()));
      int32 bitSizeB = static_cast<int32>(GetTypeBitSize(ostB->GetType()));
      return IsMemoryOverlap(offsetA, bitSizeA, offsetB, bitSizeB);
    }
  }
  return true;
}

// return true if can filter this aliasElemOst, otherwise return false;
bool TypeBasedAliasAnalysis::FilterAliasElemOfRHSForIassign(const OriginalSt *aliasElemOst, const OriginalSt *lhsOst,
                                                            const OriginalSt *rhsOst) {
  if (!MeOption::tbaa) {
    return false;
  }
  // only for type-safe : iassign (lhs, rhs),
  // if rhs is alias with lhs, their memories overlap completely (not partially);
  // if rhs is not alias with rhs, their memories completely not overlap.
  // Rhs may def itself if they overlap, but its value is the same as before.
  // So we skip inserting maydef for ost the same as rhs here
  return (aliasElemOst == rhsOst && rhsOst->GetTyIdx() == lhsOst->GetTyIdx());
}
} // namespace maple