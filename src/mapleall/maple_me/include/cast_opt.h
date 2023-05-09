/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_CAST_OPT_H
#define MAPLE_ME_INCLUDE_CAST_OPT_H
#include "mir_nodes.h"
#include "me_ir.h"

namespace maple {
// The order matters
enum CastKind {
  CAST_intTrunc = 0,
  CAST_zext = 1,
  CAST_sext = 2,
  CAST_int2fp = 3,
  CAST_fp2int = 4,
  CAST_fpTrunc = 5,
  CAST_fpExt = 6,
  CAST_retype = 7,
  CAST_unknown = 8
};

template <typename T, std::enable_if_t<std::is_base_of<MeExpr, T>::value ||
    std::is_base_of<BaseNode, T>::value, bool> = true>
class CastInfo {
 public:
  explicit CastInfo(T *expr) : expr(expr) {}
  virtual ~CastInfo() = default;
  virtual Opcode GetOp() {
    CHECK_FATAL(false, "NYI");
  }
  PrimType GetPrimType() const {
    return expr->GetPrimType();
  }
  virtual size_t GetBitsSize() {
    CHECK_FATAL(false, "NYI");
  }
  virtual T* GetOpnd(size_t index __attribute__((unused))) {
    CHECK_FATAL(false, "NYI");
  }
  virtual PrimType GetOpndType() {
    CHECK_FATAL(false, "NYI");
  }

  bool IsInvalid() const {
    return kind == CAST_unknown;
  }
  bool IsExtension() const {
    return kind == CAST_sext || kind == CAST_zext;
  }
  CastKind kind = CAST_unknown;  // CastInfo is invalid if kind is CAST_unknown
  PrimType srcType = PTY_begin;
  PrimType dstType = PTY_end;
  T *expr = nullptr;  // expr's type must be MeExpr* or BaseNode*
};

class MeExprCastInfo : public CastInfo<MeExpr> {
 public:
  explicit MeExprCastInfo(MeExpr *expr) : CastInfo(expr) {}
  ~MeExprCastInfo() override = default;

  Opcode GetOp() override {
    return expr->GetOp();
  }

  size_t GetBitsSize() override {
    switch (GetOp()) {
      case OP_zext:
      case OP_sext:
        return static_cast<const OpMeExpr*>(expr)->GetBitsSize();
      default:
        CHECK_FATAL(false, "NYI");
    }
  }

  MeExpr* GetOpnd(size_t index) override {
    return expr->GetOpnd(index);
  }

  PrimType GetOpndType() override {
    switch (GetOp()) {
      case OP_cvt:
      case OP_retype:
        return static_cast<const OpMeExpr*>(expr)->GetOpndType();
      case OP_regread: {
        const auto *scalarExpr = static_cast<const ScalarMeExpr*>(expr);
        return scalarExpr->GetOst()->GetType()->GetPrimType();
      }
      case OP_iread: {
        const auto *ivarExpr = static_cast<const IvarMeExpr*>(expr);
        return ivarExpr->GetType()->GetPrimType();
      }
      default:
        CHECK_FATAL(false, "NYI");
    }
  }
};

class BaseNodeCastInfo : public CastInfo<BaseNode> {
 public:
  explicit BaseNodeCastInfo(BaseNode *expr) : CastInfo(expr) {}
  ~BaseNodeCastInfo() override = default;

  Opcode GetOp() override {
    return expr->GetOpCode();
  }

  size_t GetBitsSize() override {
    switch (GetOp()) {
      case OP_zext:
      case OP_sext:
        return static_cast<const ExtractbitsNode*>(expr)->GetBitsSize();
      default:
        CHECK_FATAL(false, "NYI");
    }
  }

  BaseNode* GetOpnd(size_t index) override {
    return expr->Opnd(index);
  }

  PrimType GetOpndType() override {
    switch (GetOp()) {
      case OP_retype: {
        return GetOpnd(0)->GetPrimType();
      }
      case OP_cvt:
        return static_cast<const TypeCvtNode*>(expr)->FromType();
      case OP_regread: {
        const auto *regread = static_cast<const RegreadNode*>(expr);
        PregIdx regIdx = regread->GetRegIdx();
        const MIRPreg *preg = theMIRModule->CurFunction()->GetPregItem(regIdx);
        return preg->GetPrimType();
      }
      case OP_iread: {
        const auto *iread = static_cast<const IreadNode*>(expr);
        return iread->GetType()->GetPrimType();
      }
      case OP_dread: {
        const auto *dread = static_cast<const DreadNode*>(expr);
        StIdx stIdx = dread->GetStIdx();
        MIRSymbol *symbol = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(stIdx);
        CHECK_NULL_FATAL(symbol);
        return symbol->GetType()->GetPrimType();
      }
      default:
        CHECK_FATAL(false, "NYI");
    }
  }
};

class CastOpt {
 public:
  static int IsEliminableCastPair(CastKind firstCastKind, CastKind secondCastKind,
                                  PrimType dstType, PrimType midType2, PrimType midType1, PrimType &srcType);
  template <typename T>
  static void DoComputeCastInfo(CastInfo<T> &castInfo, bool isMeExpr);
  static bool IsExplicitCastOp(Opcode op);
  static bool IsImplicitCastOp(Opcode op);
  static bool IsCompareOp(Opcode op);
};

class MeCastOpt : public CastOpt {
 public:
  static MeExpr *SimplifyCast(IRMap &irMap, MeExpr *expr);
  static void SimplifyCastForAssign(MeStmt *assignStmt);
  static void ComputeCastInfo(MeExprCastInfo &castInfo);
  static MeExpr *CreateMeExprByCastKind(IRMap &irMap, CastKind castKind, PrimType srcType, PrimType dstType,
                                        MeExpr *opnd, TyIdx dstTyIdx = TyIdx(0));
  static MeExpr *SimplifyCastPair(IRMap &irMap, const MeExprCastInfo &firstCastInfo,
                                  const MeExprCastInfo &secondCastInfo);
  static MeExpr *SimplifyCastSingle(IRMap &irMap, const MeExprCastInfo &castInfo);
  static MeExpr *TransformCvtU1ToNe(IRMap &irMap, OpMeExpr *cvtExpr);
};

class MapleCastOpt : public CastOpt {
 public:
  static void ComputeCastInfo(BaseNodeCastInfo &castInfo);
  static BaseNode *CreateMapleExprByCastKind(MIRBuilder &mirBuilder, CastKind castKind, PrimType srcType,
                                             PrimType dstType, BaseNode *opnd, TyIdx dstTyIdx = TyIdx(0));
  static BaseNode *SimplifyCast(MIRBuilder &mirBuilder, BaseNode *expr);
  static BaseNode *SimplifyCastPair(MIRBuilder &mirBuidler, const BaseNodeCastInfo &firstCastInfo,
                                    const BaseNodeCastInfo &secondCastInfo);
  static BaseNode *SimplifyCastSingle(MIRBuilder &mirBuilder, const BaseNodeCastInfo &castInfo);
  static BaseNode *TransformCvtU1ToNe(MIRBuilder &mirBuilder, const TypeCvtNode *cvtExpr);
};
}
#endif
