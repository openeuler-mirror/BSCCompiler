/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include "expand128floats.h"
#include "cfg_primitive_types.h"

namespace maple {

using F128OpRes = struct {
  Opcode positiveRes;
  Opcode negativeRes;
};

std::array cmpOps{OP_eq, OP_ne, OP_gt, OP_lt, OP_ge, OP_le};

std::unordered_map<Opcode, F128OpRes> f128OpsRes = {
  {OP_eq, {OP_eq, OP_ne}},
  {OP_ne, {OP_ne, OP_eq}},
  {OP_gt, {OP_gt, OP_le}},
  {OP_lt, {OP_lt, OP_ge}},
  {OP_ge, {OP_ge, OP_lt}},
  {OP_le, {OP_le, OP_gt}}
};

std::string Expand128Floats::GetSequentialName0(const std::string &prefix, uint32_t num) {
  std::stringstream ss;
  ss << prefix << num;
  return ss.str();
}

uint32 Expand128Floats::GetSequentialNumber() const {
  static uint32 unnamedSymbolIdx = 1;
  return unnamedSymbolIdx++;
}

std::string Expand128Floats::GetSequentialName(const std::string &prefix) {
  std::string name = GetSequentialName0(prefix, GetSequentialNumber());
  return name;
}

std::string Expand128Floats::SelectSoftFPCall(Opcode opCode, const BaseNode *node) {
  switch (opCode) {
    case OP_cvt:
    case OP_trunc:
      if (static_cast<const TypeCvtNode*>(node)->FromType() == PTY_f128) {
        switch (static_cast<const TypeCvtNode*>(node)->ptyp) {
          case PTY_i32:
            return "__fixtfsi";
          case PTY_u32:
          case PTY_a32:
            return "__fixunstfsi";
          case PTY_i64:
            return "__fixtfdi";
          case PTY_u64:
          case PTY_a64:
            return "__fixunstfdi";
          case PTY_f32:
            return "__trunctfsf2";
          case PTY_f64:
            return "__trunctfdf2";
          default:
            CHECK_FATAL(false, "unexpected destination type");
            break;
        }
      } else if (static_cast<const TypeCvtNode*>(node)->ptyp == PTY_f128) {
        switch (static_cast<const TypeCvtNode*>(node)->FromType()) {
          case PTY_i32:
            return "__floatsitf";
          case PTY_u32:
          case PTY_a32:
            return "__floatunsitf";
          case PTY_i64:
            return "__floatditf";
          case PTY_u64:
          case PTY_a64:
            return "__floatunditf";
          case PTY_f32:
            return "__extendsftf2";
          case PTY_f64:
            return "__extenddftf2";
          default:
            CHECK_FATAL(false, "unexpected source type");
            break;
        }
      }
      CHECK_FATAL(false, "unexpected source/destination type");
      return "";
    case OP_add:
      /* cast node , then -- impl op */
      return "__addtf3";
    case OP_sub:
      return "__subtf3";
    case OP_mul:
      return "__multf3";
    case OP_div:
      return "__divtf3";
    case OP_neg:
      return "__negtf2";
    case OP_cmp:
    case OP_cmpg:
    case OP_cmpl:
      ASSERT(false, "NYI"); // Check return "__cmptf2"
      break;
    case OP_le:
      return "__letf2";
    case OP_ge:
      return "__getf2";
    case OP_lt:
      return "__lttf2";
    case OP_gt:
      return "__gttf2";
    case OP_ne:
      return "__netf2";
    case OP_eq:
      return "__eqtf2";
    /* add another calls -- complex, pow, etc */
    default:
      CHECK_FATAL(false, "Operation NYI");
      return "";
  }
  CHECK_FATAL(false, "Unreachable Return");
  return "";
}

void Expand128Floats::ReplaceOpNode(BlockNode *block, BaseNode *baseNode, size_t opndId,
                                    BaseNode *currNode, MIRFunction *func, StmtNode *stmt) {
  auto opCode = currNode->GetOpCode();

  PrimType pType;
  if (std::find(cmpOps.begin(), cmpOps.end(), opCode) == cmpOps.end()) {
    pType = currNode->ptyp;
  } else {
    pType = PTY_i32;
  }

  MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
  if (opCode == OP_cvt || opCode == OP_trunc) {
    TypeCvtNode* cvtNode = static_cast<TypeCvtNode*>(currNode);
    if (cvtNode->FromType() == PTY_f128 || cvtNode->ptyp == PTY_f128) {
      args.push_back(currNode->Opnd(0));
    } else {
      return;
    }
  } else {
    bool opndIsF128 = false;
    for (size_t i = 0; i < currNode->numOpnds; ++i) {
      if (currNode->Opnd(i)->ptyp == PTY_f128) {
        opndIsF128 = true;
        break;
      }
    }
    if (!opndIsF128 && currNode->ptyp != PTY_f128) {
      return;
    }
    for (size_t i = 0; i < currNode->numOpnds; ++i) {
      args.push_back(currNode->Opnd(i));
    }
  }
  Opcode currOpCode = currNode->GetOpCode();
  std::string funcName = SelectSoftFPCall(currOpCode, currNode);
  auto cvtFunc = builder->GetOrCreateFunction(funcName, TyIdx(currNode->ptyp));
  cvtFunc->SetAttr(FUNCATTR_public);
  cvtFunc->SetAttr(FUNCATTR_extern);
  cvtFunc->SetAttr(FUNCATTR_used);
  ASSERT_NOT_NULL(cvtFunc->GetFuncSymbol());
  cvtFunc->GetFuncSymbol()->SetAppearsInCode(true);
  cvtFunc->AllocSymTab();
  MIRSymbol *stubFuncRet = builder->CreateSymbol(TyIdx(pType),
                                                 GetSequentialName("expand128floats_stub"),
                                                 kStVar, kScAuto, func, kScopeLocal);
  auto opCall = builder->CreateStmtCallAssigned(cvtFunc->GetPuidx(),
                                                args, stubFuncRet, OP_callassigned);
  block->InsertBefore(stmt, opCall);

  auto cvtDread = builder->CreateDread(*stubFuncRet, currNode->ptyp);
  if (!kOpcodeInfo.IsCompare(currOpCode)) {
    baseNode->SetOpnd(cvtDread, opndId);
  } else {
    auto *zero = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *GlobalTables::GetTypeTable().GetInt32());
    Opcode newOpCode;
    if (stmt->GetOpCode() == OP_brtrue || stmt->GetOpCode() == OP_brfalse) {
      newOpCode = f128OpsRes[currOpCode].positiveRes;
    } else {
      if (baseNode->GetOpCode() == OP_while) {
        newOpCode = f128OpsRes[currOpCode].negativeRes;
      } else {
        newOpCode = currOpCode;
      }
    }
    auto newCond = builder->CreateExprCompare(newOpCode, *GlobalTables::GetTypeTable().GetUInt1(),
                                              *GlobalTables::GetTypeTable().GetInt32(),
                                              cvtDread, builder->CreateConstval(zero));
    baseNode->SetOpnd(newCond, opndId);
  }
}

bool Expand128Floats::CheckAndUpdateOp(BlockNode *block, BaseNode *node,
                                       MIRFunction *func, StmtNode *stmt) {
  auto nOp = node->NumOpnds();
  for (size_t i = 0; i < nOp; ++i) {
    bool isCvt = CheckAndUpdateOp(block, node->Opnd(i), func, stmt);
    if (isCvt) {
      ReplaceOpNode(block, node, i, node->Opnd(i), func, stmt);
    }
  }

  if (node->GetOpCode() == OP_cvt ||
      node->GetOpCode() == OP_trunc ||
      node->GetOpCode() == OP_sub ||
      node->GetOpCode() == OP_add ||
      node->GetOpCode() == OP_div ||
      node->GetOpCode() == OP_mul ||
      node->GetOpCode() == OP_neg ||
      node->GetOpCode() == OP_cmp ||
      node->GetOpCode() == OP_le ||
      node->GetOpCode() == OP_ge ||
      node->GetOpCode() == OP_lt ||
      node->GetOpCode() == OP_gt ||
      node->GetOpCode() == OP_ne ||
      node->GetOpCode() == OP_eq) {
    return true;
  }

  return false;
}

void Expand128Floats::ProcessBody(BlockNode *block, StmtNode *stmt, MIRFunction *func) {
  StmtNode *next = nullptr;
  while (stmt != nullptr) {
    next = stmt->GetNext();

    switch (stmt->GetOpCode()) {
      case OP_if: {
        auto *iNode = static_cast<IfStmtNode*>(stmt);
        ProcessBody(block, iNode->GetThenPart(), func);
        ProcessBody(block, iNode->GetElsePart(), func);
        if (CheckAndUpdateOp(block, stmt->Opnd(0), func, stmt)) {
            ReplaceOpNode(block, stmt, 0,
                          static_cast<StmtNode*>(stmt->Opnd(0)), func, stmt);
        }
        break;
      }
      case OP_block: {
        BlockNode *bNode = static_cast<BlockNode*>(stmt);
        ProcessBody(bNode, &(*bNode->GetStmtNodes().begin()), func);
        break;
      }

      case OP_while: {
        auto *wNode = static_cast<WhileStmtNode*>(stmt);
        ASSERT(block != nullptr, "null ptr check!");
        ProcessBody(block, wNode->GetBody(), func);
        if (CheckAndUpdateOp(block, stmt->Opnd(0), func, stmt)) {
            ReplaceOpNode(block, stmt, 0,
                          static_cast<StmtNode*>(stmt->Opnd(0)), func, stmt);
        }
        break;
      }

      default: {
        auto nOp = stmt->NumOpnds();
        for (size_t i = 0; i < nOp; ++i) {
          bool isCvt = CheckAndUpdateOp(block, stmt->Opnd(i), func, stmt);
          if (isCvt) {
            ReplaceOpNode(block, stmt, i,
                          static_cast<StmtNode*>(stmt->Opnd(i)), func, stmt);
          }
        }
        break;
      }
    }
    stmt = next;
  }
}

void Expand128Floats::ProcessBody(BlockNode *block, MIRFunction *func) {
  CHECK_NULL_FATAL(block);
  ProcessBody(block, block->GetFirst(), func);
}

void Expand128Floats::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }

  SetCurrentFunction(*func);
  ProcessBody(func->GetBody(), func);
}

void M2MExpand128Floats::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}

bool M2MExpand128Floats::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(Expand128Floats, m);
  return true;
}
} // namespace maple
