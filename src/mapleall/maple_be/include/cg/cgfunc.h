/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_CGFUNC_H
#define MAPLEBE_INCLUDE_CG_CGFUNC_H

#include "becommon.h"
#include "operand.h"
#include "eh_func.h"
#include "memlayout.h"
#include "reg_info.h"
#include "cgbb.h"
#include "cfi.h"
#include "dbg.h"
#include "reaching.h"
#include "cg_cfg.h"
#include "cg_irbuilder.h"
#include "call_conv.h"
/* MapleIR headers. */
#include "mir_parser.h"
#include "mir_function.h"
#include "debug_info.h"
#include "maple_phase_manager.h"
/* Maple MP header */
#include "mempool_allocator.h"

namespace maplebe {
constexpr int32 kBBLimit = 100000;
constexpr int32 kFreqBase = 100000;
struct MemOpndCmp {
  bool operator()(const MemOperand *lhs, const MemOperand *rhs) const {
    CHECK_FATAL(lhs != nullptr, "null ptr check");
    CHECK_FATAL(rhs != nullptr, "null ptr check");
    if (lhs == rhs) {
      return false;
    }
    return (lhs->Less(*rhs));
  }
};

/* use for sync and atomic func */
enum SyncAndAtomicOp {
  kSyncAndAtomicOpAdd,
  kSyncAndAtomicOpSub,
  kSyncAndAtomicOpAnd,
  kSyncAndAtomicOpOr,
  kSyncAndAtomicOpXor,
  kSyncAndAtomicOpNand
};

class SpillMemOperandSet {
 public:
  explicit SpillMemOperandSet(MapleAllocator &mallocator) : reuseSpillLocMem(mallocator.Adapter()) {}

  virtual ~SpillMemOperandSet() = default;

  void Add(MemOperand &op) {
    (void)reuseSpillLocMem.insert(&op);
  }

  void Remove(MemOperand &op) {
    reuseSpillLocMem.erase(&op);
  }

  MemOperand *GetOne() {
    if (!reuseSpillLocMem.empty()) {
      MemOperand *res = *reuseSpillLocMem.begin();
      reuseSpillLocMem.erase(res);
      return res;
    }
    return nullptr;
  }

 private:
  MapleSet<MemOperand*, MemOpndCmp> reuseSpillLocMem;
};

class MPISel;

#if defined(TARGARM32) && TARGARM32
class LiveRange;
#endif  /* TARGARM32 */
constexpr uint32 kVRegisterNumber = 80;
constexpr uint32 kNumBBOptReturn = 30;
class CGFunc {
 public:
  enum ShiftDirection : uint8 {
    kShiftLeft,
    kShiftAright,
    kShiftLright
  };

  CGFunc(MIRModule &mod, CG &cg, MIRFunction &mirFunc, BECommon &beCommon, MemPool &memPool,
         StackMemPool &stackMp, MapleAllocator &allocator, uint32 funcId);
  virtual ~CGFunc();

  void InitFactory();

  const std::string &GetName() const {
    return func.GetName();
  }

  const MapleMap<LabelIdx, uint64> &GetLabelAndValueMap() const {
    return labelMap;
  }

  void InsertLabelMap(LabelIdx idx, uint64 value) {
    ASSERT(labelMap.find(idx) == labelMap.end(), "idx already exist");
    labelMap[idx] = value;
  }

  void LayoutStackFrame() {
    CHECK_FATAL(memLayout != nullptr, "memLayout should has been initialized in constructor");
    memLayout->LayoutStackFrame(structCopySize, maxParamStackSize);
  }

  bool HasCall() const {
    return func.HasCall();
  }

  bool HasVLAOrAlloca() const {
    return hasVLAOrAlloca;
  }

  void SetHasVLAOrAlloca(bool val) {
    hasVLAOrAlloca = val;
  }

  bool HasAlloca() const {
    return hasAlloca;
  }

  void SetHasAlloca(bool val) {
    hasAlloca = val;
  }

  void SetRD(ReachingDefinition *paramRd) {
    reachingDef = paramRd;
  }

  InsnBuilder *GetInsnBuilder() {
    return insnBuilder;
  }
  OperandBuilder *GetOpndBuilder() {
    return opndBuilder;
  }

  bool GetRDStatus() const {
    return (reachingDef != nullptr);
  }

  ReachingDefinition *GetRD() {
    return reachingDef;
  }

  EHFunc *BuildEHFunc();
  void NeedStackProtect();
  virtual void GenSaveMethodInfoCode(BB &bb) = 0;
  virtual void GenerateCleanupCode(BB &bb) = 0;
  virtual bool NeedCleanup() = 0;
  virtual void GenerateCleanupCodeForExtEpilog(BB &bb) = 0;
  virtual MemOperand *GetOrCreatSpillMem(regno_t vrNum) = 0;
  virtual void FreeSpillRegMem(regno_t vrNum) = 0;
  void CreateLmbcFormalParamInfo();
  virtual uint32 FloatParamRegRequired(MIRStructType *structType, uint32 &fpSize) = 0;
  virtual void AssignLmbcFormalParams() = 0;
  LmbcFormalParamInfo *GetLmbcFormalParamInfo(uint32 offset);
  virtual void LmbcGenSaveSpForAlloca() = 0;
  void RemoveUnreachableBB();
  Insn &BuildLocInsn(int64 fileNum, int64 lineNum, int64 columnNum);
  Insn &BuildScopeInsn(int64 id, bool isEnd);
  void GenerateLoc(StmtNode &stmt, SrcPosition &lastSrcPos, SrcPosition &lastMplPos);
  void GenerateScopeLabel(StmtNode &stmt, SrcPosition &lastSrcPos, bool &posDone);
  FreqType GetFreqFromStmt(uint32 stmtId);
  void GenerateInstruction();
  bool MemBarOpt(const StmtNode &membar);
  void UpdateCallBBFrequency();
  void HandleFunction();
  void MakeupScopeLabels(BB &bb);
  void ProcessExitBBVec();
  void AddCommonExitBB();
  virtual void MergeReturn() = 0;
  void TraverseAndClearCatchMark(BB &bb);
  void MarkCatchBBs();
  void MarkCleanupBB() const;
  void SetCleanupLabel(BB &cleanupEntry);
  bool ExitbbNotInCleanupArea(const BB &bb) const;
  uint32 GetMaxRegNum() const {
    return vReg.GetMaxRegCount();
  };
  void SetMaxRegNum(uint32 num) {
    vReg.SetMaxRegCount(num);
  }
  void IncMaxRegNum(uint32 num) {
    vReg.IncMaxRegCount(num);
  }
  void DumpCFG() const;
  void DumpBBInfo(const BB *bb) const;
  void DumpCGIR() const;
  void DumpLoop() const;
  void ClearLoopInfo();
  Operand *HandleExpr(const BaseNode &parent, BaseNode &expr);
  virtual void DetermineReturnTypeofCall() = 0;
  /* handle rc reset */
  virtual void HandleRCCall(bool begin, const MIRSymbol *retRef = nullptr) = 0;
  virtual void HandleRetCleanup(NaryStmtNode &retNode) = 0;
  /* select stmt */
  virtual void SelectDassign(DassignNode &stmt, Operand &opnd0) = 0;
  virtual void SelectDassignoff(DassignoffNode &stmt, Operand &opnd0) = 0;
  virtual void SelectRegassign(RegassignNode &stmt, Operand &opnd0) = 0;
  virtual void SelectAbort() = 0;
  virtual void SelectAssertNull(UnaryStmtNode &stmt) = 0;
  virtual void SelectAsm(AsmNode &node) = 0;
  virtual void SelectAggDassign(const DassignNode &stmt) = 0;
  virtual void SelectIassign(IassignNode &stmt) = 0;
  virtual void SelectIassignoff(IassignoffNode &stmt) = 0;
  virtual void SelectIassignfpoff(IassignFPoffNode &stmt, Operand &opnd) = 0;
  virtual void SelectIassignspoff(PrimType pTy, int32 offset, Operand &opnd) = 0;
  virtual void SelectBlkassignoff(BlkassignoffNode &bNode, Operand &src) = 0;
  virtual void SelectAggIassign(IassignNode &stmt, Operand &lhsAddrOpnd) = 0;
  virtual void SelectReturnSendOfStructInRegs(BaseNode *x) = 0;
  virtual void SelectReturn(Operand *opnd) = 0;
  virtual void SelectIgoto(Operand *opnd0) = 0;
  virtual void SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) = 0;
  virtual void SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0) = 0;
  virtual void SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0) = 0;
  virtual void SelectGoto(GotoNode &stmt) = 0;
  virtual void SelectCall(CallNode &callNode) = 0;
  virtual void SelectIcall(IcallNode &icallNode, Operand &fptrOpnd) = 0;
  virtual void SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode) = 0;
  virtual Operand *SelectIntrinsicOpWithOneParam(IntrinsicopNode &intrinsicopNode, std::string name) = 0;
  virtual Operand *SelectIntrinsicOpWithNParams(IntrinsicopNode &intrinsicopNode, PrimType retType,
                                                const std::string &name) = 0;
  virtual Operand *SelectCclz(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCctz(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCpopcount(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCparity(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCclrsb(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCisaligned(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCalignup(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCaligndown(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCSyncFetch(IntrinsicopNode &intrinsicopNode, SyncAndAtomicOp op, bool fetchBefore) = 0;
  virtual Operand *SelectCSyncBoolCmpSwap(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCSyncValCmpSwap(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCSyncLockTestSet(IntrinsicopNode &intrinsicopNode, PrimType pty) = 0;
  virtual Operand *SelectCSyncSynchronize(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCAtomicLoadN(IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCAtomicExchangeN(const IntrinsicopNode &intrinsicopNode) = 0;
  virtual Operand *SelectCAtomicFetch(IntrinsicopNode &intrinsicopNode, SyncAndAtomicOp op, bool fetchBefore) = 0;
  virtual Operand *SelectCReturnAddress(IntrinsicopNode &intrinsicopNode) = 0;
  virtual void SelectCAtomicExchange(const IntrinsiccallNode &intrinsiccallNode) = 0;
  virtual Operand *SelectCAtomicCompareExchange(const IntrinsicopNode &intrinsicopNode,
                                                bool isCompareExchangeN = false) = 0;
  virtual Operand *SelectCAtomicTestAndSet(const IntrinsicopNode &intrinsicopNode) = 0;
  virtual void SelectCAtomicClear(const IntrinsiccallNode &intrinsiccallNode) = 0;
  virtual void SelectMembar(StmtNode &membar) = 0;
  virtual void SelectComment(CommentNode &comment) = 0;
  virtual void HandleCatch() = 0;

  /* select expr */
  virtual Operand *SelectDread(const BaseNode &parent, AddrofNode &expr) = 0;
  virtual RegOperand *SelectRegread(RegreadNode &expr) = 0;
  virtual Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent, bool isAddrofoff) = 0;
  virtual Operand *SelectAddrofoff(AddrofoffNode &expr, const BaseNode &parent) = 0;
  virtual Operand &SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) = 0;
  virtual Operand &SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) = 0;
  virtual Operand *SelectIread(const BaseNode &parent, IreadNode &expr,
                               int extraOffset = 0, PrimType finalBitFieldDestType = kPtyInvalid) = 0;
  virtual Operand *SelectIreadoff(const BaseNode &parent, IreadoffNode &ireadoff) = 0;
  virtual Operand *SelectIreadfpoff(const BaseNode &parent, IreadFPoffNode &ireadoff) = 0;
  virtual Operand *SelectIntConst(MIRIntConst &intConst, const BaseNode &parent) = 0;
  virtual Operand *SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) = 0;
  virtual Operand *SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) = 0;
  virtual Operand *SelectFloat128Const(MIRFloat128Const &ldoubleConst) = 0;
  virtual Operand *SelectStrConst(MIRStrConst &strConst) = 0;
  virtual Operand *SelectStr16Const(MIRStr16Const &strConst) = 0;
  virtual void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectMadd(Operand &resOpnd, Operand &opndM0, Operand &opndM1, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectMadd(BinaryNode &node, Operand &opndM0, Operand &opndM1, Operand &opnd1,
                              const BaseNode &parent) = 0;
  virtual Operand *SelectRor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand &SelectCGArrayElemAdd(BinaryNode &node, const BaseNode &parent) = 0;
  virtual Operand *SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectLand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectLor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent,
                             bool parentIsBr = false) = 0;
  virtual void SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual void SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual void SelectNand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
  virtual Operand *SelectAbs(UnaryNode &node, Operand &opnd0) = 0;
  virtual Operand *SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectBswap(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectDepositBits(DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) = 0;
  virtual Operand *SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectRecip(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0) = 0;
  virtual Operand *SelectRound(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) = 0;
  virtual Operand *SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectSelect(TernaryNode &node, Operand &cond, Operand &opnd0, Operand &opnd1,
                                const BaseNode &parent, bool hasCompare = false) = 0;
  virtual Operand *SelectMalloc(UnaryNode &call, Operand &opnd0) = 0;
  virtual RegOperand &SelectCopy(Operand &src, PrimType srcType, PrimType dstType) = 0;
  virtual Operand *SelectAlloca(UnaryNode &call, Operand &opnd0) = 0;
  virtual Operand *SelectGCMalloc(GCMallocNode &call) = 0;
  virtual Operand *SelectJarrayMalloc(JarrayMallocNode &call, Operand &opnd0) = 0;
  virtual void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0) = 0;
  virtual Operand *SelectLazyLoad(Operand &opnd0, PrimType primType) = 0;
  virtual Operand *SelectLazyLoadStatic(MIRSymbol &st, int64 offset, PrimType primType) = 0;
  virtual Operand *SelectLoadArrayClassCache(MIRSymbol &st, int64 offset, PrimType primType) = 0;
  virtual void GenerateYieldpoint(BB &bb) = 0;
  virtual Operand &ProcessReturnReg(PrimType primType, int32 sReg) = 0;

  virtual Operand &GetOrCreateRflag() = 0;
  virtual const Operand *GetRflag() const = 0;
  virtual const Operand *GetFloatRflag() const = 0;
  virtual const LabelOperand *GetLabelOperand(LabelIdx labIdx) const = 0;
  virtual LabelOperand &GetOrCreateLabelOperand(LabelIdx labIdx) = 0;
  virtual LabelOperand &GetOrCreateLabelOperand(BB &bb) = 0;
  virtual RegOperand &CreateVirtualRegisterOperand(regno_t vRegNO) = 0;
  virtual RegOperand &GetOrCreateVirtualRegisterOperand(regno_t vRegNO) = 0;
  virtual RegOperand &GetOrCreateVirtualRegisterOperand(RegOperand &regOpnd) = 0;
  virtual RegOperand &GetOrCreateFramePointerRegOperand() = 0;
  virtual RegOperand &GetOrCreateStackBaseRegOperand() = 0;
  virtual RegOperand *GetBaseReg(const SymbolAlloc &symAlloc) = 0;
  virtual int32 GetBaseOffset(const SymbolAlloc &symbolAlloc) = 0;
  virtual RegOperand &GetZeroOpnd(uint32 size) = 0;
  virtual Operand &CreateCfiRegOperand(uint32 reg, uint32 size) = 0;
  virtual Operand &GetTargetRetOperand(PrimType primType, int32 sReg) = 0;
  virtual Operand &CreateImmOperand(PrimType primType, int64 val) = 0;
  virtual void ReplaceOpndInInsn(RegOperand &regDest, RegOperand &regSrc, Insn &insn, regno_t destNO) = 0;
  virtual void CleanupDeadMov(bool dump) = 0;
  virtual void GetRealCallerSaveRegs(const Insn &insn, std::set<regno_t> &realCallerSave) = 0;

  /* ra */
  virtual void AddtoCalleeSaved(regno_t reg) = 0;

  virtual bool IsFrameReg(const RegOperand &opnd) const = 0;
  virtual bool IsSPOrFP(const RegOperand &opnd) const = 0;
  virtual bool IsReturnReg(const RegOperand &opnd) const = 0;
  virtual bool IsSaveReg(const RegOperand &reg, MIRType &mirType, BECommon &cgBeCommon) const = 0;

  /* For Neon intrinsics */
  virtual RegOperand *SelectVectorAddLong(PrimType rTy, Operand *o1, Operand *o2, PrimType oty, bool isLow) = 0;
  virtual RegOperand *SelectVectorAddWiden(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, bool isLow) = 0;
  virtual RegOperand *SelectVectorAbs(PrimType rType, Operand *o1) = 0;
  virtual RegOperand *SelectVectorBinOp(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2,
                                        PrimType oTyp2, Opcode opc) = 0;
  virtual RegOperand *SelectVectorBitwiseOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2,
                                            PrimType oty2, Opcode opc) = 0;;
  virtual RegOperand *SelectVectorCompareZero(Operand *o1, PrimType oty1, Operand *o2, Opcode opc) = 0;
  virtual RegOperand *SelectVectorCompare(Operand *o1, PrimType oty1,  Operand *o2, PrimType oty2, Opcode opc) = 0;
  virtual RegOperand *SelectVectorFromScalar(PrimType pType, Operand *opnd, PrimType sType) = 0;
  virtual RegOperand *SelectVectorDup(PrimType rType, Operand *src, bool getLow) = 0;
  virtual RegOperand *SelectVectorAbsSubL(PrimType rType, Operand *o1, Operand *o2, PrimType oTy, bool isLow) = 0;
  virtual RegOperand *SelectVectorMadd(Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2, Operand *o3,
                                       PrimType oTyp3) = 0;
  virtual RegOperand *SelectVectorMerge(PrimType rTyp, Operand *o1, Operand *o2, int32 iNum) = 0;
  virtual RegOperand *SelectVectorMull(PrimType rType, Operand *o1, PrimType oTyp1,
      Operand *o2, PrimType oTyp2, bool isLow) = 0;
  virtual RegOperand *SelectVectorNarrow(PrimType rType, Operand *o1, PrimType otyp) = 0;
  virtual RegOperand *SelectVectorNarrow2(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2) = 0;
  virtual RegOperand *SelectVectorNeg(PrimType rType, Operand *o1) = 0;
  virtual RegOperand *SelectVectorNot(PrimType rType, Operand *o1) = 0;

  virtual RegOperand *SelectVectorPairwiseAdalp(Operand *src1, PrimType sty1, Operand *src2, PrimType sty2) = 0;
  virtual RegOperand *SelectVectorPairwiseAdd(PrimType rType, Operand *src, PrimType sType) = 0;
  virtual RegOperand *SelectVectorReverse(PrimType rtype, Operand *src, PrimType stype, uint32 size) = 0;
  virtual RegOperand *SelectVectorSetElement(Operand *eOp, PrimType eTyp, Operand *vOpd, PrimType vTyp, int32 lane) = 0;
  virtual RegOperand *SelectVectorShift(PrimType rType, Operand *o1,
                                        PrimType oty1, Operand *o2, PrimType oty2, Opcode opc) = 0;
  virtual RegOperand *SelectVectorShiftImm(PrimType rType, Operand *o1, Operand *imm, int32 sVal, Opcode opc) = 0;
  virtual RegOperand *SelectVectorShiftRNarrow(PrimType rType, Operand *o1, PrimType oType,
                                               Operand *o2, bool isLow) = 0;
  virtual RegOperand *SelectVectorSubWiden(PrimType resType, Operand *o1, PrimType otyp1,
                                           Operand *o2, PrimType otyp2, bool isLow, bool isWide) = 0;
  virtual RegOperand *SelectVectorSum(PrimType rtype, Operand *o1, PrimType oType) = 0;
  virtual RegOperand *SelectVectorWiden(PrimType rType, Operand *o1, PrimType otyp, bool isLow) = 0;
  virtual RegOperand *SelectVectorMovNarrow(PrimType rType, Operand *opnd, PrimType oType) = 0;
  virtual RegOperand *SelectVectorIntrinsics(const IntrinsicopNode &intrinsicNode) = 0;

  virtual void HandleFuncCfg(CGCFG *cfg) { AddCommonExitBB(); }

  /* For ebo issue. */
  virtual Operand *GetTrueOpnd() {
    return nullptr;
  }
  virtual void ClearUnreachableGotInfos(BB &bb) {
    (void)bb;
  };
  virtual void ClearUnreachableConstInfos(BB &bb) {
    (void)bb;
  };
  LabelIdx CreateLabel();

  RegOperand *GetVirtualRegisterOperand(regno_t vRegNO) {
    std::unordered_map<regno_t, RegOperand*> &vRegOperandTable = vReg.vRegOperandTable;
    auto it = vRegOperandTable.find(vRegNO);
    return it == vRegOperandTable.end() ? nullptr : it->second;
  }

  Operand &CreateCfiImmOperand(int64 val, uint32 size) const {
    return *memPool->New<cfi::ImmOperand>(val, size);
  }

  Operand &CreateCfiStrOperand(const std::string &str) const {
    return *memPool->New<cfi::StrOperand>(str, *memPool);
  }

  bool IsSpecialPseudoRegister(PregIdx spr) const {
    return spr < 0;
  }

  regno_t NewVReg(RegType regType, uint32 size) {
    if (CGOptions::UseGeneralRegOnly()) {
      CHECK_FATAL(regType != kRegTyFloat, "cannot use float | SIMD register with --general-reg-only");
    }
    return vReg.GetNextVregNO(regType, size);
  }

  virtual regno_t NewVRflag() {
    return 0;
  }

  RegType GetRegTyFromPrimTy(PrimType primType) const {
    switch (primType) {
      case PTY_u1:
      case PTY_i8:
      case PTY_u8:
      case PTY_i16:
      case PTY_u16:
      case PTY_i32:
      case PTY_u32:
      case PTY_i64:
      case PTY_u64:
      case PTY_a32:
      case PTY_a64:
      case PTY_ptr:
      case PTY_i128:
      case PTY_u128:
      case PTY_agg:
        return kRegTyInt;
      case PTY_f32:
      case PTY_f64:
      case PTY_f128:
      case PTY_v2i32:
      case PTY_v2u32:
      case PTY_v2i64:
      case PTY_v2u64:
      case PTY_v2f32:
      case PTY_v2f64:
      case PTY_v4i16:
      case PTY_v4u16:
      case PTY_v4i32:
      case PTY_v4u32:
      case PTY_v4f32:
      case PTY_v8i8:
      case PTY_v8u8:
      case PTY_v8i16:
      case PTY_v8u16:
      case PTY_v16i8:
      case PTY_v16u8:
      case PTY_v1i64:
      case PTY_v1u64:
        return kRegTyFloat;
      default:
        ASSERT(false, "Unexpected pty");
        return kRegTyUndef;
    }
  }

  /* return Register Type */
  virtual RegType GetRegisterType(regno_t rNum) const {
    CHECK(rNum < vReg.VRegTableSize(), "index out of range in GetVRegSize");
    return vReg.VRegTableGetType(rNum);
  }

#if defined(TARGX86_64) && TARGX86_64
  uint32 GetMaxVReg() const {
    return vReg.GetCount() + opndBuilder->GetCurrentVRegNum();
  }
#else
  uint32 GetMaxVReg() const {
    return vReg.GetCount();
  }
#endif

  uint32 GetSSAvRegCount() const {
    return ssaVRegCount;
  }

  void SetSSAvRegCount(uint32 count) {
    ssaVRegCount = count;
  }

  uint32 GetVRegSize(regno_t vregNum) {
    CHECK(vregNum < vReg.VRegTableSize(), "index out of range in GetVRegSize");
    return GetOrCreateVirtualRegisterOperand(vregNum).GetSize() / kBitsPerByte;
  }

  MIRSymbol *GetRetRefSymbol(BaseNode &expr);

  void PatchLongBranch();

  virtual uint32 MaxCondBranchDistance() {
    return INT_MAX;
  }

  virtual void InsertJumpPad(Insn *insn) {
    return;
  }

  virtual LabelIdx GetLabelInInsn(Insn &insn) {
    return 0;
  }

  Operand *CreateDbgImmOperand(int64 val) const {
    return memPool->New<mpldbg::ImmOperand>(val);
  }

  uint32 NumBBs() const {
    return bbCnt;
  }

#if defined(DEBUG) && DEBUG
  StIdx GetLocalVarReplacedByPreg(PregIdx reg) {
    auto it = pregsToVarsMap->find(reg);
    return it != pregsToVarsMap->end() ? it->second : StIdx();
  }
#endif

  void IncTotalNumberOfInstructions() {
    totalInsns++;
  }

  void DecTotalNumberOfInstructions() {
    totalInsns--;
  }

  uint32 GetTotalNumberOfInstructions() const {
    return totalInsns + insnBuilder->GetCreatedInsnNum();
  }

  int32 GetStructCopySize() const {
    return structCopySize;
  }

  int32 GetMaxParamStackSize() const {
    return maxParamStackSize;
  }

  virtual void ProcessLazyBinding() = 0;

  /* Debugging support */
  void SetDebugInfo(DebugInfo *dbgInfo) {
    debugInfo = dbgInfo;
  }

  void AddDIESymbolLocation(const MIRSymbol &sym, SymbolAlloc *loc, bool isParam);

  virtual void DBGFixCallFrameLocationOffsets() {};

  /* Get And Set private members */
  CG *GetCG() {
    return cg;
  }

  const CG *GetCG() const {
    return cg;
  }

  MIRModule &GetMirModule() {
    return mirModule;
  }

  const MIRModule &GetMirModule() const {
    return mirModule;
  }

  template <typename T>
  MIRConst *NewMirConst(T &mirConst) {
    MIRConst *newConst = mirModule.GetMemPool()->New<T>(mirConst.GetValue(), mirConst.GetType());
    return newConst;
  }

  uint32 GetMIRSrcFileEndLineNum() const {
    auto &srcFileInfo = mirModule.GetSrcFileInfo();
    if (!srcFileInfo.empty()) {
      return srcFileInfo.back().second;
    } else {
      return 0;
    }
  }

  MIRFunction &GetFunction() {
    return func;
  }

  const MIRFunction &GetFunction() const {
    return func;
  }

  EHFunc *GetEHFunc() {
    return ehFunc;
  }

  const EHFunc *GetEHFunc() const {
    return ehFunc;
  }

  void SetEHFunc(EHFunc &ehFunction) {
    ehFunc = &ehFunction;
  }

  uint32 GetLabelIdx() const {
    return labelIdx;
  }

  void SetLabelIdx(uint32 idx) {
    labelIdx = idx;
  }

  LabelNode *GetStartLabel() {
    return startLabel;
  }

  const LabelNode *GetStartLabel() const {
    return startLabel;
  }

  void SetStartLabel(LabelNode &label) {
    startLabel = &label;
  }

  LabelNode *GetEndLabel() {
    return endLabel;
  }

  const LabelNode *GetEndLabel() const {
    return endLabel;
  }

  void SetEndLabel(LabelNode &label) {
    endLabel = &label;
  }

  LabelNode *GetCleanupLabel() {
    return cleanupLabel;
  }

  const LabelNode *GetCleanupLabel() const {
    return cleanupLabel;
  }

  void SetCleanupLabel(LabelNode &node) {
    cleanupLabel = &node;
  }

  const LabelNode *GetReturnLabel() const {
    return returnLabel;
  }

  void SetReturnLabel(LabelNode &label) {
    returnLabel = &label;
  }

  BB *GetFirstBB() {
    return firstBB;
  }

  const BB *GetFirstBB() const {
    return firstBB;
  }

  void SetFirstBB(BB &bb) {
    firstBB = &bb;
  }

  BB *GetCleanupBB() {
    return cleanupBB;
  }

  const BB *GetCleanupBB() const {
    return cleanupBB;
  }

  void SetCleanupBB(BB &bb) {
    cleanupBB = &bb;
    cleanupBB->SetIsCleanup(true);
  }

  BB *GetLastBB() {
    return lastBB;
  }

  const BB *GetLastBB() const {
    return lastBB;
  }

  void SetLastBB(BB &bb) {
    lastBB = &bb;
  }

  BB *GetReturnBB() {
    return returnBB;
  }

  const BB *GetReturnBB() const {
    return returnBB;
  }

  void SetPrologureBB(BB &bb) {
    prologureBB = &bb;
  }

  BB *GetPrologureBB() {
    return prologureBB != nullptr ? prologureBB : firstBB;
  }

  void SetReturnBB(BB &bb) {
    returnBB = &bb;
    returnBB->SetKind(BB::kBBReturn);
    GetExitBBsVec().emplace_back(returnBB);
  }

  BB *GetCurBB() {
    return curBB;
  }

  const BB *GetCurBB() const {
    return curBB;
  }

  void SetCurBB(BB &bb) {
    curBB = &bb;
  }

  BB *GetDummyBB() {
    return dummyBB;
  }

  const BB *GetDummyBB() const {
    return dummyBB;
  }

  BB *GetCommonExitBB() {
    return commonExitBB;
  }

  LabelIdx GetFirstCGGenLabelIdx() const {
    return firstCGGenLabelIdx;
  }

  MapleVector<BB*> &GetExitBBsVec() {
    return exitBBVec;
  }

  const MapleVector<BB*> GetExitBBsVec() const {
    return exitBBVec;
  }

  size_t ExitBBsVecSize() const {
    return exitBBVec.size();
  }

  bool IsExitBBsVecEmpty() const {
    return exitBBVec.empty();
  }

  void EraseExitBBsVec(MapleVector<BB*>::iterator it) {
    exitBBVec.erase(it);
  }

  void PushBackExitBBsVec(BB &bb) {
    exitBBVec.emplace_back(&bb);
  }

  void ClearExitBBsVec() {
    exitBBVec.clear();
  }

  bool IsExtendReg(regno_t vregNum) {
    return extendSet.find(vregNum) != extendSet.end();
  }

  void InsertExtendSet(regno_t vregNum) {
    (void)extendSet.insert(vregNum);
  }

  void RemoveFromExtendSet(regno_t vregNum) {
    (void)extendSet.erase(vregNum);
  }

  bool IsExitBB(const BB &currentBB) {
    for (BB *exitBB : exitBBVec) {
      if (exitBB == &currentBB) {
        return true;
      }
    }
    return false;
  }

  BB *GetExitBB(int32 index) {
    return exitBBVec.at(index);
  }

  const BB *GetExitBB(int32 index) const {
    return exitBBVec.at(index);
  }

  void PushBackNoReturnCallBBsVec(BB &bb) {
    noReturnCallBBVec.emplace_back(&bb);
  }

  void SetLab2BBMap(int32 index, BB &bb) {
    lab2BBMap[index] = &bb;
  }

  BB *GetBBFromLab2BBMap(uint32 index) {
    return lab2BBMap[index];
  }

  MapleUnorderedMap<LabelIdx, BB*> &GetLab2BBMap() {
    return lab2BBMap;
  }

  void DumpCFGToDot(const std::string &fileNamePrefix);

  BECommon &GetBecommon() {
    return beCommon;
  }

  const BECommon GetBecommon() const {
    return beCommon;
  }

  MemLayout *GetMemlayout() {
    return memLayout;
  }

  const MemLayout *GetMemlayout() const {
    return memLayout;
  }

  void SetMemlayout(MemLayout &layout) {
    memLayout = &layout;
  }

  RegisterInfo *GetTargetRegInfo() {
    return targetRegInfo;
  }

  void SetTargetRegInfo(RegisterInfo &regInfo) {
    targetRegInfo = &regInfo;
  }

  MemPool *GetMemoryPool() {
    return memPool;
  }

  const MemPool *GetMemoryPool() const {
    return memPool;
  }

  StackMemPool &GetStackMemPool() {
    return stackMp;
  }

  MapleAllocator *GetFuncScopeAllocator() {
    return funcScopeAllocator;
  }

  const MapleAllocator *GetFuncScopeAllocator() const {
    return funcScopeAllocator;
  }

  const MapleMap<uint32, MIRSymbol*> GetEmitStVec() const {
    return emitStVec;
  }

  MIRSymbol* GetEmitSt(uint32 id) {
    return emitStVec[id];
  }

  void AddEmitSt(uint32 id, MIRSymbol &symbol) {
    CHECK_FATAL(symbol.GetKonst()->GetKind() == kConstAggConst, "not a kConstAggConst");
    MIRAggConst *arrayConst = safe_cast<MIRAggConst>(symbol.GetKonst());
    for (size_t i = 0; i < arrayConst->GetConstVec().size(); ++i) {
      CHECK_FATAL(arrayConst->GetConstVecItem(i)->GetKind() == kConstLblConst, "not a kConstLblConst");
      MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
      ++switchLabelCnt[lblConst->GetValue()];
    }
    emitStVec[id] = &symbol;
  }

  void UpdateEmitSt(const BB &bb, LabelIdx oldLabelIdx, LabelIdx newLabelIdx) {
    MIRSymbol *st = GetEmitSt(bb.GetId());
    MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st->GetKonst());
    MIRType *etype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64));
    MIRConst *mirConst = GetMemoryPool()->New<MIRLblConst>(newLabelIdx,
        GetFunction().GetPuidx(), *etype);
    for (size_t i = 0; i < arrayConst->GetConstVec().size(); ++i) {
      CHECK_FATAL(arrayConst->GetConstVecItem(i)->GetKind() == kConstLblConst, "not a kConstLblConst");
      MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
      if (oldLabelIdx == lblConst->GetValue()) {
        arrayConst->SetConstVecItem(i, *mirConst);
        ++switchLabelCnt[newLabelIdx];

        CHECK_FATAL(switchLabelCnt[oldLabelIdx] > 0, "error labelIdx");
        --switchLabelCnt[oldLabelIdx];
        if (switchLabelCnt[oldLabelIdx] == 0) {
          (void)switchLabelCnt.erase(oldLabelIdx);
        }
      }
    }
  }

  void DeleteEmitSt(uint32 id) {
    MIRSymbol &symbol = *emitStVec[id];
    CHECK_FATAL(symbol.GetKonst()->GetKind() == kConstAggConst, "not a kConstAggConst");
    MIRAggConst *arrayConst = safe_cast<MIRAggConst>(symbol.GetKonst());
    for (size_t i = 0; i < arrayConst->GetConstVec().size(); ++i) {
      CHECK_FATAL(arrayConst->GetConstVecItem(i)->GetKind() == kConstLblConst, "not a kConstLblConst");
      MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));

      LabelIdx labelIdx = lblConst->GetValue();
      CHECK_FATAL(switchLabelCnt[labelIdx] > 0, "error labelIdx");
      --switchLabelCnt[labelIdx];
      if (switchLabelCnt[labelIdx] == 0) {
        (void)switchLabelCnt.erase(labelIdx);
      }
    }
    (void)emitStVec.erase(id);
  }

  bool InSwitchTable(LabelIdx label) const {
    if (switchLabelCnt.empty()) {
      return false;
    }
    return (switchLabelCnt.find(label) != switchLabelCnt.end());
  }

  MapleVector<CGFuncLoops*> &GetLoops() {
    return loops;
  }

  const MapleVector<CGFuncLoops*> GetLoops() const {
    return loops;
  }

  void PushBackLoops(CGFuncLoops &loop) {
    loops.emplace_back(&loop);
  }

  MapleVector<LmbcFormalParamInfo*> &GetLmbcParamVec() {
    return lmbcParamVec;
  }

  void IncLmbcArgsInRegs(RegType ty) {
    if (ty == kRegTyInt) {
      lmbcIntArgs++;
    } else {
      lmbcFpArgs++;
    }
  }

  int32 GetLmbcArgsInRegs(RegType ty) const {
    return ty == kRegTyInt ? lmbcIntArgs : lmbcFpArgs;
  }

  void ResetLmbcArgsInRegs() {
    lmbcIntArgs = 0;
    lmbcFpArgs = 0;
  }

  void IncLmbcTotalArgs() {
    lmbcTotalArgs++;
  }

  uint32 GetLmbcTotalArgs() const {
    return lmbcTotalArgs;
  }

  void ResetLmbcTotalArgs() {
    lmbcTotalArgs = 0;
  }

  void SetSpSaveReg(regno_t reg) {
    spSaveReg = reg;
  }

  regno_t GetSpSaveReg() const {
    return spSaveReg;
  }

  MapleVector<BB*> &GetAllBBs() {
    return bbVec;
  }

  std::size_t GetAllBBSize() const {
    return bbVec.size();
  }

  BB *GetBBFromID(uint32 id) {
    return bbVec[id];
  }
  void ClearBBInVec(uint32 id) {
    bbVec[id] = nullptr;
  }

#if defined(TARGARM32) && TARGARM32
  MapleVector<BB*> &GetSortedBBs() {
    return sortedBBs;
  }

  const MapleVector<BB*> &GetSortedBBs() const {
    return sortedBBs;
  }

  void SetSortedBBs(const MapleVector<BB*> &bbVec) {
    sortedBBs = bbVec;
  }

  MapleVector<LiveRange*> &GetLrVec() {
    return lrVec;
  }

  const MapleVector<LiveRange*> &GetLrVec() const {
    return lrVec;
  }

  void SetLrVec(const MapleVector<LiveRange*> &newLrVec) {
    lrVec = newLrVec;
  }
#endif  /* TARGARM32 */

  CGCFG *GetTheCFG() {
    return theCFG;
  }

  void SetTheCFG(CGCFG *cfg) {
    theCFG = cfg;
  }

  const CGCFG *GetTheCFG() const {
    return theCFG;
  }

  regno_t GetVirtualRegNOFromPseudoRegIdx(PregIdx idx) const {
    return regno_t(idx + kBaseVirtualRegNO);
  }

  bool GetHasProEpilogue() const {
    return hasProEpilogue;
  }

  void SetHasProEpilogue(bool state) {
    hasProEpilogue = state;
  }

  int32 GetDbgCallFrameOffset() const {
    return dbgCallFrameOffset;
  }

  void SetDbgCallFrameOffset(int32 val) {
    dbgCallFrameOffset = val;
  }

  BB *CreateNewBB() {
    BB *bb = memPool->New<BB>(bbCnt++, *funcScopeAllocator);
    bbVec.emplace_back(bb);
    return bb;
  }

  BB *CreateNewBB(bool unreachable, BB::BBKind kind, uint32 frequencyVal) {
    BB *newBB = CreateNewBB();
    newBB->SetKind(kind);
    newBB->SetUnreachable(unreachable);
    newBB->SetFrequency(frequencyVal);
    return newBB;
  }

  BB *CreateNewBB(LabelIdx label, bool unreachable, BB::BBKind kind, uint32 frequencyVal) {
    BB *newBB = CreateNewBB(unreachable, kind, frequencyVal);
    newBB->AddLabel(label);
    SetLab2BBMap(label, *newBB);
    return newBB;
  }

  void UpdateFrequency(const StmtNode &stmt) {
    bool withFreqInfo = func.HasFreqMap() && !func.GetLastFreqMap().empty();
    if (!withFreqInfo) {
      return;
    }
    auto it = func.GetLastFreqMap().find(stmt.GetStmtID());
    if (it != func.GetLastFreqMap().end()) {
      frequency = it->second;
    }
  }

  BB *StartNewBBImpl(bool stmtIsCurBBLastStmt, StmtNode &stmt) {
    BB *newBB = CreateNewBB();
    ASSERT(newBB != nullptr, "newBB should not be nullptr");
    if (stmtIsCurBBLastStmt) {
      ASSERT(curBB != nullptr, "curBB should not be nullptr");
      curBB->SetLastStmt(stmt);
      curBB->AppendBB(*newBB);
      newBB->SetFirstStmt(*stmt.GetNext());
    } else {
      newBB->SetFirstStmt(stmt);
      if (curBB != nullptr) {
        if (stmt.GetPrev() != nullptr) {
          ASSERT(stmt.GetPrev()->GetNext() == &stmt, " the next of stmt's prev should be stmt self");
        }
        curBB->SetLastStmt(*stmt.GetPrev());
        curBB->AppendBB(*newBB);
      }
    }
    return newBB;
  }

  BB *StartNewBB(StmtNode &stmt) {
    BB *bb = curBB;
    if (stmt.GetNext() != nullptr && stmt.GetNext()->GetOpCode() != OP_label) {
      bb = StartNewBBImpl(true, stmt);
    }
    return bb;
  }

  void SetCurBBKind(BB::BBKind bbKind) const {
    curBB->SetKind(bbKind);
  }

  void SetVolStore(bool val) {
    isVolStore = val;
  }

  void SetVolReleaseInsn(Insn *insn) {
    volReleaseInsn = insn;
  }

  bool IsAfterRegAlloc() const {
    return isAfterRegAlloc;
  }

  void SetIsAfterRegAlloc() {
    isAfterRegAlloc = true;
  }

  const MapleString &GetShortFuncName() const {
    return shortFuncName;
  }

  size_t GetLSymSize() const {
    return lSymSize;
  }

  bool HasTakenLabel() const{
    return hasTakenLabel;
  }

  void SetHasTakenLabel() {
    hasTakenLabel = true;
  }

  virtual InsnVisitor *NewInsnModifier() = 0;

  MapleVector<DBGExprLoc*> &GetDbgCallFrameLocations(bool isParam) {
    return isParam ? dbgParamCallFrameLocations : dbgLocalCallFrameLocations;
  }

  bool HasAsm() const {
    return hasAsm;
  }

  uint32 GetUniqueID() const {
    return func.GetPuidx();
  }
  void SetUseFP(bool canUseFP) {
    useFP = canUseFP;
  }

  bool UseFP() const {
    return useFP;
  }

  void SetSeenFP(bool seen) {
    seenFP = seen;
  }

  bool SeenFP() const {
    return seenFP;
  }

  void UpdateAllRegisterVregMapping(MapleMap<regno_t, PregIdx> &newMap);

  void RegisterVregMapping(regno_t vRegNum, PregIdx pidx) {
    vregsToPregsMap[vRegNum] = pidx;
  }

  void SetHasAsm() {
    hasAsm = true;
  }

  void SetStackProtectInfo(StackProtectKind kind) {
    stackProtectInfo |= kind;
  }

  uint8 GetStackProtectInfo() const {
    return stackProtectInfo;
  }

  void SetNeedStackProtect(bool val) {
    needStackProtect = val;
  }

  bool GetNeedStackProtect() const {
    return needStackProtect;
  }

  virtual void Link2ISel(MPISel *p) {
    (void)p;
  }

  void SetISel(MPISel *p) {
    isel = p;
  }

  MPISel *GetISel() {
    return isel;
  }

  MIRPreg *GetPseudoRegFromVirtualRegNO(const regno_t vRegNO, bool afterSSA = false) const {
    PregIdx pri = afterSSA ? VRegNOToPRegIdx(vRegNO) : GetPseudoRegIdxFromVirtualRegNO(vRegNO);
    if (pri == -1) {
      return nullptr;
    }
    return GetFunction().GetPregTab()->PregFromPregIdx(pri);
  }

  void SetPriority(uint32 funcPriority) {
    priority = funcPriority;
  }

  uint32 GetPriority() const {
    return priority;
  }

 protected:
  uint32 firstNonPregVRegNO;
  VregInfo vReg;                          /* for assigning a number for each CG virtual register */
  uint32 ssaVRegCount = 0;                /* vreg  count in ssa */
  size_t lSymSize;                        /* size of local symbol table imported */
  MapleVector<BB*> bbVec;
  MapleUnorderedMap<PregIdx, MemOperand*> pRegSpillMemOperands;
  MapleUnorderedMap<regno_t, MemOperand*> spillRegMemOperands;
  MapleUnorderedMap<uint32, SpillMemOperandSet*> reuseSpillLocMem;
  LabelIdx firstCGGenLabelIdx;
  MapleMap<LabelIdx, uint64> labelMap;
#if defined(DEBUG) && DEBUG
  MapleMap<PregIdx, StIdx> *pregsToVarsMap = nullptr;
#endif
  MapleMap<regno_t, PregIdx> vregsToPregsMap;
  uint32 totalInsns = 0;
  int32 structCopySize = 0;
  int32 maxParamStackSize = 0;

  bool hasVLAOrAlloca = false;
  bool hasAlloca = false;
  bool hasProEpilogue = false;
  bool isVolLoad = false;
  bool isVolStore = false;
  bool isAfterRegAlloc = false;
  bool isAggParamInReg = false;
  bool hasTakenLabel = false;
  uint32 frequency = 0;
  DebugInfo *debugInfo = nullptr;  /* debugging info */
  MapleVector<DBGExprLoc*> dbgParamCallFrameLocations;
  MapleVector<DBGExprLoc*> dbgLocalCallFrameLocations;
  RegOperand *aggParamReg = nullptr;
  ReachingDefinition *reachingDef = nullptr;

  int32 dbgCallFrameOffset = 0;
  CG *cg;
  MIRModule &mirModule;
  MemPool *memPool;
  StackMemPool &stackMp;

  PregIdx GetPseudoRegIdxFromVirtualRegNO(const regno_t vRegNO) const {
    if (IsVRegNOForPseudoRegister(vRegNO)) {
      return PregIdx(vRegNO - kBaseVirtualRegNO);
    }
    return VRegNOToPRegIdx(vRegNO);
  }

  bool IsVRegNOForPseudoRegister(regno_t vRegNum) const {
    /* 0 is not allowed for preg index */
    uint32 n = static_cast<uint32>(vRegNum);
    return (kBaseVirtualRegNO < n && n < firstNonPregVRegNO);
  }

  PregIdx VRegNOToPRegIdx(regno_t vRegNum) const {
    auto it = vregsToPregsMap.find(vRegNum);
    if (it == vregsToPregsMap.end()) {
      return PregIdx(-1);
    }
    return it->second;
  }

  VirtualRegNode &GetVirtualRegNodeFromPseudoRegIdx(PregIdx idx) {
    return vReg.VRegTableElementGet(GetVirtualRegNOFromPseudoRegIdx(idx));
  }

  PrimType GetTypeFromPseudoRegIdx(PregIdx idx) {
    VirtualRegNode &vRegNode = GetVirtualRegNodeFromPseudoRegIdx(idx);
    RegType regType = vRegNode.GetType();
    ASSERT(regType == kRegTyInt || regType == kRegTyFloat, "");
    uint32 size = vRegNode.GetSize();  /* in bytes */
    ASSERT(size == sizeof(int32) || size == sizeof(int64), "");
    return (regType == kRegTyInt ? (size == sizeof(int32) ? PTY_i32 : PTY_i64)
                                 : (size == sizeof(float) ? PTY_f32 : PTY_f64));
  }

  int64 GetPseudoRegisterSpillLocation(PregIdx idx) {
    const SymbolAlloc *symLoc = memLayout->GetSpillLocOfPseduoRegister(idx);
    return static_cast<int64>(GetBaseOffset(*symLoc));
  }

  virtual MemOperand *GetPseudoRegisterSpillMemoryOperand(PregIdx idx) = 0;

  uint32 GetSpillLocation(uint32 size) {
    uint32 offset = RoundUp(nextSpillLocation, static_cast<uint64>(size));
    nextSpillLocation = offset + size;
    return offset;
  }

  /* See if the symbol is a structure parameter that requires a copy. */
  bool IsParamStructCopy(const MIRSymbol &symbol) {
    if (symbol.GetStorageClass() == kScFormal &&
        GetBecommon().GetTypeSize(symbol.GetTyIdx().GetIdx()) > k16ByteSize) {
      return true;
    }
    return false;
  }

  PrimType GetPrimTypeFromSize(uint32 byteSize, PrimType defaultType) const {
    switch (byteSize) {
      case 1:
        return PTY_i8;
      case 2:
        return PTY_i16;
      case 4:
        return PTY_i32;
      default:
        return defaultType;
    }
  }

  BB *CreateAtomicBuiltinBB(bool isBBIf = true) {
    LabelIdx atomicBBLabIdx = CreateLabel();
    BB *atomicBB = CreateNewBB();
    if (isBBIf) {
      atomicBB->SetKind(BB::kBBIf);
    }
    atomicBB->SetAtomicBuiltIn();
    atomicBB->AddLabel(atomicBBLabIdx);
    SetLab2BBMap(static_cast<int32>(atomicBBLabIdx), *atomicBB);
    GetCurBB()->AppendBB(*atomicBB);
    return atomicBB;
  }

 private:
  CGFunc &operator=(const CGFunc &cgFunc);
  CGFunc(const CGFunc&);
  StmtNode *HandleFirstStmt();
  bool CheckSkipMembarOp(const StmtNode &stmt);
  MIRFunction &func;
  EHFunc *ehFunc = nullptr;

  InsnBuilder *insnBuilder = nullptr;
  OperandBuilder *opndBuilder = nullptr;

  uint32 bbCnt = 0;
  uint32 labelIdx = 0;          /* local label index number */
  LabelNode *startLabel = nullptr;    /* start label of the function */
  LabelNode *returnLabel = nullptr;   /* return label of the function */
  LabelNode *cleanupLabel = nullptr;  /* label to indicate the entry of cleanup code. */
  LabelNode *endLabel = nullptr;      /* end label of the function */

  BB *firstBB = nullptr;
  BB *prologureBB = nullptr;  /* the BB to placing prologure's instructions(callee save/cfi) */
  BB *returnBB = nullptr;
  BB *cleanupBB = nullptr;
  BB *lastBB = nullptr;
  BB *curBB = nullptr;
  BB *dummyBB;   /* use this bb for add some instructions to bb that is no curBB. */
  BB *commonExitBB = nullptr;  /* this post-dominate all BBs */

  Insn *volReleaseInsn = nullptr;  /* use to record the release insn for volatile strore */
  MapleVector<BB*> exitBBVec;
  MapleVector<BB*> noReturnCallBBVec;
  MapleSet<regno_t> extendSet;  /* use to mark regs which spilled 32 bits but loaded 64 bits. */
  MapleUnorderedMap<LabelIdx, BB*> lab2BBMap;
  BECommon &beCommon;
  MemLayout *memLayout = nullptr;
  RegisterInfo *targetRegInfo = nullptr;
  MapleAllocator *funcScopeAllocator;
  MapleMap<uint32, MIRSymbol*> emitStVec;  /* symbol that needs to be emit as a local symbol. i.e, switch table */
  MapleUnorderedMap<LabelIdx, int32> switchLabelCnt;  /* label in switch table */
#if defined(TARGARM32) && TARGARM32
  MapleVector<BB*> sortedBBs;
  MapleVector<LiveRange*> lrVec;
#endif  /* TARGARM32 */
  MapleVector<CGFuncLoops*> loops;
  MapleVector<LmbcFormalParamInfo*> lmbcParamVec;
  CGCFG *theCFG = nullptr;
  MapleSet<uint32> scpIdSet;
  const MapleString shortFuncName;
  int32 lmbcIntArgs = 0;
  int32 lmbcFpArgs = 0;
  uint32 lmbcTotalArgs = 0;
  uint32 nextSpillLocation = 0;
  regno_t spSaveReg = 0;

  bool hasAsm = false;
  bool useFP = true;
  bool seenFP = true;

  /* save stack protect kinds which can trigger stack protect */
  uint8 stackProtectInfo = 0;
  bool needStackProtect = false;
  uint32 priority = 0;

  /* cross reference isel class pointer */
  MPISel *isel = nullptr;
};  /* class CGFunc */

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgLayoutFrame, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgHandleFunction, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPatchLongBranch, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgFixCFLocOsft, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgGenCfi, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgEmission, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgGenProEpiLog, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgIsolateFastPath, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_CGFUNC_H */
