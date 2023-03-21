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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EMITTER_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EMITTER_H

#include "asm_emit.h"

namespace maplebe {
using namespace maple;

class AArch64AsmEmitter : public AsmEmitter {
 public:
  AArch64AsmEmitter(CG &cg, const std::string &asmFileName) : AsmEmitter(cg, asmFileName) {}
  ~AArch64AsmEmitter() override = default;

  void EmitRefToMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) override;
  void EmitRefToMethodInfo(FuncEmitInfo &funcEmitInfo, Emitter &emitter) override;
  void EmitMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) override;
  void EmitFastLSDA(FuncEmitInfo &funcEmitInfo) override;
  void EmitFullLSDA(FuncEmitInfo &funcEmitInfo) override;
  void EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) override;
  void EmitJavaInsnAddr(FuncEmitInfo &funcEmitInfo) override;
  void RecordRegInfo(FuncEmitInfo &funcEmitInfo) const;
  void Run(FuncEmitInfo &funcEmitInfo) override;

 private:
  /* cfi & dbg need target info ? */
  void EmitAArch64CfiInsn(Emitter &emitter, const Insn &insn) const;
  void EmitAArch64DbgInsn(FuncEmitInfo &funcEmitInfo, Emitter &emitter, const Insn &insn) const;

  void EmitAArch64Insn(Emitter &emitter, Insn &insn) const;
  void EmitClinit(Emitter &emitter, const Insn &insn) const;
  void EmitAdrpLdr(Emitter &emitter, const Insn &insn) const;
  void EmitCounter(Emitter &emitter, const Insn &insn) const;
  void EmitCCounter(Emitter &emitter, const Insn &insn) const;
  void EmitInlineAsm(Emitter &emitter, const Insn &insn) const;
  void EmitClinitTail(Emitter &emitter, const Insn &insn) const;
  void EmitLazyLoad(Emitter &emitter, const Insn &insn) const;
  void EmitAdrpLabel(Emitter &emitter, const Insn &insn) const;
  void EmitLazyLoadStatic(Emitter &emitter, const Insn &insn) const;
  void EmitArrayClassCacheLoad(Emitter &emitter, const Insn &insn) const;
  void EmitGetAndAddInt(Emitter &emitter, const Insn &insn) const;
  void EmitGetAndSetInt(Emitter &emitter, const Insn &insn) const;
  void EmitCompareAndSwapInt(Emitter &emitter, const Insn &insn) const;
  void EmitStringIndexOf(Emitter &emitter, const Insn &insn) const;
  void EmitLazyBindingRoutine(Emitter &emitter, const Insn &insn) const;
  void EmitCheckThrowPendingException(Emitter &emitter) const;
  void EmitCTlsDescRel(Emitter &emitter, const Insn &insn) const;  // emit instrinsic for local-exec TLS model
  void EmitCTlsDescCall(Emitter &emitter, const Insn &insn) const; // emit instrinsic for general dynamic TLS mode
  void EmitCTlsDescGot(Emitter &emitter, const Insn &insn) const;  // emit instrinsic for initial-exec TLS model
  void EmitCTlsLoadTdata(Emitter &emitter, const Insn &insn) const;     // emit instrinsic for warmup-dynamic TLS model
  void EmitCTlsLoadTbss(Emitter &emitter, const Insn &insn) const;     // emit instrinsic for warmup-dynamic TLS model
  void EmitSyncLockTestSet(Emitter &emitter, const Insn &insn) const;

  void PrepareVectorOperand(RegOperand *regOpnd, uint32 &compositeOpnds, Insn &insn) const;
  bool CheckInsnRefField(const Insn &insn, uint32 opndIndex) const;

  void EmitCallWithLocalAlias(Emitter &emitter, FuncNameOperand &func, const std::string &mdName) const;
};
}  /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EMITTER_H */
