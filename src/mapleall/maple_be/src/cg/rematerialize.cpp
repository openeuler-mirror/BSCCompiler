/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "rematerialize.h"
#include "reg_alloc_color_ra.h"

namespace maplebe {
bool Rematerializer::IsRematerializableForAddrof(CGFunc &cgFunc, const LiveRange &lr,
                                                 const LoopAnalysis &loopInfo) const {
  const MIRSymbol *symbol = rematInfo.sym;
  if (symbol->IsDeleted()) {
    return false;
  }
  if (symbol->IsThreadLocal()) {
    return false;
  }
  /* cost too much to remat */
  if ((symbol->GetStorageClass() == kScFormal) && (symbol->GetSKind() == kStVar) &&
      ((fieldID != 0) || (GlobalTables::GetTypeTable().GetTypeFromTyIdx(
          symbol->GetType()->GetTypeIndex().GetIdx())->GetSize() > k16ByteSize))) {
    return false;
  }
  if (!addrUpper && CGOptions::IsPIC() && ((symbol->GetStorageClass() == kScGlobal) ||
      (symbol->GetStorageClass() == kScExtern))) {
    /* check if in loop  */
    bool useInLoop = false;
    bool defOutLoop = false;
    for (auto luIt: lr.GetLuMap()) {
      BB *bb = cgFunc.GetBBFromID(luIt.first);
      LiveUnit *curLu = luIt.second;
      if (loopInfo.GetBBLoopParent(bb->GetId()) != nullptr && curLu->GetUseNum() != 0) {
        useInLoop = true;
      }
      if (loopInfo.GetBBLoopParent(bb->GetId()) == nullptr && curLu->GetDefNum() != 0) {
        defOutLoop = true;
      }
    }
    return !(useInLoop && defOutLoop);
  }
  return true;
}

bool Rematerializer::IsRematerializableForDread(CGFunc &cgFunc, RematLevel rematLev) const {
  const MIRSymbol *symbol = rematInfo.sym;
  if (symbol->IsDeleted()) {
    return false;
  }
  // cost greater than benefit
  if (symbol->IsThreadLocal()) {
    return false;
  }
  MIRStorageClass storageClass = symbol->GetStorageClass();
  if ((storageClass == kScAuto) || (storageClass == kScFormal)) {
    /* cost too much to remat. */
    return false;
  }
  uint32 offset = 0;
  if (fieldID != 0) {
    ASSERT(symbol->GetType()->IsMIRStructType(), "non-zero fieldID for non-structure");
    MIRStructType *structType = static_cast<MIRStructType*>(symbol->GetType());
    offset = structType->GetFieldOffsetFromBaseAddr(fieldID).byteOffset;
  }
  if (rematLev < kRematDreadGlobal && !symbol->IsLocal()) {
    return false;
  }
  return IsRematerializableForDread(offset);
}

bool Rematerializer::IsRematerializable(CGFunc &cgFunc, RematLevel rematLev,
                                        const LiveRange &lr, const LoopAnalysis &loopInfo) const {
  if (rematLev == kRematOff) {
    return false;
  }
  switch (op) {
    case OP_undef:
      return false;
    case OP_constval: {
      const MIRConst *mirConst = rematInfo.mirConst;
      if (mirConst->GetKind() != kConstInt) {
        return false;
      }
      const MIRIntConst *intConst = static_cast<const MIRIntConst*>(rematInfo.mirConst);
      int64 val = intConst->GetExtValue();
      return IsRematerializableForConstval(val, lr.GetSpillSize());
    }
    case OP_addrof: {
      if (rematLev < kRematAddr) {
        return false;
      }
      return IsRematerializableForAddrof(cgFunc, lr, loopInfo);
    }
    case OP_dread: {
      if (rematLev < kRematDreadLocal) {
        return false;
      }
      return IsRematerializableForDread(cgFunc, rematLev);
    }
    default:
      return false;
  }
}

std::vector<Insn*> Rematerializer::Rematerialize(CGFunc &cgFunc, RegOperand &regOp,
    const LiveRange &lr) {
  switch (op) {
    case OP_constval: {
      ASSERT(rematInfo.mirConst->GetKind() == kConstInt, "Unsupported constant");
      return RematerializeForConstval(cgFunc, regOp, lr);
    }
    case OP_dread: {
      const MIRSymbol *symbol = rematInfo.sym;
      PrimType symType = symbol->GetType()->GetPrimType();
      int32 offset = 0;
      if (fieldID != 0) {
        ASSERT(symbol->GetType()->IsMIRStructType(), "non-zero fieldID for non-structure");
        MIRStructType *structType = static_cast<MIRStructType*>(symbol->GetType());
        symType = structType->GetFieldType(fieldID)->GetPrimType();
        offset = static_cast<int32>(structType->GetFieldOffsetFromBaseAddr(fieldID).byteOffset);
      }
      return RematerializeForDread(cgFunc, regOp, offset, symType);
    }
    case OP_addrof: {
      const MIRSymbol *symbol = rematInfo.sym;
      int32 offset = 0;
      if (fieldID != 0) {
        ASSERT(symbol->GetType()->IsMIRStructType() || symbol->GetType()->IsMIRUnionType(),
            "non-zero fieldID for non-structure");
        MIRStructType *structType = static_cast<MIRStructType*>(symbol->GetType());
        offset = static_cast<int32>(structType->GetFieldOffsetFromBaseAddr(fieldID).byteOffset);
      }
      return RematerializeForAddrof(cgFunc, regOp, offset);
    }
    default:
      ASSERT(false, "Unexpected op in live range");
  }

  return std::vector<Insn*>();
}

}  /* namespace maplebe */
