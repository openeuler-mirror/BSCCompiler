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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REG_INFO_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REG_INFO_H
#include "reg_info.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"
#include "aarch64_abi.h"

namespace maplebe {

class AArch64RegInfo : public RegisterInfo {
 public:
  explicit AArch64RegInfo(MapleAllocator &mallocator) : RegisterInfo(mallocator) {}

  ~AArch64RegInfo() override = default;

  bool IsGPRegister(regno_t regNO) const override {
    return AArch64isa::IsGPRegister(static_cast<AArch64reg>(regNO));
  }
  /* phys reg which can be pre-Assignment */
  bool IsPreAssignedReg(regno_t regNO) const override {
    return AArch64Abi::IsParamReg(static_cast<AArch64reg>(regNO));
  }
  regno_t GetIntRetReg(uint32 idx) override {
    CHECK_FATAL(idx < AArch64Abi::kNumIntParmRegs, "index out of range in IntRetReg");
    return AArch64Abi::kIntReturnRegs[idx];
  }
  regno_t GetFpRetReg(uint32 idx) override {
    CHECK_FATAL(idx < AArch64Abi::kNumFloatParmRegs, "index out of range in FloatRetReg");
    return AArch64Abi::kFloatReturnRegs[idx];
  }
  bool IsAvailableReg(regno_t regNO) const override {
    return AArch64Abi::IsAvailableReg(static_cast<AArch64reg>(regNO));
  }
  /* Those registers can not be overwrite. */
  bool IsUntouchableReg(regno_t regNO) const override{
    if ((regNO == RSP) || (regNO == RFP) || regNO == RZR) {
      return true;
    }
    /* when yieldpoint is enabled, the RYP(x19) can not be used. */
    if (GetCurrFunction()->GetCG()->GenYieldPoint() && (regNO == RYP)) {
      return true;
    }
    return false;
  }
  uint32 GetIntRegsParmsNum() override {
    return AArch64Abi::kNumIntParmRegs;
  }
  uint32 GetFloatRegsParmsNum() override {
    return AArch64Abi::kNumFloatParmRegs;
  }
  uint32 GetIntRetRegsNum() override {
    return AArch64Abi::kNumIntParmRegs;
  }
  uint32 GetFpRetRegsNum() override {
    return AArch64Abi::kNumFloatParmRegs;
  }
  uint32 GetNormalUseOperandNum() override {
    return AArch64Abi::kNormalUseOperandNum;
  }
  uint32 GetIntParamRegIdx(regno_t regNO) const override {
    return static_cast<uint32>(regNO - R0);
  }
  uint32 GetFpParamRegIdx(regno_t regNO) const override {
    return static_cast<uint32>(regNO - V0);
  }
  regno_t GetLastParamsIntReg() override {
    return R7;
  }
  regno_t GetLastParamsFpReg() override {
    return V7;
  }
  uint32 GetAllRegNum() override {
    return kAllRegNum;
  }
  regno_t GetInvalidReg() override {
    return kRinvalid;
  }
  bool IsVirtualRegister(const RegOperand &regOpnd) override {
    return regOpnd.GetRegisterNumber() > kAllRegNum;
  }
  bool IsVirtualRegister(regno_t regno) override {
    return regno > kAllRegNum;
  }
  regno_t GetReservedSpillReg() override {
    return R16;
  }
  regno_t GetSecondReservedSpillReg() override {
    return R17;
  }
  regno_t GetYieldPointReg() const override {
    return RYP;
  }
  regno_t GetStackPointReg() const override {
    return RSP;
  }

  void Init() override;
  void Fini() override;
  void SaveCalleeSavedReg(MapleSet<regno_t> savedRegs) override;
  bool IsSpecialReg(regno_t regno) const override;
  bool IsCalleeSavedReg(regno_t regno) const override;
  bool IsYieldPointReg(regno_t regno) const override;
  bool IsUnconcernedReg(regno_t regNO) const override;
  bool IsUnconcernedReg(const RegOperand &regOpnd) const override;
  bool IsFramePointReg(regno_t regNO) const override {
    return (regNO == RFP);
  }
  bool IsReservedReg(regno_t regNO, bool doMultiPass) const override;
  bool IsSpillRegInRA(regno_t regNO, bool has3RegOpnd) override;
  RegOperand *GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag) override;
  Insn *BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
  Insn *BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
  MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t vrNum,
      bool isDest, Insn &insn, regno_t regNum, bool &isOutOfRange) override;

  regno_t GetIntSpillFillReg(size_t idx) const override {
    static regno_t intRegs[kSpillMemOpndNum] = { R10, R11, R12, R13 };
    ASSERT(idx < kSpillMemOpndNum, "index out of range");
    return intRegs[idx];
  }
  regno_t GetFpSpillFillReg(size_t idx) const override {
    static regno_t fpRegs[kSpillMemOpndNum] = { V16, V17, V18, V19 };
    ASSERT(idx < kSpillMemOpndNum, "index out of range");
    return fpRegs[idx];
  }
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REG_INFO_H */
