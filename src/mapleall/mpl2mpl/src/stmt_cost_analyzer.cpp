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
#include "stmt_cost_analyzer.h"
#include <fstream>
#include <iostream>
#include "factory.h"
#include "global_tables.h"
#include "mpl_logging.h"
#include "ssa_tab.h"

namespace maple {
static constexpr int64 kFreeInsn = 10;
static constexpr int64 kHalfInsn = 50;
static constexpr int64 kOneInsn = 100;
static constexpr int64 kDoubleInsn = 200;
static constexpr int64 kInfinityFuncNumInsns = 100000;

static int64 HandleSwitch(StmtNode &stmt, StmtCostAnalyzer&) {
  SwitchNode &switchNode = static_cast<SwitchNode&>(stmt);
  return static_cast<int64>((switchNode.GetSwitchTable().size() + 1) * kSizeScale);
}

static int64 HandleCall(StmtNode &stmt, StmtCostAnalyzer &ia) {
  int64 cost = 0;
  CallNode &callNode = static_cast<CallNode&>(stmt);
  // call insn itself.
  cost += kOneInsn;
  // handle all the params.
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    cost += ia.GetExprCost(stmt.Opnd(i));
  }
  // solve assign
  auto &returnVector = callNode.GetReturnVec();
  if (returnVector.empty()) {
    return cost;
  }
  CHECK_FATAL(returnVector.size() == 1, "Error.");
  auto stIdx = returnVector.at(0).first;
  auto fieldID = returnVector.at(0).second.GetFieldID();
  if (returnVector.at(0).second.IsReg()) {
    return kOneInsn;
  }
  auto *mirType = ia.GetMIRTypeFromStIdxAndField(stIdx, fieldID);
  cost += ia.GetMoveCost(mirType->GetSize());
  return cost;
}

static int64 HandleIcall(StmtNode &stmt, StmtCostAnalyzer &ia) {
  int64 cost = 0;
  IcallNode &icall = static_cast<IcallNode&>(stmt);
  // call insn itself.
  cost += kOneInsn;
  // handle all the params.
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    cost += ia.GetExprCost(stmt.Opnd(i));
  }
  // solve assign
  auto &returnVector = icall.GetReturnVec();
  if (returnVector.empty()) {
    return cost;
  }
  CHECK_FATAL(returnVector.size() == 1, "Error.");
  auto stIdx = returnVector.at(0).first;
  auto fieldID = returnVector.at(0).second.GetFieldID();
  auto *mirType = ia.GetMIRTypeFromStIdxAndField(stIdx, fieldID);
  cost += ia.GetMoveCost(mirType->GetSize());
  return cost;
}

static int64 HandleDassign(StmtNode &stmt, StmtCostAnalyzer &ia) {
  int64 cost = 0;
  DassignNode &dassign = static_cast<DassignNode&>(stmt);
  auto stIdx = dassign.GetStIdx();
  bool isGlobal = stIdx.IsGlobal();
  if (isGlobal) {
    // need 2 insn to find the addr of global.
    cost += kDoubleInsn;
  }
  auto *mirType = ia.GetMIRTypeFromStIdxAndField(stIdx, dassign.GetFieldID());
  bool needStore = isGlobal || mirType->GetSize() > ia.GetTargetInfo()->GetMaxMoveBytes();
  if (needStore) {
    cost += ia.GetMoveCost(mirType->GetSize());
  } else {
    cost += kFreeInsn;
  }
  cost += ia.GetExprCost(dassign.GetRHS());
  return cost;
}

static int64 HandleIassign(StmtNode &stmt, StmtCostAnalyzer &ia) {
  int64 cost = 0;
  IassignNode &iassign = static_cast<IassignNode&>(stmt);
  auto *type = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign.GetTyIdx()));
  auto *mirType =
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetPointedTyIdxWithFieldID(iassign.GetFieldID()));
  cost += ia.GetMoveCost(mirType->GetSize());
  cost += ia.GetExprCost(iassign.GetRHS());
  return cost;
}

static int64 HandleIf(StmtNode &stmt, StmtCostAnalyzer &ia) {
  int64 cost = 0;
  IfStmtNode &ifNode = static_cast<IfStmtNode&>(stmt);
  // one insn for brtrue or brfalse.
  cost += kOneInsn;
  cost += ia.GetExprCost(ifNode.Opnd(0));
  cost += ia.GetStmtsCost(ifNode.GetThenPart());
  cost += ia.GetStmtsCost(ifNode.GetElsePart());
  return cost;
}

static int64 HandleWhile(StmtNode &stmt, StmtCostAnalyzer &ia) {
  int64 cost = 0;
  WhileStmtNode &whileNode = static_cast<WhileStmtNode&>(stmt);
  cost += ia.GetExprCost(whileNode.Opnd(0));
  cost += ia.GetStmtsCost(whileNode.GetBody());
  return cost;
}

static int64 HandleIntrinsicCall(StmtNode &stmt, StmtCostAnalyzer &ia) {
  IntrinsiccallNode &intrinsic = static_cast<IntrinsiccallNode&>(stmt);
  return static_cast<int64>(ia.GetTargetInfo()->GetIntrinsicInsnNum(intrinsic.GetIntrinsic()) * kSizeScale);
}

using GetStmtCostFactory = FunctionFactory<Opcode, int64, StmtNode&, StmtCostAnalyzer&>;
static void InitStmtCostFactory() {
  RegisterFactoryFunction<GetStmtCostFactory>(OP_switch, HandleSwitch);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_call, HandleCall);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_callassigned, HandleCall);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_icall, HandleIcall);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_icallassigned, HandleIcall);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_dassign, HandleDassign);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_iassign, HandleIassign);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_if, HandleIf);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_while, HandleWhile);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_dowhile, HandleWhile);
  RegisterFactoryFunction<GetStmtCostFactory>(OP_intrinsiccall, HandleIntrinsicCall);
}

void StmtCostAnalyzer::Init() {
  ti = TargetInfo::CreateTargetInfo(alloc);
  InitStmtCostFactory();
}

int64 StmtCostAnalyzer::GetStmtsCost(BlockNode *block) {
  if (block == nullptr) {
    return 0;
  }
  int64 cost = 0;
  for (auto &stmt : block->GetStmtNodes()) {
    cost += GetStmtCost(&stmt);
  }
  return cost;
}

int64 StmtCostAnalyzer::GetMoveCost(size_t sizeInByte) const {
  return static_cast<int64>(((sizeInByte + ti->GetMaxMoveBytes() - 1) / ti->GetMaxMoveBytes()) * kSizeScale);
}

static bool IsConstZero(const BaseNode *opnd) {
  CHECK_FATAL(opnd != nullptr, "Null pointer check.");
  while (opnd->GetOpCode() == OP_cvt) {
    opnd = opnd->Opnd(0);
    CHECK_FATAL(opnd != nullptr, "Null pointer check.");
  }
  if (opnd->GetOpCode() == OP_constval) {
    return static_cast<const ConstvalNode*>(opnd)->GetConstVal()->IsZero();
  }
  return false;
}

int64 StmtCostAnalyzer::GetMeExprCost(MeExpr *meExpr) {
  if (meExpr == nullptr) {
    return 0;
  }
  BaseNode &expr = meExpr->EmitExpr(alloc);
  return GetExprCost(&expr);
}

int64 StmtCostAnalyzer::GetMeStmtCost(MeStmt *meStmt) {
  if (meStmt == nullptr) {
    return 0;
  }
  // We need save and update module curFunc before emit because
  // some Emit (such as VarMeExpr::Emit) will call module.CurFunction()
  MIRModule *mirModule = curFunc->GetModule();
  MIRFunction *oldCurFunc = mirModule->CurFunction();
  mirModule->SetCurFunction(curFunc);
  StmtNode &stmt = meStmt->EmitStmt(alloc);
  // Recover module curFunc after emit
  mirModule->SetCurFunction(oldCurFunc);
  return GetStmtCost(&stmt);
}

int32 StmtCostAnalyzer::EstimateNumInsns(MeStmt *meStmt) {
  auto stmtSizeCost = GetMeStmtCost(meStmt);
  // 100 cost per insn
  return static_cast<int32>(stmtSizeCost) / static_cast<int32>(kSizeScale);
}

int32 StmtCostAnalyzer::EstimateNumCycles(MeStmt *meStmt) {
  // we should use more accurate cycle model if needed
  return EstimateNumInsns(meStmt);
}

int32 StmtCostAnalyzer::EstimateNumInsns(StmtNode *stmt) {
  auto stmtSizeCost = GetStmtCost(stmt);
  // 100 cost per insn
  return static_cast<int32>(stmtSizeCost) / static_cast<int32>(kSizeScale);
}

int32 StmtCostAnalyzer::EstimateNumCycles(StmtNode *stmt) {
  // we should use more accurate cycle model if needed
  return EstimateNumInsns(stmt);
}

int64 StmtCostAnalyzer::GetExprCost(BaseNode *expr) {
  if (expr == nullptr) {
    return 0;
  }
  int64 cost = 0;
  Opcode op = expr->GetOpCode();
  switch (op) {
    case OP_dread: {
      DreadNode *dread = static_cast<DreadNode*>(expr);
      auto stIdx = dread->GetStIdx();
      bool isGlobal = stIdx.IsGlobal();
      if (isGlobal) {
        // need 2 insn to find the addr of global.
        cost += kDoubleInsn;
      }
      auto *mirType = GetMIRTypeFromStIdxAndField(stIdx, dread->GetFieldID());
      bool needStore = isGlobal || mirType->GetSize() > ti->GetMaxMoveBytes();
      if (needStore) {
        cost += GetMoveCost(mirType->GetSize());
      } else {
        cost += kFreeInsn;
      }
      break;
    }
    case OP_iread: {
      IreadNode *iread = static_cast<IreadNode*>(expr);
      auto *mirType = iread->GetType();
      cost += GetMoveCost(mirType->GetSize());
      break;
    }
    case OP_addrof: {
      AddrofNode *addrofNode = static_cast<AddrofNode*>(expr);
      auto stIdx = addrofNode->GetStIdx();
      if (stIdx.IsGlobal()) {
        cost += kDoubleInsn;
      } else {
        cost += kOneInsn;
      }
      break;
    }
    case OP_iaddrof: {
      // it's usually an add insn.
      cost += kOneInsn;
      break;
    }
    case OP_addroffunc: {
      cost += kDoubleInsn;
      break;
    }
    case OP_constval: {
      MIRConst *mirConst = static_cast<ConstvalNode*>(expr)->GetConstVal();
      if (mirConst->IsZero()) {
        break;
      }
      if (mirConst->GetKind() == kConstInt) {
        cost += kFreeInsn;
        break;
      }
      cost += kDoubleInsn;
      break;
    }
    case OP_conststr:
    case OP_conststr16: {
      cost += kDoubleInsn;
      break;
    }
    case OP_cvt: {
      TypeCvtNode *cvt = static_cast<TypeCvtNode*>(expr);
      PrimType fromType = cvt->FromType();
      PrimType toType = cvt->GetPrimType();
      if (IsPrimitiveFloat(fromType) || IsPrimitiveFloat(toType)) {
        cost += kOneInsn;
      }
      break;
    }
    case OP_retype:
    case OP_trunc:
    case OP_ceil:
    case OP_floor:
    case OP_round: {
      cost += kFreeInsn;
      break;
    }
    case OP_bnot:
    case OP_abs: {
      cost += kOneInsn;
      break;
    }
    case OP_intrinsicop:
    case OP_intrinsicopwithtype: {
      IntrinsicopNode *intrin = static_cast<IntrinsicopNode*>(expr);
      cost += static_cast<int64>(ti->GetIntrinsicInsnNum(intrin->GetIntrinsic()) * kSizeScale);
      break;
    }
    case OP_add:
    case OP_sub: {
      cost += kHalfInsn;
      break;
    }
    case OP_mul:
    case OP_div: {
      cost += kOneInsn;
      break;
    }
    case OP_rem: {
      cost += kDoubleInsn;
      break;
    }
    case OP_land:
    case OP_lior:
    case OP_cand:
    case OP_cior:
    case OP_band:
    case OP_bior:
    case OP_bxor: {
      cost += kHalfInsn;
      break;
    }
    case OP_sext:
    case OP_zext:
    case OP_ashr:
    case OP_lshr:
    case OP_shl:
    case OP_ror: {
      cost += kHalfInsn;
      break;
    }
    case OP_eq:
    case OP_ge:
    case OP_gt:
    case OP_le:
    case OP_lt:
    case OP_ne:
    case OP_cmp:
    case OP_cmpl:
    case OP_cmpg: {
      CompareNode *cmp = static_cast<CompareNode*>(expr);
      bool cmpWithZero = IsConstZero(cmp->Opnd(0)) || IsConstZero(cmp->Opnd(1));
      cost += cmpWithZero ? 0 : kOneInsn;
      break;
    }
    case OP_jstry:
    case OP_try:
    case OP_jscatch:
    case OP_catch:
    case OP_finally:
    case OP_cleanuptry:
    case OP_endtry:
    case OP_syncenter:
    case OP_syncexit: {
      return kInfinityFuncNumInsns;
    }
    default: {
      cost += kOneInsn;
      break;
    }
  }
  for (size_t i = 0; i < expr->NumOpnds(); ++i) {
    cost += GetExprCost(expr->Opnd(i));
  }
  return cost;
}

MIRType *StmtCostAnalyzer::GetMIRTypeFromStIdxAndField(const StIdx idx, FieldID fieldID) const {
  auto *symbol = curFunc->GetLocalOrGlobalSymbol(idx);
  ASSERT_NOT_NULL(symbol);
  auto *mirType = symbol->GetType();
  if (fieldID != 0) {
    ASSERT_NOT_NULL(mirType);
    mirType = static_cast<MIRStructType*>(mirType)->GetFieldType(fieldID);
  }
  return mirType;
}

int64 StmtCostAnalyzer::GetStmtCost(StmtNode *stmt) {
  if (stmt == nullptr) {
    return 0;
  }
  Opcode op = stmt->GetOpCode();
  auto function = CreateProductFunction<GetStmtCostFactory>(op);
  if (function != nullptr) {
    // Update module curFunc
    MIRModule *mirModule = curFunc->GetModule();
    MIRFunction *oldCurFunc = mirModule->CurFunction();
    mirModule->SetCurFunction(curFunc);
    auto ret = function(*stmt, *this);
    // Recover module curFunc
    mirModule->SetCurFunction(oldCurFunc);
    return ret;
  }
  int64 cost = 0;
  switch (op) {
    case OP_brtrue:
    case OP_brfalse: {
      cost += kOneInsn;
      break;
    }
    case OP_eval:
    case OP_free:
    case OP_abort:
    case OP_goto:
    case OP_label: {
      return 0;
    }
    case OP_return: {
      cost += kOneInsn;
      // opnd need to be analyzed.
      break;
    }
    default: {
      break;
    }
  }
  for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
    cost += GetExprCost(stmt->Opnd(i));
  }
  return cost;
}
}  // namespace maple

