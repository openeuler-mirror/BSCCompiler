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
  void DoLoadZeroToMoveTransfer(const Insn&, short, const InsnSet&) const;
  void DoLoadToMoveTransfer(Insn&, short, short, const InsnSet&);
  bool CheckStoreOpCode(MOperator opCode) const;
  static bool CheckNewAmount(Insn &insn, uint32 newAmount);

 private:
  void StrLdrIndexModeOpt(Insn &currInsn);
  bool CheckReplaceReg(Insn &defInsn, Insn &currInsn, InsnSet &replaceRegDefSet, regno_t replaceRegNo);
  bool CheckDefInsn(Insn &defInsn, Insn &currInsn);
  bool CheckNewMemOffset(Insn &insn, AArch64MemOperand *newMemOpnd, uint32 opndIdx);
  AArch64MemOperand *HandleArithImmDef(AArch64RegOperand &replace, Operand *oldOffset, int64 defVal);
  AArch64MemOperand *SelectReplaceMem(Insn &defInsn, Insn &curInsn, RegOperand &base, Operand *offset);
  AArch64MemOperand *SelectReplaceExt(Insn &defInsn, RegOperand &base, bool isSigned);
  bool CanDoMemProp(Insn *insn);
  bool CanDoIndexOpt(AArch64MemOperand &MemOpnd);
  void MemPropInit();
  void SelectPropMode(AArch64MemOperand &currMemOpnd);
  int64 GetOffsetForNewIndex(Insn &defInsn, Insn &insn, regno_t baseRegNO, uint32 memOpndSize);
  AArch64MemOperand *SelectIndexOptMode(Insn &insn, AArch64MemOperand &curMemOpnd);
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
  bool removeDefInsn = false;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_STRLDR_H */