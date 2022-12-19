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

#include "insn.h"
#include "isa.h"
#include "cg.h"
namespace maplebe {
bool Insn::IsMachineInstruction() const {
  return md && md->IsPhysicalInsn() && Globals::GetInstance()->GetTarget()->IsTargetInsn(mOp);
}
/* phi is not physical insn */
bool Insn::IsPhi() const {
  return md ? md->IsPhi() : false;
}
bool Insn::IsLoad() const {
  return md ? md->IsLoad() : false;
}
bool Insn::IsStore() const {
  return md ? md->IsStore() : false;
}
bool Insn::IsMove() const {
  return md ? md->IsMove() : false;
}
bool Insn::IsBranch() const {
  return md ? md->IsBranch() : false;
}
bool Insn::IsCondBranch() const {
  return md ? md->IsCondBranch() : false;
}
bool Insn::IsUnCondBranch() const {
  return md ? md->IsUnCondBranch() : false;
}
bool Insn::IsBasicOp() const {
  return md ? md->IsBasicOp() : false;
}
bool Insn::IsConversion() const {
  return md? md->IsConversion() : false;
}
bool Insn::IsUnaryOp() const {
  return md ? md->IsUnaryOp() : false;
}
bool Insn::IsShift() const {
  return md ? md->IsShift() : false;
}
bool Insn::IsCall() const {
  return md ? md->IsCall() : false;
}
bool Insn::IsTailCall() const {
  return md ? md->IsTailCall() : false;
}
bool Insn::IsAsmInsn() const {
  return md ? md->IsInlineAsm() : false;
}
bool Insn::IsDMBInsn() const {
  return md ? md->IsDMB() : false;
}
bool Insn::IsAtomic() const {
  return md ? md->IsAtomic() : false;
}
bool Insn::IsVolatile() const {
  return md ? md->IsVolatile() : false;
}
bool Insn::IsMemAccessBar() const {
  return md ? md->IsMemAccessBar() : false;
}
bool Insn::IsMemAccess() const {
  return md ? md->IsMemAccess() : false;
}
bool Insn::CanThrow() const {
  return md ? md->CanThrow() : false;
}
bool Insn::IsVectorOp() const {
  return md ? md->IsVectorOp() : false;
}
bool Insn::HasLoop() const {
  return md ? md->HasLoop() : false;
}
uint32 Insn::GetLatencyType() const {
  return md ? md->GetLatencyType() : false;
}
uint32 Insn::GetAtomicNum() const {
  return md ? md->GetAtomicNum() : false;
}
bool Insn::IsSpecialIntrinsic() const {
  return md ? md->IsSpecialIntrinsic() : false;
}
bool Insn::IsLoadPair() const {
  return md ? md->IsLoadPair() : false;
}
bool Insn::IsStorePair() const {
  return md ? md->IsStorePair() : false;
}
bool Insn::IsLoadStorePair() const {
  return md ? md->IsLoadStorePair() : false;
}
bool Insn::IsLoadLabel() const {
  return md && md->IsLoad() && GetOperand(kInsnSecondOpnd).GetKind() == Operand::kOpdBBAddress;
}
bool Insn::OpndIsDef(uint32 id) const {
  return md ? md->GetOpndDes(id)->IsDef() : false;
}
bool Insn::OpndIsUse(uint32 id) const {
  return md ? md->GetOpndDes(id)->IsUse() : false;
}
bool Insn::IsClinit() const {
  return Globals::GetInstance()->GetTarget()->IsClinitInsn(mOp);
}
bool Insn::IsComment() const {
  return mOp == abstract::MOP_comment && !md->IsPhysicalInsn();
}

bool Insn::IsImmaterialInsn() const {
  return IsComment();
}
Operand *Insn::GetMemOpnd() const {
  for (uint32 i = 0; i < opnds.size(); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      return &opnd;
    }
  }
  return nullptr;
}
void Insn::SetMemOpnd(MemOperand *memOpnd) {
  for (uint32 i = 0; i < static_cast<uint32>(opnds.size()); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      SetOperand(i, *memOpnd);
      return;
    }
  }
}

bool Insn::IsRegDefined(regno_t regNO) const {
  return GetDefRegs().count(regNO);
}

std::set<uint32> Insn::GetDefRegs() const {
  std::set<uint32> defRegNOs;
  size_t opndNum = opnds.size();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = GetOperand(i);
    auto *regProp = md->opndMD[i];
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
        if (memOpnd.GetAddrMode() == MemOperand::kBOI &&
            (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
          ASSERT(!defRegNOs.count(base->GetRegisterNumber()), "duplicate def in one insn");
          defRegNOs.emplace(base->GetRegisterNumber());
        }
      }
    } else if (opnd.IsConditionCode() || opnd.IsRegister()) {
      defRegNOs.emplace(static_cast<RegOperand&>(opnd).GetRegisterNumber());
    }
  }
  return defRegNOs;
}

#if DEBUG
void Insn::Check() const {
  if (!md) {
    CHECK_FATAL(false, " need machine description for target insn ");
  }
  for (uint32 i = 0; i < GetOperandSize(); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.GetKind() != md->GetOpndDes(i)->GetOperandType()) {
      CHECK_FATAL(false, " operand type does not match machine description ");
    }
  }
}
#endif

Insn *Insn::Clone(const MemPool &memPool) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *Insn::GetCallTargetOperand() const {
  ASSERT(IsCall() || IsTailCall(), "should be call");
  return &GetOperand(kInsnFirstOpnd);
}

ListOperand *Insn::GetCallArgumentOperand() {
  ASSERT(IsCall(), "should be call");
  ASSERT(GetOperand(1).IsList(), "should be list");
  return &static_cast<ListOperand&>(GetOperand(kInsnSecondOpnd));
}


void Insn::CommuteOperands(uint32 dIndex, uint32 sIndex) {
  Operand *tempCopy = opnds[sIndex];
  opnds[sIndex] = opnds[dIndex];
  opnds[dIndex] = tempCopy;
}

uint32 Insn::GetBothDefUseOpnd() const {
  size_t opndNum = opnds.size();
  uint32 opndIdx = kInsnMaxOpnd;
  if (md->GetAtomicNum() > 1) {
    return opndIdx;
  }
  for (uint32 i = 0; i < opndNum; ++i) {
    auto *opndProp = md->GetOpndDes(i);
    if (opndProp->IsRegUse() && opndProp->IsDef()) {
      opndIdx = i;
    }
    if (opnds[i]->IsMemoryAccessOperand()) {
      auto *MemOpnd = static_cast<MemOperand*>(opnds[i]);
      if (!MemOpnd->IsIntactIndexed()) {
        opndIdx = i;
      }
    }
  }
  return opndIdx;
}

uint32 Insn::GetMemoryByteSize() const {
  ASSERT(IsMemAccess(), "must be memory access insn");
  uint32 res = 0;
  for (size_t i = 0 ; i < opnds.size(); ++i) {
    if (md->GetOpndDes(i)->GetOperandType() == Operand::kOpdMem) {
      res = md->GetOpndDes(i)->GetSize();
    }
  }
  ASSERT(res, "cannot access empty memory");
  if (IsLoadStorePair()) {
    res = res << 1;
  }
  res = res >> 3;
  return res;
}

bool Insn::ScanReg(regno_t regNO) const {
  uint32 opndNum = GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto listElem : listOpnd.GetOperands()) {
        auto *regOpnd = static_cast<RegOperand*>(listElem);
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
    } else if (opnd.IsRegister()) {
      if (static_cast<RegOperand&>(opnd).GetRegisterNumber() == regNO) {
        return true;
      }
    }
  }
  return false;
}

bool Insn::MayThrow() const {
  if (md->IsMemAccess() && !IsLoadLabel()) {
    auto *memOpnd = static_cast<MemOperand*>(GetMemOpnd());
    ASSERT(memOpnd != nullptr, "CG invalid memory operand.");
    if (memOpnd->IsStackMem()) {
      return false;
    }
  }
  return md->CanThrow();
}

void Insn::SetMOP(const InsnDesc &idesc) {
  mOp = idesc.GetOpc();
  md = &idesc;
}

void Insn::Dump() const {
  ASSERT(md != nullptr, "md should not be nullptr");
  LogInfo::MapleLogger() << "< " << GetId() << " > ";
  LogInfo::MapleLogger() << md->name << "(" << mOp << ")";

  for (uint32 i = 0; i < GetOperandSize(); ++i) {
    Operand &opnd = GetOperand(i);
    LogInfo::MapleLogger() << " (opnd" << i << ": ";
    Globals::GetInstance()->GetTarget()->DumpTargetOperand(opnd, *md->GetOpndDes(i));
    LogInfo::MapleLogger() << ")";
  }

  if (ssaImplicitDefOpnd != nullptr) {
    LogInfo::MapleLogger() << " (implicitDefOpnd: ";
    std::array<const std::string, kRegTyLast> prims = { "U", "R", "V", "C", "X", "Vra" };
    uint32 regType = ssaImplicitDefOpnd->GetRegisterType();
    ASSERT(regType < kRegTyLast, "unexpected regType");
    LogInfo::MapleLogger() << (ssaImplicitDefOpnd->IsVirtualRegister() ? "vreg:" : " reg:") << prims[regType];
    regno_t reg = ssaImplicitDefOpnd->GetRegisterNumber();
    reg = ssaImplicitDefOpnd->IsVirtualRegister() ? reg : (reg - 1);
    LogInfo::MapleLogger() << reg;
    uint32 vb = ssaImplicitDefOpnd->GetValidBitsNum();
    if (ssaImplicitDefOpnd->GetValidBitsNum() != ssaImplicitDefOpnd->GetSize()) {
      LogInfo::MapleLogger() << " Vb: [" << vb << "]";
    }
    LogInfo::MapleLogger() << " Sz: [" << ssaImplicitDefOpnd->GetSize() << "]" ;
    LogInfo::MapleLogger() << ")";
  }

  if (IsVectorOp()) {
    auto *vInsn = static_cast<const VectorInsn*>(this);
    if (vInsn->GetNumOfRegSpec() != 0) {
      LogInfo::MapleLogger() << " (vecSpec: " << vInsn->GetNumOfRegSpec() << ")";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

VectorRegSpec *VectorInsn::GetAndRemoveRegSpecFromList() {
  if (regSpecList.size() == 0) {
    VectorRegSpec *vecSpec = CG::GetCurCGFuncNoConst()->GetMemoryPool()->New<VectorRegSpec>() ;
    return vecSpec;
  }
  VectorRegSpec *ret = regSpecList.back();
  regSpecList.pop_back();
  return ret;
}
}
