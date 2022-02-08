/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REACHING_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REACHING_H

#include "reaching.h"
#include "aarch64_operand.h"

namespace maplebe {
class AArch64ReachingDefinition : public ReachingDefinition {
 public:
  AArch64ReachingDefinition(CGFunc &func, MemPool &memPool) : ReachingDefinition(func, memPool) {}
  ~AArch64ReachingDefinition() override = default;
  std::vector<Insn*> FindRegDefBetweenInsn(uint32 regNO, Insn *startInsn, Insn *endInsn,
                                           bool findAll = false) const final;
  std::vector<Insn*> FindRegDefBetweenInsnGlobal(uint32 regNO, Insn *startInsn, Insn *endInsn) const final;
  std::vector<Insn*> FindMemDefBetweenInsn(uint32 offset, const Insn *startInsn, Insn *endInsn) const final;
  bool FindRegUseBetweenInsn(uint32 regNO, Insn *startInsn, Insn *endInsn, InsnSet &useInsnSet) const final;
  bool FindRegUseBetweenInsnGlobal(uint32 regNO, Insn *startInsn, Insn *endInsn, BB* movBB) const final;
  bool FindMemUseBetweenInsn(uint32 offset, Insn *startInsn, const Insn *endInsn,
                             InsnSet &useInsnSet) const final;
  bool HasRegDefBetweenInsnGlobal(uint32 regNO, Insn &startInsn, Insn &endInsn);
  bool DFSFindRegDefBetweenBB(const BB &startBB, const BB &endBB, uint32 regNO,
                              std::vector<VisitStatus> &visitedBB) const;
  InsnSet FindDefForRegOpnd(Insn &insn, uint32 indexOrRegNO, bool isRegNO = false) const final;
  InsnSet FindDefForMemOpnd(Insn &insn, uint32 indexOrOffset, bool isOffset = false) const final;
  InsnSet FindUseForMemOpnd(Insn &insn, uint8 index, bool secondMem = false) const final;
  bool FindRegUsingBetweenInsn(uint32 regNO, Insn *startInsn, const Insn *endInsn) const;

 protected:
  void InitStartGen() final;
  void InitEhDefine(BB &bb) final;
  void InitGenUse(BB &bb, bool firstTime = true) final;
  void GenAllAsmDefRegs(BB &bb, Insn &insn, uint32 index) final;
  void GenAllAsmUseRegs(BB &bb, Insn &insn, uint32 index) final;
  void GenAllCallerSavedRegs(BB &bb, Insn &insn) final;
  bool IsRegKilledByCallInsn(const Insn &insn, regno_t regNO) const;
  bool KilledByCallBetweenInsnInSameBB(const Insn &startInsn, const Insn &endInsn, regno_t regNO) const final;
  void AddRetPseudoInsn(BB &bb) final;
  void AddRetPseudoInsns() final;
  bool IsCallerSavedReg(uint32 regNO) const final;
  void FindRegDefInBB(uint32 regNO, BB &bb, InsnSet &defInsnSet) const final;
  void FindMemDefInBB(uint32 offset, BB &bb, InsnSet &defInsnSet) const final;
  void DFSFindDefForRegOpnd(const BB &startBB, uint32 regNO, std::vector<VisitStatus> &visitedBB,
                            InsnSet &defInsnSet) const final;
  void DFSFindDefForMemOpnd(const BB &startBB, uint32 offset, std::vector<VisitStatus> &visitedBB,
                            InsnSet &defInsnSet) const final;
  int32 GetStackSize() const final;

 private:
  void InitInfoForMemOperand(Insn &insn, Operand &opnd, bool isDef);
  void InitInfoForListOpnd(const BB &bb, Operand &opnd);
  void InitInfoForConditionCode(const BB &bb);
  void InitInfoForRegOpnd(const BB &bb, Operand &opnd, bool isDef);
  void InitMemInfoForClearStackCall(Insn &callInsn);
  inline bool CallInsnClearDesignateStackRef(const Insn &callInsn, int64 offset) const;
  int64 GetEachMemSizeOfPair(MOperator opCode) const;
  bool DFSFindRegInfoBetweenBB(const BB startBB, const BB &endBB, uint32 regNO, std::vector<VisitStatus> &visitedBB,
                               std::list<bool> &pathStatus, DumpType infoType) const;
  bool DFSFindRegDomianBetweenBB(const BB startBB, uint32 regNO, std::vector<VisitStatus> &visitedBB) const;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REACHING_H */
