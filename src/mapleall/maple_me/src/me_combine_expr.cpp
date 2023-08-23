/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "me_combine_expr.h"

#include "cast_opt.h"
#include "me_expr_utils.h"
#include "me_ir.h"
#include "me_irmap_build.h"
#include "mir_type.h"
#include "opcodes.h"

namespace maple {

bool ExprCombiner::HasOneUse(const MeExpr &expr) {
  auto site = useInfo.GetUseSitesOfExpr(&expr);
  return site && site->size() == 1 && site->front().GetRef() == 1;
}

MeExpr *ExprCombiner::SimplifyIntrinsicByDUB(const MeExpr &expr, const IntVal &demanded) {
  auto intrinsicID = static_cast<const NaryMeExpr &>(expr).GetIntrinsic();
  if (intrinsicID >= INTRN_C_rev_4 && intrinsicID <= INTRN_C_bswap64) {
    auto bitwidth = GetPrimTypeBitSize(expr.GetPrimType());
    uint32 lzeroNum = demanded.CountLeadingZeros();
    uint32 rzeroNum = demanded.CountTrallingZeros();
    // aligndown to 8
    lzeroNum &= ~(k8BitSize - 1);
    rzeroNum &= ~(k8BitSize - 1);
    if (demanded.GetBitWidth() > bitwidth) {
      lzeroNum -= demanded.GetBitWidth() - bitwidth;
    }
    // if there is exactly 1 byte which is not zero
    if (lzeroNum + rzeroNum + k8BitSize == bitwidth) {
      auto shiftOp = lzeroNum > rzeroNum ? OP_lshr : OP_shl;
      auto shiftNum = lzeroNum > rzeroNum ? lzeroNum - rzeroNum : rzeroNum - lzeroNum;
      auto newOpnd0 = irMap.CreateMeExprBinary(shiftOp, expr.GetPrimType(), *expr.GetOpnd(0),
                                               *irMap.CreateIntConstMeExpr(shiftNum, expr.GetPrimType()));
      return newOpnd0;
    }
  }

  return nullptr;
}

MeExpr *ExprCombiner::SimplifyDemandedUseBits(MeExpr &expr, const IntVal &demanded, BitValue &known, uint32 depth) {
  // exprs having multi uses are not simplified, otherwise it will block some epre oppotunities
  if (depth != 0 && !HasOneUse(expr)) {
    return nullptr;
  }

  auto op = expr.GetOp();
  auto bitWidth = demanded.GetBitWidth();
  BitValue lhsKnown(bitWidth);
  BitValue rhsKnown(bitWidth);
  switch (op) {
    case OP_band: {
      auto opnd0 = expr.GetOpnd(0);
      auto opnd1 = expr.GetOpnd(1);
      if (auto newExpr = SimplifyDemandedUseBits(*opnd1, demanded, rhsKnown, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 1);
        return nullptr;
      }
      if (auto newExpr = SimplifyDemandedUseBits(*opnd0, demanded & ~rhsKnown.zeroBits, lhsKnown, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 0);
        return nullptr;
      }
      return SimplifyBinOpByDemanded(irMap, expr, demanded, known, lhsKnown, rhsKnown);
    }
    case OP_bior: {
      auto opnd0 = expr.GetOpnd(0);
      auto opnd1 = expr.GetOpnd(1);
      if (auto newExpr = SimplifyDemandedUseBits(*opnd1, demanded, rhsKnown, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 1);
        return nullptr;
      }
      if (auto newExpr = SimplifyDemandedUseBits(*opnd0, demanded & ~rhsKnown.oneBits, lhsKnown, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 0);
        return nullptr;
      }
      return SimplifyBinOpByDemanded(irMap, expr, demanded, known, lhsKnown, rhsKnown);
    }
    case OP_bxor: {
      auto opnd0 = expr.GetOpnd(0);
      auto opnd1 = expr.GetOpnd(1);
      if (auto newExpr = SimplifyDemandedUseBits(*opnd1, demanded, rhsKnown, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 1);
        return nullptr;
      }
      if (auto newExpr = SimplifyDemandedUseBits(*opnd0, demanded, lhsKnown, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 0);
        return nullptr;
      }
      return SimplifyBinOpByDemanded(irMap, expr, demanded, known, lhsKnown, rhsKnown);
    }
    case OP_cvt: {
      MeExprCastInfo castInfo(&expr);
      MeCastOpt::ComputeCastInfo(castInfo);
      if (!(castInfo.kind == CAST_zext || castInfo.kind == CAST_intTrunc)) {
        break;
      }
      auto opnd = expr.GetOpnd(0);
      auto srcWidth = GetPrimTypeActualBitSize(opnd->GetPrimType());
      auto demandedByOpnd = demanded.TruncOrExtend(static_cast<uint16>(srcWidth), false);
      BitValue opndBits(srcWidth);
      if (auto newExpr = SimplifyDemandedUseBits(*opnd, demandedByOpnd, opndBits, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 0);
        return nullptr;
      }
      known = opndBits.TruncOrExtend(bitWidth);
      break;
    }
    case OP_zext: {
      auto opnd = expr.GetOpnd(0);
      auto srcWidth = static_cast<OpMeExpr&>(expr).GetBitsSize();
      auto demandedByOpnd = demanded.TruncOrExtend(static_cast<uint16>(srcWidth), false);
      BitValue opndBits(srcWidth);
      if (auto newExpr = SimplifyDemandedUseBits(*opnd, demandedByOpnd, opndBits, depth + 1)) {
        ReplaceExprInExpr(expr, *newExpr, 0);
        return nullptr;
      }
      known = opndBits.TruncOrExtend(bitWidth);
      break;
    }
    case OP_intrinsicop: {
      return SimplifyIntrinsicByDUB(expr, demanded);
    }
    default:
      ComputeBitValueOfExpr(expr, demanded, known, depth);
      break;
  }
  return nullptr;
}

MeExpr *ExprCombiner::SimplifyByDemandedBits(OpMeExpr &expr) {
  if (!IsPrimitiveInteger(expr.GetPrimType())) {
    return nullptr;
  }
  auto bitWidth = GetPrimTypeBitSize(expr.GetPrimType());
  BitValue knownBits(bitWidth);
  IntVal demandedBits(IntVal::kAllOnes, bitWidth, false);

  return SimplifyDemandedUseBits(expr, demandedBits, knownBits, 0);
}

MeExpr *ExprCombiner::VisitBior(OpMeExpr &expr) {
  if (auto res = SimplifyByDemandedBits(expr)) {
    return res;
  }
  return nullptr;
}

MeExpr *ExprCombiner::VisitBxor(OpMeExpr &expr) {
  if (auto res = SimplifyByDemandedBits(expr)) {
    return res;
  }
  return nullptr;
}

MeExpr *ExprCombiner::VisitBand(OpMeExpr &expr) {
  if (auto res = SimplifyByDemandedBits(expr)) {
    return res;
  }
  return nullptr;
}

/// return ture if \p c is equal after trunc by \p type
bool IsEqualByTrunc(const IntVal &c, PrimType type) {
  return c.Trunc(type).GetZXTValue() == c.GetZXTValue();
}

bool IsCvtNeedTrunc(const OpMeExpr& expr) {
  if (expr.GetOp() == OP_cvt) {
    return GetPrimTypeActualBitSize(expr.GetPrimType()) < GetPrimTypeActualBitSize(expr.GetOpndType());
  }
  if (expr.GetOp() == OP_zext) {
    return GetPrimTypeActualBitSize(expr.GetOpnd(0)->GetPrimType()) > expr.GetBitsSize();
  }
  return false;
}

// fold cmp(and(sh(a, c3), c2), c1)
static MeExpr *FoldCmpAndShift(IRMap &irMap, Opcode cmpOp, const MeExpr &andOpnd0, IntVal c1, const IntVal &c2) {
  auto shiftOp = andOpnd0.GetOp();
  if (shiftOp == OP_cvt || shiftOp == OP_zext) {
    if (IsCvtNeedTrunc(static_cast<const OpMeExpr&>(andOpnd0))) {
      return nullptr;
    }
    auto opnd0 = andOpnd0.GetOpnd(0);
    if (IsEqualByTrunc(c1, opnd0->GetPrimType()) && IsEqualByTrunc(c2, opnd0->GetPrimType())) {
      return FoldCmpAndShift(irMap, cmpOp, *opnd0, c1.Trunc(opnd0->GetPrimType()), c2.Trunc(opnd0->GetPrimType()));
    }
    return nullptr;
  }
  if (!IsShift(shiftOp)) {
    return nullptr;
  }
  auto c3 = GetIntConst(*andOpnd0.GetOpnd(1));
  MeExpr *shiftOpnd0 = andOpnd0.GetOpnd(0);
  if (!c3) {
    return nullptr;
  }
  IntVal newC1Val;
  IntVal newC2Val;
  bool shiftedOutBitsUsed;
  auto type = andOpnd0.GetPrimType();
  if (!c2.IsSigned()) {
    type = GetUnsignedPrimType(type);
  }
  if (c1.TruncOrExtend(type).GetZXTValue() != c1.GetZXTValue()) {
    return nullptr;
  }
  c1.Assign(c1.TruncOrExtend(type));
  if (shiftOp == OP_shl) {
    if ((c1.IsSigned() && c1.GetSignBit()) || (c2.IsSigned() && c2.GetSignBit())) {
      return nullptr;
    }
    newC1Val = c1.LShr(c3->GetValue(), type);
    newC2Val = c2.LShr(c3->GetValue(), type);
    shiftedOutBitsUsed = newC1Val.Shl(c3->GetValue(), type) != c1;
  } else if (shiftOp == OP_lshr) {
    if ((c1.IsSigned() && c1.GetSignBit()) || (c2.IsSigned() && c2.GetSignBit())) {
      return nullptr;
    }
    newC1Val = c1.Shl(c3->GetValue(), type);
    newC2Val = c2.Shl(c3->GetValue(), type);
    shiftedOutBitsUsed = newC1Val.LShr(c3->GetValue(), type) != c1;
  } else {
    ASSERT(shiftOp == OP_ashr, "Unknown shift opcode");
    newC1Val = c1.Shl(c3->GetValue(), type);
    newC2Val = c2.Shl(c3->GetValue(), type);
    shiftedOutBitsUsed = newC1Val.LShr(c3->GetValue(), type) != c1;
    if (newC2Val.AShr(c3->GetValue(), type) != c2) {
      return nullptr;
    }
  }

  // if shifted out bits are used, always not eq
  if (shiftedOutBitsUsed) {
    if (cmpOp == OP_eq) {
      return irMap.CreateIntConstMeExpr(0, PTY_u1);
    }
    if (cmpOp == OP_ne) {
      return irMap.CreateIntConstMeExpr(1, PTY_u1);
    }
  } else {
    auto newAnd = irMap.CreateMeExprBinary(OP_band, type, *shiftOpnd0, *irMap.CreateIntConstMeExpr(newC2Val, type));
    auto newCmp = irMap.CreateMeExprCompare(cmpOp, PTY_u1, type, *newAnd, *irMap.CreateIntConstMeExpr(newC1Val, type));
    return newCmp;
  }
  (void)shiftOpnd0;
  return nullptr;
}

// fold cmp(binop(x, c2), c1)
MeExpr *ExprCombiner::FoldCmpBinOpWithConst(OpMeExpr &cmpExpr, const MeExpr &binOpExpr, const IntVal &c1) {
  if (!HasOneUse(cmpExpr)) {
    return nullptr;
  }
  auto cmpOp = cmpExpr.GetOp();
  if (!IsCompareHasReverseOp(cmpOp)) {
    return nullptr;
  }
  auto op = binOpExpr.GetOp();
  auto opnd0 = binOpExpr.GetOpnd(0);
  if (op == OP_cvt || op == OP_zext) {
    if (IsCvtNeedTrunc(static_cast<const OpMeExpr&>(binOpExpr))) {
      return nullptr;
    }
    if (IsEqualByTrunc(c1, opnd0->GetPrimType())) {
      return FoldCmpBinOpWithConst(cmpExpr, *opnd0, c1.Trunc(opnd0->GetPrimType()));
    }
  }
  if (cmpExpr.GetOpnd(1)->GetMeOp() != kMeOpConst && cmpOp != OP_eq && cmpOp != OP_ne) {
    cmpOp = GetReverseCmpOp(cmpOp);
  }
  if (op == OP_band) {
    const MIRIntConst *c2 = nullptr;
    uint8_t c2Idx = 0;
    std::tie(c2, c2Idx) = GetIntConstOpndOfBinExpr(binOpExpr);
    opnd0 = binOpExpr.GetOpnd(1u - c2Idx);
    if (!c2) {
      return nullptr;
    }
    if (auto res = FoldCmpAndShift(irMap, cmpOp, *opnd0, c1, c2->GetValue())) {
      return res;
    }
  }
  return nullptr;
}

MeExpr *ExprCombiner::FoldCmpWithConst(OpMeExpr &expr) {
  const MIRIntConst *c1 = nullptr;
  uint8_t c1Idx = 0;
  std::tie(c1, c1Idx) = GetIntConstOpndOfBinExpr(expr);
  MeExpr *binOpnd = expr.GetOpnd(1u - c1Idx);
  if (!c1 || binOpnd->GetMeOp() != kMeOpOp) {
    return nullptr;
  }

  if (auto res = FoldCmpBinOpWithConst(expr, *binOpnd, c1->GetValue())) {
    return res;
  }
  return nullptr;
}

MeExpr *ExprCombiner::VisitCmp(OpMeExpr &expr) {
  MeExpr *opnd0 = expr.GetOpnd(0);
  MeExpr *opnd1 = expr.GetOpnd(1);
  if (opnd0->GetMeOp() == kMeOpConst || opnd1->GetMeOp() == kMeOpConst) {
    if (auto res = FoldCmpWithConst(expr)) {
      return res;
    }
  }
  return nullptr;
}

MeExpr *ExprCombiner::VisitOpExpr(OpMeExpr &expr) {
  if (IsPrimitiveVector(expr.GetPrimType())) {
    return nullptr;
  }
  auto opcode = expr.GetOp();
  switch (opcode) {
    case OP_add:
    case OP_sub:
    case OP_mul:
    case OP_lnot:
    case OP_div:
    case OP_rem:
    case OP_ashr:
    case OP_shl:
    case OP_max:
    case OP_min:
    case OP_bior: {
      return VisitBior(expr);
    }
    case OP_bxor: {
      return VisitBxor(expr);
    }
    case OP_cand:
    case OP_land:
    case OP_cior:
    case OP_lior:
    case OP_lshr:
      break;
    case OP_band: {
      return VisitBand(expr);
    }
    case OP_ne:
    case OP_eq:
    case OP_lt:
    case OP_le:
    case OP_ge:
    case OP_gt:
    case OP_cmp:
    case OP_cmpl:
    case OP_cmpg: {
      return VisitCmp(expr);
    }
    default:
      break;
  }
  return nullptr;
}
void ExprCombiner::ReplaceExprInExpr(MeExpr &parentExpr, MeExpr &newExpr, size_t opndIdx) {
  if (debug) {
    LogInfo::MapleLogger() << "replace expr \nOld:\n";
    parentExpr.GetOpnd(opndIdx)->Dump(&irMap);
    LogInfo::MapleLogger() << "\nNew:\n";
    newExpr.Dump(&irMap);
  }
  useInfo.ReplaceUseInfoInExpr(parentExpr.GetOpnd(opndIdx), &newExpr);
  parentExpr.SetOpnd(opndIdx, &newExpr);
}

void ExprCombiner::ReplaceExprInStmt(MeStmt &parentStmt, MeExpr &newExpr, size_t opndIdx) {
  if (debug) {
    LogInfo::MapleLogger() << "replace expr \nOld:\n";
    parentStmt.GetOpnd(opndIdx)->Dump(&irMap);
    LogInfo::MapleLogger() << "\nNew:\n";
    newExpr.Dump(&irMap);
  }
  useInfo.DelUseInfoInExpr(parentStmt.GetOpnd(opndIdx), &parentStmt);
  useInfo.CollectUseInfoInExpr(&newExpr, &parentStmt);
  parentStmt.SetOpnd(opndIdx, &newExpr);
  irMap.SimplifyCastForAssign(&parentStmt);
}

MeExpr *ExprCombiner::VisitMeExpr(MeExpr &expr) {
  if (IsPrimitiveVector(expr.GetPrimType())) {
    return nullptr;
  }
  switch (expr.GetMeOp()) {
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpSizeoftype:
    case kMeOpFieldsDist:
    case kMeOpVar:
    case kMeOpReg:
    case kMeOpIvar:
      return nullptr;
    case kMeOpOp: {
      OpMeExpr &opexp = static_cast<OpMeExpr &>(expr);
      for (uint8_t i = 0; i < opexp.GetNumOpnds(); i++) {
        auto newOpnd = VisitMeExpr(*opexp.GetOpnd(i));
        if (newOpnd) {
          ReplaceExprInExpr(expr, *newOpnd, i);
        }
      }
      if (auto newExpr = VisitOpExpr(opexp)) {
        return newExpr;
      }
      return nullptr;
    }
    case kMeOpNary: {
      NaryMeExpr &opexp = static_cast<NaryMeExpr &>(expr);
      for (uint8_t i = 0; i < opexp.GetNumOpnds(); i++) {
        auto newOpnd = VisitMeExpr(*opexp.GetOpnd(i));
        if (newOpnd) {
          ReplaceExprInExpr(expr, *newOpnd, i);
        }
      }
      return nullptr;
    }
    default:
      break;
  }
  return nullptr;
}

void ExprCombiner::VisitStmt(MeStmt &stmt) {
  if (debug) {
    LogInfo::MapleLogger() << "visit stmt: ";
    stmt.Dump(&irMap);
    LogInfo::MapleLogger() << "\n";
  }
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    auto newExpr = VisitMeExpr(*stmt.GetOpnd(0));
    if (newExpr) {
      ReplaceExprInStmt(stmt, *newExpr, i);
    }
  }
}

void CombineExpr::Execute() {
  auto useInfo = irMap.GetExprUseInfo();
  if (useInfo.IsInvalid()) {
    useInfo.CollectUseInfoInFunc(&irMap, &dom, kUseInfoOfAllExpr);
  }
  ExprCombiner combiner(irMap, useInfo, debug);
  for (auto bIt = dom.GetReversePostOrder().begin(); bIt != dom.GetReversePostOrder().end(); ++bIt) {
    auto curBB = func.GetCfg()->GetBBFromID(BBId((*bIt)->GetID()));
    for (auto &meStmt : curBB->GetMeStmts()) {
      combiner.VisitStmt(meStmt);
    }
  }
  useInfo.InvalidUseInfo();
}

void MECombineExpr::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MECombineExpr::PhaseRun(maple::MeFunction &f) {
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  CombineExpr combineExpr(f, *irMap, *dom, DEBUGFUNC_NEWPM(f));
  combineExpr.Execute();
  return true;
}
}  // namespace maple