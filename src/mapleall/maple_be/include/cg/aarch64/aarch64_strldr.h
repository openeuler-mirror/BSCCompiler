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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_STRLDR_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_STRLDR_H

#include "strldr.h"
#include "aarch64_reaching.h"
#include "aarch64_operand.h"

namespace maplebe {
using namespace maple;
enum MemPropMode : uint8 {
  kUndef,
  kPropBase,
  kPropOffset,
  kPropSignedExtend,
  kPropUnsignedExtend,
  kPropShift
};

class AArch64StoreLoadOpt : public StoreLoadOpt {
 public:
  AArch64StoreLoadOpt(CGFunc &func, MemPool &memPool)
      : StoreLoadOpt(func, memPool), localAlloc(&memPool), str2MovMap(localAlloc.Adapter()) {}
  ~AArch64StoreLoadOpt() override = default;
  void Run() final;
  void DoStoreLoadOpt();
  void DoLoadZeroToMoveTransfer(const Insn &strInsn, short strSrcIdx,
                                const InsnSet &memUseInsnSet) const;
  void DoLoadToMoveTransfer(Insn &strInsn, short strSrcIdx,
                            short memSeq, const InsnSet &memUseInsnSet);
  bool CheckStoreOpCode(MOperator opCode) const;

 private:
  void StrLdrIndexModeOpt(Insn &currInsn);
  bool CheckReplaceReg(Insn &defInsn, Insn &currInsn, InsnSet &replaceRegDefSet, regno_t replaceRegNo);
  bool CheckDefInsn(Insn &defInsn, Insn &currInsn);
  bool CheckNewMemOffset(const Insn &insn, MemOperand *newMemOpnd, uint32 opndIdx);
  MemOperand *HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal,
                                VaryType varyType = kNotVary);
  MemOperand *SelectReplaceMem(Insn &defInsn, Insn &curInsn, RegOperand &base, Operand *offset);
  MemOperand *SelectReplaceExt(const Insn &defInsn, RegOperand &base, bool isSigned);
  bool CanDoMemProp(const Insn *insn);
  bool CanDoIndexOpt(const MemOperand &currMemOpnd);
  void MemPropInit();
  void SelectPropMode(const MemOperand &currMemOpnd);
  int64 GetOffsetForNewIndex(Insn &defInsn, Insn &insn, regno_t baseRegNO, uint32 memOpndSize) const;
  MemOperand *SelectIndexOptMode(Insn &insn, const MemOperand &curMemOpnd);
  bool ReplaceMemOpnd(Insn &insn, regno_t regNo, RegOperand &base, Operand *offset);
  void MemProp(Insn &insn);
  void ProcessStrPair(Insn &insn);
  void ProcessStr(Insn &insn);
  void GenerateMoveLiveInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd,
                            Insn &ldrInsn, Insn &strInsn, short memSeq);
  void GenerateMoveDeadInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd,
                            Insn &ldrInsn, Insn &strInsn, short memSeq);
  bool HasMemBarrier(const Insn &ldrInsn, const Insn &strInsn) const;
  bool IsAdjacentBB(Insn &defInsn, Insn &curInsn) const;
  MapleAllocator localAlloc;
  /* the max number of mov insn to optimize. */
  static constexpr uint8 kMaxMovNum = 2;
  MapleMap<Insn*, Insn*[kMaxMovNum]> str2MovMap;
  MemPropMode propMode = kUndef;
  uint32 amount = 0;
  uint32 memSize = 0;
  bool removeDefInsn = false;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_STRLDR_H */
