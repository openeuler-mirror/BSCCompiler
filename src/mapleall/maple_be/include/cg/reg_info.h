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

namespace maplebe {

class RegisterInfo {
 public:
  explicit RegisterInfo(MapleAllocator &mallocator)
      : memAllocator(&mallocator),
        allIntRegs(mallocator.Adapter()),
        allFpRegs(mallocator.Adapter()),
        allregs(mallocator.Adapter()) {}

  virtual ~RegisterInfo() {
    memAllocator = nullptr;
    cgFunc = nullptr;
  }

  virtual void Init() = 0;
  virtual void Fini() = 0;
  bool IsUntouchableReg(uint32 regNO) const;
  virtual uint32 GetAllRegNum() = 0;
  virtual uint32 GetInvalidReg() = 0;
  void AddToAllRegs(regno_t regNo) {
    (void)allregs.insert(regNo);
  }
  const MapleSet<regno_t> &GetAllRegs() const {
    return allregs;
  }
  void AddToIntRegs(regno_t regNo) {
    (void)allIntRegs.insert(regNo);
  }
  const MapleSet<regno_t> &GetIntRegs() const {
    return allIntRegs;
  }
  void AddToFpRegs(regno_t regNo) {
    (void)allFpRegs.insert(regNo);
  }
  const MapleSet<regno_t> &GetFpRegs() const {
    return allFpRegs;
  }
  void SetCurrFunction(CGFunc &func) {
    cgFunc = &func;
  }
  CGFunc *GetCurrFunction() const {
    return cgFunc;
  }
  virtual RegOperand &GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag) = 0;
  virtual ListOperand *CreateListOperand() = 0;
  virtual Insn *BuildMovInstruction(Operand &opnd0, Operand &opnd1) = 0;
  virtual bool IsSpecialReg(regno_t regno) const = 0;
  virtual bool IsCalleeSavedReg(regno_t regno) const = 0;
  virtual bool IsYieldPointReg(regno_t regNO) const = 0;
  virtual bool IsUnconcernedReg(regno_t regNO) const = 0;
  virtual bool IsUnconcernedReg(const RegOperand &regOpnd) const = 0;
  virtual void SaveCalleeSavedReg(MapleSet<regno_t> savedRegs) = 0;
  virtual bool IsVirtualRegister(const CGRegOperand &regOpnd) {
    return false;
  }
  virtual bool IsUnconcernedReg(const CGRegOperand &regOpnd) const {
    return false;
  }
 private:
  MapleAllocator *memAllocator;
  MapleSet<regno_t> allIntRegs;
  MapleSet<regno_t> allFpRegs;
  MapleSet<regno_t> allregs;
  CGFunc *cgFunc = nullptr;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_INFO_H */
