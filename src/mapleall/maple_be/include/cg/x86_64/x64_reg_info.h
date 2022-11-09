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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_REG_INFO_H
#define MAPLEBE_INCLUDE_CG_X64_X64_REG_INFO_H

#include "reg_info.h"
#include "x64_isa.h"
#include "x64_abi.h"

namespace maplebe {
static const std::map<regno_t, uint32> x64IntParamsRegIdx =
    {{x64::RAX, 0}, {x64::RDI, 1}, {x64::RSI, 2}, {x64::RDX, 3}, {x64::RCX, 4}, {x64::R8, 5}, {x64::R9, 6}};

class X64RegInfo : public RegisterInfo {
 public:
  X64RegInfo(MapleAllocator &mallocator): RegisterInfo(mallocator) {}

  ~X64RegInfo() override = default;

  void Init() override;
  void Fini() override;
  void SaveCalleeSavedReg(MapleSet<regno_t> savedRegs) override;
  bool IsSpecialReg(regno_t regno) const override;
  bool IsCalleeSavedReg(regno_t regno) const override;
  bool IsYieldPointReg(regno_t regNO) const override;
  bool IsUnconcernedReg(regno_t regNO) const override;
  bool IsUnconcernedReg(const RegOperand &regOpnd) const override;
  bool IsFramePointReg(regno_t regNO) const override {
    return (regNO == x64::RBP);
  }
  bool IsReservedReg(regno_t regNO, bool doMultiPass) const override {
    return false;
  }
  RegOperand *GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag) override;
  Insn *BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
  Insn *BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
  MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t vrNum,
      bool isDest, Insn &insn, regno_t regNum, bool &isOutOfRange) override;
  bool IsGPRegister(regno_t regNO) const override {
    return x64::IsGPRegister(static_cast<x64::X64reg>(regNO));
  }
  /* Those registers can not be overwrite. */
  bool IsUntouchableReg(regno_t regNO) const override{
    return false;
  }
  /* Refactor later: Integrate parameters and return Reg */
  uint32 GetIntRegsParmsNum() override {
    /*Parms: rdi, rsi, rdx, rcx, r8, r9; Ret: rax, rdx */
    return x64::kNumIntParmRegs + 1;
  }
  uint32 GetIntRetRegsNum() override {
    return x64::kNumIntReturnRegs;
  }
  uint32 GetFpRetRegsNum() override {
    return x64::kNumFloatReturnRegs;
  }
  regno_t GetLastParamsIntReg() override {
    return x64::R9;
  }
  uint32 GetNormalUseOperandNum() override {
    return 0;
  }
  regno_t GetIntRetReg(uint32 idx) override {
    CHECK_FATAL(idx < x64::kNumIntReturnRegs, "index out of range in IntRetReg");
    return static_cast<regno_t>(x64::kIntReturnRegs[idx]);
  }
  regno_t GetFpRetReg(uint32 idx) override {
    CHECK_FATAL(idx < x64::kNumFloatReturnRegs, "index out of range in FloatRetReg");
    return static_cast<regno_t>(x64::kFloatReturnRegs[idx]);
  }
  /* phys reg which can be pre-Assignment:
   * INT param regs -- rdi, rsi, rdx, rcx, r8, r9
   * INT return regs -- rdx, rax
   * FP param regs -- xmm0 ~ xmm7
   * FP return regs -- xmm0 ~ xmm1
   */
  bool IsPreAssignedReg(regno_t regNO) const override {
    return x64::IsParamReg(static_cast<x64::X64reg>(regNO)) ||
           regNO == x64::RAX || regNO == x64::RDX;
  }
  uint32 GetIntParamRegIdx(regno_t regNO) const override {
    const std::map<regno_t, uint32>::const_iterator iter = x64IntParamsRegIdx.find(regNO);
    CHECK_FATAL(iter != x64IntParamsRegIdx.end(), "index out of range in IntParamsRegs");
    return iter->second;
  }
  uint32 GetFpParamRegIdx(regno_t regNO) const override {
    return static_cast<uint32>(regNO - x64::V0);
  }
  regno_t GetLastParamsFpReg() override {
    return x64::kRinvalid;
  }
  uint32 GetFloatRegsParmsNum() override {
    return x64::kNumFloatParmRegs;
  }
  uint32 GetFloatRegsRetsNum() {
    return x64::kNumFloatReturnRegs;
  }
  uint32 GetAllRegNum() override {
    return x64::kAllRegNum;
  }
  regno_t GetInvalidReg() override {
    return x64::kRinvalid;
  }
  bool IsAvailableReg(regno_t regNO) const override {
    return x64::IsAvailableReg(static_cast<x64::X64reg>(regNO));
  }
  bool IsVirtualRegister(const RegOperand &regOpnd) override {
    return regOpnd.GetRegisterNumber() > x64::kAllRegNum;
  }
  bool IsVirtualRegister(regno_t regno) override {
    return regno > x64::kAllRegNum;
  }
  regno_t GetReservedSpillReg() override {
    return x64::kRinvalid;
  }
  regno_t GetSecondReservedSpillReg() override {
    return x64::kRinvalid;
  }
  regno_t GetYieldPointReg() const override {
    return x64::kRinvalid;
  }
  regno_t GetStackPointReg() const override {
    return x64::RSP;
  }
  bool IsSpillRegInRA(regno_t regNO, bool has3RegOpnd) override {
    return x64::IsSpillRegInRA(static_cast<x64::X64reg>(regNO), has3RegOpnd);
  }

  regno_t GetIntSpillFillReg(size_t idx) const override {
    static regno_t intRegs[kSpillMemOpndNum] = { 0 };
    ASSERT(idx < kSpillMemOpndNum, "index out of range");
    return intRegs[idx];
  }
  regno_t GetFpSpillFillReg(size_t idx) const override {
    static regno_t fpRegs[kSpillMemOpndNum] = { 0 };
    ASSERT(idx < kSpillMemOpndNum, "index out of range");
    return fpRegs[idx];
  }
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_REG_INFO_H */
