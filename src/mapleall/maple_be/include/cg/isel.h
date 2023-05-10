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

#ifndef MAPLEBE_INCLUDE_CG_ISEL_H
#define MAPLEBE_INCLUDE_CG_ISEL_H

#include "cgfunc.h"

namespace maplebe {
struct MirTypeInfo {
  PrimType primType;
  int32 offset = 0;
  uint32 size = 0;  /* for aggType */
};
/* macro expansion instruction selection */
class MPISel {
 public:
  MPISel(MemPool &mp, AbstractIRBuilder &aIRBuilder, CGFunc &f) : isMp(&mp), cgFunc(&f), aIRBuilder(aIRBuilder) {}

  virtual ~MPISel() {
    isMp = nullptr;
    cgFunc = nullptr;
  }

  void DoMPIS();

  CGFunc *GetCurFunc() const {
    return cgFunc;
  }

  Operand *HandleExpr(const BaseNode &parent, BaseNode &expr);

  void SelectDassign(const DassignNode &stmt, Operand &opndRhs);
  void SelectDassignoff(const DassignoffNode &stmt, Operand &opnd0);
  void SelectIassign(const IassignNode &stmt, Operand &opndAddr, Operand &opndRhs);
  void SelectIassignoff(const IassignoffNode &stmt);
  RegOperand *SelectRegread(RegreadNode &expr);
  void SelectRegassign(RegassignNode &stmt, Operand &opnd0);
  Operand* SelectDread(const BaseNode &parent, const AddrofNode &expr);
  Operand* SelectBand(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand* SelectAdd(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand* SelectSub(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand* SelectNeg(const UnaryNode &node, Operand &opnd0, const BaseNode &parent);
  Operand* SelectCvt(const BaseNode &parent, const TypeCvtNode &node, Operand &opnd0);
  Operand *SelectDepositBits(const DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  virtual Operand* SelectExtractbits(const BaseNode &parent, ExtractbitsNode &node, Operand &opnd0);
  virtual Operand *SelectAbs(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  Operand *SelectAlloca(UnaryNode &node, Operand &opnd0);
  Operand *SelectCGArrayElemAdd(const BinaryNode &node);
  ImmOperand *SelectIntConst(const MIRIntConst &intConst, PrimType primType) const;
  void SelectCallCommon(StmtNode &stmt, const MPISel &iSel) const;
  void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  Operand *SelectShift(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  void SelectShift(Operand &resOpnd, Operand &opnd0, Operand &opnd1, Opcode shiftDirect,
                   PrimType opnd0Type, PrimType opnd1Type);
  void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  virtual void SelectReturn(NaryStmtNode &retNode) = 0;
  virtual void SelectReturn(bool noOpnd) = 0;
  virtual void SelectIntAggCopyReturn(MemOperand &symbolMem, uint64 aggSize) = 0;
  virtual void SelectAggIassign(IassignNode &stmt, Operand &addrOpnd, Operand &opndRhs) = 0;
  virtual void SelectAggCopy(MemOperand &lhs, MemOperand &rhs, uint32 copySize) = 0;
  virtual void SelectGoto(GotoNode &stmt) = 0;
  virtual void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) = 0;
  virtual void SelectIgoto(Operand &opnd0) = 0;
  virtual void SelectCall(CallNode &callNode) = 0;
  virtual void SelectIcall(IcallNode &icallNode, Operand &opnd0) = 0;
  virtual void SelectIntrinCall(IntrinsiccallNode &intrinsiccallNode) = 0;
  virtual Operand *SelectBswap(IntrinsicopNode &node, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectFloatingConst(MIRConst &floatingConst, PrimType primType, const BaseNode &parent) const = 0;
  virtual Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent) = 0;
  virtual Operand *SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) = 0;
  virtual Operand *SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) = 0;
  virtual Operand &ProcessReturnReg(PrimType primType, int32 sReg) = 0 ;
  virtual void SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode) = 0;
  Operand *SelectBior(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand *SelectBxor(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand *SelectIread(const BaseNode &parent, const IreadNode &expr, int extraOffset = 0);
  Operand *SelectIreadoff(const BaseNode &parent, const IreadoffNode &ireadoff);
  virtual Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectSelect(TernaryNode &expr, Operand &cond, Operand &trueOpnd, Operand &falseOpnd,
                                const BaseNode &parent) = 0;
  virtual Operand *SelectStrLiteral(ConststrNode &constStr) = 0;
  virtual Operand *SelectCclz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCctz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCsin(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCsinh(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCasin(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCcos(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCcosh(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCacos(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCatan(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCexp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectClog(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectClog10(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCsinf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCsinhf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCasinf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCcosf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCcoshf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCacosf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCatanf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCexpf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectClogf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectClog10f(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCffs(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCmemcmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCstrlen(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCstrcmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCstrncmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCstrchr(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual Operand *SelectCstrrchr(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
  virtual void SelectAsm(AsmNode &node) = 0;
  virtual void SelectAggDassign(MirTypeInfo &lhsInfo, MemOperand &symbolMem, Operand &rOpnd, const DassignNode &s) = 0;
  Operand *SelectBnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent);
  Operand *SelectMin(const BinaryNode &node, Operand &opnd0, Operand &opnd1);
  Operand *SelectMax(const BinaryNode &node, Operand &opnd0, Operand &opnd1);
  Operand *SelectRetype(const TypeCvtNode &node, Operand &opnd0);
  void SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);

  template <typename T>
  Operand *SelectLiteral(T &c, MIRFunction &func, uint32 labelIdx) const {
    MIRSymbol *st = func.GetSymTab()->CreateSymbol(kScopeLocal);
    std::string lblStr(".LB_");
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
    std::string funcName = funcSt->GetName();
    (void)lblStr.append(funcName).append(std::to_string(labelIdx));
    st->SetNameStrIdx(lblStr);
    st->SetStorageClass(kScPstatic);
    st->SetSKind(kStConst);
    st->SetKonst(&c);
    PrimType primType = c.GetType().GetPrimType();
    st->SetTyIdx(TyIdx(primType));
    uint32 typeBitSize = GetPrimTypeBitSize(primType);

    if (cgFunc->GetMirModule().IsCModule() && (T::GetPrimType() == PTY_f32 || T::GetPrimType() == PTY_f64)) {
      return &GetOrCreateMemOpndFromSymbol(*st, typeBitSize, 0);
    }
    CHECK_FATAL(false, "NIY");
    return nullptr;
  }
 protected:
  MemPool *isMp;
  CGFunc *cgFunc;
  void SelectCopy(Operand &dest, Operand &src, PrimType toType, PrimType fromType = PTY_unknown);
  RegOperand &SelectCopy2Reg(Operand &src, PrimType toType, PrimType fromType = PTY_unknown);
  void SelectIntCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
  void SelectCvtInt2Float(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
  void SelectFloatCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
  void SelectCvtFloat2Int(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
  PrimType GetIntegerPrimTypeFromSize(bool isSigned, uint32 bitSize) const;
  std::pair<FieldID, MIRType*> GetFieldIdAndMirTypeFromMirNode(const BaseNode &node);
  MirTypeInfo GetMirTypeInfoFormFieldIdAndMirType(FieldID fieldId, MIRType *mirType);
  MirTypeInfo GetMirTypeInfoFromMirNode(const BaseNode &node);
  MemOperand *GetOrCreateMemOpndFromIreadNode(const IreadNode &expr, PrimType primType, int offset);

  virtual void SelectCvtFloat2Float(Operand &resOpnd, Operand &srcOpnd, PrimType fromType, PrimType toType) {
    CHECK_FATAL(false, "NYI");
  }
  virtual void SelectCvtFloat2Int(Operand &resOpnd, Operand &srcOpnd, PrimType itype, PrimType ftype) {
    CHECK_FATAL(false, "NYI");
  }
  virtual bool IsSymbolRequireIndirection(const MIRSymbol &symbol) {
    return false;
  }
 private:
  StmtNode *HandleFuncEntry() const;
  void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opndRhs);
  void SelectDassignStruct(MIRSymbol &symbol, MemOperand &symbolMem, Operand &opndRhs);
  virtual void HandleFuncExit() const = 0;
  virtual MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId = 0,
                                                   RegOperand *baseReg = nullptr) = 0;
  virtual MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, uint32 opndSize, int64 offset) const = 0;
  virtual Operand &GetTargetRetOperand(PrimType primType, int32 sReg) = 0;
  void SelectBasicOp(Operand &resOpnd, Operand &opnd0, Operand &opnd1, MOperator mOp, PrimType primType);
  /*
   * Support conversion between all types and registers
   * only Support conversion between registers and memory
   * alltypes -> reg -> mem
   */
  void SelectCopyInsn(Operand &dest, Operand &src, PrimType type);
  void SelectNeg(Operand &resOpnd, Operand &opnd0, PrimType primType) const;
  void SelectBnot(Operand &resOpnd, Operand &opnd0, PrimType primType) const;
  void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectExtractbits(RegOperand &resOpnd, RegOperand &opnd0, uint8 bitOffset, uint8 bitSize, PrimType primType);
  virtual RegOperand &GetTargetBasicPointer(PrimType primType) = 0;
  virtual RegOperand &GetTargetStackPointer(PrimType primType) = 0;
  void SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  virtual void SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;

  AbstractIRBuilder &aIRBuilder;
};
MAPLE_FUNC_PHASE_DECLARE(InstructionSelector, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_BEGIN(AbstractBuilder, maplebe::CGFunc)
  AbstractIRBuilder *GetResult() {
    return aIRBuilder;
  }
  AbstractIRBuilder  *aIRBuilder = nullptr;
MAPLE_FUNC_PHASE_DECLARE_END
}
#endif  /* MAPLEBE_INCLUDE_CG_ISEL_H */
