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
        regLiveness(alloc.Adapter()),
        rememberRegs(alloc.Adapter()) {
    availRegSet.resize(regInfo->GetAllRegNum());
  }

  ~DefaultO0RegAllocator() override {
    regInfo = nullptr;
  }

  bool AllocateRegisters() override;

  void InitAvailReg();

  bool AllocatePhysicalRegister(const RegOperand &opnd);
  void ReleaseReg(regno_t reg);
  void ReleaseReg(const RegOperand &regOpnd);
  void AllocHandleDestList(Insn &insn, Operand &opnd, uint32 idx);
  void AllocHandleDest(Insn &insn, Operand &opnd, uint32 idx);
  void AllocHandleSrcList(Insn &insn, Operand &opnd, uint32 idx);
  void AllocHandleSrc(Insn &insn, Operand &opnd, uint32 idx);
  void SaveCalleeSavedReg(const RegOperand &regOpnd);

 protected:
  Operand *HandleRegOpnd(Operand &opnd);
  Operand *HandleMemOpnd(Operand &opnd);
  Operand *AllocSrcOpnd(Operand &opnd);
  Operand *AllocDestOpnd(Operand &opnd, const Insn &insn);
  uint32 GetRegLivenessId(regno_t regNo);
  bool CheckRangesOverlap(const std::pair<uint32, uint32> &range1,
                          const MapleVector<std::pair<uint32, uint32>> &ranges2) const;
  void SetupRegLiveness(BB *bb);
  void SetupRegLiveness(MemOperand &opnd, uint32 insnId);
  void SetupRegLiveness(ListOperand &opnd, uint32 insnId, bool isDef);
  void SetupRegLiveness(RegOperand &opnd, uint32 insnId, bool isDef);

  MapleSet<regno_t> calleeSaveUsed;
  MapleVector<bool> availRegSet;
  MapleMap<uint32, regno_t> regMap;     /* virtual-register-to-physical-register map */
  MapleSet<uint8> liveReg;              /* a set of currently live physical registers */
  MapleSet<Operand*> allocatedSet;      /* already allocated */
  MapleMap<regno_t, MapleVector<std::pair<uint32, uint32>>> regLiveness;
  MapleVector<regno_t> rememberRegs;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_ALLOC_BASIC_H */
