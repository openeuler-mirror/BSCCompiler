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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H

#include "aarch64_isa.h"
#include "insn.h"
#include "string_utils.h"
#include "aarch64_operand.h"
#include "common_utils.h"
namespace maplebe {
class AArch64Insn : public Insn {
 public:
  AArch64Insn(MemPool &memPool, MOperator mOp) : Insn(memPool, mOp) {}

  AArch64Insn(const AArch64Insn &originalInsn, MemPool &memPool) : Insn(memPool, originalInsn.mOp) {
    InitWithOriginalInsn(originalInsn, *CG::GetCurCGFuncNoConst()->GetMemoryPool());
  }

  ~AArch64Insn() override = default;

  AArch64Insn &operator=(const AArch64Insn &p) = default;

  bool IsReturn() const override {
    return mOp == MOP_xret;
  }

  bool IsFixedInsn() const override {
    return mOp == MOP_clinit || mOp == MOP_clinit_tail;
  }

  bool IsComment() const override {
    return mOp == MOP_comment;
  }

  bool IsGoto() const override {
    return mOp == MOP_xuncond;
  }

  bool IsImmaterialInsn() const override {
    return IsComment();
  }

  bool IsMachineInstruction() const override {
    return (mOp > MOP_undef && mOp < MOP_comment);
  }

  bool IsPseudoInstruction() const override {
    return (mOp >= MOP_pseudo_param_def_x && mOp <= MOP_pseudo_eh_def_x);
  }

  bool IsReturnPseudoInstruction() const override {
    return (mOp == MOP_pseudo_ret_int || mOp == MOP_pseudo_ret_float);
  }

  bool OpndIsDef(uint32 id) const override;
  bool OpndIsUse(uint32 id) const override;
  bool IsEffectiveCopy() const override {
    return CopyOperands() >= 0;
  }

  uint32 GetResultNum() const override;
  uint32 GetOpndNum() const override;
  Operand *GetResult(uint32 i) const override;
  Operand *GetOpnd(uint32 i) const override;
  Operand *GetMemOpnd() const override;
  void SetMemOpnd(MemOperand *memOpnd) override;
  void SetOpnd(uint32 i, Operand &opnd) override;
  void SetResult(uint32 index, Operand &res) override;
  int32 CopyOperands() const override;
  bool IsGlobal() const final {
    return (mOp == MOP_xadrp || mOp == MOP_xadrpl12);
  }

  bool IsDecoupleStaticOp() const final {
    if (mOp == MOP_lazy_ldr_static) {
      Operand *opnd1 = opnds[1];
      CHECK_FATAL(opnd1 != nullptr, "opnd1 is null!");
      auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
      return StringUtils::StartsWith(stImmOpnd->GetName(), namemangler::kDecoupleStaticValueStr);
    }
    return false;
  }

  bool IsCall() const final;
  bool IsAsmInsn() const final;
  bool IsTailCall() const final;
  bool IsClinit() const final;
  bool IsLazyLoad() const final;
  bool IsAdrpLdr() const final;
  bool IsArrayClassCache() const final;
  bool HasLoop() const final;
  bool IsSpecialIntrinsic() const final;
  bool CanThrow() const final;
  bool IsIndirectCall() const final {
    return mOp == MOP_xblr;
  }

  bool IsCallToFunctionThatNeverReturns() final;
  bool MayThrow() final;
  bool IsBranch() const final;
  bool IsCondBranch() const final;
  bool IsUnCondBranch() const final;
  bool IsMove() const final;
  bool IsMoveRegReg() const final;
  bool IsPhi() const final;
  bool IsLoad() const final;
  bool IsLoadLabel() const final;
  bool IsLoadStorePair() const final;
  bool IsStore() const final;
  bool IsLoadPair() const final;
  bool IsStorePair() const final;
  bool IsLoadAddress() const final;
  bool IsAtomic() const final;
  bool IsYieldPoint() const override;
  bool IsVolatile() const override;
  bool IsFallthruCall() const final {
    return (mOp == MOP_xblr || mOp == MOP_xbl);
  }
  bool IsMemAccessBar() const override;
  bool IsMemAccess() const override;
  bool IsVectorOp() const final;

  Operand *GetCallTargetOperand() const override {
    ASSERT(IsCall() || IsTailCall(), "should be call");
    return &GetOperand(0);
  }
  uint32 GetAtomicNum() const override;
  ListOperand *GetCallArgumentOperand() override {
    ASSERT(IsCall(), "should be call");
    ASSERT(GetOperand(1).IsList(), "should be list");
    return &static_cast<ListOperand&>(GetOperand(1));
  }

  bool IsTargetInsn() const override {
    return true;
  }

  bool IsDMBInsn() const override;

  void PrepareVectorOperand(AArch64RegOperand *regOpnd, uint32 &compositeOpnds) const;

  void Emit(const CG&, Emitter&) const override;

  void Dump() const override;

  bool Check() const override;

  bool IsDefinition() const override;

  bool IsDestRegAlsoSrcReg() const override;

  bool IsPartDef() const override;

  uint32 GetLatencyType() const override;

  uint32 GetJumpTargetIdx() const override;

  uint32 GetJumpTargetIdxFromMOp(MOperator mOp) const override;

  MOperator FlipConditionOp(MOperator flippedOp, uint32 &targetIdx) override;

  bool CheckRefField(size_t opndIndex, bool isEmit) const;

  uint8 GetLoadStoreSize() const;

  bool IsRegDefined(regno_t regNO) const override;

  std::set<uint32> GetDefRegs() const override;

  uint32 GetBothDefUseOpnd() const override;

  bool IsRegDefOrUse(regno_t regNO) const;

 private:
  void CheckOpnd(const Operand &opnd, const OpndProp &mopd) const;
  void EmitClinit(const CG&, Emitter&) const;
  void EmitAdrpLdr(const CG&, Emitter&) const;
  void EmitLazyBindingRoutine(Emitter&) const;
  void EmitClinitTail(Emitter&) const;
  void EmitAdrpLabel(Emitter&) const;
  void EmitLazyLoad(Emitter&) const;
  void EmitLazyLoadStatic(Emitter&) const;
  void EmitArrayClassCacheLoad(Emitter&) const;
  void EmitCheckThrowPendingException(const CG&, Emitter&) const;
  void EmitGetAndAddInt(Emitter &emitter) const;
  void EmitGetAndSetInt(Emitter &emitter) const;
  void EmitCompareAndSwapInt(Emitter &emitter) const;
  void EmitStringIndexOf(Emitter &emitter) const;
  void EmitCounter(const CG&, Emitter&) const;
  void EmitInlineAsm(const CG&, Emitter&) const;
};

struct VectorRegSpec {
  VectorRegSpec() : vecLane(-1), vecLaneMax(0), vecElementSize(0), compositeOpnds(0) {}

  VectorRegSpec(PrimType type, int16 lane = -1, uint16 compositeOpnds = 0) :
      vecLane(lane),
      vecLaneMax(GetVecLanes(type)),
      vecElementSize(GetVecEleSize(type)),
      compositeOpnds(compositeOpnds) {}

  VectorRegSpec(uint16 laneNum, uint16 eleSize, int16 lane = -1, uint16 compositeOpnds = 0) :
      vecLane(lane),
      vecLaneMax(laneNum),
      vecElementSize(eleSize),
      compositeOpnds(compositeOpnds) {}

  int16 vecLane;         /* -1 for whole reg, 0 to 15 to specify individual lane */
  uint16 vecLaneMax;     /* Maximum number of lanes for this vregister */
  uint16 vecElementSize; /* element size in each Lane */
  uint16 compositeOpnds; /* Number of enclosed operands within this composite operand */
};

class AArch64VectorInsn : public AArch64Insn {
 public:
  AArch64VectorInsn(MemPool &memPool, MOperator opc)
      : AArch64Insn(memPool, opc),
        regSpecList(localAlloc.Adapter()) {
    regSpecList.clear();
  }

  ~AArch64VectorInsn() override = default;

  void ClearRegSpecList() {
    regSpecList.clear();
  }

  VectorRegSpec *GetAndRemoveRegSpecFromList() {
    if (regSpecList.size() == 0) {
      VectorRegSpec *vecSpec = CG::GetCurCGFuncNoConst()->GetMemoryPool()->New<VectorRegSpec>();
      return vecSpec;
    }
    VectorRegSpec *ret = regSpecList.back();
    regSpecList.pop_back();
    return ret;
  }

  size_t GetNumOfRegSpec() const {
    if (IsVectorOp() && !regSpecList.empty()) {
      return regSpecList.size();
    }
    return 0;
  }

  MapleVector<VectorRegSpec*> &GetRegSpecList() {
    return regSpecList;
  }

  void SetRegSpecList(const MapleVector<VectorRegSpec*> &vec) {
    regSpecList = vec;
  }

  void PushRegSpecEntry(VectorRegSpec *v) {
    regSpecList.emplace(regSpecList.begin(), v); /* add at front  */
  }

 private:
  MapleVector<VectorRegSpec*> regSpecList;
};

class AArch64cleancallInsn : public AArch64Insn {
 public:
  AArch64cleancallInsn(MemPool &memPool, MOperator opc)
      : AArch64Insn(memPool, opc), refSkipIndex(-1) {}

  AArch64cleancallInsn(const AArch64cleancallInsn &originalInsn, MemPool &memPool)
      : AArch64Insn(originalInsn, memPool) {
    refSkipIndex = originalInsn.refSkipIndex;
  }
  AArch64cleancallInsn &operator=(const AArch64cleancallInsn &p) = default;
  ~AArch64cleancallInsn() override = default;

  void SetRefSkipIndex(int32 index) {
    refSkipIndex = index;
  }

 private:
  int32 refSkipIndex;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H */
