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
#ifndef MAPLEBE_INCLUDE_CG_INSN_H
#define MAPLEBE_INCLUDE_CG_INSN_H
/* C++ headers */
#include <cstddef>  /* for nullptr */
#include <string>
#include <vector>
#include <list>
#include "operand.h"
#include "mpl_logging.h"

/* Maple IR header */
#include "types_def.h"  /* for uint32 */
#include "common_utils.h"

namespace maplebe {
/* forward declaration */
class BB;
class CG;
class Emitter;
class DepNode;
struct InsnDescription;
class Insn {
 public:
  enum RetType : uint8 {
    kRegNull,   /* no return type */
    kRegFloat,  /* return register is V0 */
    kRegInt     /* return register is R0 */
  };
  /* MCC_DecRefResetPair clear 2 stack position, MCC_ClearLocalStackRef clear 1 stack position */
  static constexpr uint8 kMaxStackOffsetSize = 2;

  Insn(MemPool &memPool, MOperator opc)
      : mOp(opc),
        localAlloc(&memPool),
        opnds(localAlloc.Adapter()),
        registerBinding(localAlloc.Adapter()),
        comment(&memPool) {}
  Insn(MemPool &memPool, MOperator opc, Operand &opnd0) : Insn(memPool, opc) { opnds.emplace_back(&opnd0); }
  Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1) : Insn(memPool, opc) {
    opnds.emplace_back(&opnd0);
    opnds.emplace_back(&opnd1);
  }
  Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1, Operand &opnd2) : Insn(memPool, opc) {
    opnds.emplace_back(&opnd0);
    opnds.emplace_back(&opnd1);
    opnds.emplace_back(&opnd2);
  }
  Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1, Operand &opnd2, Operand &opnd3)
      : Insn(memPool, opc) {
    opnds.emplace_back(&opnd0);
    opnds.emplace_back(&opnd1);
    opnds.emplace_back(&opnd2);
    opnds.emplace_back(&opnd3);
  }
  Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1, Operand &opnd2, Operand &opnd3, Operand &opnd4)
      : Insn(memPool, opc) {
    opnds.emplace_back(&opnd0);
    opnds.emplace_back(&opnd1);
    opnds.emplace_back(&opnd2);
    opnds.emplace_back(&opnd3);
    opnds.emplace_back(&opnd4);
  }
  virtual ~Insn() = default;

  MOperator GetMachineOpcode() const {
    return mOp;
  }
#ifdef TARGX86_64
  void SetMOP(const InsnDescription &idesc);
#else
  void SetMOP(MOperator mOp) {
    this->mOp = mOp;
  }
#endif

  void AddOperand(Operand &opnd) {
    opnds.emplace_back(&opnd);
  }

  Insn &AddOperandChain(Operand &opnd) {
    AddOperand(opnd);
    return *this;
  }
  /* use carefully which might cause insn to illegal */
  void CommuteOperands(uint32 dIndex, uint32 sIndex);
  void CleanAllOperand() {
    opnds.clear();
  }

  void PopBackOperand() {
    opnds.pop_back();
  }

  Operand &GetOperand(uint32 index) const {
    ASSERT(index < opnds.size(), "index out of range");
    return *opnds[index];
  }

  void ResizeOpnds(uint32 newSize) {
    opnds.resize(static_cast<std::size_t>(newSize));
  }

  uint32 GetOperandSize() const {
    return static_cast<uint32>(opnds.size());
  }

  void SetOperand(uint32 index, Operand &opnd) {
    ASSERT(index <= opnds.size(), "index out of range");
    opnds[index] = &opnd;
  }

  virtual void SetResult(uint32 index, Operand &res) {
    (void)index;
    (void)res;
  }

  void SetRetSize(uint32 size) {
    ASSERT(IsCall(), "Insn should be a call.");
    retSize = size;
  }

  uint32 GetRetSize() const {
    ASSERT(IsCall(), "Insn should be a call.");
    return retSize;
  }

  virtual bool IsMachineInstruction() const;

  virtual bool IsPseudoInstruction() const {
    return false;
  }

  virtual bool IsReturnPseudoInstruction() const {
    return false;
  }

  virtual bool OpndIsDef(uint32) const {
    return false;
  }

  virtual bool OpndIsUse(uint32) const {
    return false;
  }

  virtual bool OpndIsMayDef(uint32) const {
    return false;
  }

  virtual bool IsEffectiveCopy() const {
    return false;
  }

  virtual int32 CopyOperands() const {
    return -1;
  }

  virtual bool IsSameRes() {
    return false;
  }

  virtual bool IsPCLoad() const {
    return false;
  }

  virtual uint32 GetOpndNum() const {
    return 0;
  }

  virtual uint32 GetResultNum() const {
    return 0;
  }

  virtual Operand *GetOpnd(uint32 index) const {
    (void)index;
    return nullptr;
  }

  virtual Operand *GetMemOpnd() const {
    return nullptr;
  }

  virtual void SetMemOpnd(MemOperand *memOpnd) {
    (void)memOpnd;
  }

  virtual Operand *GetResult(uint32 index) const{
    (void)index;
    return nullptr;
  }

  virtual void SetOpnd(uint32 index, Operand &opnd) {
    (void)index;
    (void)opnd;
  }

  virtual bool IsGlobal() const {
    return false;
  }

  virtual bool IsDecoupleStaticOp() const {
    return false;
  }

  virtual bool IsCall() const;

  virtual bool IsAsmInsn() const {
    return false;
  }

  virtual bool IsTailCall() const {
    return false;
  }

  virtual bool IsClinit() const {
    return false;
  }

  virtual bool IsLazyLoad() const {
    return false;
  }

  virtual bool IsAdrpLdr() const {
    return false;
  }

  virtual bool IsArrayClassCache() const {
    return false;
  }

  virtual bool IsReturn() const {
    return false;
  }

  virtual bool IsFixedInsn() const {
    return false;
  }

  virtual bool CanThrow() const {
    return false;
  }

  virtual bool MayThrow() {
    return false;
  }

  virtual bool IsIndirectCall() const {
    return false;
  }

  virtual bool IsCallToFunctionThatNeverReturns() {
    return false;
  }

  virtual bool IsLoadLabel() const {
    return false;
  }

  virtual bool IsBranch() const {
    return false;
  }

  virtual bool IsCondBranch() const;

  virtual bool IsUnCondBranch() const {
    return false;
  }

  virtual bool IsMove() const;

  virtual bool IsMoveRegReg() const {
    return false;
  }

  virtual bool IsBasicOp() const;

  virtual bool IsUnaryOp() const;

  virtual bool IsShift() const;

  virtual bool IsPhi() const{
    return false;
  }

  virtual bool IsLoad() const;

  virtual bool IsStore() const;

  virtual bool IsConversion() const;

  virtual bool IsLoadPair() const {
    return false;
  }

  virtual bool IsStorePair() const {
    return false;
  }

  virtual bool IsLoadStorePair() const {
    return false;
  }

  virtual bool IsLoadAddress() const {
    return false;
  }

  virtual bool IsAtomic() const {
    return false;
  }

  virtual bool NoAlias() const {
    return false;
  }

  virtual bool NoOverlap() const {
    return false;
  }

  virtual bool IsVolatile() const {
    return false;
  }

  virtual bool IsMemAccessBar() const {
    return false;
  }

  virtual bool IsMemAccess() const {
    return false;
  }

  virtual bool HasSideEffects() const {
    return false;
  }

  virtual bool HasLoop() const {
    return false;
  }

  virtual bool IsSpecialIntrinsic() const {
    return false;
  }

  virtual bool IsComment() const {
    return false;
  }

  virtual bool IsGoto() const {
    return false;
  }

  virtual bool IsImmaterialInsn() const {
    return false;
  }

  virtual bool IsYieldPoint() const {
    return false;
  }

  virtual bool IsPartDef() const {
    return false;
  }

  virtual bool IsTargetInsn() const {
    return false;
  }

  virtual bool IsCfiInsn() const {
    return false;
  }

  virtual bool IsDbgInsn() const {
    return false;
  }

  virtual bool IsFallthruCall() const {
    return false;
  }

  virtual bool IsDMBInsn() const {
    return false;
  }

  virtual bool IsVectorOp() const {
    return false;
  }

  virtual Operand *GetCallTargetOperand() const {
    return nullptr;
  }

  virtual uint32 GetAtomicNum() const {
    return 1;
  }
  /*
   * returns a ListOperand
   * Note that we don't really need this for Emit
   * Rather, we need it for register allocation, to
   * correctly state the live ranges for operands
   * use for passing call arguments
   */
  virtual ListOperand *GetCallArgumentOperand() {
    return nullptr;
  }

  bool IsAtomicStore() const {
    return IsStore() && IsAtomic();
  }

  void SetCondDef() {
    flags |= kOpCondDef;
  }

  bool IsCondDef() const {
    return flags & kOpCondDef;
  }

  bool AccessMem() const {
    return IsLoad() || IsStore();
  }

  bool IsFrameDef() const {
    return isFrameDef;
  }

  void SetFrameDef(bool b) {
    isFrameDef = b;
  }

  bool IsAsmDefCondCode() const {
    return asmDefCondCode;
  }

  void SetAsmDefCondCode() {
    asmDefCondCode = true;
  }

  bool IsAsmModMem() const {
    return asmModMem;
  }

  void SetAsmModMem() {
    asmModMem = true;
  }

  virtual uint32 GetUnitType() {
    return 0;
  }

#if TARGAARCH64 || TARGRISCV64
  virtual void Dump() const = 0;
#else
  virtual void Dump() const;
#endif

#if !RELEASE
  virtual bool Check() const {
    return true;
  }

#else
  virtual bool Check() const = 0;
#endif

  void SetComment(const std::string &str) {
    comment = str;
  }

  void SetComment(const MapleString &str) {
    comment = str;
  }

  const MapleString &GetComment() const {
    return comment;
  }

  void AppendComment(const std::string &str) {
    comment += str;
  }

  void MarkAsSaveRetValToLocal() {
    flags |= kOpDassignToSaveRetValToLocal;
  }

  bool IsSaveRetValToLocal() const {
    return ((flags & kOpDassignToSaveRetValToLocal) != 0);
  }

  void MarkAsAccessRefField(bool cond) {
    if (cond) {
      flags |= kOpAccessRefField;
    }
  }

  bool IsAccessRefField() const {
    return ((flags & kOpAccessRefField) != 0);
  }

#if TARGAARCH64 || TARGRISCV64
  virtual bool IsRegDefined(regno_t regNO) const = 0;

  virtual std::set<uint32> GetDefRegs() const = 0;

  virtual uint32 GetBothDefUseOpnd() const {
    CHECK_FATAL(false, "impl in sub");
  };

  virtual bool IsDefinition() const = 0;
#endif

  virtual bool IsDestRegAlsoSrcReg() const {
    return false;
  }

  Insn *GetPreviousMachineInsn() const {
    for (Insn *returnInsn = prev; returnInsn != nullptr; returnInsn = returnInsn->prev) {
      ASSERT(returnInsn->bb == bb, "insn and it's prev insn must have same bb");
      if (returnInsn->IsMachineInstruction()) {
        return returnInsn;
      }
    }
    return nullptr;
  }

  Insn *GetNextMachineInsn() const {
    for (Insn *returnInsn = next; returnInsn != nullptr; returnInsn = returnInsn->next) {
      CHECK_FATAL(returnInsn->bb == bb, "insn and it's next insn must have same bb");
      if (returnInsn->IsMachineInstruction()) {
        return returnInsn;
      }
    }
    return nullptr;
  }

  virtual uint32 GetLatencyType() const {
    return 0;
  }

  virtual uint32 GetJumpTargetIdx() const {
    return 0;
  }

  virtual uint32 GetJumpTargetIdxFromMOp(MOperator mOperator) const {
    (void)mOperator;
    return 0;
  }

  virtual MOperator FlipConditionOp(MOperator flippedOp, uint32 &targetIdx) {
    (void)flippedOp;
    (void)targetIdx;
    return 0;
  }

  void SetPrev(Insn *prev) {
    this->prev = prev;
  }

  Insn *GetPrev() {
    return prev;
  }

  const Insn *GetPrev() const {
    return prev;
  }

  void SetNext(Insn *next) {
    this->next = next;
  }

  Insn *GetNext() const {
    return next;
  }

  void SetBB(BB *bb) {
    this->bb = bb;
  }

  BB *GetBB() {
    return bb;
  }

  const BB *GetBB() const {
    return bb;
  }

  void SetId(uint32 id) {
    this->id = id;
  }

  uint32 GetId() const {
    return id;
  }

  void SetAddress(uint32 addr) {
    address = addr;
  }

  uint32 GetAddress() const {
    return address;
  }

  void SetNopNum(uint32 num) {
    nopNum = num;
  }

  uint32 GetNopNum() const {
    return nopNum;
  }

  void SetNeedSplit(bool flag) {
    needSplit = flag;
  }

  bool IsNeedSplit() const {
    return needSplit;
  }

  void SetIsThrow(bool isThrow) {
    this->isThrow = isThrow;
  }

  bool GetIsThrow() const {
    return isThrow;
  }

  void SetDoNotRemove(bool doNotRemove) {
    this->doNotRemove = doNotRemove;
  }

  bool GetDoNotRemove() const {
    return doNotRemove;
  }

  void SetIsSpill() {
    this->isSpill = true;
  }

  bool GetIsSpill() const {
    return isSpill;
  }

  void SetIsReload() {
    this->isReload = true;
  }

  bool GetIsReload() const {
    return isReload;
  }

  bool IsSpillInsn() const {
    return (isSpill || isReload);
  }

  void SetIsCallReturnUnsigned(bool unSigned) {
    ASSERT(IsCall(), "Insn should be a call.");
    this->isCallReturnUnsigned = unSigned;
  }

  bool GetIsCallReturnUnsigned() const {
    ASSERT(IsCall(), "Insn should be a call.");
    return isCallReturnUnsigned;
  }

  bool GetIsCallReturnSigned() const {
    ASSERT(IsCall(), "Insn should be a call.");
    return !isCallReturnUnsigned;
  }

  void SetRetType(RetType retType) {
    this->retType = retType;
  }

  RetType GetRetType() const {
    return retType;
  }

  void SetClearStackOffset(short index, int64 offset) {
    CHECK_FATAL(index < kMaxStackOffsetSize, "out of clearStackOffset's range");
    clearStackOffset[index] = offset;
  }

  int64 GetClearStackOffset(short index) const {
    CHECK_FATAL(index < kMaxStackOffsetSize, "out of clearStackOffset's range");
    return clearStackOffset[index];
  }

  /* if function name is MCC_ClearLocalStackRef or MCC_DecRefResetPair, will clear designate stack slot */
  bool IsClearDesignateStackCall() const {
    return clearStackOffset[0] != -1 || clearStackOffset[1] != -1;
  }

  void SetDepNode(DepNode &depNode) {
    this->depNode = &depNode;
  }

  DepNode *GetDepNode() {
    return depNode;
  }

  const DepNode *GetDepNode() const {
    return depNode;
  }

  void SetIsPhiMovInsn(bool val) {
    isPhiMovInsn = val;
  }

  bool IsPhiMovInsn() const {
    return isPhiMovInsn;
  }

  void InitWithOriginalInsn(const Insn &originalInsn, MemPool &memPool) {
    prev = originalInsn.prev;
    next = originalInsn.next;
    bb = originalInsn.bb;
    flags = originalInsn.flags;
    mOp = originalInsn.mOp;
    uint32 opndNum = originalInsn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; i++) {
      opnds.emplace_back(originalInsn.opnds[i]->Clone(memPool));
    }
  }

  std::vector<LabelOperand*> GetLabelOpnd() const {
    std::vector<LabelOperand*> labelOpnds;
    for (uint32 i = 0; i < opnds.size(); i++) {
      if (opnds[i]->IsLabelOpnd()) {
        labelOpnds.emplace_back(static_cast<LabelOperand*>(opnds[i]));
      }
    }
    return labelOpnds;
  }

  void SetInsnDescrption(const InsnDescription &newMD) {
    md = &newMD;
  }

  const InsnDescription *GetInsnDescrption() const {
    return md;
  }

  void AddRegBinding(uint32 regA, uint32 regB) {
    (void)registerBinding.emplace(regA, regB);
  }

  const MapleMap<uint32, uint32>& GetRegBinding() const {
    return registerBinding;
  }

 protected:
  MOperator mOp;
  MapleAllocator localAlloc;
  MapleVector<Operand*> opnds;
  Insn *prev = nullptr;
  Insn *next = nullptr;
  BB *bb = nullptr;        /* BB to which this insn belongs */
  uint32 flags = 0;
  bool isPhiMovInsn = false;

 private:
  MapleMap<uint32, uint32> registerBinding; /* used for inline asm only */
  enum OpKind : uint32 {
    kOpUnknown = 0,
    kOpCondDef = 0x1,
    kOpAccessRefField = (1ULL << 30),  /* load-from/store-into a ref flag-fieldGetMachineOpcode() */
    kOpDassignToSaveRetValToLocal = (1ULL << 31) /* save return value to local flag */
  };

  uint32 id = 0;
  uint32 address = 0;
  uint32 nopNum = 0;
  RetType retType = kRegNull;    /* if this insn is call, it represent the return register type R0/V0 */
  uint32 retSize = 0;  /* Byte size of the return value if insn is a call. */
  /* record the stack cleared by MCC_ClearLocalStackRef or MCC_DecRefResetPair */
  int64 clearStackOffset[kMaxStackOffsetSize] = { -1, -1 };
  DepNode *depNode = nullptr; /* For dependence analysis, pointing to a dependence node. */
  MapleString comment;
  bool isThrow = false;
  bool doNotRemove = false;  /* caller reg cross call */
  bool isCallReturnUnsigned = false;   /* for call insn only. false: signed, true: unsigned */
  bool isSpill = false;   /* used as hint for optimization */
  bool isReload = false;  /* used as hint for optimization */
  bool isFrameDef = false;
  bool asmDefCondCode = false;
  bool asmModMem = false;
  bool needSplit = false;

  /* for multiple architecture */
  const InsnDescription *md = nullptr;
};

struct InsnIdCmp {
  bool operator()(const Insn *lhs, const Insn *rhs) const {
    CHECK_FATAL(lhs != nullptr, "lhs is nullptr in InsnIdCmp");
    CHECK_FATAL(rhs != nullptr, "rhs is nullptr in InsnIdCmp");
    return lhs->GetId() < rhs->GetId();
  }
};
using InsnSet = std::set<Insn*, InsnIdCmp>;
using InsnMapleSet = MapleSet<Insn*, InsnIdCmp>;
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_INSN_H */
