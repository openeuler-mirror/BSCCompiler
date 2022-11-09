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
#ifndef MAPLEBE_INCLUDE_CG_REG_INFO_H
#define MAPLEBE_INCLUDE_CG_REG_INFO_H

#include "isa.h"
#include "insn.h"

namespace maplebe {
constexpr size_t kSpillMemOpndNum = 4;

class RegisterInfo {
 public:
  explicit RegisterInfo(MapleAllocator &mallocator)
      : allIntRegs(mallocator.Adapter()),
        allFpRegs(mallocator.Adapter()),
        allregs(mallocator.Adapter()) {}

  virtual ~RegisterInfo() {
    cgFunc = nullptr;
  }

  virtual void Init() = 0;
  virtual void Fini() = 0;
  const MapleSet<regno_t> &GetAllRegs() const {
    return allregs;
  }
  const MapleSet<regno_t> &GetRegsFromType(RegType regTy) const {
    if (regTy == kRegTyInt) {
      return allIntRegs;
    } else if (regTy == kRegTyFloat) {
      return allFpRegs;
    } else {
      CHECK_FATAL(false, "NIY, unsupported reg type");
    }
  }
  void AddToAllRegs(regno_t regNo) {
    (void)allregs.insert(regNo);
  }
  void AddToIntRegs(regno_t regNo) {
    (void)allIntRegs.insert(regNo);
  }
  void AddToFpRegs(regno_t regNo) {
    (void)allFpRegs.insert(regNo);
  }
  void SetCurrFunction(CGFunc &func) {
    cgFunc = &func;
  }
  CGFunc *GetCurrFunction() const {
    return cgFunc;
  }
  virtual RegOperand *GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag = 0) = 0;
  virtual bool IsGPRegister(regno_t regNO) const = 0;
  virtual bool IsPreAssignedReg(regno_t regNO) const = 0;
  virtual uint32 GetIntParamRegIdx(regno_t regNO) const = 0;
  virtual uint32 GetFpParamRegIdx(regno_t regNO) const = 0;
  virtual bool IsSpecialReg(regno_t regno) const = 0;
  virtual bool IsAvailableReg(regno_t regNO) const = 0;
  virtual bool IsCalleeSavedReg(regno_t regno) const = 0;
  virtual bool IsYieldPointReg(regno_t regNO) const = 0;
  virtual bool IsUntouchableReg(uint32 regNO) const = 0;
  virtual bool IsUnconcernedReg(regno_t regNO) const = 0;
  virtual bool IsUnconcernedReg(const RegOperand &regOpnd) const = 0;
  virtual bool IsVirtualRegister(const RegOperand &regOpnd) = 0;
  virtual bool IsVirtualRegister(regno_t regno) = 0;
  virtual bool IsFramePointReg(regno_t regNO) const = 0;
  virtual bool IsReservedReg(regno_t regNO, bool doMultiPass) const = 0;
  virtual void SaveCalleeSavedReg(MapleSet<regno_t> savedRegs) = 0;
  virtual uint32 GetIntRegsParmsNum() = 0;
  virtual uint32 GetIntRetRegsNum() = 0;
  virtual uint32 GetFpRetRegsNum() = 0;
  virtual uint32 GetFloatRegsParmsNum() = 0;
  virtual regno_t GetLastParamsIntReg() = 0;
  virtual regno_t GetLastParamsFpReg() = 0;
  virtual regno_t GetIntRetReg(uint32 idx)  = 0;
  virtual regno_t GetFpRetReg(uint32 idx)  = 0;
  virtual regno_t GetReservedSpillReg() = 0;
  virtual regno_t GetSecondReservedSpillReg() = 0;
  virtual regno_t GetYieldPointReg() const = 0;
  virtual regno_t GetStackPointReg() const = 0;
  virtual uint32 GetAllRegNum() = 0;
  virtual uint32 GetNormalUseOperandNum() = 0;
  virtual regno_t GetInvalidReg() = 0;
  virtual bool IsSpillRegInRA(regno_t regNO, bool has3RegOpnd) = 0;
  virtual Insn *BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) = 0;
  virtual Insn *BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) = 0;
  virtual MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t vrNum,
      bool isDest, Insn &insn, regno_t regNum, bool &isOutOfRange) = 0;

  // used in color ra
  virtual regno_t GetIntSpillFillReg(size_t idx) const = 0;
  virtual regno_t GetFpSpillFillReg(size_t idx) const = 0;
 private:
  MapleSet<regno_t> allIntRegs;
  MapleSet<regno_t> allFpRegs;
  MapleSet<regno_t> allregs;
  CGFunc *cgFunc = nullptr;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_INFO_H */
