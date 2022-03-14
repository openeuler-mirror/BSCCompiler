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
#include "x64_abi.h"

namespace maplebe {

class X64RegInfo : public RegisterInfo {
 public:
  X64RegInfo(MapleAllocator &mallocator): RegisterInfo(mallocator) {
  }

  ~X64RegInfo() override = default;

  uint32 GetAllRegNum() override {
    return x64::kAllRegNum;
  };
  uint32 GetInvalidReg() override {
    return x64::kRinvalid;
  };
  bool IsVirtualRegister(const CGRegOperand &regOpnd) override {
    return regOpnd.GetRegisterNumber() > x64::kAllRegNum;
  }

  void Init() override;
  void Fini() override;
  void SaveCalleeSavedReg(MapleSet<regno_t> savedRegs) override;
  bool IsSpecialReg(regno_t regno) const override;
  bool IsCalleeSavedReg(regno_t regno) const override;
  bool IsYieldPointReg(regno_t regNO) const override;
  bool IsUnconcernedReg(regno_t regNO) const override;
  bool IsUnconcernedReg(const RegOperand &regOpnd) const override {
    return false;
  }
  bool IsUnconcernedReg(const CGRegOperand &regOpnd) const override;
  RegOperand &GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag = 0) override;
  ListOperand *CreateListOperand() override;
  Insn *BuildMovInstruction(Operand &opnd0, Operand &opnd1) override;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_REG_INFO_H */