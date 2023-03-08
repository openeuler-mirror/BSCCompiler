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

#include "aarch64_rematerialize.h"
#include "aarch64_insn.h"
#include "aarch64_cgfunc.h"
#include "reg_alloc_color_ra.h"


namespace maplebe {
bool AArch64Rematerializer::IsRematerializableForConstval(int64 val, uint32 bitLen) const {
  if (val >= -kMax16UnsignedImm && val <= kMax16UnsignedImm) {
    return true;
  }
  auto uval = static_cast<uint64>(val);
  if (IsMoveWidableImmediate(uval, bitLen)) {
    return true;
  }
  return IsBitmaskImmediate(uval, bitLen);
}

bool AArch64Rematerializer::IsRematerializableForDread(int32 offset) const {
  /* check stImm.GetOffset() is in addri12 */
  return IsBitSizeImmediate(static_cast<uint64>(static_cast<int64>(offset)), kMaxImmVal12Bits, 0);
}

std::vector<Insn*> AArch64Rematerializer::RematerializeForConstval(CGFunc &cgFunc,
    RegOperand &regOp, const LiveRange &lr) {
  std::vector<Insn*> insns;
  auto intConst = static_cast<const MIRIntConst*>(rematInfo.mirConst);
  ImmOperand *immOp = &cgFunc.GetOpndBuilder()->CreateImm(
      GetPrimTypeBitSize(intConst->GetType().GetPrimType()), intConst->GetExtValue());
  MOperator movOp = (lr.GetSpillSize() == k32BitSize) ? MOP_wmovri32 : MOP_xmovri64;
  insns.push_back(&cgFunc.GetInsnBuilder()->BuildInsn(movOp, regOp, *immOp));
  return insns;
}

std::vector<Insn*> AArch64Rematerializer::RematerializeForAddrof(CGFunc &cgFunc,
    RegOperand &regOp, int32 offset) {
  std::vector<Insn*> insns;
  auto &a64Func = static_cast<AArch64CGFunc&>(cgFunc);
  const MIRSymbol *symbol = rematInfo.sym;

  StImmOperand &stImm = a64Func.CreateStImmOperand(*symbol, offset, 0);
  if ((symbol->GetStorageClass() == kScAuto) || (symbol->GetStorageClass() == kScFormal)) {
    SymbolAlloc *symLoc = cgFunc.GetMemlayout()->GetSymAllocInfo(symbol->GetStIndex());
    ImmOperand *offsetOp = &cgFunc.GetOpndBuilder()->CreateImm(k64BitSize,
        static_cast<int64>(cgFunc.GetBaseOffset(*symLoc)) + offset);
    Insn *insn = &cgFunc.GetInsnBuilder()->BuildInsn(MOP_xaddrri12, regOp,
        *cgFunc.GetBaseReg(*symLoc), *offsetOp);
    if (cgFunc.GetCG()->GenerateVerboseCG()) {
      std::string comm = "local/formal var: " + symbol->GetName();
      insn->SetComment(comm);
    }
    insns.push_back(insn);
  } else {
    Insn *insn = &cgFunc.GetInsnBuilder()->BuildInsn(MOP_xadrp, regOp, stImm);
    insns.push_back(insn);
    if (!addrUpper && CGOptions::IsPIC() && ((symbol->GetStorageClass() == kScGlobal) ||
        (symbol->GetStorageClass() == kScExtern))) {
      /* ldr     x0, [x0, #:got_lo12:Ljava_2Flang_2FSystem_3B_7Cout] */
      OfstOperand &offsetOp = a64Func.CreateOfstOpnd(*symbol, offset, 0);
      MemOperand *memOpnd = a64Func.CreateMemOperand(GetPointerBitSize(), regOp, offsetOp, *symbol);
      MOperator ldOp = (memOpnd->GetSize() == k64BitSize) ? MOP_xldr : MOP_wldr;
      insn = &cgFunc.GetInsnBuilder()->BuildInsn(ldOp, regOp, *memOpnd);
      insns.push_back(insn);
      if (offset > 0) {
        ImmOperand &ofstOpnd = cgFunc.GetOpndBuilder()->CreateImm(k32BitSize, offset);
        insns.push_back(&cgFunc.GetInsnBuilder()->BuildInsn(MOP_xaddrri12, regOp, regOp, ofstOpnd));
      }
    } else if (!addrUpper) {
      insns.push_back(&cgFunc.GetInsnBuilder()->BuildInsn(MOP_xadrpl12, regOp, regOp, stImm));
    }
  }
  return insns;
}

std::vector<Insn*> AArch64Rematerializer::RematerializeForDread(CGFunc &cgFunc,
    RegOperand &regOp, int32 offset, PrimType type) {
  std::vector<Insn*> insns;
  auto &a64Func = static_cast<AArch64CGFunc&>(cgFunc);
  RegOperand *regOp64 = &cgFunc.GetOpndBuilder()->CreatePReg(regOp.GetRegisterNumber(),
      k64BitSize, regOp.GetRegisterType());
  uint32 dataSize = GetPrimTypeBitSize(type);
  MemOperand *spillMemOp = &a64Func.GetOrCreateMemOpndAfterRa(*rematInfo.sym,
      offset, dataSize, false, regOp64, insns);
  Insn *ldInsn = cgFunc.GetTargetRegInfo()->BuildLdrInsn(dataSize, type, regOp, *spillMemOp);
  insns.push_back(ldInsn);
  return insns;
}

}  /* namespace maplebe */
