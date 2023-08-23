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
/* Maple CG headers */
#include "operand.h"
#include "isa.h"
#include "common_utils.h"
/* Maple IR headers */
#include "types_def.h"  /* for uint32 */
/* Maple Util headers */
#include "mpl_logging.h"
#include "mem_reference_table.h" /* for alias */

namespace maplebe {
/* forward declaration */
class BB;
class CG;
class Emitter;
class DepNode;
class InsnBuilder;
class OperandBuilder;

struct VectorRegSpec {
  VectorRegSpec() : vecLane(-1), vecLaneMax(0), vecElementSize(0), compositeOpnds(0) {}

  explicit VectorRegSpec(PrimType type, int16 lane = -1, uint16 compositeOpnds = 0) :
      vecLane(lane),
      vecLaneMax(GetVecLanes(type)),
      vecElementSize(GetVecEleSize(type)),
      compositeOpnds(compositeOpnds) {}

  VectorRegSpec(uint16 laneNum, uint16 eleSize, int16 lane = -1, uint16 compositeOpnds = 0) :
      vecLane(lane),
      vecLaneMax(laneNum),
      vecElementSize(eleSize),
      compositeOpnds(compositeOpnds) {}

  void Dump() const {
    if (vecElementSize == 0) {
      return;
    }
    LogInfo::MapleLogger() << vecLaneMax;
    if (vecElementSize == k8BitSize) {
      LogInfo::MapleLogger() << "B";
    } else if (vecElementSize == k16BitSize) {
      LogInfo::MapleLogger() << "H";
    } else if (vecElementSize == k32BitSize) {
      LogInfo::MapleLogger() << "S";
    } else {
      LogInfo::MapleLogger() << "D";
    }
    if (vecLane >= 0) {
      LogInfo::MapleLogger() << "[" << vecLane << "]";
    }
  }

  int16 vecLane;         /* -1 for whole reg, 0 to 15 to specify individual lane */
  uint16 vecLaneMax;     /* Maximum number of lanes for this vregister */
  uint16 vecElementSize; /* element size in each Lane */
  uint16 compositeOpnds; /* Number of enclosed operands within this composite operand */
};

class Insn {
 public:
  enum RetType : uint8 {
    kRegNull,   // no return type
    kRegFloat,  // return register is V0
    kRegInt     // return register is R0
  };
  // MCC_DecRefResetPair clear 2 stack position, MCC_ClearLocalStackRef clear 1 stack position
  static constexpr uint8 kMaxStackOffsetSize = 2;
  static constexpr int32 kUnknownProb = -1;

  Insn(MemPool &memPool, MOperator opc)
      : mOp(opc),
        localAlloc(&memPool),
        opnds(localAlloc.Adapter()),
        registerBinding(localAlloc.Adapter()),
        regSpecList(localAlloc.Adapter()),
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

  // Custom deep copy
  virtual Insn *CloneTree(MapleAllocator &allocator) const {
    auto *insn = allocator.GetMemPool()->New<Insn>(*this);
    insn->opnds.clear();
    for (auto opnd : opnds) {
      (void)insn->opnds.emplace_back(opnd->CloneTree(allocator));
    }
    return insn;
  }

  // Default shallow copy
  Insn *Clone(const MemPool /* &memPool */) const;

  MOperator GetMachineOpcode() const {
    return mOp;
  }

  void SetMOP(const InsnDesc &idesc);

  void AddOperand(Operand &opnd) {
    opnds.emplace_back(&opnd);
  }

  Insn &AddOpndChain(Operand &opnd) {
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

  void ResizeOpnds(size_t newSize) {
    opnds.resize(newSize);
  }

  uint32 GetOperandSize() const {
    return static_cast<uint32>(opnds.size());
  }

  void SetOperand(uint32 index, Operand &opnd) {
    ASSERT(index <= opnds.size(), "index out of range");
    opnds[index] = &opnd;
  }

  uint32 GetOperandSize(uint32 index) const {
    CHECK_FATAL(index < opnds.size(), "index out of range!");
    const OpndDesc *opndMD = md->GetOpndDes(index);
    return opndMD->GetSize();
  }

  void SetRetSize(uint32 size) {
    ASSERT(IsCall(), "Insn should be a call.");
    retSize = size;
  }

  uint32 GetRetSize() const {
    ASSERT(IsCall(), "Insn should be a call.");
    return retSize;
  }

  // Insn Function: check legitimacy of opnds.
  bool VerifySelf() const {
    if (this->IsCfiInsn() || this->IsDbgInsn()) {
      return true;
    }
    return md->Verify(opnds);
  }

  void SplitSelf(bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder) {
    md->Split(this, isAfterRegAlloc, insnBuilder, opndBuilder);
  }

  virtual bool IsMachineInstruction() const;

  bool OpndIsDef(uint32 opndId) const;

  bool OpndIsUse(uint32 opndId) const;

  virtual bool IsPCLoad() const {
    return false;
  }

  Operand *GetMemOpnd() const;
  uint32 GetMemOpndIdx() const;

  void SetMemOpnd(MemOperand *memOpnd);

  bool IsCall() const;
  bool IsSpecialCall() const;
  bool IsTailCall() const;
  bool IsAsmInsn() const;
  bool IsClinit() const;
  bool CanThrow() const;
  bool MayThrow() const;
  bool IsBranch() const;
  bool IsCondBranch() const;
  bool IsUnCondBranch() const;
  bool IsMove() const;
  bool IsBasicOp() const;
  bool IsUnaryOp() const;
  bool IsShift() const;
  bool IsPhi() const;
  bool IsLoad() const;
  bool IsStore() const;
  bool IsConversion() const;
  bool IsAtomic() const;
  bool IsCondDef() const;

  bool IsLoadPair() const;
  bool IsStorePair() const;
  bool IsLoadStorePair() const;
  bool IsLoadLabel() const;

  virtual bool NoAlias() const {
    return false;
  }

  bool IsVolatile() const;

  bool IsMemAccessBar() const;

  bool IsMemAccess() const;

  virtual bool HasSideEffects() const {
    return false;
  }

  bool HasLoop() const;

  virtual bool IsSpecialIntrinsic() const;

  bool IsComment() const;
  bool IsImmaterialInsn() const;

  virtual bool IsTargetInsn() const {
    return true;
  }

  virtual bool IsCfiInsn() const {
    return false;
  }

  virtual bool IsDbgInsn() const {
    return false;
  }

  bool IsDMBInsn() const;

  bool IsVectorOp() const;

  virtual Operand *GetCallTargetOperand() const;

  uint32 GetAtomicNum() const;
  /*
   * returns a ListOperand
   * Note that we don't really need this for Emit
   * Rather, we need it for register allocation, to
   * correctly state the live ranges for operands
   * use for passing call arguments
   */
  virtual ListOperand *GetCallArgumentOperand();
  bool IsAtomicStore() const {
    return IsStore() && IsAtomic();
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

  bool IsStackDef() const {
    return isStackDef;
  }

  void SetStackDef(bool flag) {
    isStackDef = flag;
  }

  bool IsStackRevert() const {
    return isStackRevert;
  }

  void SetStackRevert(bool flag) {
    isStackRevert = flag;
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

  virtual void Dump() const;

#if defined(DEBUG) && DEBUG
  virtual void Check() const;
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

  bool IsIntRegisterMov() const {
    if (md == nullptr || !md->IsPhysicalInsn() || !md->IsMove() ||
        md->opndMD.size() != kOperandNumBinary) {
      return false;
    }
    auto firstMD = md->GetOpndDes(kFirstOpnd);
    auto secondMD = md->GetOpndDes(kSecondOpnd);
    if (!firstMD->IsRegister() || !secondMD->IsRegister() ||
        !firstMD->IsIntOperand() || !secondMD->IsIntOperand() ||
        firstMD->GetSize() != secondMD->GetSize()) {
      return false;
    }
    return true;
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

  uint32 GetLatencyType() const;

  void SetPrev(Insn *prevInsn) {
    this->prev = prevInsn;
  }

  Insn *GetPrev() {
    return prev;
  }

  const Insn *GetPrev() const {
    return prev;
  }

  void SetNext(Insn *nextInsn) {
    this->next = nextInsn;
  }

  Insn *GetNext() const {
    return next;
  }

  void SetBB(BB *newBB) {
    this->bb = newBB;
  }

  BB *GetBB() {
    return bb;
  }

  const BB *GetBB() const {
    return bb;
  }

  void SetId(uint32 idVal) {
    this->id = idVal;
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

  void SetIsThrow(bool isThrowVal) {
    this->isThrow = isThrowVal;
  }

  bool GetIsThrow() const {
    return isThrow;
  }

  void SetDoNotRemove(bool doNotRemoveVal) {
    this->doNotRemove = doNotRemoveVal;
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

  void SetMayTailCall() {
    mayTailCall = true;
  }

  bool GetMayTailCall() const {
    return mayTailCall;
  }

  void SetRetType(RetType retTy) {
    this->retType = retTy;
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

  void SetDepNode(DepNode &depNodeVal) {
    this->depNode = &depNodeVal;
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

  void SetInsnDescrption(const InsnDesc &newMD) {
    md = &newMD;
  }

  const InsnDesc *GetDesc() const {
    return md;
  }

  void AddRegBinding(uint32 regA, uint32 regB) {
    (void)registerBinding.emplace(regA, regB);
  }

  const MapleMap<uint32, uint32>& GetRegBinding() const {
    return registerBinding;
  }

  void SetRefSkipIdx(int32 index) {
    refSkipIdx = index;
  }

  /* Get Size of memory write/read by insn */
  uint32 GetMemoryByteSize() const;

  /* return ture if register appears */
  virtual bool ScanReg(regno_t regNO) const;

  virtual bool IsRegDefined(regno_t regNO) const;

  virtual std::set<uint32> GetDefRegs() const;

  virtual uint32 GetBothDefUseOpnd() const;

  RegOperand *GetSSAImpDefOpnd() {
    return ssaImplicitDefOpnd;
  }

  void SetSSAImpDefOpnd(RegOperand *ssaDef) {
    ssaImplicitDefOpnd = ssaDef;
  }

  void SetProcessRHS() {
    processRHS = true;
  }

  bool HasProcessedRHS() const {
    return processRHS;
  }
  void ClearRegSpecList() {
    regSpecList.clear();
  }
  int32 GetProb() const {
    return probability;
  }
  void SetProb(int prob) {
    probability = prob;
  }

  VectorRegSpec *GetAndRemoveRegSpecFromList();

  size_t GetNumOfRegSpec() const {
    if (IsVectorOp() && !regSpecList.empty()) {
      return regSpecList.size();
    }
    return 0;
  }

  const MapleList<VectorRegSpec*> &GetRegSpecList() const {
    return regSpecList;
  }

  void SetRegSpecList(const MapleList<VectorRegSpec*> &vec) {
    regSpecList = vec;
  }

  Insn &PushRegSpecEntry(VectorRegSpec *v) {
    (void)regSpecList.emplace_back(v);
    return *this;
  }

  void SetReferenceOsts(MemDefUse *memDefUse) {
    referenceOsts = memDefUse;
  }

  const MemDefUse *GetReferenceOsts() const {
    return referenceOsts;
  }

 protected:
  MOperator mOp;
  MapleAllocator localAlloc;
  MapleVector<Operand*> opnds;
  RegOperand *ssaImplicitDefOpnd = nullptr;   /* for the opnd is both def and use is ssa */
  Insn *prev = nullptr;
  Insn *next = nullptr;
  BB *bb = nullptr;        /* BB to which this insn belongs */
  uint32 flags = 0;
  bool isPhiMovInsn = false;

 private:
  MapleMap<uint32, uint32> registerBinding; /* used for inline asm only */
  MapleList<VectorRegSpec*> regSpecList;
  enum OpKind : uint32 {
    kOpUnknown = 0,
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
  bool isStackDef = false;    // def sp in prolog
  bool isStackRevert = false;  // revert sp in epilog
  bool asmDefCondCode = false;
  bool asmModMem = false;
  bool mayTailCall = false;

  /* for dynamic language to mark reference counting */
  int32 refSkipIdx = -1;

  /* for multiple architecture */
  const InsnDesc *md = nullptr;
  /*
   * for redundant compute elimination phase,
   * indicate whether the version has been processed.
   */
  bool processRHS = false;
  // for jmp insn, probability is the prob for jumping
  int32 probability = kUnknownProb;

  /*
   * This field indicates the OstIdx list referenced by memory-related and call-related instructions,
   * the alias information is transferred from ME.
   * If the field is nullptr, the memory location referenced by the instruction is unknown,
   * and needs to be processed conservatively.
   */
  MemDefUse *referenceOsts = nullptr;
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
