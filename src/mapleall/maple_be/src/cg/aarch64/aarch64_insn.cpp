/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_insn.h"
#include <fstream>
#include "aarch64_cg.h"
#include "common_utils.h"
#include "insn.h"
#include "metadata_layout.h"

namespace maplebe {
uint32 AArch64Insn::GetResultNum() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 resNum = 0;
  for (size_t i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsDef()) {
      ++resNum;
    }
  }
  return resNum;
}

uint32 AArch64Insn::GetOpndNum() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 srcNum = 0;
  for (size_t i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsUse()) {
      ++srcNum;
    }
  }
  return srcNum;
}

void AArch64Insn::PrepareVectorOperand(RegOperand *regOpnd, uint32 &compositeOpnds) const {
  AArch64Insn *insn = const_cast<AArch64Insn*>(this);
  VectorRegSpec* vecSpec = static_cast<AArch64VectorInsn*>(insn)->GetAndRemoveRegSpecFromList();
  compositeOpnds = (vecSpec->compositeOpnds > 0) ? vecSpec->compositeOpnds : compositeOpnds;
  regOpnd->SetVecLanePosition(vecSpec->vecLane);
  switch (mOp) {
    case MOP_vanduuu:
    case MOP_vxoruuu:
    case MOP_voruuu:
    case MOP_vnotuu:
    case MOP_vextuuui: {
      regOpnd->SetVecLaneSize(k8ByteSize);
      regOpnd->SetVecElementSize(k8BitSize);
      break;
    }
    case MOP_vandvvv:
    case MOP_vxorvvv:
    case MOP_vorvvv:
    case MOP_vnotvv:
    case MOP_vextvvvi: {
      regOpnd->SetVecLaneSize(k16ByteSize);
      regOpnd->SetVecElementSize(k8BitSize);
      break;
    }
    default: {
      regOpnd->SetVecLaneSize(vecSpec->vecLaneMax);
      regOpnd->SetVecElementSize(vecSpec->vecElementSize);
      break;
    }
  }
}

uint8 AArch64Insn::GetLoadStoreSize() const {
  if (IsLoadStorePair()) {
    return k16ByteSize;
  }
  /* These are the loads and stores possible from PickLdStInsn() */
  switch (mOp) {
    case MOP_wldarb:
    case MOP_wldxrb:
    case MOP_wldaxrb:
    case MOP_wldrb:
    case MOP_wldrsb:
    case MOP_xldrsb:
    case MOP_wstrb:
    case MOP_wstlrb:
    case MOP_wstxrb:
    case MOP_wstlxrb:
      return k1ByteSize;
    case MOP_wldrh:
    case MOP_wldarh:
    case MOP_wldxrh:
    case MOP_wldaxrh:
    case MOP_wldrsh:
    case MOP_xldrsh:
    case MOP_wstrh:
    case MOP_wstlrh:
    case MOP_wstxrh:
    case MOP_wstlxrh:
      return k2ByteSize;
    case MOP_sldr:
    case MOP_wldr:
    case MOP_wldxr:
    case MOP_wldar:
    case MOP_wldaxr:
    case MOP_sstr:
    case MOP_wstr:
    case MOP_wstxr:
    case MOP_wstlr:
    case MOP_wstlxr:
    case MOP_xldrsw:
      return k4ByteSize;
    case MOP_dstr:
    case MOP_xstr:
    case MOP_xstxr:
    case MOP_xstlr:
    case MOP_xstlxr:
    case MOP_wstp:
    case MOP_sstp:
    case MOP_dldr:
    case MOP_xldr:
    case MOP_xldxr:
    case MOP_xldar:
    case MOP_xldaxr:
    case MOP_wldp:
    case MOP_sldp:
      return k8ByteSize;
    case MOP_xldp:
    case MOP_xldpsw:
    case MOP_dldp:
    case MOP_qldr:
    case MOP_xstp:
    case MOP_dstp:
    case MOP_qstr:
      return k16ByteSize;

    default:
      this->Dump();
      CHECK_FATAL(false, "Unsupported load/store op");
  }
}

Operand *AArch64Insn::GetResult(uint32 id) const {
  ASSERT(id < GetResultNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 tempIdx = 0;
  Operand* resOpnd = nullptr;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsDef()) {
      if (tempIdx == id) {
        resOpnd = opnds[i];
        break;
      } else {
        ++tempIdx;
      }
    }
  }
  return resOpnd;
}

void AArch64Insn::SetOpnd(uint32 id, Operand &opnd) {
  ASSERT(id < GetOpndNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 tempIdx = 0;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsUse()) {
      if (tempIdx == id) {
        opnds[i] = &opnd;
        return;
      } else {
        ++tempIdx;
      }
    }
  }
}

void AArch64Insn::SetResult(uint32 id, Operand &opnd) {
  ASSERT(id < GetResultNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 tempIdx = 0;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsDef()) {
      if (tempIdx == id) {
        opnds[i] = &opnd;
        return;
      } else {
        ++tempIdx;
      }
    }
  }
}

Operand *AArch64Insn::GetOpnd(uint32 id) const {
  ASSERT(id < GetOpndNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  Operand *resOpnd = nullptr;
  uint32 tempIdx = 0;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsUse()) {
      if (tempIdx == id) {
        resOpnd = opnds[i];
        break;
      } else {
        ++tempIdx;
      }
    }
  }
  return resOpnd;
}
/* Return the first memory access operand. */
Operand *AArch64Insn::GetMemOpnd() const {
  for (uint32 i = 0; i < opnds.size(); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      return &opnd;
    }
  }
  return nullptr;
}

/* Set the first memory access operand. */
void AArch64Insn::SetMemOpnd(MemOperand *memOpnd) {
  for (uint32 i = 0; i < static_cast<uint32>(opnds.size()); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      SetOperand(i, *memOpnd);
      return;
    }
  }
}

bool AArch64Insn::IsRegDefOrUse(regno_t regNO) const {
  uint32 opndNum = GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto listElem : listOpnd.GetOperands()) {
        RegOperand *regOpnd = static_cast<RegOperand*>(listElem);
        ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
        if (regNO == regOpnd->GetRegisterNumber()) {
          return true;
        }
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      RegOperand *base = memOpnd.GetBaseRegister();
      RegOperand *index = memOpnd.GetIndexRegister();
      if ((base != nullptr && base->GetRegisterNumber() == regNO) ||
          (index != nullptr && index->GetRegisterNumber() == regNO)) {
        return true;
      }
    } else if (opnd.IsConditionCode()) {
      if (regNO == kRFLAG) {
        return true;
      }
    } else if (opnd.IsRegister()) {
      if (static_cast<RegOperand&>(opnd).GetRegisterNumber() == regNO) {
        return true;
      }
    }
  }
  return false;
}

bool AArch64Insn::IsRegDefined(regno_t regNO) const {
  return GetDefRegs().count(regNO);
}

std::set<uint32> AArch64Insn::GetDefRegs() const {
  std::set<uint32> defRegNOs;
  size_t opndNum = opnds.size();
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = GetOperand(i);
    OpndProp *regProp = md->operand[i];
    bool isDef = regProp->IsDef();
    if (!isDef && !opnd.IsMemoryAccessOperand()) {
      continue;
    }
    if (opnd.IsList()) {
      for (auto *op : static_cast<ListOperand&>(opnd).GetOperands()) {
        ASSERT(op != nullptr, "invalid operand in list operand");
        defRegNOs.emplace(op->GetRegisterNumber());
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      RegOperand *base = memOpnd.GetBaseRegister();
      if (base != nullptr) {
        if (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi &&
            (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
          ASSERT(!defRegNOs.count(base->GetRegisterNumber()), "duplicate def in one insn");
          defRegNOs.emplace(base->GetRegisterNumber());
        }
      }
    } else if (opnd.IsConditionCode() || opnd.IsRegister()) {
      ASSERT(!defRegNOs.count(static_cast<RegOperand&>(opnd).GetRegisterNumber()), "duplicate def in one insn");
      defRegNOs.emplace(static_cast<RegOperand&>(opnd).GetRegisterNumber());
    }
  }
  return defRegNOs;
}

uint32 AArch64Insn::GetBothDefUseOpnd() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  size_t opndNum = opnds.size();
  uint32 opndIdx = kInsnMaxOpnd;
  for (uint32 i = 0; i < opndNum; ++i) {
    auto *opndProp = md->operand[i];
    if (opndProp->IsRegUse() && opndProp->IsDef()) {
      ASSERT(opndIdx == kInsnMaxOpnd, "Do not support in aarch64 yet");
      opndIdx = i;
    }
    if (opnds[i]->IsMemoryAccessOperand()) {
      auto *a64MemOpnd = static_cast<MemOperand*>(opnds[i]);
      if (!a64MemOpnd->IsIntactIndexed()) {
        ASSERT(opndIdx == kInsnMaxOpnd, "Do not support in aarch64 yet");
        opndIdx = i;
      }
    }
  }
  return opndIdx;
}

bool AArch64Insn::IsVolatile() const {
  return AArch64CG::kMd[mOp].IsVolatile();
}

bool AArch64Insn::IsMemAccessBar() const {
  return AArch64CG::kMd[mOp].IsMemAccessBar();
}

bool AArch64Insn::IsBranch() const {
  return AArch64CG::kMd[mOp].IsBranch();
}

bool AArch64Insn::IsCondBranch() const {
  return AArch64CG::kMd[mOp].IsCondBranch();
}

bool AArch64Insn::IsUnCondBranch() const {
  return AArch64CG::kMd[mOp].IsUnCondBranch();
}

bool AArch64Insn::IsCall() const {
  return AArch64CG::kMd[mOp].IsCall();
}

bool AArch64Insn::IsVectorOp() const {
  return AArch64CG::kMd[mOp].IsVectorOp();
}

bool AArch64Insn::HasLoop() const {
  return AArch64CG::kMd[mOp].HasLoop();
}

bool AArch64Insn::IsSpecialIntrinsic() const {
  switch (mOp) {
    case MOP_vwdupur:
    case MOP_vwdupvr:
    case MOP_vxdupur:
    case MOP_vxdupvr:
    case MOP_vduprv:
    case MOP_vwinsur:
    case MOP_vxinsur:
    case MOP_vwinsvr:
    case MOP_vxinsvr:
    case MOP_get_and_addI:
    case MOP_get_and_addL:
    case MOP_compare_and_swapI:
    case MOP_compare_and_swapL:
    case MOP_string_indexof:
    case MOP_lazy_ldr:
    case MOP_get_and_setI:
    case MOP_get_and_setL:
    case MOP_tls_desc_rel: {
      return true;
    }
    default: {
      return false;
    }
  }
}

bool AArch64Insn::IsAsmInsn() const {
  return (mOp == MOP_asm);
}

bool AArch64Insn::IsTailCall() const {
  return (mOp == MOP_tail_call_opt_xbl || mOp == MOP_tail_call_opt_xblr);
}

bool AArch64Insn::IsClinit() const {
  return (mOp == MOP_clinit || mOp == MOP_clinit_tail || mOp == MOP_adrp_ldr);
}

bool AArch64Insn::IsLazyLoad() const {
  return (mOp == MOP_lazy_ldr) || (mOp == MOP_lazy_ldr_static) || (mOp == MOP_lazy_tail);
}

bool AArch64Insn::IsAdrpLdr() const {
  return mOp == MOP_adrp_ldr;
}

bool AArch64Insn::IsArrayClassCache() const {
  return mOp == MOP_arrayclass_cache_ldr;
}

bool AArch64Insn::CanThrow() const {
  return AArch64CG::kMd[mOp].CanThrow();
}

bool AArch64Insn::IsMemAccess() const {
  return AArch64CG::kMd[mOp].IsMemAccess();
}

bool AArch64Insn::MayThrow() {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  if (md->IsMemAccess() && !IsLoadLabel()) {
    auto *aarchMemOpnd = static_cast<MemOperand*>(GetMemOpnd());
    ASSERT(aarchMemOpnd != nullptr, "CG invalid memory operand.");
    RegOperand *baseRegister = aarchMemOpnd->GetBaseRegister();
    if (baseRegister != nullptr &&
        (baseRegister->GetRegisterNumber() == RFP || baseRegister->GetRegisterNumber() == RSP)) {
      return false;
    }
  }
  return md->CanThrow();
}

bool AArch64Insn::IsCallToFunctionThatNeverReturns() {
  if (IsIndirectCall()) {
    return false;
  }
  auto *target = static_cast<FuncNameOperand*>(GetCallTargetOperand());
  CHECK_FATAL(target != nullptr, "target is null in AArch64Insn::IsCallToFunctionThatNeverReturns");
  const MIRSymbol *funcSt = target->GetFunctionSymbol();
  ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
  MIRFunction *func = funcSt->GetFunction();
  return func->NeverReturns();
}

bool AArch64Insn::IsDMBInsn() const {
  return AArch64CG::kMd[mOp].IsDMB();
}

bool AArch64Insn::IsMove() const {
  return AArch64CG::kMd[mOp].IsMove();
}

bool AArch64Insn::IsMoveRegReg() const {
  return mOp == MOP_xmovrr || mOp == MOP_wmovrr || mOp == MOP_xvmovs || mOp  == MOP_xvmovd;
}

bool AArch64Insn::IsPhi() const {
  return AArch64CG::kMd[mOp].IsPhi();
}

bool AArch64Insn::IsLoad() const {
  return AArch64CG::kMd[mOp].IsLoad();
}

bool AArch64Insn::IsLoadLabel() const {
  return (mOp == MOP_wldli || mOp == MOP_xldli || mOp == MOP_sldli || mOp == MOP_dldli);
}

bool AArch64Insn::IsStore() const {
  return AArch64CG::kMd[mOp].IsStore();
}

bool AArch64Insn::IsLoadPair() const {
  return AArch64CG::kMd[mOp].IsLoadPair();
}

bool AArch64Insn::IsStorePair() const {
  return AArch64CG::kMd[mOp].IsStorePair();
}

bool AArch64Insn::IsLoadStorePair() const {
  return AArch64CG::kMd[mOp].IsLoadStorePair();
}

bool AArch64Insn::IsLoadAddress() const {
  return AArch64CG::kMd[mOp].IsLoadAddress();
}

bool AArch64Insn::IsAtomic() const {
  return AArch64CG::kMd[mOp].IsAtomic();
}

bool AArch64Insn::IsPartDef() const {
  return AArch64CG::kMd[mOp].IsPartDef();
}

bool AArch64Insn::OpndIsDef(uint32 id) const {
  return AArch64CG::kMd[mOp].GetOperand(id)->IsDef();
}

bool AArch64Insn::OpndIsUse(uint32 id) const {
  return AArch64CG::kMd[mOp].GetOperand(id)->IsUse();
}

uint32 AArch64Insn::GetLatencyType() const {
  return AArch64CG::kMd[mOp].GetLatencyType();
}

uint32 AArch64Insn::GetAtomicNum() const {
  return AArch64CG::kMd[mOp].GetAtomicNum();
}

bool AArch64Insn::IsYieldPoint() const {
  /*
   * It is a yieldpoint if loading from a dedicated
   * register holding polling page address:
   * ldr  wzr, [RYP]
   */
  if (IsLoad() && !IsLoadLabel()) {
    auto mem = static_cast<MemOperand*>(GetOpnd(0));
    return (mem != nullptr && mem->GetBaseRegister() != nullptr && mem->GetBaseRegister()->GetRegisterNumber() == RYP);
  }
  return false;
}
/* Return the copy operand id of reg1 if it is an insn who just do copy from reg1 to reg2.
 * i. mov reg2, reg1
 * ii. add/sub reg2, reg1, 0/zero register
 * iii. mul reg2, reg1, 1
 */
int32 AArch64Insn::CopyOperands() const {
  if (mOp >= MOP_xmovrr  && mOp <= MOP_xvmovrv) {
    return 1;
  }
  if (mOp == MOP_vmovuu || mOp == MOP_vmovvv) {
    return 1;
  }
  if ((mOp >= MOP_xaddrrr && mOp <= MOP_ssub) || (mOp >= MOP_xlslrri6 && mOp <= MOP_wlsrrrr)) {
    Operand &opnd2 = GetOperand(kInsnThirdOpnd);
    if (opnd2.IsIntImmediate()) {
      auto &immOpnd = static_cast<ImmOperand&>(opnd2);
      if (immOpnd.IsZero()) {
        return 1;
      }
    }
  }
  if (mOp > MOP_xmulrrr && mOp <= MOP_xvmuld) {
    Operand &opnd2 = GetOperand(kInsnThirdOpnd);
    if (opnd2.IsIntImmediate()) {
      auto &immOpnd = static_cast<ImmOperand&>(opnd2);
      if (immOpnd.GetValue() == 1) {
        return 1;
      }
    }
  }
  return -1;
}

void AArch64Insn::CheckOpnd(const Operand &opnd, const OpndProp &prop) const {
  (void)opnd;
  (void)prop;
#if DEBUG
  auto &mopd = (prop);
  switch (opnd.GetKind()) {
    case Operand::kOpdRegister:
      ASSERT(mopd.IsRegister(), "expect reg");
      break;
    case Operand::kOpdOffset:
    case Operand::kOpdImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdImmediate, "expect imm");
      break;
    case Operand::kOpdFPImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdFPImmediate, "expect fpimm");
      break;
    case Operand::kOpdFPZeroImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdFPZeroImmediate, "expect fpzero");
      break;
    case Operand::kOpdMem:
      ASSERT(mopd.GetOperandType() == Operand::kOpdMem, "expect mem");
      break;
    case Operand::kOpdBBAddress:
      ASSERT(mopd.GetOperandType() == Operand::kOpdBBAddress, "expect address");
      break;
    case Operand::kOpdList:
      ASSERT(mopd.GetOperandType() == Operand::kOpdList, "expect list operand");
      break;
    case Operand::kOpdCond:
      ASSERT(mopd.GetOperandType() == Operand::kOpdCond, "expect cond operand");
      break;
    case Operand::kOpdShift:
      ASSERT(mopd.GetOperandType() == Operand::kOpdShift, "expect LSL operand");
      break;
    case Operand::kOpdStImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdStImmediate, "expect symbol name (literal)");
      break;
    case Operand::kOpdString:
      ASSERT(mopd.GetOperandType() == Operand::kOpdString, "expect a string");
      break;
    default:
      ASSERT(false, "NYI");
      break;
  }
#endif
}

/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label operand index from the given instruction.
 * Note: MOP_xbr is a jump instruction, but the target is unknown at compile time,
 * because a register instead of label. So we don't take it as a branching instruction.
 * Howeer for special long range branch patch, the label is installed in this case.
 */
uint32 AArch64Insn::GetJumpTargetIdx() const {
  return GetJumpTargetIdxFromMOp(mOp);
}

uint32 AArch64Insn::GetJumpTargetIdxFromMOp(MOperator mOp) const {
  switch (mOp) {
    /* unconditional jump */
    case MOP_xuncond: {
      return kOperandPosition0;
    }
    case MOP_xbr: {
      CHECK_FATAL(opnds[1] != nullptr, "ERR");
      return kOperandPosition1;
    }
    /* conditional jump */
    case MOP_bmi:
    case MOP_bvc:
    case MOP_bls:
    case MOP_blt:
    case MOP_ble:
    case MOP_blo:
    case MOP_beq:
    case MOP_bpl:
    case MOP_bhs:
    case MOP_bvs:
    case MOP_bhi:
    case MOP_bgt:
    case MOP_bge:
    case MOP_bne:
    case MOP_bcc:
    case MOP_bcs:
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      return kOperandPosition1;
    }
    case MOP_wtbz:
    case MOP_xtbz:
    case MOP_wtbnz:
    case MOP_xtbnz: {
      return kOperandPosition2;
    }
    default:
      CHECK_FATAL(false, "Not a jump insn");
  }
  return kOperandPosition0;
}

MOperator AArch64Insn::FlipConditionOp(MOperator originalOp, uint32 &targetIdx) {
  targetIdx = 1;
  switch (originalOp) {
    case AArch64MOP_t::MOP_beq:
      return AArch64MOP_t::MOP_bne;
    case AArch64MOP_t::MOP_bge:
      return AArch64MOP_t::MOP_blt;
    case AArch64MOP_t::MOP_bgt:
      return AArch64MOP_t::MOP_ble;
    case AArch64MOP_t::MOP_bhi:
      return AArch64MOP_t::MOP_bls;
    case AArch64MOP_t::MOP_bhs:
      return AArch64MOP_t::MOP_blo;
    case AArch64MOP_t::MOP_ble:
      return AArch64MOP_t::MOP_bgt;
    case AArch64MOP_t::MOP_blo:
      return AArch64MOP_t::MOP_bhs;
    case AArch64MOP_t::MOP_bls:
      return AArch64MOP_t::MOP_bhi;
    case AArch64MOP_t::MOP_blt:
      return AArch64MOP_t::MOP_bge;
    case AArch64MOP_t::MOP_bne:
      return AArch64MOP_t::MOP_beq;
    case AArch64MOP_t::MOP_bpl:
      return AArch64MOP_t::MOP_bmi;
    case AArch64MOP_t::MOP_xcbnz:
      return AArch64MOP_t::MOP_xcbz;
    case AArch64MOP_t::MOP_wcbnz:
      return AArch64MOP_t::MOP_wcbz;
    case AArch64MOP_t::MOP_xcbz:
      return AArch64MOP_t::MOP_xcbnz;
    case AArch64MOP_t::MOP_wcbz:
      return AArch64MOP_t::MOP_wcbnz;
    case AArch64MOP_t::MOP_wtbnz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_wtbz);
      return AArch64MOP_t::MOP_wtbz;
    case AArch64MOP_t::MOP_wtbz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_wtbnz);
      return AArch64MOP_t::MOP_wtbnz;
    case AArch64MOP_t::MOP_xtbnz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_xtbz);
      return AArch64MOP_t::MOP_xtbz;
    case AArch64MOP_t::MOP_xtbz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_xtbnz);
      return AArch64MOP_t::MOP_xtbnz;
    default:
      break;
  }
  return AArch64MOP_t::MOP_undef;
}

bool AArch64Insn::Check() const {
#if DEBUG
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  if (md == nullptr) {
    return false;
  }
  for (uint32 i = 0; i < GetOperandSize(); ++i) {
    Operand &opnd = GetOperand(i);
    /* maybe if !opnd, break ? */
    CheckOpnd(opnd, *(md->operand[i]));
  }
  return true;
#else
  return false;
#endif
}

void AArch64Insn::Dump() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  ASSERT(md != nullptr, "md should not be nullptr");

  LogInfo::MapleLogger() << "< " << GetId() << " > ";
  LogInfo::MapleLogger() << md->name << "(" << mOp << ")";

  for (uint32 i = 0; i < GetOperandSize(); ++i) {
    Operand &opnd = GetOperand(i);
    LogInfo::MapleLogger() << " (opnd" << i << ": ";

    A64OpndDumpVisitor visitor;
    opnd.Accept(visitor);
    LogInfo::MapleLogger() << ")";
  }

  if (ssaImplicitDefOpnd) {
    LogInfo::MapleLogger() << " (implicitDefOpnd: ";
    A64OpndDumpVisitor visitor;
    ssaImplicitDefOpnd->Accept(visitor);
    LogInfo::MapleLogger() << ")";
  }

  if (IsVectorOp()) {
    auto *vInsn = static_cast<const AArch64VectorInsn*>(this);
    if (vInsn->GetNumOfRegSpec() != 0) {
      LogInfo::MapleLogger() << " (vecSpec: " << vInsn->GetNumOfRegSpec() << ")";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

bool AArch64Insn::IsDefinition() const {
  /* check if we are seeing ldp or not */
  ASSERT(AArch64CG::kMd[mOp].GetOperand(1) == nullptr ||
         !AArch64CG::kMd[mOp].GetOperand(1)->IsRegDef(), "check if we are seeing ldp or not");
  if (AArch64CG::kMd[mOp].GetOperand(0) == nullptr) {
    return false;
  }
  return AArch64CG::kMd[mOp].GetOperand(0)->IsRegDef();
}

bool AArch64Insn::IsDestRegAlsoSrcReg() const {
  auto *prop0 = (AArch64CG::kMd[mOp].GetOperand(0));
  ASSERT(prop0 != nullptr, "expect a OpndProp");
  return prop0->IsRegDef() && prop0->IsRegUse();
}

void A64OpndEmitVisitor::EmitIntReg(const RegOperand &v, uint8 opndSz) {
  CHECK_FATAL(v.GetRegisterType() == kRegTyInt, "wrong Type");
  uint8 opndSize = (opndSz == kMaxSimm32) ? v.GetSize() : opndSz;
  ASSERT((opndSize == k32BitSize || opndSize == k64BitSize), "illegal register size");
#ifdef USE_32BIT_REF
  bool r32 = (opndSize == k32BitSize) || isRefField;
#else
  bool r32 = (opndSize == k32BitSize);
#endif  /* USE_32BIT_REF */
  (void)emitter.Emit(AArch64CG::intRegNames[(r32 ? AArch64CG::kR32List : AArch64CG::kR64List)][v.GetRegisterNumber()]);
}

void A64OpndEmitVisitor::Visit(maplebe::RegOperand *v) {
  ASSERT(opndProp == nullptr || opndProp->IsRegister(),
         "operand type doesn't match");
  uint32 size = v->GetSize();
  regno_t regNO = v->GetRegisterNumber();
  uint8 opndSize = (opndProp != nullptr) ? opndProp->GetSize() : size;
  switch (v->GetRegisterType()) {
    case kRegTyInt: {
      EmitIntReg(*v, opndSize);
      break;
    }
    case kRegTyFloat: {
      ASSERT((opndSize == k8BitSize || opndSize == k16BitSize || opndSize == k32BitSize ||
          opndSize == k64BitSize || opndSize == k128BitSize), "illegal register size");
      if (opndProp->IsVectorOperand() && v->GetVecLaneSize() != 0) {
        EmitVectorOperand(*v);
      } else {
        /* FP reg cannot be reffield. 8~0, 16~1, 32~2, 64~3. 8 is 1000b, has 3 zero. */
        uint32 regSet = __builtin_ctz(static_cast<uint32>(opndSize)) - 3;
        (void)emitter.Emit(AArch64CG::intRegNames[regSet][regNO]);
      }
      break;
    }
    default:
      ASSERT(false, "NYI");
      break;
  }
}

void A64OpndEmitVisitor::Visit(maplebe::ImmOperand *v) {
  if (v->IsOfstImmediate()) {
    return Visit(static_cast<OfstOperand*>(v));
  }

  int64 value = v->GetValue();
  if (!v->IsFmov()) {
    (void)emitter.Emit((opndProp != nullptr && opndProp->IsLoadLiteral()) ? "=" : "#")
        .Emit((v->GetSize() == k64BitSize) ? value : static_cast<int64>(static_cast<int32>(value)));
    return;
  }
  /*
   * compute float value
   * use top 4 bits expect MSB of value . then calculate its fourth power
   */
  int32 exp = static_cast<int32>((((static_cast<uint32>(value) & 0x70) >> 4) ^ 0x4) - 3);
  /* use the lower four bits of value in this expression */
  const float mantissa = 1.0 + (static_cast<float>(static_cast<uint64>(value) & 0xf) / 16.0);
  float result = std::pow(2, exp) * mantissa;

  std::stringstream ss;
  ss << std::setprecision(10) << result;
  std::string res;
  ss >> res;
  size_t dot = res.find('.');
  if (dot == std::string::npos) {
    res += ".0";
    dot = res.find('.');
    CHECK_FATAL(dot != std::string::npos, "cannot find in string");
  }
  (void)res.erase(dot, 1);
  std::string integer(res, 0, 1);
  std::string fraction(res, 1);
  while (fraction.size() != 1 && fraction[fraction.size() - 1] == '0') {
    fraction.pop_back();
  }
  /* fetch the sign bit of this value */
  std::string sign = ((static_cast<uint64>(value) & 0x80) > 0) ? "-" : "";
  (void)emitter.Emit(sign + integer + "." + fraction + "e+").Emit(static_cast<int64>(dot) - 1);
}

void A64OpndEmitVisitor::Visit(maplebe::MemOperand *v) {
  auto a64v = static_cast<MemOperand*>(v);
  MemOperand::AArch64AddressingMode addressMode = a64v->GetAddrMode();
#if DEBUG
  const AArch64MD *md = &AArch64CG::kMd[emitter.GetCurrentMOP()];
  bool isLDSTpair = md->IsLoadStorePair();
  ASSERT(md->Is64Bit() || md->GetOperandSize() <= k32BitSize || md->GetOperandSize() == k128BitSize,
         "unexpected opnd size");
#endif
  if (addressMode == MemOperand::kAddrModeBOi) {
    (void)emitter.Emit("[");
    auto *baseReg = v->GetBaseRegister();
    ASSERT(baseReg != nullptr, "expect an RegOperand here");
    uint32 baseSize = baseReg->GetSize();
    if (baseSize != k64BitSize) {
      baseReg->SetSize(k64BitSize);
    }
    EmitIntReg(*baseReg);
    baseReg->SetSize(baseSize);
    OfstOperand *offset = a64v->GetOffsetImmediate();
    if (offset != nullptr) {
#ifndef USE_32BIT_REF  /* can be load a ref here */
      /*
       * Cortex-A57 Software Optimization Guide:
       * The ARMv8-A architecture allows many types of load and store accesses to be arbitrarily aligned.
       * The Cortex- A57 processor handles most unaligned accesses without performance penalties.
       */
#if DEBUG
      if (a64v->IsOffsetMisaligned(md->GetOperandSize())) {
        INFO(kLncInfo, "The Memory operand's offset is misaligned:", "");
        LogInfo::MapleLogger() << "===";
        A64OpndDumpVisitor visitor;
        v->Accept(visitor);
        LogInfo::MapleLogger() << "===\n";
      }
#endif
#endif  /* USE_32BIT_REF */
      if (a64v->IsPostIndexed()) {
        ASSERT(!a64v->IsSIMMOffsetOutOfRange(offset->GetOffsetValue(), md->Is64Bit(), isLDSTpair),
               "should not be SIMMOffsetOutOfRange");
        (void)emitter.Emit("]");
        if (!offset->IsZero()) {
          (void)emitter.Emit(", ");
          Visit(offset);
        }
      } else if (a64v->IsPreIndexed()) {
        ASSERT(!a64v->IsSIMMOffsetOutOfRange(offset->GetOffsetValue(), md->Is64Bit(), isLDSTpair),
               "should not be SIMMOffsetOutOfRange");
        if (!offset->IsZero()) {
          (void)emitter.Emit(",");
          Visit(offset);
        }
        (void)emitter.Emit("]!");
      } else {
        if (CGOptions::IsPIC() && (offset->IsSymOffset() || offset->IsSymAndImmOffset()) &&
            (offset->GetSymbol()->NeedPIC() || offset->GetSymbol()->IsThreadLocal())) {
          std::string gotEntry = offset->GetSymbol()->IsThreadLocal() ? ", #:tlsdesc_lo12:" : ", #:got_lo12:";
          (void)emitter.Emit(gotEntry + offset->GetSymbolName());
        } else {
          if (!offset->IsZero()) {
            (void)emitter.Emit(",");
            Visit(offset);
          }
        }
        (void)emitter.Emit("]");
      }
    } else {
      (void)emitter.Emit("]");
    }
  } else if (addressMode == MemOperand::kAddrModeBOrX) {
    /*
     * Base plus offset   | [base{, #imm}]  [base, Xm{, LSL #imm}]   [base, Wm, (S|U)XTW {#imm}]
     *                      offset_opnds=nullptr
     *                                      offset_opnds=64          offset_opnds=32
     *                                      imm=0 or 3               imm=0 or 2, s/u
     */
    (void)emitter.Emit("[");
    auto *baseReg = v->GetBaseRegister();
    // After ssa version support different size, the value is changed back
    baseReg->SetSize(k64BitSize);

    EmitIntReg(*baseReg);
    (void)emitter.Emit(",");
    EmitIntReg(*a64v->GetIndexRegister());
    if (a64v->ShouldEmitExtend() || v->GetBaseRegister()->GetSize() > a64v->GetIndexRegister()->GetSize()) {
      (void)emitter.Emit(",");
      /* extend, #0, of #3/#2 */
      (void)emitter.Emit(a64v->GetExtendAsString());
      if (a64v->GetExtendAsString() == "LSL" || a64v->ShiftAmount() != 0) {
        (void)emitter.Emit(" #");
        (void)emitter.Emit(a64v->ShiftAmount());
      }
    }
    (void)emitter.Emit("]");
  } else if (addressMode == MemOperand::kAddrModeLiteral) {
    CHECK_FATAL(opndProp != nullptr, "prop is nullptr in  MemOperand::Emit");
    if (opndProp->IsMemLow12()) {
      (void)emitter.Emit("#:lo12:");
    }
    (void)emitter.Emit(v->GetSymbol()->GetName());
  } else if (addressMode == MemOperand::kAddrModeLo12Li) {
    (void)emitter.Emit("[");
    EmitIntReg(*v->GetBaseRegister());

    OfstOperand *offset = a64v->GetOffsetImmediate();
    ASSERT(offset != nullptr, "nullptr check");

    (void)emitter.Emit(", #:lo12:");
    if (v->GetSymbol()->GetAsmAttr() != UStrIdx(0) &&
        (v->GetSymbol()->GetStorageClass() == kScPstatic || v->GetSymbol()->GetStorageClass() == kScPstatic)) {
      std::string asmSection = GlobalTables::GetUStrTable().GetStringFromStrIdx(v->GetSymbol()->GetAsmAttr());
      (void)emitter.Emit(asmSection);
    } else {
      if (v->GetSymbol()->GetStorageClass() == kScPstatic && v->GetSymbol()->IsLocal()) {
        PUIdx pIdx = emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
        (void)emitter.Emit(a64v->GetSymbolName() + std::to_string(pIdx));
      } else {
        (void)emitter.Emit(a64v->GetSymbolName());
      }
    }
    if (!offset->IsZero()) {
      (void)emitter.Emit("+");
      (void)emitter.Emit(std::to_string(offset->GetOffsetValue()));
    }
    (void)emitter.Emit("]");
  } else {
    ASSERT(false, "nyi");
  }
}

void A64OpndEmitVisitor::Visit(LabelOperand *v) {
  emitter.EmitLabelRef(v->GetLabelIndex());
}

void A64OpndEmitVisitor::Visit(CondOperand *v) {
  (void)emitter.Emit(CondOperand::ccStrs[v->GetCode()]);
}

void A64OpndEmitVisitor::Visit(ExtendShiftOperand *v) {
  ASSERT(v->GetShiftAmount() <= k4BitSize && v->GetShiftAmount() >= 0,
         "shift amount out of range in ExtendShiftOperand");
  auto emitExtendShift = [this, v](const std::string &extendKind)->void {
    (void)emitter.Emit(extendKind);
    if (v->GetShiftAmount() != 0) {
      (void)emitter.Emit(" #").Emit(v->GetShiftAmount());
    }
  };
  switch (v->GetExtendOp()) {
    case ExtendShiftOperand::kUXTB:
      emitExtendShift("UXTB");
      break;
    case ExtendShiftOperand::kUXTH:
      emitExtendShift("UXTH");
      break;
    case ExtendShiftOperand::kUXTW:
      emitExtendShift("UXTW");
      break;
    case ExtendShiftOperand::kUXTX:
      emitExtendShift("UXTX");
      break;
    case ExtendShiftOperand::kSXTB:
      emitExtendShift("SXTB");
      break;
    case ExtendShiftOperand::kSXTH:
      emitExtendShift("SXTH");
      break;
    case ExtendShiftOperand::kSXTW:
      emitExtendShift("SXTW");
      break;
    case ExtendShiftOperand::kSXTX:
      emitExtendShift("SXTX");
      break;
    default:
      ASSERT(false, "should not be here");
      break;
  }
}

void A64OpndEmitVisitor::Visit(BitShiftOperand *v) {
  (void)emitter.Emit((v->GetShiftOp() == BitShiftOperand::kLSL) ? "LSL #" :
      ((v->GetShiftOp() == BitShiftOperand::kLSR) ? "LSR #" : "ASR #")).Emit(v->GetShiftAmount());
}

void A64OpndEmitVisitor::Visit(StImmOperand *v) {
  CHECK_FATAL(opndProp != nullptr, "opndProp is nullptr in  StImmOperand::Emit");
  const MIRSymbol *symbol = v->GetSymbol();
  const bool isThreadLocal = symbol->IsThreadLocal();
  const bool isLiteralLow12 = opndProp->IsLiteralLow12();
  const bool hasGotEntry = CGOptions::IsPIC() && symbol->NeedPIC();
  bool hasPrefix = false;
  if (isThreadLocal) {
    (void)emitter.Emit(":tlsdesc");
    hasPrefix = true;
  }
  if (!hasPrefix && hasGotEntry) {
    (void)emitter.Emit(":got");
    hasPrefix = true;
  }
  if (isLiteralLow12) {
    std::string lo12String = hasPrefix ? "_lo12" : ":lo12";
    (void)emitter.Emit(lo12String);
    hasPrefix = true;
  }
  if (hasPrefix) {
    (void)emitter.Emit(":");
  }
  if (symbol->GetAsmAttr() != UStrIdx(0) &&
      (symbol->GetStorageClass() == kScPstatic || symbol->GetStorageClass() == kScPstatic)) {
    std::string asmSection = GlobalTables::GetUStrTable().GetStringFromStrIdx(symbol->GetAsmAttr());
    (void)emitter.Emit(asmSection);
  } else {
    if (symbol->GetStorageClass() == kScPstatic && symbol->GetSKind() != kStConst && symbol->IsLocal()) {
      (void)emitter.Emit(symbol->GetName() +
          std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()));
    } else {
      (void)emitter.Emit(v->GetName());
    }
  }
  if (!hasGotEntry && v->GetOffset() != 0) {
    (void)emitter.Emit("+" + std::to_string(v->GetOffset()));
  }
}

void A64OpndEmitVisitor::Visit(FuncNameOperand *v) {
  (void)emitter.Emit(v->GetName());
}

void A64OpndEmitVisitor::Visit(LogicalShiftLeftOperand *v) {
  (void)emitter.Emit(" LSL #").Emit(v->GetShiftAmount());
}

void A64OpndEmitVisitor::Visit(CommentOperand *v) {
  (void)emitter.Emit(v->GetComment());
}

void A64OpndEmitVisitor::Visit(ListOperand *v) {
  (void)opndProp;
  size_t nLeft = v->GetOperands().size();
  if (nLeft == 0) {
    return;
  }

  for (auto it = v->GetOperands().begin(); it != v->GetOperands().end(); ++it) {
    Visit(*it);
    if (--nLeft >= 1) {
      (void)emitter.Emit(", ");
    }
  }
}

void A64OpndEmitVisitor::Visit(OfstOperand *v) {
  int64 value = v->GetValue();
  if (v->IsImmOffset()) {
    (void)emitter.Emit((opndProp != nullptr && opndProp->IsLoadLiteral()) ? "=" : "#")
        .Emit((v->GetSize() == k64BitSize) ? value : static_cast<int64>(static_cast<int32>(value)));
    return;
  }
  const MIRSymbol *symbol = v->GetSymbol();
  if (CGOptions::IsPIC() && symbol->NeedPIC()) {
    (void)emitter.Emit(":got:" + symbol->GetName());
  } else if (symbol->GetStorageClass() == kScPstatic && symbol->GetSKind() != kStConst && symbol->IsLocal()) {
    (void)emitter.Emit(symbol->GetName() +
        std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()));
  } else {
    (void)emitter.Emit(symbol->GetName());
  }
  if (value != 0) {
    (void)emitter.Emit("+" + std::to_string(value));
  }
}

void A64OpndEmitVisitor::EmitVectorOperand(const RegOperand &v) {
  std::string width;
  switch (v.GetVecElementSize()) {
    case k8BitSize:
      width = "b";
      break;
    case k16BitSize:
      width = "h";
      break;
    case k32BitSize:
      width = "s";
      break;
    case k64BitSize:
      width = "d";
      break;
    default:
      CHECK_FATAL(false, "unexpected value size for vector element");
      break;
  }
  (void)emitter.Emit(AArch64CG::vectorRegNames[v.GetRegisterNumber()]);
  int32 lanePos = v.GetVecLanePosition();
  if (lanePos == -1) {
    (void)emitter.Emit("." + std::to_string(v.GetVecLaneSize()) + width);
  } else {
    (void)emitter.Emit("." + width + "[" + std::to_string(lanePos) + "]");
  }
}

void A64OpndDumpVisitor::Visit(RegOperand *v) {
  std::array<const std::string, kRegTyLast> prims = { "U", "R", "V", "C", "X", "Vra" };
  std::array<const std::string, kRegTyLast> classes = { "[U]", "[I]", "[F]", "[CC]", "[X87]", "[Vra]" };
  uint32 regType = v->GetRegisterType();
  ASSERT(regType < kRegTyLast, "unexpected regType");

  regno_t reg = v->GetRegisterNumber();
  reg = v->IsVirtualRegister() ? reg : (reg - 1);
  uint32 vb = v->GetValidBitsNum();
  LogInfo::MapleLogger() << (v->IsVirtualRegister() ? "vreg:" : " reg:") <<
      prims[regType] << reg << " " << classes[regType];
  if (v->GetValidBitsNum() != v->GetSize()) {
    LogInfo::MapleLogger() << " Vb: [" << vb << "]";
  }
  LogInfo::MapleLogger() << " Sz: [" << v->GetSize() << "]" ;
}

void A64OpndDumpVisitor::Visit(ImmOperand *v) {
  LogInfo::MapleLogger() << "imm:" << v->GetValue();
}
void A64OpndDumpVisitor::Visit(MemOperand *a64v) {
  LogInfo::MapleLogger() << "Mem:";
  LogInfo::MapleLogger() << " size:" << a64v->GetSize() << " ";
  LogInfo::MapleLogger() << " isStack:" << a64v->IsStackMem() << "-" << a64v->IsStackArgMem() << " ";
  switch (a64v->GetAddrMode()) {
    case MemOperand::kAddrModeBOi: {
      LogInfo::MapleLogger() << "base:";
      Visit(a64v->GetBaseRegister());
      LogInfo::MapleLogger() << "offset:";
      Visit(a64v->GetOffsetOperand());
      switch (a64v->GetIndexOpt()) {
        case MemOperand::kIntact:
          LogInfo::MapleLogger() << "  intact";
          break;
        case MemOperand::kPreIndex:
          LogInfo::MapleLogger() << "  pre-index";
          break;
        case MemOperand::kPostIndex:
          LogInfo::MapleLogger() << "  post-index";
          break;
        default:
          break;
      }
      break;
    }
    case MemOperand::kAddrModeBOrX: {
      LogInfo::MapleLogger() << "base:";
      Visit(a64v->GetBaseRegister());
      LogInfo::MapleLogger() << "offset:";
      Visit(a64v->GetIndexRegister());
      LogInfo::MapleLogger() << " " << a64v->GetExtendAsString();
      LogInfo::MapleLogger() << " shift: " << a64v->ShiftAmount();
      LogInfo::MapleLogger() << " extend: " << a64v->GetExtendAsString();
      break;
    }
    case MemOperand::kAddrModeLiteral:
      LogInfo::MapleLogger() << "literal: " << a64v->GetSymbolName();
      break;
    case MemOperand::kAddrModeLo12Li: {
      LogInfo::MapleLogger() << "base:";
      Visit(a64v->GetBaseRegister());
      LogInfo::MapleLogger() << "offset:";
      OfstOperand *offOpnd = a64v->GetOffsetImmediate();
      LogInfo::MapleLogger() << "#:lo12:";
      if (a64v->GetSymbol()->GetStorageClass() == kScPstatic && a64v->GetSymbol()->IsLocal()) {
        PUIdx pIdx = CG::GetCurCGFunc()->GetMirModule().CurFunction()->GetPuidx();
        LogInfo::MapleLogger() << a64v->GetSymbolName() << std::to_string(pIdx);
      } else {
        LogInfo::MapleLogger() << a64v->GetSymbolName();
      }
      LogInfo::MapleLogger() << "+" << std::to_string(offOpnd->GetOffsetValue());
      break;
    }
    default:
      ASSERT(false, "error memoperand dump");
      break;
  }
}

void A64OpndDumpVisitor::Visit(CondOperand *v) {
  LogInfo::MapleLogger() << "CC: " << CondOperand::ccStrs[v->GetCode()];
}
void A64OpndDumpVisitor::Visit(StImmOperand *v) {
  LogInfo::MapleLogger() << v->GetName();
  LogInfo::MapleLogger() << "+offset:" << v->GetOffset();
}
void A64OpndDumpVisitor::Visit(BitShiftOperand *v) {
  BitShiftOperand::ShiftOp shiftOp = v->GetShiftOp();
  uint32 shiftAmount = v->GetShiftAmount();
  LogInfo::MapleLogger() << ((shiftOp == BitShiftOperand::kLSL) ? "LSL: " :
      ((shiftOp == BitShiftOperand::kLSR) ? "LSR: " : "ASR: "));
  LogInfo::MapleLogger() << shiftAmount;
}
void A64OpndDumpVisitor::Visit(ExtendShiftOperand *v) {
  auto dumpExtendShift = [v](const std::string &extendKind)->void {
    LogInfo::MapleLogger() << extendKind;
    if (v->GetShiftAmount() != 0) {
      LogInfo::MapleLogger() << " : " << v->GetShiftAmount();
    }
  };
  switch (v->GetExtendOp()) {
    case ExtendShiftOperand::kUXTB:
      dumpExtendShift("UXTB");
      break;
    case ExtendShiftOperand::kUXTH:
      dumpExtendShift("UXTH");
      break;
    case ExtendShiftOperand::kUXTW:
      dumpExtendShift("UXTW");
      break;
    case ExtendShiftOperand::kUXTX:
      dumpExtendShift("UXTX");
      break;
    case ExtendShiftOperand::kSXTB:
      dumpExtendShift("SXTB");
      break;
    case ExtendShiftOperand::kSXTH:
      dumpExtendShift("SXTH");
      break;
    case ExtendShiftOperand::kSXTW:
      dumpExtendShift("SXTW");
      break;
    case ExtendShiftOperand::kSXTX:
      dumpExtendShift("SXTX");
      break;
    default:
      ASSERT(false, "should not be here");
      break;
  }
}
void A64OpndDumpVisitor::Visit(LabelOperand *v) {
  LogInfo::MapleLogger() << "label:" << v->GetLabelIndex();
}
void A64OpndDumpVisitor::Visit(FuncNameOperand *v) {
  LogInfo::MapleLogger() << "func :" << v->GetName();
}
void A64OpndDumpVisitor::Visit(LogicalShiftLeftOperand *v) {
  LogInfo::MapleLogger() << "LSL: " << v->GetShiftAmount();
}
void A64OpndDumpVisitor::Visit(PhiOperand *v) {
  auto &phiList = v->GetOperands();
  for (auto it = phiList.begin(); it != phiList.end();) {
    Visit(it->second);
    LogInfo::MapleLogger() << " fBB<" << it->first << ">";
    LogInfo::MapleLogger() << (++it == phiList.end() ? "" : " ,");
  }
}
void A64OpndDumpVisitor::Visit(ListOperand *v) {
  auto &opndList = v->GetOperands();
  for (auto it = opndList.begin(); it != opndList.end();) {
    Visit(*it);
    LogInfo::MapleLogger() << (++it == opndList.end() ? "" : " ,");
  }
}
}  /* namespace maplebe */
