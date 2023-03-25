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
using StmtIndex = size_t;
using StmtInfoId = size_t;

constexpr PUIdx kInvalidPuIdx = std::numeric_limits<PUIdx>::max();
constexpr LabelIdx kInvalidLabelIdx = std::numeric_limits<LabelIdx>::max();

struct DefUsePositions {
  MapleVector<size_t> definePositions;
  MapleVector<size_t> usePositions;
  explicit DefUsePositions(MapleAllocator &alloc)
      : definePositions(alloc.Adapter()),
        usePositions(alloc.Adapter()) {}
};

class StmtInfo {
 public:
  StmtInfo(MeStmt *stmt, PUIdx puIdx, MapleAllocator &alloc)
      : allocator(alloc),
        meStmt(stmt),
        puIdx(puIdx),
        hashCandidate(allocator.Adapter()),
        symbolDefUse(allocator.Adapter()),
        regDefUse(allocator.Adapter()),
        locationsJumpFrom(allocator.Adapter()),
        locationsJumpTo(allocator.Adapter()) {
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
    if (meStmt->GetOp() == OP_intrinsiccall ||
        meStmt->GetOp() == OP_intrinsiccallassigned ||
        meStmt->GetOp() == OP_intrinsiccallwithtypeassigned) {
      hashCandidate.emplace_back(static_cast<IntrinsiccallMeStmt *>(meStmt)->GetIntrinsic());
    }
    if (meStmt->GetVarLHS() != nullptr) {
      GetExprHashCandidate(*meStmt->GetVarLHS());
    }
    if (meStmt->GetOp() == OP_iassign) {
      GetExprHashCandidate(*static_cast<IassignMeStmt *>(meStmt)->GetLHSVal());
    }
    hashCandidate.emplace_back(meStmt->NumMeStmtOpnds());
    for (size_t i = 0; i < meStmt->NumMeStmtOpnds(); ++i) {
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
      valid &= (ivar.GetFieldID() == 0);
    }
    if (meExpr.GetMeOp() == kMeOpVar) {
      auto &var = static_cast<VarMeExpr &>(meExpr);
      hashCandidate.emplace_back(var.GetFieldID());
      valid &= (var.GetFieldID() == 0);
    }
    if (meExpr.GetMeOp() == kMeOpAddrof) {
      auto &addr = static_cast<AddrofMeExpr &>(meExpr);
      hashCandidate.emplace_back(addr.GetFieldID());
      valid &= (addr.GetFieldID() == 0);
    }
    if (meExpr.GetMeOp() == kMeOpOp) {
      auto &opExpr = static_cast<OpMeExpr &>(meExpr);
      hashCandidate.emplace_back(opExpr.GetFieldID());
      hashCandidate.emplace_back(opExpr.GetBitsOffSet());
      hashCandidate.emplace_back(opExpr.GetBitsSize());
      valid &= (opExpr.GetFieldID() == 0);
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
    for (size_t i = 0; i < hashCandidate.size(); ++i) {
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

  const FreqType GetFrequency() const {
    return frequency;
  }

  void SetFrequency(FreqType freq) {
    frequency = freq;
  }

  DefUsePositions &GetDefUsePositions(OriginalSt &ost) {
    if (ost.IsPregOst()) {
      return regDefUse.insert({ost.GetPregIdx(), DefUsePositions(allocator)}).first->second;
    } else {
      return symbolDefUse.insert(
          {ost.GetMIRSymbol()->GetStIdx(), DefUsePositions(allocator)}).first->second;
    }
  }

  MapleUnorderedMap<StIdx, DefUsePositions> &GetSymbolDefUse() {
    return symbolDefUse;
  }

  MapleUnorderedMap<PregIdx, DefUsePositions> &GetRegDefUse() {
    return regDefUse;
  }

  MapleVector<uint32> &GetLocationsJumpFrom() {
    return locationsJumpFrom;
  }

  MapleVector<uint32> &GetLocationsJumpTo() {
    return locationsJumpTo;
  }

 private:
  MapleAllocator &allocator;
  StmtNode *stmt = nullptr;
  BlockNode *currBlock = nullptr;
  MeStmt *meStmt = nullptr;
  PUIdx puIdx = kInvalidPuIdx;
  FreqType frequency = 0;
  bool valid = true;
  MapleVector<uint32> hashCandidate;
  MapleUnorderedMap<StIdx, DefUsePositions> symbolDefUse;
  MapleUnorderedMap<PregIdx, DefUsePositions> regDefUse;
  MapleVector<uint32> locationsJumpFrom;
  MapleVector<uint32> locationsJumpTo;
};

class StmtInfoHash {
 public:
  size_t operator()(const StmtInfo &stmtInfo) const {
    auto hashCode = stmtInfo.GetHashCandidateAt(0);
    for (size_t i = 1; i < stmtInfo.GetHashCandidateSize(); ++i) {
      hashCode ^= stmtInfo.GetHashCandidateAt(i);
    }
    return hashCode;
  }
};
}
#endif  // MAPLE_IPA_INCLUDE_STMT_IDENTIFY_H
