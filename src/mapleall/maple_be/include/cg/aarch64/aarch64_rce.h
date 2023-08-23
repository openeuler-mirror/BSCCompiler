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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_RCE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_RCE_H

#include "cg_rce.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
static std::size_t g_hashSeed = 0;
using InsnPtr = Insn*;
/*
 * Redundancy elimination with the same right value
 * Example:
 *   mov   R140, #31161
 *   movk  R140(use){implicit-def: R141}, #40503, LSL #16
 *   ...
 *   mov   R149, #31161
 *   movk  R149(use){implicit-def: R150}, #40503, LSL #16     ===> redundant
 *
 * 1) do not support vector ops & mem ops currently
 */
class AArch64RedundantComputeElim : public RedundantComputeElim {
 public:
  AArch64RedundantComputeElim(CGFunc &f, CGSSAInfo &info, MemPool &mp)
      : RedundantComputeElim(f, info, mp),
        candidates(rceAlloc.Adapter()) {}
  ~AArch64RedundantComputeElim() override {}

  void Run() override;
  void DumpHash() const;

 private:
  struct InsnRHSHash {
    std::size_t operator()(const InsnPtr &insn) const {
      std::string hashS = (insn->IsVectorOp() ? std::to_string(++g_hashSeed) :
                                                std::to_string(insn->GetMachineOpcode()));
      uint32 opndNum = insn->GetOperandSize();
      hashS += std::to_string(opndNum);
      for (uint i = 0; i < opndNum; ++i) {
        Operand &opnd = insn->GetOperand(i);
        Operand::OperandType opndKind = opnd.GetKind();
        if (opnd.IsRegister() && !static_cast<RegOperand&>(opnd).IsPhysicalRegister() && !insn->OpndIsDef(i)) {
          hashS += static_cast<RegOperand&>(opnd).GetHashContent();
        } else if (opndKind == Operand::kOpdImmediate) {
          hashS += static_cast<ImmOperand&>(opnd).GetHashContent();
        } else if (opndKind == Operand::kOpdExtend) {
          hashS += static_cast<ExtendShiftOperand&>(opnd).GetHashContent();
        } else if (opndKind == Operand::kOpdShift) {
          hashS += static_cast<BitShiftOperand&>(opnd).GetHashContent();
        } else if (opnd.IsRegister() && insn->OpndIsDef(i) && insn->OpndIsUse(i)) {
          hashS += static_cast<RegOperand&>(opnd).GetHashContent();
        } else if (opnd.IsRegister() && insn->OpndIsDef(i)) {
          continue;
        } else {
          hashS += std::to_string(++g_hashSeed);
        }
      }
      return std::hash<std::string>{}(hashS);
    }
  };

  struct InsnRHSEqual {
    bool operator()(const InsnPtr &insn1, const InsnPtr &insn2) const {
      if (insn1->GetMachineOpcode() != insn2->GetMachineOpcode()) {
        return false;
      }
      if (insn1->IsVectorOp()) {
        return false;
      }
      uint32 opndNum1 = insn1->GetOperandSize();
      uint32 opndNum2 = insn2->GetOperandSize();
      if (opndNum1 != opndNum2) {
        return false;
      }
      for (uint32 i = 0; i < opndNum1; ++i) {
        Operand &opnd1 = insn1->GetOperand(i);
        Operand::OperandType opk1 = opnd1.GetKind();
        Operand &opnd2 = insn2->GetOperand(i);
        /* There are cases that the operand is both def and use */
        if (opnd1.IsRegister() && opnd2.IsRegister() && insn1->OpndIsDef(i) && insn2->OpndIsDef(i) &&
            !insn1->OpndIsUse(i) && !insn2->OpndIsUse(i)) {
          continue;
        }
        if (opk1 == Operand::kOpdRegister) {
          if (!static_cast<RegOperand&>(opnd1).Equals(opnd2)) {
            return false;
          }
          if (static_cast<RegOperand&>(opnd1).IsPhysicalRegister() ||
              static_cast<RegOperand&>(opnd2).IsPhysicalRegister()) {
            return false;
          }
        } else if (opk1 == Operand::kOpdImmediate) {
          if (!static_cast<ImmOperand&>(opnd1).Equals(opnd2)) {
            return false;
          }
        } else if (opk1 == Operand::kOpdExtend) {
          if (!static_cast<ExtendShiftOperand&>(opnd1).Equals(opnd2)) {
            return false;
          }
        } else if (opk1 == Operand::kOpdShift) {
          if (!static_cast<BitShiftOperand&>(opnd1).Equals(opnd2)) {
            return false;
          }
        } else {
          return false;
        }
      }
      return true;
    }
  };

  bool DoOpt(BB *bb);
  bool IsBothDefUseCase(VRegVersion &version) const;
  bool CheckFakeOptmization(const Insn &existInsn) const;
  void CheckCondition(const Insn &existInsn, const Insn &curInsn);
  void CheckBothDefAndUseChain(RegOperand *curDstOpnd, RegOperand *existDstOpnd);
  std::size_t ComputeDefUseHash(const Insn &insn, const RegOperand *replaceOpnd) const;
  DUInsnInfo *GetDefUseInsnInfo(VRegVersion &defVersion);
  MOperator GetNewMop(const RegOperand &curDstOpnd, const RegOperand &existDstOpnd) const;
  void Optimize(BB &curBB, Insn &curInsn, RegOperand &curDstOpnd, RegOperand &existDstOpnd) const;
  MapleUnorderedSet<InsnPtr, InsnRHSHash, InsnRHSEqual> candidates;
  bool doOpt = true;
  bool isBothDefUse = false;
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_RCE_H */
