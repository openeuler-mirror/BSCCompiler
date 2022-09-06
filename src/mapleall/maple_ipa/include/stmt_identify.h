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
#ifndef MAPLE_IPA_INCLUDE_STMT_IDENTIFY_H
#define MAPLE_IPA_INCLUDE_STMT_IDENTIFY_H

#include <sys/types.h>
#include <cstddef>
#include <limits>
#include <unordered_map>
#include <vector>
#include "irmap.h"
#include "me_ir.h"
#include "mir_nodes.h"
#include "opcodes.h"
#include "types_def.h"
namespace maple {
constexpr PUIdx kInvalidPuIdx = std::numeric_limits<PUIdx>::max();
constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();
constexpr LabelIdx kInvalidLabelIdx = std::numeric_limits<LabelIdx>::max();

struct DefUsePositions {
  std::vector<size_t> definePositions;
  std::vector<size_t> usePositions;
};

class StmtInfo {
 public:
  StmtInfo(MeStmt *stmt, PUIdx puIdx) : meStmt(stmt), puIdx(puIdx) {
    if (stmt) {
      CreateHashCandidate();
    }
  }
  virtual ~StmtInfo() = default;

  bool IsValid() {
    switch (hashCandidate[0]) {
      case OP_switch:
      case OP_return:
      case OP_asm:
        return false;
      default:
        return valid;
    }
  }

  bool IsCall() {
    switch (hashCandidate[0]) {
      case OP_call:
      case OP_callassigned:
      case OP_icall:
      case OP_icallassigned:
      case OP_icallprotoassigned:{
        return true;
      }
      default: {
        return false;
      }
    }
  }

  void CreateHashCandidate() {
    hashCandidate.emplace_back(meStmt->GetOp());
    if (meStmt->GetOp() == OP_call || meStmt->GetOp() == OP_callassigned) {
      hashCandidate.emplace_back(static_cast<CallMeStmt *>(meStmt)->GetPUIdx());
    }
    if (meStmt->GetVarLHS() != nullptr) {
      GetExprHashCandidate(*meStmt->GetVarLHS());
    }
    if (meStmt->GetOp() == OP_iassign) {
      GetExprHashCandidate(*static_cast<IassignMeStmt *>(meStmt)->GetLHSVal());
    }
    hashCandidate.emplace_back(meStmt->NumMeStmtOpnds());
    for (auto i = 0; i < meStmt->NumMeStmtOpnds(); ++i) {
      GetExprHashCandidate(*meStmt->GetOpnd(i));
    }
  }

  void GetExprHashCandidate(MeExpr &meExpr) {
    hashCandidate.emplace_back(meExpr.GetOp());
    hashCandidate.emplace_back(meExpr.GetPrimType());
    if (meExpr.GetMeOp() == kMeOpIvar) {
      auto &ivar = static_cast<IvarMeExpr &>(meExpr);
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetTyIdx());
      hashCandidate.emplace_back(static_cast<MIRPtrType*>(type)->GetPointedType()->GetPrimType());
      hashCandidate.emplace_back(ivar.GetFieldID());
      valid &= ivar.GetFieldID() == 0;
    }
    if (meExpr.GetMeOp() == kMeOpVar) {
      auto &var = static_cast<VarMeExpr &>(meExpr);
      hashCandidate.emplace_back(var.GetFieldID());
      valid &= var.GetFieldID() == 0;
    }
    if (meExpr.GetMeOp() == kMeOpAddrof) {
      auto &addr = static_cast<AddrofMeExpr &>(meExpr);
      hashCandidate.emplace_back(addr.GetFieldID());
      valid &= addr.GetFieldID() == 0;
    }
    if (meExpr.GetMeOp() == kMeOpOp) {
      auto &opExpr = static_cast<OpMeExpr &>(meExpr);
      hashCandidate.emplace_back(opExpr.GetFieldID());
      valid &= opExpr.GetFieldID() == 0;
    }
    for (auto i = 0; i < meExpr.GetNumOpnds(); ++i) {
      GetExprHashCandidate(*meExpr.GetOpnd(i));
    }
  }

  void DumpMeStmt(const IRMap *irMap) const {
    if (meStmt) {
      meStmt->Dump(irMap);
    }
  }

  void DumpStmtNode() const {
    if (stmt) {
      stmt->Dump(0);
    }
  }

  void ClearMeStmt() {
    meStmt = nullptr;
  }

  MeStmt *GetMeStmt() {
    return meStmt;
  }

  void SetStmtNode(StmtNode *node) {
    stmt = node;
  }

  StmtNode *GetStmtNode() {
    return stmt;
  }

  void SetCurrBlock(BlockNode *block) {
    currBlock = block;
  }

  BlockNode *GetCurrBlock() {
    return currBlock;
  }

  const bool operator==(const StmtInfo &rhs) const {
    if (hashCandidate.size() != rhs.hashCandidate.size()) {
      return false;
    }
    for (auto i = 0; i < hashCandidate.size(); ++i) {
      if (hashCandidate[i] != rhs.hashCandidate[i]) {
        return false;
      }
    }
    return true;
  }

  const uint8 GetHashCandidateAt(uint index) const {
    return hashCandidate[index];
  }

  const size_t GetHashCandidateSize() const {
    return hashCandidate.size();
  }

  const PUIdx GetPuIdx() const {
    return puIdx;
  }

  const uint64 GetFrequency() const {
    return frequency;
  }

  void SetFrequency(uint64 freq) {
    frequency = freq;
  }

  DefUsePositions &GetDefUsePositions(OriginalSt &ost) {
    if (ost.IsPregOst()) {
      return regDefUse[ost.GetPregIdx()];
    } else {
      return symbolDefUse[ost.GetMIRSymbol()->GetStIdx()];
    }
  }

  std::unordered_map<StIdx, DefUsePositions> &GetSymbolDefUse() {
    return symbolDefUse;
  }

  std::unordered_map<PregIdx, DefUsePositions> &GetRegDefUse() {
    return regDefUse;
  }

  std::vector<uint32> &GetLocationsJumpFrom() {
    return locationsJumpFrom;
  }

  std::vector<uint32> &GetLocationsJumpTo() {
    return locationsJumpTo;
  }

 private:
  StmtNode *stmt = nullptr;
  BlockNode *currBlock = nullptr;
  MeStmt *meStmt = nullptr;
  PUIdx puIdx = kInvalidPuIdx;
  uint64 frequency = 0;
  bool valid = true;
  std::vector<uint32> hashCandidate;
  std::unordered_map<StIdx, DefUsePositions> symbolDefUse;
  std::unordered_map<PregIdx, DefUsePositions> regDefUse;
  std::vector<uint32> locationsJumpFrom;
  std::vector<uint32> locationsJumpTo;
};

class StmtInfoHash {
 public:
  size_t operator()(const StmtInfo &stmtInfo) const {
    auto hashCode = stmtInfo.GetHashCandidateAt(0);
    for (auto i = 1; i < stmtInfo.GetHashCandidateSize(); ++i) {
      hashCode ^= stmtInfo.GetHashCandidateAt(i);
    }
    return hashCode;
  }
};
}
#endif  // MAPLE_IPA_INCLUDE_STMT_IDENTIFY_H
