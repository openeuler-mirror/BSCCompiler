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
#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_BASIC_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_BASIC_H
#include "reg_alloc.h"
#include "operand.h"
#include "cgfunc.h"

namespace maplebe {
class DefaultO0RegAllocator : public RegAllocator {
 public:
  DefaultO0RegAllocator(CGFunc &cgFunc, MemPool &memPool)
      : RegAllocator(cgFunc, memPool),
        calleeSaveUsed(alloc.Adapter()),
        availRegSet(alloc.Adapter()),
        regMap(std::less<uint32>(), alloc.Adapter()),
        liveReg(std::less<uint8>(), alloc.Adapter()),
        allocatedSet(std::less<Operand*>(), alloc.Adapter()),
        regLiveness(std::less<Operand*>(), alloc.Adapter()),
        rememberRegs(alloc.Adapter()) {
    regInfo = cgFunc.GetTargetRegInfo();
    availRegSet.resize(regInfo->GetAllRegNum(), false);
  }

  ~DefaultO0RegAllocator() override = default;

  bool AllocateRegisters() override;

  void InitAvailReg();

#ifdef TARGX86_64
  bool AllocatePhysicalRegister(const CGRegOperand &opnd);
#else
  bool AllocatePhysicalRegister(const RegOperand &opnd);
#endif
  void ReleaseReg(regno_t reg);
#ifdef TARGX86_64
  void ReleaseReg(const CGRegOperand &regOpnd);
#else
  void ReleaseReg(const RegOperand &regOpnd);
#endif
  void GetPhysicalRegisterBank(RegType regType, uint8 &start, uint8 &end) const;
  void AllocHandleDestList(Insn &insn, Operand &opnd, uint32 idx);
  void AllocHandleDest(Insn &insn, Operand &opnd, uint32 idx);
  void AllocHandleSrcList(Insn &insn, Operand &opnd, uint32 idx);
  void AllocHandleSrc(Insn &insn, Operand &opnd, uint32 idx);
  void AllocHandleCallee(Insn &insn);
  bool IsSpecialReg(regno_t reg) const;
#ifdef TARGX86_64
  void SaveCalleeSavedReg(const CGRegOperand &opnd);
#else
  void SaveCalleeSavedReg(const RegOperand &opnd);
#endif

 protected:
  Operand *HandleRegOpnd(Operand &opnd);
  Operand *HandleMemOpnd(Operand &opnd);
  Operand *AllocSrcOpnd(Operand &opnd);
  Operand *AllocDestOpnd(Operand &opnd, const Insn &insn);

  uint32 GetRegLivenessId(Operand *opnd);
  void SetupRegLiveness(BB *bb);

  RegisterInfo *regInfo = nullptr;
  MapleSet<regno_t> calleeSaveUsed;
  MapleVector<uint32_t> availRegSet;
  MapleMap<uint32, regno_t> regMap;  /* virtual-register-to-physical-register map */
  MapleSet<uint8> liveReg;              /* a set of currently live physical registers */
  MapleSet<Operand*> allocatedSet;      /* already allocated */
  MapleMap<Operand*, uint32> regLiveness;
  MapleVector<regno_t> rememberRegs;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_ALLOC_BASIC_H */
