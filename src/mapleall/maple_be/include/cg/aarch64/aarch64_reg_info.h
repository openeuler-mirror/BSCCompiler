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

  uint32 GetAllRegNum() override {
    return kAllRegNum;
  };
  uint32 GetInvalidReg() override {
    return kRinvalid;
  };

  void Init() override;
  void Fini() override;
  void SaveCalleeSavedReg(MapleSet<regno_t> savedRegs) override;
  bool IsSpecialReg(regno_t regno) const override;
  bool IsCalleeSavedReg(regno_t regno) const override;
  bool IsYieldPointReg(regno_t regno) const override;
  bool IsUnconcernedReg(regno_t regNO) const override;
  bool IsUnconcernedReg(const RegOperand &regOpnd) const override;
  RegOperand &GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag) override;
  ListOperand *CreateListOperand() override;
  Insn *BuildMovInstruction(Operand &opnd0, Operand &opnd1) override;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REG_INFO_H */
