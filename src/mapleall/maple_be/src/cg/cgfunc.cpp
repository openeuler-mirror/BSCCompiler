/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved
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
#include <utility>
#if defined(DEBUG) && DEBUG
#include <iomanip>
#endif
#include "cg.h"
#include "insn.h"
#include "loop.h"
#include "mir_builder.h"
#include "factory.h"
#include "debug_info.h"
#include "cfgo.h"
#include "optimize_common.h"
#include "me_function.h"

namespace maplebe {
using namespace maple;

#define JAVALANG (GetMirModule().IsJavaModule())

Operand *HandleDread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
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
  auto &constValNode = static_cast<ConstvalNode&>(expr);
  MIRConst *mirConst = constValNode.GetConstVal();
  ASSERT(mirConst != nullptr, "get constval of constvalnode failed");
  if (mirConst->GetKind() == kConstInt) {
    auto *mirIntConst = safe_cast<MIRIntConst>(mirConst);
    return cgFunc.SelectIntConst(*mirIntConst, parent);
  } else if (mirConst->GetKind() == kConstFloatConst) {
    auto *mirFloatConst = safe_cast<MIRFloatConst>(mirConst);
    return cgFunc.SelectFloatConst(*mirFloatConst, parent);
  } else if (mirConst->GetKind() == kConstDoubleConst) {
    auto *mirDoubleConst = safe_cast<MIRDoubleConst>(mirConst);
    return cgFunc.SelectDoubleConst(*mirDoubleConst, parent);
  } else if (mirConst->GetKind() == kConstFloat128Const) {
    auto *mirFloat128Const = safe_cast<MIRFloat128Const>(mirConst);
    return cgFunc.SelectFloat128Const(*mirFloat128Const);
  } else {
    CHECK_FATAL(false, "NYI");
  }
  return nullptr;
}

Operand *HandleConstStr(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &constStrNode = static_cast<ConststrNode&>(expr);
#if TARGAARCH64 || TARGRISCV64
  if (CGOptions::IsArm64ilp32()) {
    return cgFunc.SelectStrConst(*cgFunc.GetMemoryPool()->New<MIRStrConst>(
        constStrNode.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a32))));
  } else {
    return cgFunc.SelectStrConst(*cgFunc.GetMemoryPool()->New<MIRStrConst>(
        constStrNode.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64))));
  }
#else
  return cgFunc.SelectStrConst(*cgFunc.GetMemoryPool()->New<MIRStrConst>(
      constStrNode.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)PTY_a32)));
#endif
}

Operand *HandleConstStr16(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  auto &constStr16Node = static_cast<Conststr16Node&>(expr);
#if TARGAARCH64 || TARGRISCV64
  if (CGOptions::IsArm64ilp32()) {
    return cgFunc.SelectStr16Const(*cgFunc.GetMemoryPool()->New<MIRStr16Const>(
        constStr16Node.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a32))));
  } else {
    return cgFunc.SelectStr16Const(*cgFunc.GetMemoryPool()->New<MIRStr16Const>(
        constStr16Node.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64))));
  }
#else
  return cgFunc.SelectStr16Const(*cgFunc.GetMemoryPool()->New<MIRStr16Const>(
      constStr16Node.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)PTY_a32)));
#endif
}

Operand *HandleAdd(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 && expr.Opnd(0)->GetOpCode() == OP_mul &&
      !IsPrimitiveVector(expr.GetPrimType()) &&
      !IsPrimitiveFloat(expr.GetPrimType()) && expr.Opnd(0)->Opnd(0)->GetOpCode() != OP_constval &&
      expr.Opnd(0)->Opnd(1)->GetOpCode() != OP_constval) {
    return cgFunc.SelectMadd(static_cast<BinaryNode&>(expr),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(0)->Opnd(0)),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(0)->Opnd(1)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
  } else if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 && expr.Opnd(1)->GetOpCode() == OP_mul &&
             !IsPrimitiveVector(expr.GetPrimType()) &&
             !IsPrimitiveFloat(expr.GetPrimType()) && expr.Opnd(1)->Opnd(0)->GetOpCode() != OP_constval &&
             expr.Opnd(1)->Opnd(1)->GetOpCode() != OP_constval) {
    return cgFunc.SelectMadd(static_cast<BinaryNode&>(expr),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(1)->Opnd(0)),
                             *cgFunc.HandleExpr(*expr.Opnd(0), *expr.Opnd(1)->Opnd(1)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
  } else {
    return cgFunc.SelectAdd(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
  }
}

Operand *HandleCGArrayElemAdd(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return &cgFunc.SelectCGArrayElemAdd(static_cast<BinaryNode&>(expr), parent);
}

BaseNode *IsConstantInVectorFromScalar(BaseNode *expr) {
  if (expr->op != OP_intrinsicop) {
    return nullptr;
  }
  IntrinsicopNode *intrn = static_cast<IntrinsicopNode *>(expr);
  switch (intrn->GetIntrinsic()) {
    case INTRN_vector_from_scalar_v8u8: case INTRN_vector_from_scalar_v8i8:
    case INTRN_vector_from_scalar_v4u16: case INTRN_vector_from_scalar_v4i16:
    case INTRN_vector_from_scalar_v2u32: case INTRN_vector_from_scalar_v2i32:
    case INTRN_vector_from_scalar_v1u64: case INTRN_vector_from_scalar_v1i64:
    case INTRN_vector_from_scalar_v16u8: case INTRN_vector_from_scalar_v16i8:
    case INTRN_vector_from_scalar_v8u16: case INTRN_vector_from_scalar_v8i16:
    case INTRN_vector_from_scalar_v4u32: case INTRN_vector_from_scalar_v4i32:
    case INTRN_vector_from_scalar_v2u64: case INTRN_vector_from_scalar_v2i64: {
      if (intrn->Opnd(0) != nullptr && intrn->Opnd(0)->op == OP_constval) {
        return intrn->Opnd(0);
      }
      break;
    }
    default:
      break;
  }
  return nullptr;
}

Operand *HandleShift(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  BaseNode *cExpr = IsConstantInVectorFromScalar(expr.Opnd(1));
  if (cExpr == nullptr) {
    return cgFunc.SelectShift(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                              *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
  } else {
    return cgFunc.SelectShift(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                              *cgFunc.HandleExpr(*expr.Opnd(1), *cExpr), parent);
  }
}

Operand *HandleRor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectRor(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleMpy(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectMpy(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleDiv(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectDiv(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleRem(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectRem(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleAddrof(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &addrofNode = static_cast<AddrofNode&>(expr);
  return cgFunc.SelectAddrof(addrofNode, parent, false);
}

Operand *HandleAddrofoff(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &addrofoffNode = static_cast<AddrofoffNode&>(expr);
  return cgFunc.SelectAddrofoff(addrofoffNode, parent);
}

Operand *HandleAddroffunc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &addroffuncNode = static_cast<AddroffuncNode&>(expr);
  return &cgFunc.SelectAddrofFunc(addroffuncNode, parent);
}

Operand *HandleAddrofLabel(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &addrofLabelNode = static_cast<AddroflabelNode&>(expr);
  return &cgFunc.SelectAddrofLabel(addrofLabelNode, parent);
}

Operand *HandleIread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &ireadNode = static_cast<IreadNode&>(expr);
  return cgFunc.SelectIread(parent, ireadNode);
}

Operand *HandleIreadoff(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &ireadNode = static_cast<IreadoffNode&>(expr);
  return cgFunc.SelectIreadoff(parent, ireadNode);
}

Operand *HandleIreadfpoff(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &ireadNode = static_cast<IreadFPoffNode&>(expr);
  return cgFunc.SelectIreadfpoff(parent, ireadNode);
}

Operand *HandleSub(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectSub(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleBand(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectBand(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleBior(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectBior(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleBxor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectBxor(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleAbs(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectAbs(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleBnot(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectBnot(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleExtractBits(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  ExtractbitsNode &node = static_cast<ExtractbitsNode&>(expr);
  uint8 bitOffset = node.GetBitsOffset();
  uint8 bitSize = node.GetBitsSize();
  if (!CGOptions::IsBigEndian() && (bitSize == k8BitSize || bitSize == k16BitSize) &&
      GetPrimTypeBitSize(node.GetPrimType()) != k64BitSize &&
      (bitOffset == 0 || bitOffset == k8BitSize || bitOffset == k16BitSize || bitOffset == k24BitSize) &&
      expr.Opnd(0)->GetOpCode() == OP_iread && node.GetOpCode() == OP_extractbits) {
    return cgFunc.SelectRegularBitFieldLoad(node, parent);
  }
  return cgFunc.SelectExtractbits(static_cast<ExtractbitsNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleDepositBits(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectDepositBits(static_cast<DepositbitsNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                                  *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleLnot(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectLnot(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleLand(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectLand(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                           *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleLor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  if (parent.IsCondBr()) {
    return cgFunc.SelectLor(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent, true);
  } else {
    return cgFunc.SelectLor(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
  }
}

Operand *HandleMin(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectMin(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleMax(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectMax(static_cast<BinaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                          *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

Operand *HandleNeg(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectNeg(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleRecip(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectRecip(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleSqrt(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectSqrt(static_cast<UnaryNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleCeil(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectCeil(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleFloor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectFloor(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleRetype(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  (void)parent;
  return cgFunc.SelectRetype(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleCvt(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectCvt(parent, static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

Operand *HandleRound(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectRound(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

Operand *HandleTrunc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  return cgFunc.SelectTrunc(static_cast<TypeCvtNode&>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static bool HasCompare(const BaseNode *expr) {
  if (kOpcodeInfo.IsCompare(expr->GetOpCode())) {
    return true;
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    if (HasCompare(expr->Opnd(i))) {
      return true;
    }
  }
  return false;
}

Operand *HandleSelect(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  /* 0,1,2 represent the first opnd and the second opnd and the third opnd of expr */
  bool hasCompare = false;
  if (HasCompare(expr.Opnd(1)) || HasCompare(expr.Opnd(2))) {
    hasCompare = true;
  }
  Operand &trueOpnd = *cgFunc.HandleExpr(expr, *expr.Opnd(1));
  Operand &falseOpnd = *cgFunc.HandleExpr(expr, *expr.Opnd(2));
  Operand *cond = cgFunc.HandleExpr(expr, *expr.Opnd(0));
  return cgFunc.SelectSelect(static_cast<TernaryNode&>(expr), *cond, trueOpnd, falseOpnd, parent, hasCompare);
}

Operand *HandleCmp(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  // fix opnd type before select insn
  PrimType targetPtyp = parent.GetPrimType();
  if (kOpcodeInfo.IsCompare(parent.GetOpCode())) {
    targetPtyp = static_cast<const CompareNode&>(parent).GetOpndType();
  } else if (kOpcodeInfo.IsTypeCvt(parent.GetOpCode())) {
    targetPtyp = static_cast<const TypeCvtNode&>(parent).FromType();
  }
  if (IsPrimitiveInteger(targetPtyp) && targetPtyp != expr.GetPrimType()) {
    expr.SetPrimType(targetPtyp);
  }
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
Operand *HandleVectorAddLong(const BaseNode &expr, CGFunc &cgFunc, bool isLow) {
  Operand *o1 = cgFunc.HandleExpr(expr, *expr.Opnd(0));
  Operand *o2 = cgFunc.HandleExpr(expr, *expr.Opnd(1));
  return cgFunc.SelectVectorAddLong(expr.GetPrimType(), o1, o2, expr.Opnd(0)->GetPrimType(), isLow);
}

Operand *HandleVectorAddWiden(const BaseNode &expr, CGFunc &cgFunc, bool isLow) {
  Operand *o1 = cgFunc.HandleExpr(expr, *expr.Opnd(0));
  Operand *o2 = cgFunc.HandleExpr(expr, *expr.Opnd(1));
  return cgFunc.SelectVectorAddWiden(o1, expr.Opnd(0)->GetPrimType(), o2, expr.Opnd(1)->GetPrimType(), isLow);
}

Operand *HandleVectorFromScalar(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  return cgFunc.SelectVectorFromScalar(intrnNode.GetPrimType(), cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0)),
                                       intrnNode.Opnd(0)->GetPrimType());
}

Operand *HandleVectorAbsSubL(const IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow) {
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand 1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand 2 */
  return cgFunc.SelectVectorAbsSubL(intrnNode.GetPrimType(), opnd1, opnd2, intrnNode.Opnd(0)->GetPrimType(), isLow);
}

Operand *HandleVectorMerge(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand2 */
  BaseNode *index = intrnNode.Opnd(2);                                 /* index operand */
  int32 iNum = 0;
  if (index->GetOpCode() == OP_constval) {
    MIRConst *mirConst = static_cast<ConstvalNode *>(index)->GetConstVal();
    iNum = static_cast<int32>(safe_cast<MIRIntConst>(mirConst)->GetExtValue());
    PrimType ty = intrnNode.Opnd(0)->GetPrimType();
    if (!IsPrimitiveVector(ty)) {
      iNum = 0;
    } else {
      iNum *= static_cast<int32>(GetPrimTypeSize(ty) / GetVecLanes(ty));  /* 64x2: 0-1 -> 0-8 */
    }
  } else {                                                                /* 32x4: 0-3 -> 0-12 */
    CHECK_FATAL(0, "VectorMerge does not have const index");
  }
  return cgFunc.SelectVectorMerge(intrnNode.GetPrimType(), opnd1, opnd2, iNum);
}

Operand *HandleVectorGetHigh(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  return cgFunc.SelectVectorDup(rType, opnd1, false);
}

Operand *HandleVectorGetLow(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  return cgFunc.SelectVectorDup(rType, opnd1, true);
}

Operand *HandleVectorPairwiseAdd(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  Operand *src = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));     /* vector src operand */
  PrimType sType = intrnNode.Opnd(0)->GetPrimType();
  return cgFunc.SelectVectorPairwiseAdd(intrnNode.GetPrimType(), src, sType);
}

Operand *HandleVectorPairwiseAdalp(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  BaseNode *arg1 = intrnNode.Opnd(0);
  BaseNode *arg2 = intrnNode.Opnd(1);
  Operand *src1 = cgFunc.HandleExpr(intrnNode, *arg1);                  /* vector src operand 1 */
  Operand *src2 = cgFunc.HandleExpr(intrnNode, *arg2);                  /* vector src operand 2 */
  return cgFunc.SelectVectorPairwiseAdalp(src1, arg1->GetPrimType(), src2, arg2->GetPrimType());
}

Operand *HandleVectorSetElement(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  BaseNode *arg0 = intrnNode.Opnd(0);                                  /* uint32_t operand */
  Operand *opnd0 = cgFunc.HandleExpr(intrnNode, *arg0);
  PrimType aType = arg0->GetPrimType();

  BaseNode *arg1 = intrnNode.Opnd(1);                                  /* vector operand == result */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *arg1);
  PrimType vType = arg1->GetPrimType();

  BaseNode *arg2 = intrnNode.Opnd(2);                                  /* lane const operand */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *arg2);
  int32 laneNum = -1;
  if (opnd2->IsConstImmediate()) {
    MIRConst *mirConst = static_cast<ConstvalNode*>(arg2)->GetConstVal();
    laneNum = static_cast<int32>(safe_cast<MIRIntConst>(mirConst)->GetExtValue());
  } else {
    CHECK_FATAL(0, "VectorSetElement does not have lane const");
  }
  return cgFunc.SelectVectorSetElement(opnd0, aType, opnd1, vType, laneNum);
}

Operand *HandleVectorReverse(const IntrinsicopNode &intrnNode, CGFunc &cgFunc, uint32 size) {
  BaseNode *argExpr = intrnNode.Opnd(0);                               /* src operand */
  Operand *src = cgFunc.HandleExpr(intrnNode, *argExpr);
  MIRType *type = intrnNode.GetIntrinDesc().GetReturnType();
  ASSERT(type != nullptr, "null ptr check");
  auto revVecType = type->GetPrimType();
  return cgFunc.SelectVectorReverse(revVecType, src, revVecType, size);
}

Operand *HandleVectorShiftNarrow(const IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow) {
  PrimType rType = intrnNode.GetPrimType();                          /* vector result */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* shift const */
  if (!opnd2->IsConstImmediate()) {
    CHECK_FATAL(0, "VectorShiftNarrow does not have shift const");
  }
  return cgFunc.SelectVectorShiftRNarrow(rType, opnd1, intrnNode.Opnd(0)->GetPrimType(), opnd2, isLow);
}

Operand *HandleVectorSubWiden(const IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow, bool isWide) {
  PrimType resType = intrnNode.GetPrimType();                          /* uint32_t result */
  Operand *o1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));
  Operand *o2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));
  return cgFunc.SelectVectorSubWiden(resType, o1, intrnNode.Opnd(0)->GetPrimType(),
                                     o2, intrnNode.Opnd(1)->GetPrimType(), isLow, isWide);
}

Operand *HandleVectorSum(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  PrimType resType = intrnNode.GetPrimType();                          /* uint32_t result */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand */
  return cgFunc.SelectVectorSum(resType, opnd1, intrnNode.Opnd(0)->GetPrimType());
}

Operand *HandleVectorMadd(const IntrinsicopNode &intrnNode, CGFunc &cgFunc) {
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand 1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand 2 */
  Operand *opnd3 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(2));   /* vector operand 3 */
  PrimType oTyp1 = intrnNode.Opnd(0)->GetPrimType();
  PrimType oTyp2 = intrnNode.Opnd(1)->GetPrimType();
  PrimType oTyp3 = intrnNode.Opnd(2)->GetPrimType();
  return cgFunc.SelectVectorMadd(opnd1, oTyp1, opnd2, oTyp2, opnd3, oTyp3);
}

Operand *HandleVectorMull(const IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector operand 1 */
  Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1));   /* vector operand 2 */
  PrimType oTyp1 = intrnNode.Opnd(0)->GetPrimType();
  PrimType oTyp2 = intrnNode.Opnd(1)->GetPrimType();
  return cgFunc.SelectVectorMull(rType, opnd1, oTyp1, opnd2, oTyp2, isLow);
}

Operand *HandleVectorNarrow(const IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector opnd 1 */
  if (isLow) {
    return cgFunc.SelectVectorNarrow(rType, opnd1, intrnNode.Opnd(0)->GetPrimType());
  } else {
    Operand *opnd2 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(1)); /* vector opnd 2 */
    return cgFunc.SelectVectorNarrow2(rType, opnd1, intrnNode.Opnd(0)->GetPrimType(), opnd2,
                                      intrnNode.Opnd(1)->GetPrimType());
  }
}

Operand *HandleVectorWiden(const IntrinsicopNode &intrnNode, CGFunc &cgFunc, bool isLow) {
  PrimType rType = intrnNode.GetPrimType();                            /* result operand */
  Operand *opnd1 = cgFunc.HandleExpr(intrnNode, *intrnNode.Opnd(0));   /* vector opnd 1 */
  return cgFunc.SelectVectorWiden(rType, opnd1, intrnNode.Opnd(0)->GetPrimType(), isLow);
}

Operand *HandleVectorMovNarrow(const IntrinsicopNode &intrinsicNode, CGFunc &cgFunc) {
  PrimType rType = intrinsicNode.GetPrimType();                            /* result operand */
  Operand *opnd = cgFunc.HandleExpr(intrinsicNode, *intrinsicNode.Opnd(0));   /* vector opnd 1 */
  return cgFunc.SelectVectorMovNarrow(rType, opnd, intrinsicNode.Opnd(0)->GetPrimType());
}

Operand *HandleVectorIntrinsics(const IntrinsicopNode &intrinsicNode, CGFunc &cgFunc) {
  return cgFunc.SelectVectorIntrinsics(intrinsicNode);
}

Operand *HandleIntrinOp(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc) {
  auto &intrinsicopNode = static_cast<IntrinsicopNode&>(expr);
  auto intrinsicId = intrinsicopNode.GetIntrinsic();
  IntrinDesc &intrinsicDesc = IntrinDesc::intrinTable[intrinsicId];
  switch (intrinsicId) {
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
      return cgFunc.SelectLazyLoadStatic(*st, mirIntConst->GetExtValue(), intrinsicopNode.GetPrimType());
    }
    case INTRN_MPL_READ_ARRAYCLASS_CACHE_ENTRY: {
      auto addrOfNode = static_cast<AddrofNode*>(intrinsicopNode.Opnd(0));
      MIRSymbol *st = cgFunc.GetMirModule().CurFunction()->GetLocalOrGlobalSymbol(addrOfNode->GetStIdx());
      auto constNode = static_cast<ConstvalNode*>(intrinsicopNode.Opnd(1));
      CHECK_FATAL(constNode != nullptr, "null ptr check");
      auto mirIntConst = static_cast<MIRIntConst*>(constNode->GetConstVal());
      return cgFunc.SelectLoadArrayClassCache(*st, mirIntConst->GetExtValue(), intrinsicopNode.GetPrimType());
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
    case INTRN_C_fabsl:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "fabsl");
    // int
    case INTRN_C_ffs:
      return cgFunc.SelectIntrinsicOpWithOneParam(intrinsicopNode, "ffs");
    // libc mem* and str* functions as intrinsicops
    case INTRN_C_memcmp:
      return cgFunc.SelectIntrinsicOpWithNParams(intrinsicopNode, PTY_i32, "memcmp");
    case INTRN_C_strlen:
      return cgFunc.SelectIntrinsicOpWithNParams(intrinsicopNode, PTY_u64, "strlen");
    case INTRN_C_strcmp:
      return cgFunc.SelectIntrinsicOpWithNParams(intrinsicopNode, PTY_i32, "strcmp");
    case INTRN_C_strncmp:
      return cgFunc.SelectIntrinsicOpWithNParams(intrinsicopNode, PTY_i32, "strncmp");
    case INTRN_C_strchr:
      return cgFunc.SelectIntrinsicOpWithNParams(intrinsicopNode, PTY_a64, "strchr");
    case INTRN_C_strrchr:
      return cgFunc.SelectIntrinsicOpWithNParams(intrinsicopNode, PTY_a64, "strrchr");
    case INTRN_C_rev16_2:
    case INTRN_C_rev_4:
    case INTRN_C_rev_8:
    case INTRN_C_bswap64:
    case INTRN_C_bswap32:
    case INTRN_C_bswap16:
      return cgFunc.SelectBswap(intrinsicopNode, *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
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
    case INTRN_C___sync_add_and_fetch_1:
    case INTRN_C___sync_add_and_fetch_2:
    case INTRN_C___sync_add_and_fetch_4:
    case INTRN_C___sync_add_and_fetch_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpAdd, false);
    case INTRN_C___sync_sub_and_fetch_1:
    case INTRN_C___sync_sub_and_fetch_2:
    case INTRN_C___sync_sub_and_fetch_4:
    case INTRN_C___sync_sub_and_fetch_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpSub, false);
    case INTRN_C___sync_fetch_and_add_1:
    case INTRN_C___sync_fetch_and_add_2:
    case INTRN_C___sync_fetch_and_add_4:
    case INTRN_C___sync_fetch_and_add_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpAdd, true);
    case INTRN_C___sync_fetch_and_sub_1:
    case INTRN_C___sync_fetch_and_sub_2:
    case INTRN_C___sync_fetch_and_sub_4:
    case INTRN_C___sync_fetch_and_sub_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpSub, true);
    case INTRN_C___sync_bool_compare_and_swap_1:
    case INTRN_C___sync_bool_compare_and_swap_2:
    case INTRN_C___sync_bool_compare_and_swap_4:
    case INTRN_C___sync_bool_compare_and_swap_8:
      return cgFunc.SelectCSyncBoolCmpSwap(intrinsicopNode);
    case INTRN_C___sync_val_compare_and_swap_1:
    case INTRN_C___sync_val_compare_and_swap_2:
    case INTRN_C___sync_val_compare_and_swap_4:
    case INTRN_C___sync_val_compare_and_swap_8:
      return cgFunc.SelectCSyncValCmpSwap(intrinsicopNode);
    case INTRN_C___sync_lock_test_and_set_1:
      return cgFunc.SelectCSyncLockTestSet(intrinsicopNode, PTY_i8);
    case INTRN_C___sync_lock_test_and_set_2:
      return cgFunc.SelectCSyncLockTestSet(intrinsicopNode, PTY_i16);
    case INTRN_C___sync_lock_test_and_set_4:
      return cgFunc.SelectCSyncLockTestSet(intrinsicopNode, PTY_i32);
    case INTRN_C___sync_lock_test_and_set_8:
      return cgFunc.SelectCSyncLockTestSet(intrinsicopNode, PTY_i64);
    case INTRN_C___sync_fetch_and_and_1:
    case INTRN_C___sync_fetch_and_and_2:
    case INTRN_C___sync_fetch_and_and_4:
    case INTRN_C___sync_fetch_and_and_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpAnd, true);
    case INTRN_C___sync_and_and_fetch_1:
    case INTRN_C___sync_and_and_fetch_2:
    case INTRN_C___sync_and_and_fetch_4:
    case INTRN_C___sync_and_and_fetch_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpAnd, false);
    case INTRN_C___sync_fetch_and_or_1:
    case INTRN_C___sync_fetch_and_or_2:
    case INTRN_C___sync_fetch_and_or_4:
    case INTRN_C___sync_fetch_and_or_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpOr, true);
    case INTRN_C___sync_or_and_fetch_1:
    case INTRN_C___sync_or_and_fetch_2:
    case INTRN_C___sync_or_and_fetch_4:
    case INTRN_C___sync_or_and_fetch_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpOr, false);
    case INTRN_C___sync_fetch_and_xor_1:
    case INTRN_C___sync_fetch_and_xor_2:
    case INTRN_C___sync_fetch_and_xor_4:
    case INTRN_C___sync_fetch_and_xor_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpXor, true);
    case INTRN_C___sync_xor_and_fetch_1:
    case INTRN_C___sync_xor_and_fetch_2:
    case INTRN_C___sync_xor_and_fetch_4:
    case INTRN_C___sync_xor_and_fetch_8:
      return cgFunc.SelectCSyncFetch(intrinsicopNode, kSyncAndAtomicOpXor, false);
    case INTRN_C___sync_synchronize:
      return cgFunc.SelectCSyncSynchronize(intrinsicopNode);
    case INTRN_C___atomic_load_n:
      return cgFunc.SelectCAtomicLoadN(intrinsicopNode);
    case INTRN_C___atomic_fetch_add:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpAdd, true);
    case INTRN_C___atomic_fetch_sub:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpSub, true);
    case INTRN_C___atomic_fetch_and:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpAnd, true);
    case INTRN_C___atomic_fetch_or:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpOr, true);
    case INTRN_C___atomic_fetch_xor:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpXor, true);
    case INTRN_C___atomic_fetch_nand:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpNand, true);
    case INTRN_C___atomic_add_fetch:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpAdd, false);
    case INTRN_C___atomic_sub_fetch:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpSub, false);
    case INTRN_C___atomic_and_fetch:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpAnd, false);
    case INTRN_C___atomic_or_fetch:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpOr, false);
    case INTRN_C___atomic_xor_fetch:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpXor, false);
    case INTRN_C___atomic_exchange_n:
      return cgFunc.SelectCAtomicExchangeN(intrinsicopNode);
    case INTRN_C___atomic_nand_fetch:
      return cgFunc.SelectCAtomicFetch(intrinsicopNode, kSyncAndAtomicOpNand, false);
    case INTRN_C___atomic_compare_exchange_n :
    case INTRN_C___atomic_compare_exchange :
      return cgFunc.SelectCAtomicCompareExchange(intrinsicopNode);

    case INTRN_C__builtin_return_address:
    case INTRN_C__builtin_extract_return_addr:
      return cgFunc.SelectCReturnAddress(intrinsicopNode);

    case INTRN_vector_abs_v8i8: case INTRN_vector_abs_v4i16:
    case INTRN_vector_abs_v2i32: case INTRN_vector_abs_v1i64:
    case INTRN_vector_abs_v16i8: case INTRN_vector_abs_v8i16:
    case INTRN_vector_abs_v4i32: case INTRN_vector_abs_v2i64:
      return HandleAbs(parent, intrinsicopNode, cgFunc);

    case INTRN_vector_addl_low_v8i8: case INTRN_vector_addl_low_v8u8:
    case INTRN_vector_addl_low_v4i16: case INTRN_vector_addl_low_v4u16:
    case INTRN_vector_addl_low_v2i32: case INTRN_vector_addl_low_v2u32:
      return HandleVectorAddLong(intrinsicopNode, cgFunc, true);

    case INTRN_vector_addl_high_v8i8: case INTRN_vector_addl_high_v8u8:
    case INTRN_vector_addl_high_v4i16: case INTRN_vector_addl_high_v4u16:
    case INTRN_vector_addl_high_v2i32: case INTRN_vector_addl_high_v2u32:
      return HandleVectorAddLong(intrinsicopNode, cgFunc, false);

    case INTRN_vector_addw_low_v8i8: case INTRN_vector_addw_low_v8u8:
    case INTRN_vector_addw_low_v4i16: case INTRN_vector_addw_low_v4u16:
    case INTRN_vector_addw_low_v2i32: case INTRN_vector_addw_low_v2u32:
      return HandleVectorAddWiden(intrinsicopNode, cgFunc, true);

    case INTRN_vector_addw_high_v8i8: case INTRN_vector_addw_high_v8u8:
    case INTRN_vector_addw_high_v4i16: case INTRN_vector_addw_high_v4u16:
    case INTRN_vector_addw_high_v2i32: case INTRN_vector_addw_high_v2u32:
      return HandleVectorAddWiden(intrinsicopNode, cgFunc, false);

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

    case INTRN_vector_labssub_low_v8u8: case INTRN_vector_labssub_low_v8i8:
    case INTRN_vector_labssub_low_v4u16: case INTRN_vector_labssub_low_v4i16:
    case INTRN_vector_labssub_low_v2u32: case INTRN_vector_labssub_low_v2i32:
      return HandleVectorAbsSubL(intrinsicopNode, cgFunc, true);

    case INTRN_vector_labssub_high_v8u8: case INTRN_vector_labssub_high_v8i8:
    case INTRN_vector_labssub_high_v4u16: case INTRN_vector_labssub_high_v4i16:
    case INTRN_vector_labssub_high_v2u32: case INTRN_vector_labssub_high_v2i32:
      return HandleVectorAbsSubL(intrinsicopNode, cgFunc, false);

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

    case INTRN_vector_pairwise_adalp_v8i8: case INTRN_vector_pairwise_adalp_v4i16:
    case INTRN_vector_pairwise_adalp_v2i32: case INTRN_vector_pairwise_adalp_v8u8:
    case INTRN_vector_pairwise_adalp_v4u16: case INTRN_vector_pairwise_adalp_v2u32:
    case INTRN_vector_pairwise_adalp_v16i8: case INTRN_vector_pairwise_adalp_v8i16:
    case INTRN_vector_pairwise_adalp_v4i32: case INTRN_vector_pairwise_adalp_v16u8:
    case INTRN_vector_pairwise_adalp_v8u16: case INTRN_vector_pairwise_adalp_v4u32:
      return HandleVectorPairwiseAdalp(intrinsicopNode, cgFunc);

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

    case INTRN_vector_mull_low_v8u8: case INTRN_vector_mull_low_v8i8:
    case INTRN_vector_mull_low_v4u16: case INTRN_vector_mull_low_v4i16:
    case INTRN_vector_mull_low_v2u32: case INTRN_vector_mull_low_v2i32:
      return HandleVectorMull(intrinsicopNode, cgFunc, true);

    case INTRN_vector_mull_high_v8u8: case INTRN_vector_mull_high_v8i8:
    case INTRN_vector_mull_high_v4u16: case INTRN_vector_mull_high_v4i16:
    case INTRN_vector_mull_high_v2u32: case INTRN_vector_mull_high_v2i32:
      return HandleVectorMull(intrinsicopNode, cgFunc, false);

    case INTRN_vector_narrow_low_v8u16: case INTRN_vector_narrow_low_v8i16:
    case INTRN_vector_narrow_low_v4u32: case INTRN_vector_narrow_low_v4i32:
    case INTRN_vector_narrow_low_v2u64: case INTRN_vector_narrow_low_v2i64:
      return HandleVectorNarrow(intrinsicopNode, cgFunc, true);

    case INTRN_vector_narrow_high_v8u16: case INTRN_vector_narrow_high_v8i16:
    case INTRN_vector_narrow_high_v4u32: case INTRN_vector_narrow_high_v4i32:
    case INTRN_vector_narrow_high_v2u64: case INTRN_vector_narrow_high_v2i64:
      return HandleVectorNarrow(intrinsicopNode, cgFunc, false);

    case INTRN_vector_reverse_v8u8: case INTRN_vector_reverse_v8i8:
    case INTRN_vector_reverse_v4u16: case INTRN_vector_reverse_v4i16:
    case INTRN_vector_reverse_v16u8: case INTRN_vector_reverse_v16i8:
    case INTRN_vector_reverse_v8u16: case INTRN_vector_reverse_v8i16:
      return HandleVectorReverse(intrinsicopNode, cgFunc, k32BitSize);

    case INTRN_vector_reverse16_v16u8: case INTRN_vector_reverse16_v16i8:
    case INTRN_vector_reverse16_v8u8: case INTRN_vector_reverse16_v8i8:
      return HandleVectorReverse(intrinsicopNode, cgFunc, k16BitSize);

    case INTRN_vector_reverse64_v16u8: case INTRN_vector_reverse64_v16i8:
    case INTRN_vector_reverse64_v8u8: case INTRN_vector_reverse64_v8i8:
    case INTRN_vector_reverse64_v8u16: case INTRN_vector_reverse64_v8i16:
    case INTRN_vector_reverse64_v4u16: case INTRN_vector_reverse64_v4i16:
    case INTRN_vector_reverse64_v4u32: case INTRN_vector_reverse64_v4i32:
    case INTRN_vector_reverse64_v2u32: case INTRN_vector_reverse64_v2i32:
      return HandleVectorReverse(intrinsicopNode, cgFunc, k64BitSize);

    case INTRN_vector_shr_narrow_low_v8u16: case INTRN_vector_shr_narrow_low_v8i16:
    case INTRN_vector_shr_narrow_low_v4u32: case INTRN_vector_shr_narrow_low_v4i32:
    case INTRN_vector_shr_narrow_low_v2u64: case INTRN_vector_shr_narrow_low_v2i64:
      return HandleVectorShiftNarrow(intrinsicopNode, cgFunc, true);

    case INTRN_vector_subl_low_v8i8: case INTRN_vector_subl_low_v8u8:
    case INTRN_vector_subl_low_v4i16: case INTRN_vector_subl_low_v4u16:
    case INTRN_vector_subl_low_v2i32: case INTRN_vector_subl_low_v2u32:
      return HandleVectorSubWiden(intrinsicopNode, cgFunc, true, false);

    case INTRN_vector_subl_high_v8i8: case INTRN_vector_subl_high_v8u8:
    case INTRN_vector_subl_high_v4i16: case INTRN_vector_subl_high_v4u16:
    case INTRN_vector_subl_high_v2i32: case INTRN_vector_subl_high_v2u32:
      return HandleVectorSubWiden(intrinsicopNode, cgFunc, false, false);

    case INTRN_vector_subw_low_v8i8: case INTRN_vector_subw_low_v8u8:
    case INTRN_vector_subw_low_v4i16: case INTRN_vector_subw_low_v4u16:
    case INTRN_vector_subw_low_v2i32: case INTRN_vector_subw_low_v2u32:
      return HandleVectorSubWiden(intrinsicopNode, cgFunc, true, true);

    case INTRN_vector_subw_high_v8i8: case INTRN_vector_subw_high_v8u8:
    case INTRN_vector_subw_high_v4i16: case INTRN_vector_subw_high_v4u16:
    case INTRN_vector_subw_high_v2i32: case INTRN_vector_subw_high_v2u32:
      return HandleVectorSubWiden(intrinsicopNode, cgFunc, false, true);

    case INTRN_vector_widen_low_v8u8: case INTRN_vector_widen_low_v8i8:
    case INTRN_vector_widen_low_v4u16: case INTRN_vector_widen_low_v4i16:
    case INTRN_vector_widen_low_v2u32: case INTRN_vector_widen_low_v2i32:
      return HandleVectorWiden(intrinsicopNode, cgFunc, true);

    case INTRN_vector_widen_high_v8u8: case INTRN_vector_widen_high_v8i8:
    case INTRN_vector_widen_high_v4u16: case INTRN_vector_widen_high_v4i16:
    case INTRN_vector_widen_high_v2u32: case INTRN_vector_widen_high_v2i32:
      return HandleVectorWiden(intrinsicopNode, cgFunc, false);

    case INTRN_vector_mov_narrow_v2i64: case INTRN_vector_mov_narrow_v2u64:
    case INTRN_vector_mov_narrow_v4i32: case INTRN_vector_mov_narrow_v4u32:
    case INTRN_vector_mov_narrow_v8i16: case INTRN_vector_mov_narrow_v8u16:
      return HandleVectorMovNarrow(intrinsicopNode, cgFunc);
    default: {
      if (!intrinsicDesc.IsVectorOp()) {
        CHECK_FATAL(false, "Unsupported intrinsicop.");
        return nullptr;
      }
      return HandleVectorIntrinsics(intrinsicopNode, cgFunc);
    }
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
  RegisterFactoryFunction<HandleExprFactory>(OP_ror, HandleRor);
  RegisterFactoryFunction<HandleExprFactory>(OP_mul, HandleMpy);
  RegisterFactoryFunction<HandleExprFactory>(OP_div, HandleDiv);
  RegisterFactoryFunction<HandleExprFactory>(OP_rem, HandleRem);
  RegisterFactoryFunction<HandleExprFactory>(OP_addrof, HandleAddrof);
  RegisterFactoryFunction<HandleExprFactory>(OP_addrofoff, HandleAddrofoff);
  RegisterFactoryFunction<HandleExprFactory>(OP_addroffunc, HandleAddroffunc);
  RegisterFactoryFunction<HandleExprFactory>(OP_addroflabel, HandleAddrofLabel);
  RegisterFactoryFunction<HandleExprFactory>(OP_iread, HandleIread);
  RegisterFactoryFunction<HandleExprFactory>(OP_ireadoff, HandleIreadoff);
  RegisterFactoryFunction<HandleExprFactory>(OP_ireadfpoff, HandleIreadfpoff);
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
  if (newBB->GetId() == 1) {
    newBB->SetFrequency(kFreqBase);
  }
  cgFunc.SetLab2BBMap(newBB->GetLabIdx(), *newBB);
  cgFunc.SetCurBB(*newBB);

  if (cgFunc.GetCleanupLabel() == &label) {
    cgFunc.SetCleanupBB(*newBB);
  } else if (cgFunc.GetReturnLabel() == &label) {
    cgFunc.SetReturnBB(*newBB);
  }
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
    if ((constValNode->GetConstVal()->IsZero() && (condOp == OP_brfalse)) ||
        (!constValNode->GetConstVal()->IsZero() && (condOp == OP_brtrue))) {
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
      ASSERT(((primType == PTY_f32) || (primType == PTY_f64)), "we don't support half-precision FP operands yet");
      zeroOpnd = &cgFunc.CreateImmOperand(primType, 0);
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
    if (!cgFunc.GetFunction().StructReturnedInRegs()) {
      opnd = cgFunc.HandleExpr(retNode, *retNode.Opnd(0));
    } else {
      cgFunc.SelectReturnSendOfStructInRegs(retNode.Opnd(0));
    }
  }
  cgFunc.SelectReturn(opnd);
  cgFunc.SetCurBBKind(BB::kBBGoto);
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
    return;
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

void HandleDassignoff(StmtNode &stmt, CGFunc &cgFunc) {
  auto &dassignoffNode = static_cast<DassignoffNode&>(stmt);
  BaseNode *rhs = dassignoffNode.GetRHS();
  CHECK_FATAL(rhs->GetOpCode() == OP_constval, "dassignoffNode without constval");
  Operand *opnd0 = cgFunc.HandleExpr(dassignoffNode, *rhs);
  cgFunc.SelectDassignoff(dassignoffNode, *opnd0);
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

void HandleIassignoff(StmtNode &stmt, CGFunc &cgFunc) {
  ASSERT(stmt.GetOpCode() == OP_iassignoff, "expect iassignoff");
  auto &iassignoffNode = static_cast<IassignoffNode&>(stmt);
  cgFunc.SelectIassignoff(iassignoffNode);
}

void HandleIassignfpoff(StmtNode &stmt, CGFunc &cgFunc) {
  ASSERT(stmt.GetOpCode() == OP_iassignfpoff, "expect iassignfpoff");
  auto &iassignfpoffNode = static_cast<IassignFPoffNode&>(stmt);
  cgFunc.SelectIassignfpoff(iassignfpoffNode, *cgFunc.HandleExpr(stmt, *stmt.Opnd(0)));
}

void HandleIassignspoff(StmtNode &stmt, CGFunc &cgFunc) {
  ASSERT(stmt.GetOpCode() == OP_iassignspoff, "expect iassignspoff");
  auto &baseNode = static_cast<IassignFPoffNode&>(stmt); /* same as FP */
  BaseNode *rhs = baseNode.GetRHS();
  ASSERT(rhs != nullptr, "get rhs of iassignspoffNode failed");
  Operand *opnd0 = cgFunc.HandleExpr(baseNode, *rhs);
  cgFunc.SelectIassignspoff(baseNode.GetPrimType(), baseNode.GetOffset(), *opnd0);
}

void HandleBlkassignoff(StmtNode &stmt, CGFunc &cgFunc) {
  ASSERT(stmt.GetOpCode() == OP_blkassignoff, "expect blkassignoff");
  auto &baseNode = static_cast<BlkassignoffNode&>(stmt);
  Operand *src = cgFunc.HandleExpr(baseNode, *baseNode.Opnd(1));
  cgFunc.SelectBlkassignoff(baseNode, *src);
}

void HandleEval(const StmtNode &stmt, CGFunc &cgFunc) {
  (void)cgFunc.HandleExpr(stmt, *static_cast<const UnaryStmtNode&>(stmt).Opnd(0));
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

void HandleCatchOp(const StmtNode &stmt, const CGFunc &cgFunc) {
  (void)stmt;
  (void)cgFunc;
  ASSERT(stmt.GetNext()->GetOpCode() == OP_call, "The next statement of OP_catch should be OP_call.");
}

void HandleAssertNull(StmtNode &stmt, CGFunc &cgFunc) {
  auto &cgAssertNode = static_cast<UnaryStmtNode&>(stmt);
  cgFunc.SelectAssertNull(cgAssertNode);
}

void HandleAbort(const StmtNode &stmt, CGFunc &cgFunc) {
  (void)stmt;
  cgFunc.SelectAbort();
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
  RegisterFactoryFunction<HandleStmtFactory>(OP_icallproto, HandleICall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccall, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallassigned, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallwithtype, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallwithtypeassigned, HandleIntrinCall);
  RegisterFactoryFunction<HandleStmtFactory>(OP_dassign, HandleDassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_dassignoff, HandleDassignoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_regassign, HandleRegassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassign, HandleIassign);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassignoff, HandleIassignoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassignfpoff, HandleIassignfpoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_iassignspoff, HandleIassignspoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_blkassignoff, HandleBlkassignoff);
  RegisterFactoryFunction<HandleStmtFactory>(OP_eval, HandleEval);
  RegisterFactoryFunction<HandleStmtFactory>(OP_rangegoto, HandleRangeGoto);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membarrelease, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membaracquire, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membarstoreload, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_membarstorestore, HandleMembar);
  RegisterFactoryFunction<HandleStmtFactory>(OP_comment, HandleComment);
  RegisterFactoryFunction<HandleStmtFactory>(OP_catch, HandleCatchOp);
  RegisterFactoryFunction<HandleStmtFactory>(OP_abort, HandleAbort);
  RegisterFactoryFunction<HandleStmtFactory>(OP_assertnonnull, HandleAssertNull);
  RegisterFactoryFunction<HandleStmtFactory>(OP_callassertnonnull, HandleAssertNull);
  RegisterFactoryFunction<HandleStmtFactory>(OP_assignassertnonnull, HandleAssertNull);
  RegisterFactoryFunction<HandleStmtFactory>(OP_returnassertnonnull, HandleAssertNull);
  RegisterFactoryFunction<HandleStmtFactory>(OP_asm, HandleAsm);
}

/* member of CGFunc */
void CGFunc::InitFactory() {
  InitHandleExprFactory();
}

CGFunc::CGFunc(MIRModule &mod, CG &cg, MIRFunction &mirFunc, BECommon &beCommon, MemPool &memPool,
               StackMemPool &stackMp, MapleAllocator &allocator, uint32 funcId)
    : bbVec(allocator.Adapter()),
      pRegSpillMemOperands(allocator.Adapter()),
      spillRegMemOperands(allocator.Adapter()),
      reuseSpillLocMem(allocator.Adapter()),
      labelMap(std::less<LabelIdx>(), allocator.Adapter()),
      vregsToPregsMap(std::less<regno_t>(), allocator.Adapter()),
      hasVLAOrAlloca(mirFunc.HasVlaOrAlloca()),
      dbgParamCallFrameLocations(allocator.Adapter()),
      dbgLocalCallFrameLocations(allocator.Adapter()),
      cg(&cg),
      mirModule(mod),
      memPool(&memPool),
      stackMp(stackMp),
      func(mirFunc),
      exitBBVec(allocator.Adapter()),
      noReturnCallBBVec(allocator.Adapter()),
      extendSet(allocator.Adapter()),
      lab2BBMap(allocator.Adapter()),
      beCommon(beCommon),
      funcScopeAllocator(&allocator),
      emitStVec(allocator.Adapter()),
      switchLabelCnt(allocator.Adapter()),
#if defined(TARGARM32) && TARGARM32
      sortedBBs(allocator.Adapter()),
      lrVec(allocator.Adapter()),
#endif  /* TARGARM32 */
      loops(allocator.Adapter()),
      lmbcParamVec(allocator.Adapter()),
      scpIdSet(allocator.Adapter()),
      shortFuncName(cg.ExtractFuncName(mirFunc.GetName()) + "." + std::to_string(funcId), &memPool) {
  mirModule.SetCurFunction(&func);
  SetMemlayout(*GetCG()->CreateMemLayout(memPool, beCommon, func, allocator));
  GetMemlayout()->SetCurrFunction(*this);
  SetTargetRegInfo(*GetCG()->CreateRegisterInfo(memPool, allocator));
  GetTargetRegInfo()->SetCurrFunction(*this);
  if (func.GetAttr(FUNCATTR_varargs) || func.HasVlaOrAlloca()) {
    SetHasVLAOrAlloca(true);
  }
  SetHasAlloca(func.HasVlaOrAlloca());

  dummyBB = CreateNewBB();
  vReg.SetCount(kBaseVirtualRegNO + func.GetPregTab()->Size());
  firstNonPregVRegNO = vReg.GetCount();
  /* maximum register count initial be increased by 1024 */
  SetMaxRegNum(vReg.GetCount() + 1024);
  if (func.GetMayWriteToAddrofStack()) {
    SetStackProtectInfo(kAddrofStack);
  }
  vReg.vRegOperandTable.clear();

  insnBuilder = memPool.New<InsnBuilder>(memPool);
  opndBuilder = memPool.New<OperandBuilder>(memPool, func.GetPregTab()->Size());

  vReg.VRegTableResize(GetMaxRegNum());
  /* func.GetPregTab()->_preg_table[0] is nullptr, so skip it */
  ASSERT(func.GetPregTab()->PregFromPregIdx(0) == nullptr, "PregFromPregIdx(0) must be nullptr");
  for (size_t i = 1; i < func.GetPregTab()->Size(); ++i) {
    PrimType primType = func.GetPregTab()->PregFromPregIdx(i)->GetPrimType();
    uint32 byteLen = GetPrimTypeSize(primType);
    if (byteLen < k4ByteSize) {
      byteLen = k4ByteSize;
    }
    if (primType == PTY_u128 || primType == PTY_i128) {
      byteLen = k8ByteSize;
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
  StmtNode *stmt = block->GetFirst();
  if (stmt == nullptr) {
    return nullptr;
  }
  bool withFreqInfo = func.HasFreqMap() && !func.GetLastFreqMap().empty();
  if (withFreqInfo) {
    frequency = kFreqBase;
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

bool CGFunc::CheckSkipMembarOp(const StmtNode &stmt) {
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
    return true;
  }
#if TARGAARCH64 || TARGRISCV64
  if ((!CGOptions::UseBarriersForVolatile()) && (nextStmt->GetOpCode() == OP_membaracquire)) {
    isVolLoad = true;
  }
#endif /* TARGAARCH64 */
  return false;
}

void CGFunc::RemoveUnreachableBB() {
  OptimizationPattern *pattern = memPool->New<UnreachBBPattern>(*this);
  for (BB *bb = firstBB; bb != nullptr; bb = bb->GetNext()) {
    (void)pattern->Optimize(*bb);
    if (bb->GetPreds().size() == 0 && bb->GetSuccs().size() == 0) {
      auto it = find(noReturnCallBBVec.begin(), noReturnCallBBVec.end(), bb);
      if (it != noReturnCallBBVec.end()) {
        (void)noReturnCallBBVec.erase(it);
      }
    }
  }
}

Insn &CGFunc::BuildLocInsn(int64 fileNum, int64 lineNum, int64 columnNum) {
  Operand *o0 = CreateDbgImmOperand(fileNum);
  Operand *o1 = CreateDbgImmOperand(lineNum);
  Operand *o2 = CreateDbgImmOperand(columnNum);
  Insn &loc =
      GetInsnBuilder()->BuildDbgInsn(mpldbg::OP_DBG_loc).AddOpndChain(*o0).AddOpndChain(*o1).AddOpndChain(*o2);
  return loc;
}

Insn &CGFunc::BuildScopeInsn(int64 id, bool isEnd) {
  Operand *o0 = CreateDbgImmOperand(id);
  Operand *o1 = CreateDbgImmOperand(isEnd ? 1 : 0);
  Insn &scope = GetInsnBuilder()->BuildDbgInsn(mpldbg::OP_DBG_scope).AddOpndChain(*o0).AddOpndChain(*o1);
  return scope;
}

void CGFunc::GenerateLoc(StmtNode &stmt, SrcPosition &lastSrcPos, SrcPosition &lastMplPos) {
  /* insert Insn for .loc before cg for the stmt */
  if (cg->GetCGOptions().WithLoc() && stmt.op != OP_label && stmt.op != OP_comment) {
    /* if original src file location info is availiable for this stmt,
     * use it and skip mpl file location info for this stmt
     */
    bool hasLoc = false;
    SrcPosition &newSrcPos = stmt.GetSrcPos();
    if (!newSrcPos.IsValid()) {
      return;
    }

    if (cg->GetCGOptions().WithSrc() && !lastSrcPos.IsEq(newSrcPos)) {
      /* .loc for original src file */
      curBB->AppendInsn(BuildLocInsn(newSrcPos.FileNum(), newSrcPos.LineNum(), newSrcPos.Column()));
      lastSrcPos.UpdateWith(newSrcPos);
      hasLoc = true;
    }
    /* .loc for mpl file, skip if already has .loc from src for this stmt */
    if (cg->GetCGOptions().WithMpl() && !hasLoc && !lastMplPos.IsEqMpl(newSrcPos)) {
      curBB->AppendInsn(BuildLocInsn(1, newSrcPos.MplLineNum(), 0));
      lastMplPos.UpdateWith(newSrcPos);
    }
  }
}

void CGFunc::GenerateScopeLabel(StmtNode &stmt, SrcPosition &lastSrcPos, bool &posDone) {
  /* insert lable for scope begin and end .LScp.1B .LScp.1E */
  MIRFunction &mirFunc = GetFunction();
  DebugInfo *dbgInfo = GetMirModule().GetDbgInfo();
  if (cg->GetCGOptions().WithDwarf() && stmt.op != OP_comment) {
    SrcPosition newSrcPos = stmt.GetSrcPos();
    if (!newSrcPos.IsValid()) {
      return;
    }
    // check if newSrcPos is done
    if (posDone && lastSrcPos.IsEq(newSrcPos)) {
      return;
    }
    std::unordered_set<uint32> idSetB;
    std::unordered_set<uint32> idSetE;
    idSetB.clear();
    idSetE.clear();
    dbgInfo->GetCrossScopeId(&mirFunc, idSetB, true, lastSrcPos, newSrcPos);
    dbgInfo->GetCrossScopeId(&mirFunc, idSetE, false, lastSrcPos, newSrcPos);
    for (auto id : idSetE) {
      // skip if begin label is not in yet
      if (scpIdSet.find(id) == scpIdSet.end()) {
        continue;
      }
      curBB->AppendInsn(BuildScopeInsn(id, true));
      (void)scpIdSet.erase(id);
    }
    for (auto id : idSetB) {
      // skip if begin label is already in
      if (scpIdSet.find(id) != scpIdSet.end()) {
        continue;
      }
      curBB->AppendInsn(BuildScopeInsn(id, false));
      (void)scpIdSet.insert(id);
    }
    lastSrcPos.UpdateWith(newSrcPos);
    posDone = false;
  }
}

FreqType CGFunc::GetFreqFromStmt(uint32 stmtId) {
  FreqType freq = GetFunction().GetFreqFromLastStmt(stmtId);
  if (freq != -1) {
    return freq;
  }
  return GetFunction().GetFreqFromFirstStmt(stmtId);
}
LmbcFormalParamInfo *CGFunc::GetLmbcFormalParamInfo(uint32 offset) {
  MapleVector<LmbcFormalParamInfo*> &paramVec = GetLmbcParamVec();
  for (auto *param : paramVec) {
    uint32 paramOffset = param->GetOffset();
    uint32 paramSize = param->GetSize();
    if (paramOffset <= offset && offset < (paramOffset + paramSize)) {
      return param;
    }
  }
  return nullptr;
}

/*
 * For formals of lmbc, the formal list is deleted if there is no
 * passing of aggregate by value.
 */
void CGFunc::CreateLmbcFormalParamInfo() {
  if (GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
    return;
  }
  PrimType primType;
  uint32 offset;
  uint32 typeSize;
  MIRFunction &lmbcFunc = GetFunction();
  if (lmbcFunc.GetFormalCount() > 0) {
    /* Whenever lmbc cannot delete call type info, the prototype is available */
    uint32 stackOffset = 0;
    for (size_t idx = 0; idx < lmbcFunc.GetFormalCount(); ++idx) {
      MIRSymbol *sym = lmbcFunc.GetFormal(idx);
      MIRType *type;
      TyIdx tyIdx;
      if (sym) {
        tyIdx = lmbcFunc.GetFormalDefVec()[idx].formalTyIdx;
        type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
      } else {
        FormalDef vec = (GetBecommon().GetMIRModule().CurFunction())->GetFormalDefAt(idx);
        tyIdx = vec.formalTyIdx;
        type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
      }
      primType = type->GetPrimType();
      offset = stackOffset;
      typeSize = static_cast<uint32>(GetBecommon().GetTypeSize(tyIdx));
      stackOffset += (typeSize + 7) & (-8);
      LmbcFormalParamInfo *info = GetMemoryPool()->New<LmbcFormalParamInfo>(primType, offset, typeSize);
      lmbcParamVec.push_back(info);
      if (idx == 0 && lmbcFunc.IsFirstArgReturn()) {
        info->SetIsReturn();
      }
      if (type->GetKind() == kTypeStruct) {
        MIRStructType *structType = static_cast<MIRStructType *>(type);
        info->SetType(structType);
        uint32 fpSize;
        uint32 numFpRegs = FloatParamRegRequired(structType, fpSize);
        if (numFpRegs > 0) {
          info->SetIsPureFloat();
          info->SetNumRegs(numFpRegs);
          info->SetFpSize(fpSize);
        }
      }
    }
  } else {
    /* No aggregate pass by value here */
    for (StmtNode *stmt = lmbcFunc.GetBody()->GetFirst(); stmt != nullptr; stmt = stmt->GetNext()) {
      if (stmt->GetOpCode() == OP_label) {
        continue;
      }
      if (stmt->GetOpCode() != OP_regassign) {
        break;
      }
      RegassignNode *regAssignNode = static_cast<RegassignNode *>(stmt);
      BaseNode *operand = regAssignNode->Opnd(0);
      if (operand->GetOpCode() != OP_ireadfpoff) {
        break;
      }
      IreadFPoffNode *ireadNode = static_cast<IreadFPoffNode *>(operand);
      primType = ireadNode->GetPrimType();
      typeSize = GetPrimTypeSize(primType);
      if (ireadNode->GetOffset() < 0) {
        continue;
      }
      offset = static_cast<uint32>(ireadNode->GetOffset());
      CHECK_FATAL((offset % k8ByteSize) == 0, "");  /* scalar only, no struct for now */
      LmbcFormalParamInfo *info = GetMemoryPool()->New<LmbcFormalParamInfo>(primType, offset, typeSize);
      lmbcParamVec.push_back(info);
    }
  }
  std::sort(lmbcParamVec.begin(), lmbcParamVec.end(),
            [] (const LmbcFormalParamInfo *x, const LmbcFormalParamInfo *y)
            { return x->GetOffset() < y->GetOffset(); }
            );

  /* When a scalar param address is taken, its regassign is not in the 1st block */
  for (StmtNode *stmt = lmbcFunc.GetBody()->GetFirst(); stmt != nullptr; stmt = stmt->GetNext()) {
    if (stmt->GetOpCode() != OP_regassign) {
      break;
    }
    if (stmt->GetOpCode() == OP_label) {
      continue;
    }
    RegassignNode *regAssignNode = static_cast<RegassignNode *>(stmt);
    BaseNode *operand = regAssignNode->Opnd(0);
    if (operand->GetOpCode() != OP_ireadfpoff) {
      break;
    }
    IreadFPoffNode *ireadNode = static_cast<IreadFPoffNode *>(operand);
    if (ireadNode->GetOffset() < 0) {
      continue;
    }
    LmbcFormalParamInfo *info = GetLmbcFormalParamInfo(static_cast<uint32>(ireadNode->GetOffset()));
    ASSERT_NOT_NULL(info);
    info->SetHasRegassign();
  }

  AssignLmbcFormalParams();
}


void CGFunc::GenerateInstruction() {
  InitHandleExprFactory();
  InitHandleStmtFactory();
  StmtNode *secondStmt = HandleFirstStmt();

  /* First Pass: Creates the doubly-linked list of BBs (next,prev) */
  volReleaseInsn = nullptr;

  SrcPosition pos = GetFunction().GetScope()->GetRangeLow();
  if (!pos.IsValid()) {
    pos = GetFunction().GetSrcPosition();
  }
  SrcPosition lastScpPos = pos;
  SrcPosition lastStmtPos = pos;
  SrcPosition lastLocPos = SrcPosition();
  SrcPosition lastMplPos = SrcPosition();
  std::set<uint32> bbFreqSet;
  bool posDone = false;
  scpIdSet.clear();
  for (StmtNode *stmt = secondStmt; stmt != nullptr; stmt = stmt->GetNext()) {
    /* insert Insn for scope begin/end labels */
    if (lastStmtPos.IsBfOrEq(stmt->GetSrcPos())) {
      GenerateScopeLabel(*stmt, lastScpPos, posDone);
      lastStmtPos = stmt->GetSrcPos();
    }
    /* insert Insn for .loc before cg for the stmt */
    GenerateLoc(*stmt, lastLocPos, lastMplPos);
    BB *tmpBB = curBB;
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
    int64_t freq = GetFreqFromStmt(stmt->GetStmtID());
    if (freq != -1) {
      if (tmpBB != curBB) {
        if (curBB->GetFirstInsn() == nullptr && curBB->GetLabIdx() == 0 && bbFreqSet.count(tmpBB->GetId()) == 0) {
          tmpBB->SetFrequency(static_cast<uint32>(freq));
          bbFreqSet.insert(tmpBB->GetId());
        } else if ((curBB->GetFirstInsn() != nullptr  || curBB->GetLabIdx() != 0) &&
                   bbFreqSet.count(curBB->GetId()) == 0) {
          curBB->SetFrequency(static_cast<uint32>(freq));
          bbFreqSet.insert(tmpBB->GetId());
        }
      } else if (bbFreqSet.count(curBB->GetId()) == 0) {
        curBB->SetFrequency(static_cast<uint32>(freq));
        bbFreqSet.insert(curBB->GetId());
      }
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

void CGFunc::TraverseAndClearCatchMark(BB &bb) {
  /* has bb been visited */
  if (bb.GetInternalFlag3() != 0) {
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
    if (bb->IsCleanup()) {
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
 * Mark CleanupBB
 * Note: Cleanup bb and func body bbs are seperated, no edges between them.
 * No ehSuccs or eh_prevs between cleanup bbs.
 */
void CGFunc::MarkCleanupBB() const {
  /* there is no cleanup BB in the function */
  if (cleanupBB == nullptr) {
    return;
  }
  ASSERT(ExitbbNotInCleanupArea(*cleanupBB), "exitBB created in cleanupArea.");
  ASSERT(cleanupBB->GetEhSuccs().empty(), "CG internal error. Cleanup bb should not have ehSuccs.");

#if defined(DEBUG) && DEBUG  /* Please don't remove me. */
  /* Check if all of the cleanup bb is at bottom of the function. */
  bool isCleanupArea = true;
  if (!mirModule.IsCModule()) {
    FOR_ALL_BB_REV_CONST(bb, this) {
      if (bb == GetLastBB()) {
        continue;
      }
      if (isCleanupArea) {
        ASSERT(bb->IsCleanup(), "CG internal error, cleanup BBs should be at the bottom of the function.");
      } else {
        ASSERT(!bb->IsCleanup(), "CG internal error, cleanup BBs should be at the bottom of the function.");
      }

      if (bb == cleanupBB) {
        isCleanupArea = false;
      }
    }
  }
#endif  /* DEBUG */
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
bool CGFunc::MemBarOpt(const StmtNode &membar) {
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
      ASSERT(fsym != nullptr, "null ptr check");
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

void CGFunc::MakeupScopeLabels(BB &bb) {
  /* insert leftover scope-end labels */
  if (!scpIdSet.empty()) {
    std::set<uint32>::const_reverse_iterator rit;
    for (rit=scpIdSet.rbegin(); rit != scpIdSet.rend(); ++rit) {
      bb.AppendInsn(BuildScopeInsn(*rit, true));
    }
  }
}

void CGFunc::ProcessExitBBVec() {
  ASSERT(exitBBVec.size() == 1, "there must be one BB_return in func");
  MakeupScopeLabels(*exitBBVec[0]);
}

void CGFunc::AddCommonExitBB() {
  // create fake commonExitBB
  commonExitBB = CreateNewBB(true, BB::kBBFallthru, 0);
  ASSERT(commonExitBB != nullptr, "cannot create fake commonExitBB");
  for (BB *cgbb : exitBBVec) {
    if (!cgbb->IsUnreachable()) {
      commonExitBB->PushBackPreds(*cgbb);
    }
  }
  for (BB *cgbb : noReturnCallBBVec) {
    commonExitBB->PushBackPreds(*cgbb);
  }
}

void CGFunc::UpdateCallBBFrequency() {
  if (!func.HasFreqMap() || func.GetLastFreqMap().empty()) {
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
  ProcessExitBBVec();
  LmbcGenSaveSpForAlloca();

  if (func.IsJava()) {
    GenerateCleanupCodeForExtEpilog(*cleanupBB);
  } else if (!func.GetModule()->IsCModule()) {
    GenerateCleanupCode(*cleanupBB);
  }
  GenSaveMethodInfoCode(*firstBB);
  /* build control flow graph */
  theCFG = memPool->New<CGCFG>(*this);
  theCFG->MarkLabelTakenBB();
  theCFG->BuildCFG();
  RemoveUnreachableBB();
  AddCommonExitBB();
  if (mirModule.GetSrcLang() != kSrcLangC) {
    MarkCatchBBs();
  }
  MarkCleanupBB();
  DetermineReturnTypeofCall();
  theCFG->UnreachCodeAnalysis();
  if (mirModule.GetSrcLang() == kSrcLangC) {
    theCFG->WontExitAnalysis();
  }
  if (CGOptions::IsLazyBinding() && !GetCG()->IsLibcore()) {
    ProcessLazyBinding();
  }
  if (GetCG()->DoPatchLongBranch()) {
    PatchLongBranch();
  }
  NeedStackProtect();
}

void CGFunc::AddDIESymbolLocation(const MIRSymbol &sym, SymbolAlloc *loc, bool isParam) {
  ASSERT(debugInfo != nullptr, "debugInfo is null!");
  ASSERT(loc->GetMemSegment() != nullptr, "only support those variable that locate at stack now");
  DBGDie *sdie = debugInfo->GetLocalDie(&func, sym.GetNameStrIdx());
  if (sdie == nullptr) {
    return;
  }

  DBGExprLoc *exprloc = sdie->GetExprLoc();
  CHECK_FATAL(exprloc != nullptr, "exprloc is null in CGFunc::AddDIESymbolLocation");
  exprloc->SetSymLoc(loc);

  GetDbgCallFrameLocations(isParam).push_back(exprloc);
}

void CGFunc::DumpCFG() const {
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
  ASSERT(funcSt != nullptr, "null ptr check");
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

void CGFunc::DumpBBInfo(const BB *bb) const {
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
  if (!bb->GetSuccs().empty()) {
    LogInfo::MapleLogger() << "succs: ";
    for (auto *succBB : bb->GetSuccs()) {
      LogInfo::MapleLogger() << succBB->GetId() << " ";
    }
  }
  if (!bb->GetPreds().empty()) {
    LogInfo::MapleLogger() << "preds: ";
    for (auto *predBB : bb->GetPreds()) {
      LogInfo::MapleLogger() << predBB->GetId() << " ";
    }
  }
  if (!bb->GetEhSuccs().empty()) {
    LogInfo::MapleLogger() << "eh_succs: ";
    for (auto *ehSuccBB : bb->GetEhSuccs()) {
      LogInfo::MapleLogger() << ehSuccBB->GetId() << " ";
    }
  }
  if (!bb->GetEhPreds().empty()) {
    LogInfo::MapleLogger() << "eh_preds: ";
    for (auto *ehPredBB : bb->GetEhPreds()) {
      LogInfo::MapleLogger() << ehPredBB->GetId() << " ";
    }
  }
  LogInfo::MapleLogger() << "===\n";
  LogInfo::MapleLogger() << "frequency:" << bb->GetFrequency() << "\n";
}

void CGFunc::DumpCGIR() const {
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
  ASSERT(funcSt != nullptr, "null ptr check");
  LogInfo::MapleLogger() << "\n******  CGIR for " << funcSt->GetName() << " *******\n";
  FOR_ALL_BB_CONST(bb, this) {
    if (bb->IsUnreachable()) {
      continue;
    }
    DumpBBInfo(bb);
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

void CGFunc::DumpCFGToDot(const std::string &fileNamePrefix) {
  std::ofstream file(fileNamePrefix + GetName());
  file << "digraph {" << std::endl;
  for (auto *bb : GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    auto &succs = bb->GetSuccs();
    if (succs.empty()) {
      continue;
    }
    file << "  " << bb->GetId() << "->{";
    for (auto *succ : succs) {
      file << succ->GetId() << " ";
    }
    file << "};";
  }
  file << "}" << std::endl;
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
    Insn *insn = bb->GetLastMachineInsn();
    while (insn->IsImmaterialInsn()) {
      insn = insn->GetPrev();
    }
    BB *tbb = GetBBFromLab2BBMap(GetLabelInInsn(*insn));
    if ((tbb->GetInternalFlag1() - bb->GetInternalFlag1()) < MaxCondBranchDistance()) {
      continue;
    }
    InsertJumpPad(insn);
  }
}

void CGFunc::UpdateAllRegisterVregMapping(MapleMap<regno_t, PregIdx> &newMap) {
  vregsToPregsMap.clear();
  for (auto &it : std::as_const(newMap)) {
    vregsToPregsMap[it.first] = it.second;
  }
}

/* there are two stack protector:
 * 1. stack protector all: for all function
 * 2. stack protector strong: for some functon that
 *   <1> invoke alloca functon;
 *   <2> use stack address (address of or array symbol);
 *   <3> callee use return stack slot;
 * */
void CGFunc::NeedStackProtect() {
  ASSERT(GetNeedStackProtect() == false, "no stack protect default");
  CG *currCG = GetCG();
  if (currCG->IsStackProtectorAll()) {
    SetNeedStackProtect(true);
    return;
  }

  if (!currCG->IsStackProtectorStrong()) {
    return;
  }

  if (HasAlloca()) {
    SetNeedStackProtect(true);
    return;
  }

  /* check if function use stack address or callee function return stack slot */
  auto stackProInfo = GetStackProtectInfo();
  if ((stackProInfo & kAddrofStack) != 0 || (stackProInfo & kRetureStackSlot) != 0) {
    SetNeedStackProtect(true);
    return;
  }
}

bool CgHandleFunction::PhaseRun(maplebe::CGFunc &f) {
  f.HandleFunction();
  if (!f.GetCG()->GetCGOptions().DoEmitCode() || f.GetCG()->GetCGOptions().DoDumpCFG()) {
    f.DumpCFG();
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgHandleFunction, handlefunction)

bool CgPatchLongBranch::PhaseRun(maplebe::CGFunc &f) {
  f.PatchLongBranch();
  if (!f.GetCG()->GetCGOptions().DoEmitCode() || f.GetCG()->GetCGOptions().DoDumpCFG()) {
    f.DumpCFG();
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgPatchLongBranch, patchlongbranch)

bool CgFixCFLocOsft::PhaseRun(maplebe::CGFunc &f) {
  if (f.GetCG()->GetCGOptions().WithDwarf()) {
    f.DBGFixCallFrameLocationOffsets();
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgFixCFLocOsft, dbgfixcallframeoffsets)
}  /* namespace maplebe */
