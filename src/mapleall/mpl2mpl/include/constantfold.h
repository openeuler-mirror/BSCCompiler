/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_CONSTANTFOLD_H
#define MPL2MPL_INCLUDE_CONSTANTFOLD_H
#include "mir_nodes.h"
#include "phase_impl.h"
#include "me_verify.h"

#include <optional>

namespace maple {
class ConstantFold : public FuncOptimizeImpl {
 public:
  struct CFConfig {
    bool expandIaddrof;
  };

  ConstantFold(MIRModule &mod, KlassHierarchy *kh, bool trace)
      : FuncOptimizeImpl(mod, kh, trace), mirModule(&mod), cfc({false}) {}

  explicit ConstantFold(MIRModule &mod, CFConfig conf = { false })
      : FuncOptimizeImpl(mod, nullptr, false), mirModule(&mod), cfc(conf) {}

  // Fold an expression.
  // It returns a new expression if there was something to fold, or
  // nullptr otherwise.
  BaseNode *Fold(BaseNode *node);

  // Simplify a statement
  // It returns the original statement or the changed statement if a
  // simplification happened. If the statement can be deleted after a
  // simplification, it returns nullptr.
  StmtNode *Simplify(StmtNode *node);
  StmtNode *SimplifyIassignWithAddrofBaseNode(IassignNode &node, const AddrofNode &base) const;

  FuncOptimizeImpl *Clone() override {
    return new ConstantFold(*this);
  }

  void ProcessFunc(MIRFunction *func) override;
  ~ConstantFold() override {
    mirModule = nullptr;
  }

  template <class T> T CalIntValueFromFloatValue(T value, const MIRType &resultType) const;
  MIRConst *FoldFloorMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType, bool isFloor = true) const;
  MIRConst *FoldRoundMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const;
  MIRConst *FoldTypeCvtMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const;
  MIRConst *FoldSignExtendMIRConst(Opcode opcode, PrimType resultType, uint8 size, const IntVal &val) const;
  static MIRConst *FoldIntConstBinaryMIRConst(Opcode opcode, PrimType resultType, const MIRIntConst &intConst0,
                                              const MIRIntConst &intConst1);
  MIRConst *FoldConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                        const MIRConst &const0, const MIRConst &const1) const;
  static bool IntegerOpIsOverflow(Opcode op, PrimType primType, int64 cstA, int64 cstB);
  static MIRIntConst *FoldIntConstUnaryMIRConst(Opcode opcode, PrimType resultType, const MIRIntConst *constNode);

  static MeExpr *FoldCmpExpr(IRMap &irMap, const MeExpr &cmp1, const MeExpr &cmp2, bool isAnd);
  static MeExpr *FoldOrOfAnds(IRMap &irMap, const MeExpr &and1, const MeExpr &and2);
 private:
  StmtNode *SimplifyBinary(BinaryStmtNode *node);
  StmtNode *SimplifyBlock(BlockNode *node);
  StmtNode *SimplifyCondGoto(CondGotoNode *node);
  StmtNode *SimplifyCondGotoSelect(CondGotoNode *node) const;
  StmtNode *SimplifyDassign(DassignNode *node);
  StmtNode *SimplifyIassignWithIaddrofBaseNode(IassignNode &node, const IaddrofNode &base);
  StmtNode *SimplifyIassign(IassignNode *node);
  StmtNode *SimplifyNary(NaryStmtNode *node);
  StmtNode *SimplifyIcall(IcallNode *node);
  StmtNode *SimplifyIf(IfStmtNode *node);
  StmtNode *SimplifySwitch(SwitchNode *node);
  StmtNode *SimplifyUnary(UnaryStmtNode *node);
  StmtNode *SimplifyAsm(AsmNode* node);
  StmtNode *SimplifyWhile(WhileStmtNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldArray(ArrayNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldBase(BaseNode *node) const;
  std::pair<BaseNode*, std::optional<IntVal>> FoldBinary(BinaryNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldCompare(CompareNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldDepositbits(DepositbitsNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldExtractbits(ExtractbitsNode *node);
  ConstvalNode *FoldSignExtend(Opcode opcode, PrimType resultType, uint8 size, const ConstvalNode &cst) const;
  std::pair<BaseNode*, std::optional<IntVal>> FoldIread(IreadNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldSizeoftype(SizeoftypeNode *node) const;
  std::pair<BaseNode*, std::optional<IntVal>> FoldRetype(RetypeNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldGcmallocjarray(JarrayMallocNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldUnary(UnaryNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldTernary(TernaryNode *node);
  std::pair<BaseNode*, std::optional<IntVal>> FoldTypeCvt(TypeCvtNode *node);
  ConstvalNode *FoldCeil(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
  ConstvalNode *FoldFloor(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
  ConstvalNode *FoldRound(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
  ConstvalNode *FoldTrunk(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
  ConstvalNode *FoldTypeCvt(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
  ConstvalNode *FoldConstComparison(Opcode opcode, PrimType resultType, PrimType opndType, const ConstvalNode &const0,
                                    const ConstvalNode &const1) const;
  ConstvalNode *FoldConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                const ConstvalNode &const1) const;
  ConstvalNode *FoldIntConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                       const ConstvalNode &const0, const ConstvalNode &const1) const;
  MIRIntConst *FoldIntConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                              const MIRIntConst &intConst0, const MIRIntConst &intConst1) const;
  ConstvalNode *FoldIntConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                   const ConstvalNode &const1) const;
  ConstvalNode *FoldFPConstComparison(Opcode opcode, PrimType resultType, PrimType opndType, const ConstvalNode &const0,
                                      const ConstvalNode &const1) const;
  bool ConstValueEqual(int64 leftValue, int64 rightValue) const;
  bool ConstValueEqual(float leftValue, float rightValue) const;
  bool ConstValueEqual(double leftValue, double rightValue) const;
  template<typename T>
  bool FullyEqual(T leftValue, T rightValue) const;
  template<typename T>
  int64 ComparisonResult(Opcode op, T *leftConst, T *rightConst) const;
  MIRIntConst *FoldFPConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                             const MIRConst &leftConst, const MIRConst &rightConst) const;
  ConstvalNode *FoldFPConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                  const ConstvalNode &const1) const;
  ConstvalNode *FoldConstUnary(Opcode opcode, PrimType resultType, ConstvalNode &constNode) const;
  template <typename T>
  ConstvalNode *FoldFPConstUnary(Opcode opcode, PrimType resultType, ConstvalNode *constNode) const;
  template <>
  ConstvalNode *FoldFPConstUnary<MIRFloat128Const>(Opcode opcode, PrimType resultType, ConstvalNode *constNode) const;
  BaseNode *NegateTree(BaseNode *node) const;
  BaseNode *Negate(BaseNode *node) const;
  BaseNode *Negate(UnaryNode *node) const;
  BaseNode *Negate(const ConstvalNode *node) const;
  BinaryNode *NewBinaryNode(BinaryNode *old, Opcode op, PrimType primType, BaseNode *lhs, BaseNode *rhs) const;
  UnaryNode *NewUnaryNode(UnaryNode *old, Opcode op, PrimType primType, BaseNode *expr) const;
  std::pair<BaseNode*, std::optional<IntVal>> DispatchFold(BaseNode *node);
  BaseNode *PairToExpr(PrimType resultType, const std::pair<BaseNode*, std::optional<IntVal>> &pair) const;
  BaseNode *SimplifyDoubleCompare(CompareNode &compareNode) const;
  CompareNode *FoldConstComparisonReverse(Opcode opcode, PrimType resultType, PrimType opndType,
                                          BaseNode &l, BaseNode &r) const;
  MIRModule *mirModule;
  CFConfig cfc;
};

MAPLE_MODULE_PHASE_DECLARE(M2MConstantFold)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CONSTANTFOLD_H
