/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved
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
#include "cgfunc.h"
#if DEBUG
#include <iomanip>
#endif
#include "cg.h"
#include "insn.h"
#include "loop.h"
#include "mir_builder.h"
#include "factory.h"
#include "debug_info.h"

namespace maplebe {
using namespace maple;

#define JAVALANG (GetMirModule().IsJavaModule())

Operand *HandleDread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &dreadNode = static_cast<AddrofNode&>(expr);
  return cgFunc.SelectDread(parent, dreadNode);
}

Operand *HandleRegread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &regReadNode = static_cast<RegreadNode&>(expr);
  if (regReadNode.GetRegIdx() == -kSregRetval0 || regReadNode.GetRegIdx() == -kSregRetval1) {
    return &cgFunc.ProcessReturnReg(regReadNode.GetPrimType(), -(regReadNode.GetRegIdx()));
  }
  return cgFunc.SelectRegread(regReadNode);
}

Operand *HandleConstVal(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &constValNode = static_cast<ConstvalNode&>(expr);
  MIRConst *mirConst = constValNode.GetConstVal();
  ASSERT(mirConst != nullptr, "get constval of constvalnode failed");
  if (mirConst->GetKind() == kConstInt) {
    auto *mirIntConst = safe_cast<MIRIntConst>(mirConst);
    return cgFunc.SelectIntConst(*mirIntConst);
  } else if (mirConst->GetKind() == kConstFloatConst) {
    auto *mirFloatConst = safe_cast<MIRFloatConst>(mirConst);
    return cgFunc.SelectFloatConst(*mirFloatConst);
  } else if (mirConst->GetKind() == kConstDoubleConst) {
    auto *mirDoubleConst = safe_cast<MIRDoubleConst>(mirConst);
    return cgFunc.SelectDoubleConst(*mirDoubleConst);
  } else {
    CHECK_FATAL(false, "NYI");
  }
  return nullptr;
}

Operand *HandleConstStr(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &constStrNode = static_cast<ConststrNode&>(expr);
#if TARGAARCH64 || TARGRISCV64
  return cgFunc.SelectStrConst(*cgFunc.GetMemoryPool()->New<MIRStrConst>(
      constStrNode.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)PTY_a64)));
#else
  return cgFunc.SelectStrConst(*cgFunc.GetMemoryPool()->New<MIRStrConst>(
      constStrNode.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)PTY_a32)));
#endif
}

Operand *HandleConstStr16(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &constStr16Node = static_cast<Conststr16Node&>(expr);
#if TARGAARCH64 || TARGRISCV64
  return cgFunc.SelectStr16Const(*cgFunc.GetMemoryPool()->New<MIRStr16Const>(
      constStr16Node.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)PTY_a64)));
#else
  return cgFunc.SelectStr16Const(*cgFunc.GetMemoryPool()->New<MIRStr16Const>(
      constStr16Node.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)PTY_a32)));
#endif
}

Operand *HandleAdd(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 && expr.Opnd(0)->GetOpCode() == OP_mul &&
      !IsPrimitiveFloat(expr.GetPrimType()) && expr.Opnd(0)->Opnd(0)->GetOpCode() != OP_constval &&
      expr.Opnd(0)->Opnd(1)->GetOpCode() != OP_constval) {
    return cgFunc.SelectMadd(static_cast<BinaryNode&>(expr),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(0)->Opnd(0)),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(0)->Opnd(1)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
  } else if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 && expr.Opnd(1)->GetOpCode() == OP_mul &&
             !IsPrimitiveFloat(expr.GetPrimType()) && expr.Opnd(1)->Opnd(0)->GetOpCode() != OP_constval &&
             expr.Opnd(1)->Opnd(1)->GetOpCode() != OP_constval) {
    return cgFunc.SelectMadd(static_cast<BinaryNode&>(expr),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(1)->Opnd(0)),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(1)->Opnd(1)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
  } else {
    return cgFunc.SelectAdd(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
  }
}

Operand *HandleCGArrayElemAdd(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return &cgFunc.SelectCGArrayElemAdd(static_cast<BinaryNode&>(expr));
}

Operand *HandleShift(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectShift(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleMpy(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectMpy(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleDiv(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectDiv(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleRem(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectRem(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleAddrof(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &addrofNode = static_cast<AddrofNode&>(expr);
  return cgFunc.SelectAddrof(addrofNode);
}

Operand *HandleAddroffunc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &addroffuncNode = static_cast<AddroffuncNode&>(expr);
  return &cgFunc.SelectAddrofFunc(addroffuncNode);
}

Operand *HandleAddrofLabel(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &addrofLabelNode = static_cast<AddroflabelNode&>(expr);
  return &cgFunc.SelectAddrofLabel(addrofLabelNode);
}

Operand *HandleIread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &ireadNode = static_cast<IreadNode&>(expr);
  return cgFunc.SelectIread(parent, ireadNode);
}

Operand *HandleSub(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectSub(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleBand(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectBand(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleBior(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectBior(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleBxor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectBxor(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleAbs(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectAbs(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleBnot(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectBnot(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleExtractBits(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectExtractbits(static_cast<ExtractbitsNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleDepositBits(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectDepositBits(static_cast<DepositbitsNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                                  *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleLnot(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectLnot(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleLand(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectLand(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleLor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  if (parent.IsCondBr()) {
    return cgFunc.SelectLor(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), true);
  } else {
    return cgFunc.SelectLor(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
  }
}

Operand *HandleMin(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectMin(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleMax(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectMax(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)));
}

Operand *HandleNeg(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectNeg(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleRecip(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectRecip(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleSqrt(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectSqrt(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleCeil(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectCeil(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleFloor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectFloor(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleRetype(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectRetype(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleCvt(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectCvt(parent, static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleRound(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectRound(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleTrunc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectTrunc(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleSelect(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  /* 0,1,2 represent the first opnd and the second opnd and the third opnd of expr */
  return cgFunc.SelectSelect(static_cast<TernaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(1)), *cgFunc.HandleExpr(expr, *expr.Opnd(2)));
}

Operand *HandleCmp(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectCmpOp(static_cast<CompareNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleAlloca(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectAlloca(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleMalloc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectMalloc(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleGCMalloc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectGCMalloc(static_cast<GCMallocNode&>(expr));
}

Operand *HandleJarrayMalloc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectJarrayMalloc(static_cast<JarrayMallocNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

/* Neon intrinsic handling */
Operand *HandleVectorFromScalar(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  return cgFunc.SelectVectorFromScalar(intrnNode.GetPrimType(), cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0)),
                                       intrnNode.Opnd(0)->GetPrimType());
}

Operand *HandleVectorMerge(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand2 */
  BaseNode *index = intrnNode.Opnd(2);                                 /* index operand */
  int32 iNum = 0;
  if (index->GetOpCode() == OP_constval) {
    MIRConst *mirConst = static_cast<ConstvalNode*>(index)->GetConstVal();
    iNum = safe_cast<MIRIntConst>(mirConst)->GetValue();
    PrimType ty = intrnNode.Opnd(0)->GetPrimType();
    iNum *= GetPrimTypeSize(ty) / GetPrimTypeLanes(ty);                /* 64x2: 0-1 -> 0-8 */
  } else {                                                             /* 32x4: 0-3 -> 0-12 */
    CHECK_FATAL(0, "VectorMerge does not have const index");
  }
  return cgFunc.SelectVectorMerge(intrnNode.GetPrimType(), opnd1, opnd2, iNum);
}

Operand *HandleVectorGetHigh(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  return cgFunc.SelectVectorGetHigh(rType, opnd1);
}

Operand *HandleVectorGetLow(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  return cgFunc.SelectVectorGetLow(rType, opnd1);
}

Operand *HandleVectorGetElement(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  PrimType o1Type = intrnNode.Opnd(0)->GetPrimType();
  Operand *opndLane = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));
  int32 laneNum = -1;
  if (opndLane->IsConstImmediate()) {
    MIRConst *mirConst = static_cast<ConstvalNode*>(intrnNode.Opnd(1))->GetConstVal();
    laneNum = safe_cast<MIRIntConst>(mirConst)->GetValue();
  } else {
    CHECK_FATAL(0, "VectorGetElement does not have lane const");
  }
  return cgFunc.SelectVectorGetElement(intrnNode.GetPrimType(), opnd1, o1Type, laneNum);
}

Operand *HandleVectorPairwiseAdd(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  Operand *src = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));     /* vector src operand*/
  PrimType sType = intrnNode.Opnd(0)->GetPrimType();
  return cgFunc.SelectVectorPairwiseAdd(intrnNode.GetPrimType(), src, sType);
}

Operand *HandleVectorSetElement(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  BaseNode *arg0 = intrnNode.Opnd(0);                                  /* uint32_t operand */
  Operand *opnd0 = cgFunc.HandleExpr(intrnNode, *arg0);
  PrimType aType = arg0->GetPrimType();
  //ASSERT(GetPrimTypeBitSize(aType) <= k32BitSize, "VectorSetElement: invalid opnd0");

  BaseNode *arg1 = intrnNode.Opnd(1);                                  /* vector operand == result */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *arg1);
  PrimType vType = arg1->GetPrimType();

  BaseNode *arg2 = intrnNode.Opnd(2);                                  /* lane const operand */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *arg2);
  int32 laneNum = -1;
  if (opnd2->IsConstImmediate()) {
    MIRConst *mirConst = static_cast<ConstvalNode*>(arg2)->GetConstVal();
    laneNum = safe_cast<MIRIntConst>(mirConst)->GetValue();
  } else {
    CHECK_FATAL(0, "VectorSetElement does not have lane const");
  }
  return cgFunc.SelectVectorSetElement(opnd0, aType, opnd1, vType, laneNum);
}

Operand *HandleVectorReverse(IntrinsicopNode &intrnNode, CGFunc &cgFunc, uint32 size) {
  BaseNode *argExpr = intrnNode.Opnd(0);                               /* src operand */
  PrimType sType = argExpr->GetPrimType();
  Operand *src = cgFunc.HandleExpr(intrnNode, *argExpr);
  return cgFunc.SelectVectorReverse(intrnNode.GetPrimType(), src, sType, size);
}

Operand *HandleVectorShiftNarrow(IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow) {
  PrimType rType = intrnNode.GetPrimType();                          /* vector result */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* shift const */
  int32 sVal;
  if (opnd2->IsConstImmediate()) {
    sVal = static_cast<ImmOperand*>(opnd2)->GetValue();
  } else {
    CHECK_FATAL(0, "VectorShiftNarrow does not have shift const");
  }
  return cgFunc.SelectVectorShiftRNarrow(rType, opnd1, intrnNode.Opnd(0)->GetPrimType(), opnd2, isLow);
}

Operand *HandleVectorSum(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType resType = intrnNode.GetPrimType();                          /* uint32_t result */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  return cgFunc.SelectVectorSum(resType, opnd1, intrnNode.Opnd(0)->GetPrimType());
}

Operand *HandleVectorTableLookup(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand 1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand 2 */
  return cgFunc.SelectVectorTableLookup(rType, opnd1, opnd2);
}

Operand *HandleVectorMadd(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand 1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand 2 */
  Operand *opnd3 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(2));   /* vector operand 3 */
  PrimType oTyp1 = intrnNode.Opnd(0)->GetPrimType();
  PrimType oTyp2 = intrnNode.Opnd(1)->GetPrimType();
  PrimType oTyp3 = intrnNode.Opnd(2)->GetPrimType();
  return cgFunc.SelectVectorMadd(opnd1, oTyp1, opnd2, oTyp2, opnd3, oTyp3);
}

Operand *HandleVectorMull(IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand 1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand 2 */
  PrimType oTyp1 = intrnNode.Opnd(0)->GetPrimType();
  PrimType oTyp2 = intrnNode.Opnd(1)->GetPrimType();
  return cgFunc.SelectVectorMull(rType, opnd1, oTyp1, opnd2, oTyp2);
}

Operand *HandleVectorNarrow(IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand 1 */
  return cgFunc.SelectVectorNarrow(rType, opnd1, intrnNode.Opnd(0)->GetPrimType(), isLow);
}

Operand *HandleIntrinOp(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &intrinsicopNode = static_cast<IntrinsicopNode&>(expr);
  switch (intrinsicopNode.GetIntrinsic()) {
    case INTRN_MPL_READ_OVTABLE_ENTRY_LAZY: {
      Operand *srcOpnd = cgFunc.HandleExpr(intrinsicopNode, *intrinsicopNode.Opnd(0));
      return cgFunc.SelectLazyLoad(*srcOpnd, intrinsicopNode.GetPrimType());
    }
    case INTRN_MPL_READ_STATIC_OFFSET_TAB: {
      auto addrOfNode = static_cast<AddrofNode*>(intrinsicopNode.Opnd(0));
      MIRSymbol *st = cgFunc.GetMirModule().CurFunction()->GetLocalOrGlobalSymbol(addrOfNode->GetStIdx());
      auto constNode = static_cast<ConstvalNode*>(intrinsicopNode.Opnd(1));
      CHECK_FATAL(constNode != nullptr, "null ptr check");
      auto mirIntConst = static_cast<MIRIntConst*>(constNode->GetConstVal());
      return cgFunc.SelectLazyLoadStatic(*st, mirIntConst->GetValue(), intrinsicopNode.GetPrimType());
    }
    case INTRN_MPL_READ_ARRAYCLASS_CACHE_ENTRY: {
      auto addrOfNode = static_cast<AddrofNode*>(intrinsicopNode.Opnd(0));
      MIRSymbol *st = cgFunc.GetMirModule().CurFunction()->GetLocalOrGlobalSymbol(addrOfNode->GetStIdx());
      auto constNode = static_cast<ConstvalNode*>(intrinsicopNode.Opnd(1));
      CHECK_FATAL(constNode != nullptr, "null ptr check");
      auto mirIntConst = static_cast<MIRIntConst*>(constNode->GetConstVal());
      return cgFunc.SelectLoadArrayClassCache(*st, mirIntConst->GetValue(), intrinsicopNode.GetPrimType());
    }
    // double
    case INTRN_C_sin:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "sin");
    case INTRN_C_sinh:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "sinh");
    case INTRN_C_asin:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "asin");
    case INTRN_C_cos:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "cos");
    case INTRN_C_cosh:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "cosh");
    case INTRN_C_acos:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "acos");
    case INTRN_C_atan:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "atan");
    case INTRN_C_exp:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "exp");
    case INTRN_C_log:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "log");
    case INTRN_C_log10:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "log10");
    // float
    case INTRN_C_sinf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "sinf");
    case INTRN_C_sinhf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "sinhf");
    case INTRN_C_asinf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "asinf");
    case INTRN_C_cosf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "cosf");
    case INTRN_C_coshf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "coshf");
    case INTRN_C_acosf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "acosf");
    case INTRN_C_atanf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "atanf");
    case INTRN_C_expf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "expf");
    case INTRN_C_logf:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "logf");
    case INTRN_C_log10f:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "log10f");
    // int
    case INTRN_C_ffs:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "ffs");

    case INTRN_C_clz32:
    case INTRN_C_clz64:
      return cgFunc.SelectCclz(intrinsicopNode);
    case INTRN_C_ctz32:
    case INTRN_C_ctz64:
      return cgFunc.SelectCctz(intrinsicopNode);
    case INTRN_C_popcount32:
    case INTRN_C_popcount64:
      return cgFunc.SelectCpopcount(intrinsicopNode);
    case INTRN_C_parity32:
    case INTRN_C_parity64:
      return cgFunc.SelectCparity(intrinsicopNode);
    case INTRN_C_clrsb32:
    case INTRN_C_clrsb64:
      return cgFunc.SelectCclrsb(intrinsicopNode);
    case INTRN_C_isaligned:
      return cgFunc.SelectCisaligned(intrinsicopNode);
    case INTRN_C_alignup:
      return cgFunc.SelectCalignup(intrinsicopNode);
    case INTRN_C_aligndown:
      return cgFunc.SelectCaligndown(intrinsicopNode);

    case INTRN_vector_sum_v8u8: case INTRN_vector_sum_v8i8:
    case INTRN_vector_sum_v4u16: case INTRN_vector_sum_v4i16:
    case INTRN_vector_sum_v2u32: case INTRN_vector_sum_v2i32:
    case INTRN_vector_sum_v16u8: case INTRN_vector_sum_v16i8:
    case INTRN_vector_sum_v8u16: case INTRN_vector_sum_v8i16:
    case INTRN_vector_sum_v4u32: case INTRN_vector_sum_v4i32:
    case INTRN_vector_sum_v2u64: case INTRN_vector_sum_v2i64:
      return HandleVectorSum(intrinsicopNode, cgFunc);

    case INTRN_vector_from_scalar_v8u8: case INTRN_vector_from_scalar_v8i8:
    case INTRN_vector_from_scalar_v4u16: case INTRN_vector_from_scalar_v4i16:
    case INTRN_vector_from_scalar_v2u32: case INTRN_vector_from_scalar_v2i32:
    case INTRN_vector_from_scalar_v1u64: case INTRN_vector_from_scalar_v1i64:
    case INTRN_vector_from_scalar_v16u8: case INTRN_vector_from_scalar_v16i8:
    case INTRN_vector_from_scalar_v8u16: case INTRN_vector_from_scalar_v8i16:
    case INTRN_vector_from_scalar_v4u32: case INTRN_vector_from_scalar_v4i32:
    case INTRN_vector_from_scalar_v2u64: case INTRN_vector_from_scalar_v2i64:
      return HandleVectorFromScalar(intrinsicopNode, cgFunc);

    case INTRN_vector_merge_v8u8: case INTRN_vector_merge_v8i8:
    case INTRN_vector_merge_v4u16: case INTRN_vector_merge_v4i16:
    case INTRN_vector_merge_v2u32: case INTRN_vector_merge_v2i32:
    case INTRN_vector_merge_v1u64: case INTRN_vector_merge_v1i64:
    case INTRN_vector_merge_v16u8: case INTRN_vector_merge_v16i8:
    case INTRN_vector_merge_v8u16: case INTRN_vector_merge_v8i16:
    case INTRN_vector_merge_v4u32: case INTRN_vector_merge_v4i32:
    case INTRN_vector_merge_v2u64: case INTRN_vector_merge_v2i64:
      return HandleVectorMerge(intrinsicopNode, cgFunc);

    case INTRN_vector_set_element_v8u8: case INTRN_vector_set_element_v8i8:
    case INTRN_vector_set_element_v4u16: case INTRN_vector_set_element_v4i16:
    case INTRN_vector_set_element_v2u32: case INTRN_vector_set_element_v2i32:
    case INTRN_vector_set_element_v1u64: case INTRN_vector_set_element_v1i64:
    case INTRN_vector_set_element_v16u8: case INTRN_vector_set_element_v16i8:
    case INTRN_vector_set_element_v8u16: case INTRN_vector_set_element_v8i16:
    case INTRN_vector_set_element_v4u32: case INTRN_vector_set_element_v4i32:
    case INTRN_vector_set_element_v2u64: case INTRN_vector_set_element_v2i64:
      return HandleVectorSetElement(intrinsicopNode, cgFunc);

    case INTRN_vector_get_high_v16u8: case INTRN_vector_get_high_v16i8:
    case INTRN_vector_get_high_v8u16: case INTRN_vector_get_high_v8i16:
    case INTRN_vector_get_high_v4u32: case INTRN_vector_get_high_v4i32:
    case INTRN_vector_get_high_v2u64: case INTRN_vector_get_high_v2i64:
      return HandleVectorGetHigh(intrinsicopNode, cgFunc);

    case INTRN_vector_get_low_v16u8: case INTRN_vector_get_low_v16i8:
    case INTRN_vector_get_low_v8u16: case INTRN_vector_get_low_v8i16:
    case INTRN_vector_get_low_v4u32: case INTRN_vector_get_low_v4i32:
    case INTRN_vector_get_low_v2u64: case INTRN_vector_get_low_v2i64:
      return HandleVectorGetLow(intrinsicopNode, cgFunc);

    case INTRN_vector_get_element_v8u8: case INTRN_vector_get_element_v8i8:
    case INTRN_vector_get_element_v4u16: case INTRN_vector_get_element_v4i16:
    case INTRN_vector_get_element_v2u32: case INTRN_vector_get_element_v2i32:
    case INTRN_vector_get_element_v1u64: case INTRN_vector_get_element_v1i64:
    case INTRN_vector_get_element_v16u8: case INTRN_vector_get_element_v16i8:
    case INTRN_vector_get_element_v8u16: case INTRN_vector_get_element_v8i16:
    case INTRN_vector_get_element_v4u32: case INTRN_vector_get_element_v4i32:
    case INTRN_vector_get_element_v2u64: case INTRN_vector_get_element_v2i64:
      return HandleVectorGetElement(intrinsicopNode, cgFunc);

    case INTRN_vector_pairwise_add_v8u8: case INTRN_vector_pairwise_add_v8i8:
    case INTRN_vector_pairwise_add_v4u16: case INTRN_vector_pairwise_add_v4i16:
    case INTRN_vector_pairwise_add_v2u32: case INTRN_vector_pairwise_add_v2i32:
    case INTRN_vector_pairwise_add_v16u8: case INTRN_vector_pairwise_add_v16i8:
    case INTRN_vector_pairwise_add_v8u16: case INTRN_vector_pairwise_add_v8i16:
    case INTRN_vector_pairwise_add_v4u32: case INTRN_vector_pairwise_add_v4i32:
      return HandleVectorPairwiseAdd(intrinsicopNode, cgFunc);

    case INTRN_vector_madd_v8u8: case INTRN_vector_madd_v8i8:
    case INTRN_vector_madd_v4u16: case INTRN_vector_madd_v4i16:
    case INTRN_vector_madd_v2u32: case INTRN_vector_madd_v2i32:
      return HandleVectorMadd(intrinsicopNode, cgFunc);

    case INTRN_vector_mul_v8u8: case INTRN_vector_mul_v8i8:
    case INTRN_vector_mul_v4u16: case INTRN_vector_mul_v4i16:
    case INTRN_vector_mul_v2u32: case INTRN_vector_mul_v2i32:
      return HandleVectorMull(intrinsicopNode, cgFunc);

    case INTRN_vector_narrow_low_v8u16: case INTRN_vector_narrow_low_v8i16:
    case INTRN_vector_narrow_low_v4u32: case INTRN_vector_narrow_low_v4i32:
    case INTRN_vector_narrow_low_v2u64: case INTRN_vector_narrow_low_v2i64:
      return HandleVectorNarrow(intrinsicopNode, cgFunc, true);

    case INTRN_vector_reverse_v8u8: case INTRN_vector_reverse_v8i8:
    case INTRN_vector_reverse_v4u16: case INTRN_vector_reverse_v4i16:
    case INTRN_vector_reverse_v16u8: case INTRN_vector_reverse_v16i8:
    case INTRN_vector_reverse_v8u16: case INTRN_vector_reverse_v8i16:
      return HandleVectorReverse(intrinsicopNode, cgFunc, k32BitSize);

    case INTRN_vector_shr_narrow_low_v8u16: case INTRN_vector_shr_narrow_low_v8i16:
    case INTRN_vector_shr_narrow_low_v4u32: case INTRN_vector_shr_narrow_low_v4i32:
    case INTRN_vector_shr_narrow_low_v2u64: case INTRN_vector_shr_narrow_low_v2i64:
      return HandleVectorShiftNarrow(intrinsicopNode, cgFunc, true);

    case INTRN_vector_table_lookup_v8u8: case INTRN_vector_table_lookup_v8i8:
    case INTRN_vector_table_lookup_v16u8: case INTRN_vector_table_lookup_v16i8:
      return HandleVectorTableLookup(intrinsicopNode, cgFunc);

    default:
      ASSERT(false, "Should not reach here.");
      return nullptr;
  }
}

using HandleExprFactory = FunctionFactory<Opcode, maplebe::Operand*, const BaseNode&, BaseNode&, CGFunc&>;
void InitHandleExprFactory() {
  RegisterFactoryFunction<HandleExprFactory>(OP_dread, HandleDread);
  RegisterFactoryFunction<HandleExprFactory>(OP_regread, HandleRegread);
  RegisterFactoryFunction<HandleExprFactory>(OP_constval, HandleConstVal);
  RegisterFactoryFunction<HandleExprFactory>(OP_conststr, HandleConstStr);
  RegisterFactoryFunction<HandleExprFactory>(OP_conststr16, HandleConstStr16);
  RegisterFactoryFunction<HandleExprFactory>(OP_add, HandleAdd);
  RegisterFactoryFunction<HandleExprFactory>(OP_CG_array_elem_add, HandleCGArrayElemAdd);
  RegisterFactoryFunction<HandleExprFactory>(OP_ashr, HandleShift);
  RegisterFactoryFunction<HandleExprFactory>(OP_lshr, HandleShift);
  RegisterFactoryFunction<HandleExprFactory>(OP_shl, HandleShift);
  RegisterFactoryFunction<HandleExprFactory>(OP_mul, HandleMpy);
  RegisterFactoryFunction<HandleExprFactory>(OP_div, HandleDiv);
  RegisterFactoryFunction<HandleExprFactory>(OP_rem, HandleRem);
  RegisterFactoryFunction<HandleExprFactory>(OP_addrof, HandleAddrof);
  RegisterFactoryFunction<HandleExprFactory>(OP_addroffunc, HandleAddroffunc);
  RegisterFactoryFunction<HandleExprFactory>(OP_addroflabel, HandleAddrofLabel);
  RegisterFactoryFunction<HandleExprFactory>(OP_iread, HandleIread);
  RegisterFactoryFunction<HandleExprFactory>(OP_sub, HandleSub);
  RegisterFactoryFunction<HandleExprFactory>(OP_band, HandleBand);
  RegisterFactoryFunction<HandleExprFactory>(OP_bior, HandleBior);
  RegisterFactoryFunction<HandleExprFactory>(OP_bxor, HandleBxor);
  RegisterFactoryFunction<HandleExprFactory>(OP_abs, HandleAbs);
  RegisterFactoryFunction<HandleExprFactory>(OP_bnot, HandleBnot);
  RegisterFactoryFunction<HandleExprFactory>(OP_sext, HandleExtractBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_zext, HandleExtractBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_extractbits, HandleExtractBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_depositbits, HandleDepositBits);
  RegisterFactoryFunction<HandleExprFactory>(OP_lnot, HandleLnot);
  RegisterFactoryFunction<HandleExprFactory>(OP_land, HandleLand);
  RegisterFactoryFunction<HandleExprFactory>(OP_lior, HandleLor);
  RegisterFactoryFunction<HandleExprFactory>(OP_min, HandleMin);
  RegisterFactoryFunction<HandleExprFactory>(OP_max, HandleMax);
  RegisterFactoryFunction<HandleExprFactory>(OP_neg, HandleNeg);
  RegisterFactoryFunction<HandleExprFactory>(OP_recip, HandleRecip);
  RegisterFactoryFunction<HandleExprFactory>(OP_sqrt, HandleSqrt);
  RegisterFactoryFunction<HandleExprFactory>(OP_ceil, HandleCeil);
  RegisterFactoryFunction<HandleExprFactory>(OP_floor, HandleFloor);
  RegisterFactoryFunction<HandleExprFactory>(OP_retype, HandleRetype);
  RegisterFactoryFunction<HandleExprFactory>(OP_cvt, HandleCvt);
  RegisterFactoryFunction<HandleExprFactory>(OP_round, HandleRound);
  RegisterFactoryFunction<HandleExprFactory>(OP_trunc, HandleTrunc);
  RegisterFactoryFunction<HandleExprFactory>(OP_select, HandleSelect);
  RegisterFactoryFunction<HandleExprFactory>(OP_le, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_ge, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_gt, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_lt, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_ne, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_eq, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_cmp, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_cmpl, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_cmpg, HandleCmp);
  RegisterFactoryFunction<HandleExprFactory>(OP_alloca, HandleAlloca);
  RegisterFactoryFunction<HandleExprFactory>(OP_malloc, HandleMalloc);
  RegisterFactoryFunction<HandleExprFactory>(OP_gcmalloc, HandleGCMalloc);
  RegisterFactoryFunction<HandleExprFactory>(OP_gcpermalloc, HandleGCMalloc);
  RegisterFactoryFunction<HandleExprFactory>(OP_gcmallocjarray, HandleJarrayMalloc);
  RegisterFactoryFunction<HandleExprFactory>(OP_gcpermallocjarray, HandleJarrayMalloc);
  RegisterFactoryFunction<HandleExprFactory>(OP_intrinsicop, HandleIntrinOp);
}

void HandleLabel(StmtNode &stmt, CGFunc &cgFunc) {
  ASSERT(stmt.GetOpCode() == OP_label, "error");
  auto &label = static_cast<LabelNode&>(stmt);
  BB *newBB = cgFunc.StartNewBBImpl(false, label);
  newBB->AddLabel(label.GetLabelIdx());
  cgFunc.SetLab2BBMap(newBB->GetLabIdx(), *newBB);
  cgFunc.SetCurBB(*newBB);
}

void HandleGoto(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.UpdateFrequency(stmt);
  auto &gotoNode = static_cast<GotoNode&>(stmt);
  cgFunc.SetCurBBKind(BB::kBBGoto);
  cgFunc.SelectGoto(gotoNode);
  cgFunc.SetCurBB(*cgFunc.StartNewBB(gotoNode));
  ASSERT(&stmt == &gotoNode, "stmt must be same as gotoNoe");

  if ((gotoNode.GetNext() != nullptr) && (gotoNode.GetNext()->GetOpCode() != OP_label)) {
    ASSERT(cgFunc.GetCurBB()->GetPrev()->GetLastStmt() == &stmt, "check the relation between BB and stmt");
  }
}

void HandleIgoto(StmtNode &stmt, CGFunc &cgFunc) {
  auto &igotoNode = static_cast<UnaryStmtNode&>(stmt);
  Operand *targetOpnd = cgFunc.HandleExpr(stmt, *igotoNode.Opnd(0));
  cgFunc.SelectIgoto(targetOpnd);
  cgFunc.SetCurBB(*cgFunc.StartNewBB(igotoNode));
}

void HandleCondbr(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.UpdateFrequency(stmt);
  auto &condGotoNode = static_cast<CondGotoNode&>(stmt);
  BaseNode *condNode = condGotoNode.Opnd(0);
  ASSERT(condNode != nullptr, "expect first operand of cond br");
  Opcode condOp = condGotoNode.GetOpCode();
  if (condNode->GetOpCode() == OP_constval) {
    auto *constValNode = static_cast<ConstvalNode*>(condNode);
    if ((constValNode->GetConstVal()->IsZero() && (OP_brfalse == condOp)) ||
        (!constValNode->GetConstVal()->IsZero() && (OP_brtrue == condOp))) {
      auto *gotoStmt = cgFunc.GetMemoryPool()->New<GotoNode>(OP_goto);
      gotoStmt->SetOffset(condGotoNode.GetOffset());
      HandleGoto(*gotoStmt, cgFunc);
      auto *labelStmt = cgFunc.GetMemoryPool()->New<LabelNode>();
      labelStmt->SetLabelIdx(cgFunc.CreateLabel());
      HandleLabel(*labelStmt, cgFunc);
    }
    return;
  }
  cgFunc.SetCurBBKind(BB::kBBIf);
  /* if condNode is not a cmp node, cmp it with zero. */
  if (!kOpcodeInfo.IsCompare(condNode->GetOpCode())) {
    Operand *opnd0 = cgFunc.HandleExpr(condGotoNode, *condNode);
    PrimType primType = condNode->GetPrimType();
    Operand *zeroOpnd = nullptr;
    if (IsPrimitiveInteger(primType)) {
      zeroOpnd = &cgFunc.CreateImmOperand(primType, 0);
    } else {
      ASSERT(((PTY_f32 == primType) || (PTY_f64 == primType)), "we don't support half-precision FP operands yet");
      zeroOpnd = &cgFunc.CreateFPImmZero(primType);
    }
    cgFunc.SelectCondGoto(condGotoNode, *opnd0, *zeroOpnd);
    cgFunc.SetCurBB(*cgFunc.StartNewBB(condGotoNode));
    return;
  }
  /*
   * Special case:
   * bgt (cmp (op0, op1), 0) ==>
   * bgt (op0, op1)
   * but skip the case cmp(op0, 0)
   */
  BaseNode *op0 = condNode->Opnd(0);
  ASSERT(op0 != nullptr, "get first opnd of a condNode failed");
  BaseNode *op1 = condNode->Opnd(1);
  ASSERT(op1 != nullptr, "get second opnd of a condNode failed");
  if ((op0->GetOpCode() == OP_cmp) && (op1->GetOpCode() == OP_constval)) {
    auto *constValNode = static_cast<ConstvalNode*>(op1);
    MIRConst *mirConst = constValNode->GetConstVal();
    auto *cmpNode = static_cast<CompareNode*>(op0);
    bool skip = false;
    if (cmpNode->Opnd(1)->GetOpCode() == OP_constval) {
      auto *constVal = static_cast<ConstvalNode*>(cmpNode->Opnd(1))->GetConstVal();
      if (constVal->IsZero()) {
        skip = true;
      }
    }
    if (!skip && mirConst->IsZero()) {
      cgFunc.SelectCondSpecialCase1(condGotoNode, *op0);
      cgFunc.SetCurBB(*cgFunc.StartNewBB(condGotoNode));
      return;
    }
  }
  /*
   * Special case:
   * brfalse(ge (cmpg (op0, op1), 0) ==>
   * fcmp op1, op2
   * blo
   */
  if ((condGotoNode.GetOpCode() == OP_brfalse) && (condNode->GetOpCode() == OP_ge) &&
      (op0->GetOpCode() == OP_cmpg) && (op1->GetOpCode() == OP_constval)) {
    auto *constValNode = static_cast<ConstvalNode*>(op1);
    MIRConst *mirConst = constValNode->GetConstVal();
    if (mirConst->IsZero()) {
      cgFunc.SelectCondSpecialCase2(condGotoNode, *op0);
      cgFunc.SetCurBB(*cgFunc.StartNewBB(condGotoNode));
      return;
    }
  }
  Operand *opnd0 = cgFunc.HandleExpr(*condNode, *condNode->Opnd(0));
  Operand *opnd1 = cgFunc.HandleExpr(*condNode, *condNode->Opnd(1));
  cgFunc.SelectCondGoto(condGotoNode, *opnd0, *opnd1);
  cgFunc.SetCurBB(*cgFunc.StartNewBB(condGotoNode));
}

void HandleReturn(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.UpdateFrequency(stmt);
  auto &retNode = static_cast<NaryStmtNode&>(stmt);
  cgFunc.HandleRetCleanup(retNode);
  ASSERT(retNode.NumOpnds() <= 1, "NYI return nodes number > 1");
  Operand *opnd = nullptr;
  if (retNode.NumOpnds() != 0) {
    opnd = cgFunc.HandleExpr(retNode, *retNode.Opnd(0));
  }
  cgFunc.SelectReturn(opnd);
  cgFunc.SetCurBBKind(BB::kBBReturn);
  cgFunc.SetCurBB(*cgFunc.StartNewBB(retNode));
}

void HandleCall(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.UpdateFrequency(stmt);
  auto &callNode = static_cast<CallNode&>(stmt);
  cgFunc.SelectCall(callNode);
  if (cgFunc.GetCurBB()->GetKind() != BB::kBBFallthru) {
    cgFunc.SetCurBB(*cgFunc.StartNewBB(callNode));
  }

  StmtNode *prevStmt = stmt.GetPrev();
  if (prevStmt == nullptr || prevStmt->GetOpCode() != OP_catch) {
    return;
  }
  if ((stmt.GetNext() != nullptr) && (stmt.GetNext()->GetOpCode() == OP_label)) {
    cgFunc.SetCurBB(*cgFunc.StartNewBBImpl(true, stmt));
  }
  cgFunc.HandleCatch();
}

void HandleICall(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.UpdateFrequency(stmt);
  auto &icallNode = static_cast<IcallNode&>(stmt);
  cgFunc.GetCurBB()->SetHasCall();
  Operand *opnd0 = cgFunc.HandleExpr(stmt, *icallNode.GetNopndAt(0));
  cgFunc.SelectIcall(icallNode, *opnd0);
  if (cgFunc.GetCurBB()->GetKind() != BB::kBBFallthru) {
    cgFunc.SetCurBB(*cgFunc.StartNewBB(icallNode));
  }
}

void HandleIntrinCall(StmtNode &stmt, CGFunc &cgFunc) {
  auto &call = static_cast<IntrinsiccallNode&>(stmt);
  cgFunc.SelectIntrinCall(call);
}

void HandleDassign(StmtNode &stmt, CGFunc &cgFunc) {
  auto &dassignNode = static_cast<DassignNode&>(stmt);
  ASSERT(dassignNode.GetOpCode() == OP_dassign, "expect dassign");
  BaseNode *rhs = dassignNode.GetRHS();
  ASSERT(rhs != nullptr, "get rhs of dassignNode failed");
  if (rhs->GetOpCode() == OP_malloc || rhs->GetOpCode() == OP_alloca) {
    UnaryStmtNode &uNode = static_cast<UnaryStmtNode &>(stmt);
    Operand *opnd0 = cgFunc.HandleExpr(dassignNode, *(uNode.Opnd()));
    cgFunc.SelectDassign(dassignNode, *opnd0);
  } else if (rhs->GetPrimType() == PTY_agg) {
    cgFunc.SelectAggDassign(dassignNode);
    return;
  }
  bool isSaveRetvalToLocal = false;
  if (rhs->GetOpCode() == OP_regread) {
    isSaveRetvalToLocal = (static_cast<RegreadNode*>(rhs)->GetRegIdx() == -kSregRetval0);
  }
  Operand *opnd0 = cgFunc.HandleExpr(dassignNode, *rhs);
  cgFunc.SelectDassign(dassignNode, *opnd0);
  if (isSaveRetvalToLocal) {
    cgFunc.GetCurBB()->GetLastInsn()->MarkAsSaveRetValToLocal();
  }
}

void HandleRegassign(StmtNode &stmt, CGFunc &cgFunc) {
  ASSERT(stmt.GetOpCode() == OP_regassign, "expect regAssign");
  auto &regAssignNode = static_cast<RegassignNode&>(stmt);
  bool isSaveRetvalToLocal = false;
  BaseNode *operand = regAssignNode.Opnd(0);
  ASSERT(operand != nullptr, "get operand of regassignNode failed");
  if (operand->GetOpCode() == OP_regread) {
    isSaveRetvalToLocal = (static_cast<RegreadNode*>(operand)->GetRegIdx() == -kSregRetval0);
  }
  Operand *opnd0 = cgFunc.HandleExpr(regAssignNode, *operand);
  cgFunc.SelectRegassign(regAssignNode, *opnd0);
  if (isSaveRetvalToLocal) {
    cgFunc.GetCurBB()->GetLastInsn()->MarkAsSaveRetValToLocal();
  }
}

void HandleIassign(StmtNode &stmt, CGFunc &cgFunc) {
  ASSERT(stmt.GetOpCode() == OP_iassign, "expect stmt");
  auto &iassignNode = static_cast<IassignNode&>(stmt);
  if ((iassignNode.GetRHS() != nullptr) && iassignNode.GetRHS()->GetPrimType() != PTY_agg) {
    cgFunc.SelectIassign(iassignNode);
  } else {
    BaseNode *addrNode = iassignNode.Opnd(0);
    if (addrNode == nullptr) {
      return;
    }
    cgFunc.SelectAggIassign(iassignNode, *cgFunc.HandleExpr(stmt, *addrNode));
  }
}

void HandleEval(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.HandleExpr(stmt, *static_cast<UnaryStmtNode&>(stmt).Opnd(0));
}

void HandleRangeGoto(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.UpdateFrequency(stmt);
  auto &rangeGotoNode = static_cast<RangeGotoNode&>(stmt);
  cgFunc.SetCurBBKind(BB::kBBRangeGoto);
  cgFunc.SelectRangeGoto(rangeGotoNode, *cgFunc.HandleExpr(rangeGotoNode, *rangeGotoNode.Opnd(0)));
  cgFunc.SetCurBB(*cgFunc.StartNewBB(rangeGotoNode));
}

void HandleMembar(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.SelectMembar(stmt);
  if (stmt.GetOpCode() != OP_membarrelease) {
    return;
  }
#if TARGAARCH64 || TARGRISCV64
  if (CGOptions::UseBarriersForVolatile()) {
    return;
  }
#endif
  StmtNode *secondStmt = stmt.GetRealNext();
  if (secondStmt == nullptr ||
      ((secondStmt->GetOpCode() != OP_iassign) && (secondStmt->GetOpCode() != OP_dassign))) {
    return;
  }
  StmtNode *thirdStmt = secondStmt->GetRealNext();
  if (thirdStmt == nullptr || thirdStmt->GetOpCode() != OP_membarstoreload) {
    return;
  }
  cgFunc.SetVolStore(true);
  cgFunc.SetVolReleaseInsn(cgFunc.GetCurBB()->GetLastInsn());
}

void HandleComment(StmtNode &stmt, CGFunc &cgFunc) {
  if (cgFunc.GetCG()->GenerateVerboseAsm() || cgFunc.GetCG()->GenerateVerboseCG()) {
    cgFunc.SelectComment(static_cast<CommentNode&>(stmt));
  }
}

void HandleCatchOp(StmtNode &stmt, CGFunc &cgFunc) {
  (void)stmt;
  (void)cgFunc;
  ASSERT(stmt.GetNext()->GetOpCode() == OP_call, "The next statement of OP_catch should be OP_call.");
}

void HandleAssertNull(StmtNode &stmt, CGFunc &cgFunc) {
  auto &cgAssertNode = static_cast<UnaryStmtNode&>(stmt);
  cgFunc.SelectAssertNull(cgAssertNode);
}

void HandleAsm(StmtNode &stmt, CGFunc &cgFunc) {
  cgFunc.SelectAsm(static_cast<AsmNode&>(stmt));
}

using HandleStmtFactory = FunctionFactory<Opcode, void, StmtNode&, CGFunc&>;
void InitHandleStmtFactory() {
  RegisterFactoryFunction<HandleStmtFactory>(OP_label, HandleLabel);
  RegisterFactoryFunction<HandleStmtFactory>(OP_goto, HandleGoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_igoto, HandleIgoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_brfalse, HandleCondbr);
  RegisterFactoryFunction<HandleStmtFactory>(OP_brtrue, HandleCondbr);
  RegisterFactoryFunction<HandleStmtFactory>(OP_return, HandleReturn);
  RegisterFactoryFunction<HandleStmtFactory>(OP_call, HandleCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_icall, HandleICall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccall, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallassigned, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallwithtype, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallwithtypeassigned, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_dassign, HandleDassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_regassign, HandleRegassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassign, HandleIassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_eval, HandleEval);
  RegisterFactoryFunction<HandleStmtFactory>(OP_rangegoto, HandleRangeGoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membarrelease, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membaracquire, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membarstoreload, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membarstorestore, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_comment, HandleComment);
  RegisterFactoryFunction<HandleStmtFactory>(OP_catch, HandleCatchOp);
  RegisterFactoryFunction<HandleStmtFactory>(OP_assertnonnull, HandleAssertNull);
  RegisterFactoryFunction<HandleStmtFactory>(OP_asm, HandleAsm);
}

CGFunc::CGFunc(MIRModule &mod, CG &cg, MIRFunction &mirFunc, BECommon &beCommon, MemPool &memPool,
               StackMemPool &stackMp, MapleAllocator &allocator, uint32 funcId)
    : vRegTable(allocator.Adapter()),
      bbVec(allocator.Adapter()),
      vRegOperandTable(allocator.Adapter()),
      pRegSpillMemOperands(allocator.Adapter()),
      spillRegMemOperands(allocator.Adapter()),
      reuseSpillLocMem(allocator.Adapter()),
      labelMap(std::less<LabelIdx>(), allocator.Adapter()),
      hasVLAOrAlloca(mirFunc.HasVlaOrAlloca()),
      dbgCallFrameLocations(allocator.Adapter()),
      cg(&cg),
      mirModule(mod),
      memPool(&memPool),
      stackMp(stackMp),
      func(mirFunc),
      exitBBVec(allocator.Adapter()),
      lab2BBMap(allocator.Adapter()),
      beCommon(beCommon),
      funcScopeAllocator(&allocator),
      emitStVec(allocator.Adapter()),
#if TARGARM32
      sortedBBs(allocator.Adapter()),
      lrVec(allocator.Adapter()),
#endif  /* TARGARM32 */
      loops(allocator.Adapter()),
      shortFuncName(cg.ExtractFuncName(mirFunc.GetName()) + "." + std::to_string(funcId), &memPool) {
  mirModule.SetCurFunction(&func);
  dummyBB = CreateNewBB();
  vRegCount = firstMapleIrVRegNO + func.GetPregTab()->Size();
  firstNonPregVRegNO = vRegCount;
  /* maximum register count initial be increased by 1024 */
  maxRegCount = vRegCount + 1024;

  vRegTable.resize(maxRegCount);
  /* func.GetPregTab()->_preg_table[0] is nullptr, so skip it */
  ASSERT(func.GetPregTab()->PregFromPregIdx(0) == nullptr, "PregFromPregIdx(0) must be nullptr");
  for (size_t i = 1; i < func.GetPregTab()->Size(); ++i) {
    PrimType primType = func.GetPregTab()->PregFromPregIdx(i)->GetPrimType();
    uint32 byteLen = GetPrimTypeSize(primType);
    if (byteLen < k4ByteSize) {
      byteLen = k4ByteSize;
    }
    new (&GetVirtualRegNodeFromPseudoRegIdx(i)) VirtualRegNode(GetRegTyFromPrimTy(primType), byteLen);
  }
  firstCGGenLabelIdx = func.GetLabelTab()->GetLabelTableSize();
  lSymSize = 0;
  if (func.GetSymTab()) {
    lSymSize = func.GetSymTab()->GetSymbolTableSize();
  }
}

CGFunc::~CGFunc() {
  mirModule.SetCurFunction(nullptr);
}

Operand *CGFunc::HandleExpr(const BaseNode &parent, BaseNode &expr) {
  auto function = CreateProductFunction<HandleExprFactory>(expr.GetOpCode());
  CHECK_FATAL(function != nullptr, "unsupported opCode in HandleExpr()");
  return function(parent, expr, *this);
}

StmtNode *CGFunc::HandleFirstStmt() {
  BlockNode *block = func.GetBody();

  ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");
  bool withFreqInfo = func.HasFreqMap() && !func.GetFreqMap().empty();
  if (withFreqInfo) {
    frequency = kFreqBase;
  }
  StmtNode *stmt = block->GetFirst();
  if (stmt == nullptr) {
    return nullptr;
  }
  ASSERT(stmt->GetOpCode() == OP_label, "The first statement should be a label");
  HandleLabel(*stmt, *this);
  firstBB = curBB;
  stmt = stmt->GetNext();
  if (stmt == nullptr) {
    return nullptr;
  }
  curBB = StartNewBBImpl(false, *stmt);
  curBB->SetFrequency(frequency);
  if (JAVALANG) {
    HandleRCCall(true);
  }
  return stmt;
}

bool CGFunc::CheckSkipMembarOp(StmtNode &stmt) {
  StmtNode *nextStmt = stmt.GetRealNext();
  if (nextStmt == nullptr) {
    return false;
  }

  Opcode opCode = stmt.GetOpCode();
  if (((opCode == OP_membaracquire) || (opCode == OP_membarrelease)) && (nextStmt->GetOpCode() == stmt.GetOpCode())) {
    return true;
  }
  if ((opCode == OP_membarstorestore) && (nextStmt->GetOpCode() == OP_membarrelease)) {
    return true;
  }
  if ((opCode == OP_membarstorestore) && func.IsConstructor() && MemBarOpt(stmt)) {
    return true;;
  }
#if TARGAARCH64 || TARGRISCV64
  if ((!CGOptions::UseBarriersForVolatile()) && (nextStmt->GetOpCode() == OP_membaracquire)) {
    isVolLoad = true;
  }
#endif /* TARGAARCH64 */
  return false;
}

void CGFunc::GenerateLoc(StmtNode *stmt, unsigned &lastSrcLoc, unsigned &lastMplLoc) {
  /* insert Insn for .loc before cg for the stmt */
  if (cg->GetCGOptions().WithLoc() && stmt->op != OP_label && stmt->op != OP_comment) {
    /* if original src file location info is availiable for this stmt,
     * use it and skip mpl file location info for this stmt
     */
    bool hasLoc = false;
    unsigned newSrcLoc = cg->GetCGOptions().WithSrc() ? stmt->GetSrcPos().LineNum() : 0;
    if (newSrcLoc != 0 && newSrcLoc != lastSrcLoc) {
      /* .loc for original src file */
      unsigned fileid = stmt->GetSrcPos().FileNum();
      Operand *o0 = CreateDbgImmOperand(fileid);
      Operand *o1 = CreateDbgImmOperand(newSrcLoc);
      Insn &loc = cg->BuildInstruction<mpldbg::DbgInsn>(mpldbg::OP_DBG_loc, *o0, *o1);
      curBB->AppendInsn(loc);
      lastSrcLoc = newSrcLoc;
      hasLoc = true;
    }
    /* .loc for mpl file, skip if already has .loc from src for this stmt */
    unsigned newMplLoc = cg->GetCGOptions().WithMpl() ? stmt->GetSrcPos().MplLineNum() : 0;
    if (newMplLoc != 0 && newMplLoc != lastMplLoc && !hasLoc) {
      unsigned fileid = 1;
      Operand *o0 = CreateDbgImmOperand(fileid);
      Operand *o1 = CreateDbgImmOperand(newMplLoc);
      Insn &loc = cg->BuildInstruction<mpldbg::DbgInsn>(mpldbg::OP_DBG_loc, *o0, *o1);
      curBB->AppendInsn(loc);
      lastMplLoc = newMplLoc;
    }
  }
}

void CGFunc::GenerateInstruction() {
  InitHandleExprFactory();
  InitHandleStmtFactory();
  StmtNode *secondStmt = HandleFirstStmt();

  /* First Pass: Creates the doubly-linked list of BBs (next,prev) */
  volReleaseInsn = nullptr;
  unsigned lastSrcLoc = 0;
  unsigned lastMplLoc = 0;
  for (StmtNode *stmt = secondStmt; stmt != nullptr; stmt = stmt->GetNext()) {
    /* insert Insn for .loc before cg for the stmt */
    GenerateLoc(stmt, lastSrcLoc, lastMplLoc);

    isVolLoad = false;
    if (CheckSkipMembarOp(*stmt)) {
      continue;
    }
    bool tempLoad = isVolLoad;

    auto function = CreateProductFunction<HandleStmtFactory>(stmt->GetOpCode());
    CHECK_FATAL(function != nullptr, "unsupported opCode or has been lowered before");
    function(*stmt, *this);

    /* skip the membar acquire if it is just after the iread. ldr + membaraquire->ldar */
    if (tempLoad && !isVolLoad) {
      stmt = stmt->GetNext();
    }

    /*
     * skip the membarstoreload if there is the pattern for volatile write( membarrelease + store + membarstoreload )
     * membarrelease + store + membarstoreload -> stlr
     */
    if (volReleaseInsn != nullptr) {
      if ((stmt->GetOpCode() != OP_membarrelease) && (stmt->GetOpCode() != OP_comment)) {
        if (!isVolStore) {
          /* remove the generated membar release insn. */
          curBB->RemoveInsn(*volReleaseInsn);
          /* skip the membarstoreload. */
          stmt = stmt->GetNext();
        }
        volReleaseInsn = nullptr;
        isVolStore = false;
      }
    }
  }

  /* Set lastbb's frequency */
  BlockNode *block = func.GetBody();
  ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");
  curBB->SetLastStmt(*block->GetLast());
  curBB->SetFrequency(frequency);
  lastBB = curBB;
  cleanupBB = lastBB->GetPrev();
  /* All stmts are handled */
  frequency = 0;
}

LabelIdx CGFunc::CreateLabel() {
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
  ASSERT(funcSt != nullptr, "Get func failed at CGFunc::CreateLabel");
  std::string funcName = funcSt->GetName();
  std::string labelStr = funcName.append(std::to_string(labelIdx++));
  return func.GetOrCreateLableIdxFromName(labelStr);
}

MIRSymbol *CGFunc::GetRetRefSymbol(BaseNode &expr) {
  Opcode opcode = expr.GetOpCode();
  if (opcode != OP_dread) {
    return nullptr;
  }
  auto &retExpr = static_cast<AddrofNode&>(expr);
  MIRSymbol *symbol = mirModule.CurFunction()->GetLocalOrGlobalSymbol(retExpr.GetStIdx());
  ASSERT(symbol != nullptr, "get symbol in mirmodule failed");
  if (symbol->IsRefType()) {
    MIRSymbol *sym = nullptr;
    for (uint32 i = 0; i < func.GetFormalCount(); i++) {
      sym = func.GetFormal(i);
      if (sym == symbol) {
        return nullptr;
      }
    }
    return symbol;
  }
  return nullptr;
}

void CGFunc::GenerateCfiPrologEpilog() {
  if (GenCfi() == false) {
    return;
  }
  Insn &ipoint = GetCG()->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_startproc);
  /* prolog */
  if (firstBB->GetFirstInsn() != nullptr) {
    firstBB->InsertInsnBefore(*firstBB->GetFirstInsn(), ipoint);
  } else {
    firstBB->AppendInsn(ipoint);
  }

#if !defined(TARGARM32)
  /*
   * always generate ".cfi_personality 155, DW.ref.__mpl_personality_v0" for Java methods.
   * we depend on this to tell whether it is a java method.
   */
  if (mirModule.IsJavaModule() && func.IsJava()) {
    Insn &personality = GetCG()->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_personality_symbol,
                                                                CreateCfiImmOperand(EHFunc::kTypeEncoding, k8BitSize),
                                                                CreateCfiStrOperand("DW.ref.__mpl_personality_v0"));
    firstBB->InsertInsnAfter(ipoint, personality);
  }
#endif

  /* epilog */
  lastBB->AppendInsn(GetCG()->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_endproc));
}

void CGFunc::TraverseAndClearCatchMark(BB &bb) {
  /* has bb been visited */
  if (bb.GetInternalFlag3()) {
    return;
  }
  bb.SetIsCatch(false);
  bb.SetInternalFlag3(1);
  for (auto succBB : bb.GetSuccs()) {
    TraverseAndClearCatchMark(*succBB);
  }
}

/*
 * Two types of successor edges, normal and eh. Any bb which is not
 * reachable by a normal successor edge is considered to be in a
 * catch block.
 * Marking it as a catch block does not automatically make it into
 * a catch block. Unreachables can be marked as such too.
 */
void CGFunc::MarkCatchBBs() {
  /* First, suspect all bb to be in catch */
  FOR_ALL_BB(bb, this) {
    bb->SetIsCatch(true);
    bb->SetInternalFlag3(0);  /* mark as not visited */
  }
  /* Eliminate cleanup section from catch */
  FOR_ALL_BB(bb, this) {
    if (bb->GetFirstStmt() == cleanupLabel) {
      bb->SetIsCatch(false);
      ASSERT(bb->GetSuccs().size() <= 1, "MarkCatchBBs incorrect cleanup label");
      BB *succ = nullptr;
      if (!bb->GetSuccs().empty()) {
        succ = bb->GetSuccs().front();
      } else {
        continue;
      }
      ASSERT(succ != nullptr, "Get front succsBB failed");
      while (1) {
        ASSERT(succ->GetSuccs().size() <= 1, "MarkCatchBBs incorrect cleanup label");
        succ->SetIsCatch(false);
        if (!succ->GetSuccs().empty()) {
          succ = succ->GetSuccs().front();
        } else {
          break;
        }
      }
    }
  }
  /* Unmark all normally reachable bb as NOT catch. */
  TraverseAndClearCatchMark(*firstBB);
}

/*
 * Mark CleanupEntryBB
 * Note: Cleanup bbs and func body bbs are seperated, no edges between them.
 * No ehSuccs or eh_prevs between cleanup bbs.
 */
void CGFunc::MarkCleanupEntryBB() {
  BB *cleanupEntry = nullptr;
  FOR_ALL_BB(bb, this) {
    bb->SetIsCleanup(0);      /* Use to mark cleanup bb */
    bb->SetInternalFlag3(0);  /* Use to mark if visited. */
    if (bb->GetFirstStmt() == this->cleanupLabel) {
      cleanupEntry = bb;
    }
  }
  /* If a function without cleanup bb, return. */
  if (cleanupEntry == nullptr) {
    return;
  }
  /* after merge bb, update cleanupBB. */
  if (cleanupEntry->GetSuccs().empty()) {
    this->cleanupBB = cleanupEntry;
  }
  SetCleanupLabel(*cleanupEntry);
  ASSERT(cleanupEntry->GetEhSuccs().empty(), "CG internal error. Cleanup bb should not have ehSuccs.");
#if DEBUG  /* Please don't remove me. */
  /* Check if all of the cleanup bb is at bottom of the function. */
  bool isCleanupArea = true;
  if (!mirModule.IsCModule()) {
    FOR_ALL_BB_REV(bb, this) {
      if (isCleanupArea) {
        ASSERT(bb->IsCleanup(), "CG internal error, cleanup BBs should be at the bottom of the function.");
      } else {
        ASSERT(!bb->IsCleanup(), "CG internal error, cleanup BBs should be at the bottom of the function.");
      }

      if (bb == cleanupEntry) {
        isCleanupArea = false;
      }
    }
  }
#endif  /* DEBUG */
  this->cleanupEntryBB = cleanupEntry;
}

/* Tranverse from current bb's successor and set isCleanup true. */
void CGFunc::SetCleanupLabel(BB &cleanupEntry) {
  /* If bb hasn't been visited, return. */
  if (cleanupEntry.GetInternalFlag3()) {
    return;
  }
  cleanupEntry.SetInternalFlag3(1);
  cleanupEntry.SetIsCleanup(1);
  for (auto tmpBB : cleanupEntry.GetSuccs()) {
    if (tmpBB->GetKind() != BB::kBBReturn) {
      SetCleanupLabel(*tmpBB);
    } else {
      ASSERT(ExitbbNotInCleanupArea(cleanupEntry), "exitBB created in cleanupArea.");
    }
  }
}

bool CGFunc::ExitbbNotInCleanupArea(const BB &bb) const {
  for (const BB *nextBB = bb.GetNext(); nextBB != nullptr; nextBB = nextBB->GetNext()) {
    if (nextBB->GetKind() == BB::kBBReturn) {
      return false;
    }
  }
  return true;
}

/*
 * Do mem barrier optimization for constructor funcs as follow:
 * membarstorestore
 * write field of this_  ==> write field of this_
 * membarrelease             membarrelease.
 */
bool CGFunc::MemBarOpt(StmtNode &membar) {
  if (func.GetFormalCount() == 0) {
    return false;
  }
  MIRSymbol *thisSym = func.GetFormal(0);
  if (thisSym == nullptr) {
    return false;
  }
  StmtNode *stmt = membar.GetNext();
  for (; stmt != nullptr; stmt = stmt->GetNext()) {
    BaseNode *base = nullptr;
    if (stmt->GetOpCode() == OP_comment) {
      continue;
    } else if (stmt->GetOpCode() == OP_iassign) {
      base = static_cast<IassignNode *>(stmt)->Opnd(0);
    } else if (stmt->GetOpCode() == OP_call) {
      auto *callNode = static_cast<CallNode*>(stmt);
      MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
      MIRSymbol *fsym = GetMirModule().CurFunction()->GetLocalOrGlobalSymbol(fn->GetStIdx(), false);
      if (fsym->GetName() == "MCC_WriteRefFieldNoDec") {
        base = callNode->Opnd(0);
      }
    }
    if (base != nullptr) {
      Opcode op = base->GetOpCode();
      if (op == OP_regread && thisSym->IsPreg() &&
          thisSym->GetPreg()->GetPregNo() == static_cast<RegreadNode*>(base)->GetRegIdx()) {
        continue;
      }
      if ((op == OP_dread || op == OP_addrof) && !thisSym->IsPreg() &&
          static_cast<AddrofNode*>(base)->GetStIdx() == thisSym->GetStIdx()) {
        continue;
      }
    }
    break;
  }

  CHECK_NULL_FATAL(stmt);
  return stmt->GetOpCode() == OP_membarrelease;
}

void CGFunc::ProcessExitBBVec() {
  if (exitBBVec.empty()) {
    LabelIdx newLabelIdx = CreateLabel();
    BB *retBB = CreateNewBB(newLabelIdx, cleanupBB->IsUnreachable(), BB::kBBReturn, cleanupBB->GetFrequency());
    cleanupBB->PrependBB(*retBB);
    exitBBVec.emplace_back(retBB);
    return;
  }
  /* split an empty exitBB */
  BB *bb = exitBBVec[0];
  if (bb->NumInsn() > 0) {
    BB *retBBPart = CreateNewBB(false, BB::kBBFallthru, bb->GetFrequency());
    ASSERT(retBBPart != nullptr, "retBBPart should not be nullptr");
    LabelIdx retBBPartLabelIdx = bb->GetLabIdx();
    if (retBBPartLabelIdx != MIRLabelTable::GetDummyLabel()) {
      retBBPart->AddLabel(retBBPartLabelIdx);
      lab2BBMap[retBBPartLabelIdx] = retBBPart;
    }
    Insn *insn = bb->GetFirstInsn();
    while (insn != nullptr) {
      bb->RemoveInsn(*insn);
      retBBPart->AppendInsn(*insn);
      insn = bb->GetFirstInsn();
    }
    bb->PrependBB(*retBBPart);
    LabelIdx newLabelIdx = CreateLabel();
    bb->AddLabel(newLabelIdx);
    lab2BBMap[newLabelIdx] = bb;
  }
}

void CGFunc::UpdateCallBBFrequency() {
  if (!func.HasFreqMap() || func.GetFreqMap().empty()) {
    return;
  }
  FOR_ALL_BB(bb, this) {
    if (bb->GetKind() != BB::kBBFallthru || !bb->HasCall()) {
      continue;
    }
    ASSERT(bb->GetSuccs().size() <= 1, "fallthru BB has only one successor.");
    if (!bb->GetSuccs().empty()) {
      bb->SetFrequency((*(bb->GetSuccsBegin()))->GetFrequency());
    }
  }
}

void CGFunc::HandleFunction() {
  /* select instruction */
  GenerateInstruction();
  /* merge multi return */
  if (!func.GetModule()->IsCModule()) {
    MergeReturn();
  }
  if (func.IsJava()) {
    ASSERT(exitBBVec.size() <= 1, "there are more than one BB_return in func");
  }
  ProcessExitBBVec();

  if (func.IsJava()) {
    GenerateCleanupCodeForExtEpilog(*cleanupBB);
  } else if (!func.GetModule()->IsCModule()) {
    GenerateCleanupCode(*cleanupBB);
  }
  GenSaveMethodInfoCode(*firstBB);
  /* build control flow graph */
  theCFG = memPool->New<CGCFG>(*this);
  theCFG->BuildCFG();
  UpdateCallBBFrequency();
  if (mirModule.GetSrcLang() != kSrcLangC) {
    MarkCatchBBs();
  }
  MarkCleanupEntryBB();
  DetermineReturnTypeofCall();
  theCFG->MarkLabelTakenBB();
  theCFG->UnreachCodeAnalysis();
  SplitStrLdrPair();
  if (CGOptions::IsLazyBinding() && !GetCG()->IsLibcore()) {
    ProcessLazyBinding();
  }
  if (GetCG()->DoPatchLongBranch()) {
    PatchLongBranch();
  }
}

void CGFunc::AddDIESymbolLocation(const MIRSymbol *sym, SymbolAlloc *loc) {
  ASSERT(debugInfo != nullptr, "debugInfo is null!");
  DBGDie *sdie = debugInfo->GetLocalDie(&func, sym->GetNameStrIdx());
  if (sdie == nullptr) {
    return;
  }

  DBGExprLoc *exprloc = sdie->GetExprLoc();
  CHECK_FATAL(exprloc != nullptr, "exprloc is null in CGFunc::AddDIESymbolLocation");
  exprloc->SetSymLoc(loc);

  GetDbgCallFrameLocations().push_back(exprloc);
}

void CGFunc::DumpCFG() const {
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
  LogInfo::MapleLogger() << "\n****** CFG built by CG for " << funcSt->GetName() << " *******\n";
  FOR_ALL_BB_CONST(bb, this) {
    LogInfo::MapleLogger() << "=== BB ( " << std::hex << bb << std::dec << " ) <" << bb->GetKindName() << "> ===\n";
    LogInfo::MapleLogger() << "BB id:" << bb->GetId() << "\n";
    if (!bb->GetPreds().empty()) {
      LogInfo::MapleLogger() << " pred [ ";
      for (auto *pred : bb->GetPreds()) {
        LogInfo::MapleLogger() << pred->GetId() << " ";
      }
      LogInfo::MapleLogger() << "]\n";
    }
    if (!bb->GetSuccs().empty()) {
      LogInfo::MapleLogger() << " succ [ ";
      for (auto *succ : bb->GetSuccs()) {
        LogInfo::MapleLogger() << succ->GetId() << " ";
      }
      LogInfo::MapleLogger() << "]\n";
    }
    const StmtNode *stmt = bb->GetFirstStmt();
    if (stmt != nullptr) {
      bool done = false;
      do {
        done = stmt == bb->GetLastStmt();
        stmt->Dump(1);
        LogInfo::MapleLogger() << "\n";
        stmt = stmt->GetNext();
      } while (!done);
    } else {
      LogInfo::MapleLogger() << "<empty BB>\n";
    }
  }
}

void CGFunc::DumpCGIR() const {
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
  LogInfo::MapleLogger() << "\n******  CGIR for " << funcSt->GetName() << " *******\n";
  FOR_ALL_BB_CONST(bb, this) {
    LogInfo::MapleLogger() << "=== BB " << " <" << bb->GetKindName();
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      LogInfo::MapleLogger() << "[labeled with " << bb->GetLabIdx();
      LogInfo::MapleLogger() << " ==> @" << func.GetLabelName(bb->GetLabIdx()) << "]";
    }

    LogInfo::MapleLogger() << "> <" << bb->GetId() << "> ";
    if (bb->GetLoop()) {
      LogInfo::MapleLogger() << "[Loop level " << bb->GetLoop()->GetLoopLevel();
      LogInfo::MapleLogger() << ", head BB " <<  bb->GetLoop()->GetHeader()->GetId() << "]";
    }
    if (bb->IsCleanup()) {
      LogInfo::MapleLogger() << "[is_cleanup] ";
    }
    if (bb->IsUnreachable()) {
      LogInfo::MapleLogger() << "[unreachable] ";
    }
    if (bb->GetFirstStmt() == cleanupLabel) {
      LogInfo::MapleLogger() << "cleanup ";
    }
    if (!bb->GetSuccs().empty()) {
      LogInfo::MapleLogger() << "succs: ";
      for (auto *succBB : bb->GetSuccs()) {
        LogInfo::MapleLogger() << succBB->GetId() << " ";
      }
    }
    if (!bb->GetEhSuccs().empty()) {
      LogInfo::MapleLogger() << "eh_succs: ";
      for (auto *ehSuccBB : bb->GetEhSuccs()) {
        LogInfo::MapleLogger() << ehSuccBB->GetId() << " ";
      }
    }
    LogInfo::MapleLogger() << "===\n";
    LogInfo::MapleLogger() << "frequency:" << bb->GetFrequency() << "\n";

    FOR_BB_INSNS_CONST(insn, bb) {
      insn->Dump();
    }
  }
}

void CGFunc::DumpLoop() const {
  for (const auto *lp : loops) {
    lp->PrintLoops(*lp);
  }
}

void CGFunc::ClearLoopInfo() {
  loops.clear();
  loops.shrink_to_fit();
  FOR_ALL_BB(bb, this) {
    bb->ClearLoopPreds();
    bb->ClearLoopSuccs();
  }
}

void CGFunc::PatchLongBranch() {
  for (BB *bb = firstBB->GetNext(); bb != nullptr; bb = bb->GetNext()) {
    bb->SetInternalFlag1(bb->GetInternalFlag1() + bb->GetPrev()->GetInternalFlag1());
  }
  BB *next = nullptr;
  for (BB *bb = firstBB; bb != nullptr; bb = next) {
    next = bb->GetNext();
    if (bb->GetKind() != BB::kBBIf && bb->GetKind() != BB::kBBGoto) {
      continue;
    }
    Insn * insn = bb->GetLastInsn();
    while (insn->IsImmaterialInsn()) {
      insn = insn->GetPrev();
    }
    LabelIdx labidx = static_cast<LabelOperand&>(insn->GetOperand(insn->GetJumpTargetIdx())).GetLabelIndex();
    BB *tbb = GetBBFromLab2BBMap(static_cast<int32>(labidx));
    if ((tbb->GetInternalFlag1() - bb->GetInternalFlag1()) < MaxCondBranchDistance()) {
      continue;
    }
    InsertJumpPad(insn);
  }
}

AnalysisResult *CgDoHandleFunc::Run(CGFunc *cgFunc, CgFuncResultMgr *cgFuncResultMgr) {
  (void)cgFuncResultMgr;
  ASSERT(cgFunc != nullptr, "Expect a cgfunc in CgDoHandleFunc");
  cgFunc->HandleFunction();
  if (!cgFunc->GetCG()->GetCGOptions().DoEmitCode() || cgFunc->GetCG()->GetCGOptions().DoDumpCFG()) {
    cgFunc->DumpCFG();
  }
  return nullptr;
}

AnalysisResult *CgFixCFLocOsft::Run(CGFunc *cgFunc, CgFuncResultMgr *m) {
  (void)m;
  if (cgFunc->GetCG()->GetCGOptions().WithDwarf()) {
    cgFunc->DBGFixCallFrameLocationOffsets();
  }
  return nullptr;
}
}  /* namespace maplebe */
