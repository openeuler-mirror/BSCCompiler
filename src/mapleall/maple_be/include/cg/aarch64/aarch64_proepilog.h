/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PROEPILOG_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PROEPILOG_H

#include "proepilog.h"
#include "cg.h"
#include "operand.h"
#include "aarch64_cgfunc.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"

namespace maplebe {
class AArch64ProEpilogAnalysis : public ProEpilogAnalysis {
 public:
  AArch64ProEpilogAnalysis(CGFunc &func, MemPool &pool, DomAnalysis &dom, PostDomAnalysis &pdom, LoopAnalysis &loop)
      : ProEpilogAnalysis(func, pool, dom, pdom, loop) {}
  ~AArch64ProEpilogAnalysis() override = default;

  bool NeedProEpilog() override;
};

class AArch64GenProEpilog : public GenProEpilog {
 public:
  AArch64GenProEpilog(CGFunc &func, MemPool &memPool, const ProEpilogSaveInfo *proepiSaveInfo = nullptr)
      : GenProEpilog(func),
        tmpAlloc(&memPool),
        saveInfo(proepiSaveInfo) {
    useFP = func.UseFP();
    if (func.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
      stackBaseReg = RFP;
    } else {
      stackBaseReg = useFP ? R29 : RSP;
    }
    AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc&>(func);
    const MapleVector<AArch64reg> &calleeSavedRegs = aarchCGFunc.GetCalleeSavedRegs();
    if (useFP) {
      storeFP = true;
    } else if (find(calleeSavedRegs.begin(), calleeSavedRegs.end(), RFP) != calleeSavedRegs.end()) {
      storeFP = true;
    } else if (find(calleeSavedRegs.begin(), calleeSavedRegs.end(), R29) != calleeSavedRegs.end()) {
      storeFP = true;
    }
    aarchCGFunc.SetStoreFP(storeFP);
  }

  ~AArch64GenProEpilog() override = default;

  bool NeedProEpilog() override;
  static MemOperand *SplitStpLdpOffsetForCalleeSavedWithAddInstruction(
      CGFunc &cgFunc, const MemOperand &mo, uint32 bitLen, AArch64reg baseRegNum = AArch64reg::kRinvalid);
  static void AppendInstructionPushPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset);
  static void AppendInstructionPushSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset);
  static void AppendInstructionPopSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset);
  static void AppendInstructionPopPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset);
  void Run() override;
 private:
  AArch64reg GetStackGuardRegister(const BB &bb) const;
  std::pair<AArch64reg, AArch64reg> GetStackGuardCheckRegister(const BB &bb) const;
  MemOperand *GetDownStack();
  RegOperand &GenStackGuard(AArch64reg regNO);
  void AddStackGuard(BB &bb);
  void GenStackGuardCheckInsn(BB &bb);
  BB &GetOrGenStackGuardCheckFailBB(BB &bb);
  void AppendInstructionAllocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty);
  void AppendInstructionAllocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty);
  void GeneratePushRegs();
  void GeneratePushUnnamedVarargRegs();
  void AppendInstructionStackCheck(AArch64reg reg, RegType rty, int32 offset);
  void GenerateProlog(BB &bb);

  void GenerateRet(BB &bb);
  bool TestPredsOfRetBB(const BB &exitBB);
  void AppendInstructionDeallocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty);
  void AppendInstructionDeallocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty);
  void GeneratePopRegs();
  void AppendJump(const MIRSymbol &funcSymbol);
  void GenerateEpilog(BB &bb);
  void GenerateEpilogForCleanup(BB &bb);
  void AppendBBtoEpilog(BB &epilogBB, BB &newBB);

  Insn &CreateAndAppendInstructionForAllocateCallFrame(int64 argsToStkPassSize, AArch64reg reg0, AArch64reg reg1,
                                                       RegType rty);
  Insn &AppendInstructionForAllocateOrDeallocateCallFrame(int64 argsToStkPassSize, AArch64reg reg0, AArch64reg reg1,
                                                          RegType rty, bool isAllocate);

  MapleAllocator tmpAlloc;
  const ProEpilogSaveInfo *saveInfo = nullptr;
  static constexpr const int32 kOffset8MemPos = 8;
  static constexpr const int32 kOffset16MemPos = 16;

  bool useFP = false;
  // To be compatible with previous code more easily，we use storeFP boolean to indicate the case
  // (1) use FP to address
  // (2) FP is clobbered
  // need to delete this and optimize the callee save process.
  bool storeFP = false;
  /* frame pointer(x29) is available as a general-purpose register if useFP is set as false */
  AArch64reg stackBaseReg = RFP;
  BB *stackChkFailBB = nullptr;     // only one stack check fail BB is need
};
}  /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PROEPILOG_H */
