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

#include "me_stack_protect.h"
#include "me_irmap_build.h"
#include "cg_options.h"

namespace maple {
bool MeStackProtect::IsStackSymbol(const OriginalSt &ost) const {
  auto *symbol = ost.GetMIRSymbol();
  if (symbol->GetStorageClass() == kScAuto) {
    return true;
  }
  return false;
}

bool MeStackProtect::IsValidWrite(const OriginalSt &targetOst, uint64 numByteToWrite,
                                  uint64 extraOffset) const {
  if (!IsStackSymbol(targetOst)) {
    return true;
  }
  auto offset = targetOst.GetOffset();
  if (offset.IsInvalid()) {
    // offset of array is set to be invalid -> it is safe to set the offset to be 0 in this case
    if (!targetOst.GetType()->IsMIRArrayType()) {
      return false;
    }
  } else {
    CHECK_FATAL(offset.val >= 0, "offset has to be >= 0");
    extraOffset += static_cast<uint64>(offset.val);
  }
  // check whether it is within bound
  if ((numByteToWrite + extraOffset) <= static_cast<uint64>(targetOst.GetType()->GetSize())) {
    return true;
  } else {
    return false;
  }
}

bool MeStackProtect::RecordExpr(std::set<int32> &visitedDefs, const ScalarMeExpr& expr) const {
  auto exprID = expr.GetExprID();
  auto it = visitedDefs.insert(exprID);
  if (!it.second) {
    return false; // visited
  }
  return true;
}

bool MeStackProtect::IsDefInvolveAddressOfStackVar(const ScalarMeExpr &expr, std::set<int32> &visitedDefs) const {
  switch (expr.GetDefBy()) {
    case kDefByMustDef: {
      // it may point the address of stack variable if
      // - the function call (call or intrinsic call) takes the addresses of stack variable as arguments, and
      // - return the addresses of stack variables
      // however, there is no need to track this, since the function call will be marked as "malicious"
      return false;
    }
    case kDefByPhi: {
      const auto defStmt = expr.GetDefPhi();
      for (const auto opnd: defStmt.GetOpnds()) {
        if (!RecordExpr(visitedDefs, *opnd)) {
          return false;
        }
        if (IsPointToAddressOfStackVar(*opnd, visitedDefs)) {
          return true;
        }
      }
      return false;
    }
    case kDefByStmt: {
      const auto defStmt = expr.GetDefStmt();
      return IsPointToAddressOfStackVar(*defStmt->GetRHS(), visitedDefs);
    }
    case kDefByNo: {
      auto ost = expr.GetOst();
      if (ost->IsPregOst()) {
        return !ost->IsSpecialPreg();
      } else {
        return IsStackSymbol(*ost);
      }
    }
    case kDefByChi: {
      const auto defStmt = expr.GetDefChi();
      const auto rhs = defStmt.GetRHS();
      if (!RecordExpr(visitedDefs, *rhs)) {
        return false;
      }
      return IsPointToAddressOfStackVar(*rhs, visitedDefs);
    }
    default: {
      CHECK_FATAL(false, "Undefined.");
    }
  }
}

bool MeStackProtect::IsMeOpExprPointedToAddressOfStackVar(const MeExpr &expr, std::set<int32> &visitedDefs) const {
  for (size_t argIdx = 0; argIdx < expr.GetNumOpnds(); ++argIdx) {
    if (IsPointToAddressOfStackVar(*expr.GetOpnd(argIdx), visitedDefs)) {
      return true;
    }
  }
  return false;
}

bool MayBeAddress(PrimType type) {
  if (type == PTY_ptr) {
    return true;
  }
  auto curPtrType = GetExactPtrPrimType();
  if (curPtrType == PTY_a32 && (type == PTY_u32 || type == PTY_a32 || type == PTY_i32)) {
    return true;
  }
  if (curPtrType == PTY_a64 && (type == PTY_u64 || type == PTY_a64 || type == PTY_i64)) {
    return true;
  }
  return false;
}

bool MeStackProtect::IsPointToAddressOfStackVar(const MeExpr &expr, std::set<int32> &visitedDefs) const {
  // check whether it may be address given the type
  if (!MayBeAddress(expr.GetPrimType())) {
    return false;
  }
  // check whether the address pointed by the ptr is stack address
  switch (expr.GetMeOp()) {
    case kMeOpReg: {
      return IsDefInvolveAddressOfStackVar(static_cast<const RegMeExpr&>(expr), visitedDefs);
    }
    case kMeOpVar: {
      return IsDefInvolveAddressOfStackVar(static_cast<const VarMeExpr&>(expr), visitedDefs);
    }
    case kMeOpAddrof: {
      return IsStackSymbol(*static_cast<const AddrofMeExpr&>(expr).GetOst());
    }
    case kMeOpOp: {
      return IsMeOpExprPointedToAddressOfStackVar(expr, visitedDefs);
    }
    default: {
      return false;
    }
  }
}

bool MeStackProtect::IsCallSafe(const MeStmt &stmt, bool isIcall, const FuncDesc *funcDesc) const {
  size_t argIdx = isIcall ? 1 : 0;
  for (; argIdx < stmt.NumMeStmtOpnds(); ++argIdx) {
    if (funcDesc != nullptr &&
        (funcDesc->IsArgUnused(argIdx) ||
         funcDesc->IsArgReadSelfOnly(argIdx) ||
         funcDesc->IsArgReadMemoryOnly(argIdx))) {
      continue;
    }
    std::set<int32> visitedDefs;
    if (IsPointToAddressOfStackVar(*stmt.GetOpnd(argIdx), visitedDefs)) {
      return false;
    }
  }
  return true;
}

bool MeStackProtect::IsWriteFromSourceSafe(const MeStmt &stmt, uint64 numOfBytesToWrite) const {
  // always write to the first opnd
  auto lhs = stmt.GetOpnd(0);
  auto lhsOp = lhs->GetOp();
  if (lhsOp != OP_addrof && lhsOp != OP_iaddrof) {
    std::set<int32> visitedDefs;
    return !IsPointToAddressOfStackVar(*lhs, visitedDefs);
  }
  OriginalSt* lhsOst;
  uint64 offset = 0;
  if (lhsOp == OP_addrof) {
    lhsOst = static_cast<AddrofMeExpr*>(lhs)->GetOst();
  } else {
    auto iaddrofExpr = static_cast<OpMeExpr*>(lhs);
    auto targetOpnd = iaddrofExpr->GetOpnd(0);
    auto targetOp = targetOpnd->GetMeOp();
    if (targetOp == kMeOpAddrof) {
      lhsOst = static_cast<AddrofMeExpr*>(targetOpnd)->GetOst();
    } else {
      std::set<int32> visitedDefs;
      return !IsPointToAddressOfStackVar(*lhs, visitedDefs);
    }
    constexpr uint8 kNumOfBitsInByte = 8;
    offset += static_cast<uint64>(iaddrofExpr->GetBitsOffSet() / kNumOfBitsInByte);
  }
  return IsValidWrite(*lhsOst, numOfBytesToWrite, offset);
}

bool MeStackProtect::IsIntrnCallSafe(const MeStmt &stmt) const {
  auto intrnCallStmt = static_cast<const IntrinsiccallMeStmt&>(stmt);
  auto intrinsicID = intrnCallStmt.GetIntrinsic();
  // no side effect --> not write anything --> safe
  if (IntrinDesc::intrinTable[intrinsicID].HasNoSideEffect()) {
    return true;
  }
  switch (intrinsicID) {
    case INTRN_C_strcpy: {
      // strcpy: <target, src>
      // - copy everything from src to target
      // size is equal to the size of second operand
      auto numOfBytesToWrite = GetPrimTypeSize(stmt.GetOpnd(1)->GetPrimType());
      return IsWriteFromSourceSafe(stmt, static_cast<uint64>(numOfBytesToWrite));
    }
    case INTRN_C_strncpy:
    case INTRN_C_memcpy:
    case INTRN_C_memset: {
      // memset, memcpy, and strncpy share the same pattern <target, src, size>:
      // - write to the memory pointed by the first opnd
      // - the number of written bytes is controlled by the third opnd
      auto sizeExpr = stmt.GetOpnd(2);
      if (sizeExpr->GetOp() == OP_constval) {
        auto numOfBytesToWrite = static_cast<ConstMeExpr*>(sizeExpr)->GetSXTIntValue();
        CHECK_FATAL(numOfBytesToWrite >= 0, "num of bytes to write cannot be negative.");
        return IsWriteFromSourceSafe(stmt, static_cast<uint64>(numOfBytesToWrite));
      } else {
        // check whether the first opnd points to stack
        std::set<int32> visitedDefs;
        return !IsPointToAddressOfStackVar(*stmt.GetOpnd(0), visitedDefs);
      }
    }
    case INTRN_C_va_start: {
      return true;
    }
    default: {
      // treat it as same as normal call
      return IsCallSafe(stmt, false);
    }
  }
}

const FuncDesc* MeStackProtect::GetFuncDesc(const MeStmt &stmt) const {
  auto callStmt = static_cast<const CallMeStmt&>(stmt);
  MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt.GetPUIdx());
  auto funcDesc = &(callee->GetFuncDesc());
  if (!funcDesc->IsConfiged()) { // only desc from .def file is trustworthy
    funcDesc = nullptr;
  }
  return funcDesc;
}

bool MeStackProtect::IsMeStmtSafe(const MeStmt &stmt) const {
  switch (stmt.GetOp()) {
    // write to stack given the address of stack variable --> may be malicious
    case OP_iassign:
    case OP_iassignoff: {
      auto accessedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
          static_cast<const IassignMeStmt&>(stmt).GetTyIdx());
      CHECK_FATAL(accessedType->IsMIRPtrType(), "need to be a pointer");
      auto numOfByteToWrite = static_cast<uint64>(static_cast<MIRPtrType*>(accessedType)->GetPointedType()->GetSize());
      return IsWriteFromSourceSafe(stmt, numOfByteToWrite);
    }
    // address of stack variable escape --> may be malicious
    case OP_icallprotoassigned:
    case OP_icallproto:
    case OP_icallassigned:
    case OP_icall: {
      return IsCallSafe(stmt, true);
    }
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_call:
    case OP_callassigned:
    case OP_superclasscallassigned:
    case OP_interfaceicallassigned:
    case OP_interfacecallassigned:
    case OP_virtualicallassigned:
    case OP_virtualcallassigned: {
      const auto funcDesc = GetFuncDesc(stmt);
      return IsCallSafe(stmt, false, funcDesc);
    }
    // intrinsic call with explicit meaning:
    // - no side effect --> safe
    // - write to stack given the address of stack variable --> may be malicious
    case OP_xintrinsiccall:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned:
    case OP_intrinsiccall: {
      return IsIntrnCallSafe(stmt);
    }
    default: {
      // may write to reg / stack without its address --> safe
      return true;
    }
  }
}

/*
 * Following potential behaviors may use the address of stack variables for malicious purposes:
 * - callees take the address of stack variables as inputs and may write to it
 * - write stack variables given its address (iassign & intrinsiccall)
 * Note: Arguments are not considered as stack variables here
 */
void MeStackProtect::CheckAddrofStack() {
  auto *mirFunc = f->GetMirFunc();
  mirFunc->CheckMayWriteToAddrofStack();
  mirFunc->UnsetMayWriteToAddrofStack();  // reset it before analysis
  if (MayWriteStack()) {
    mirFunc->SetMayWriteToAddrofStack();
  }
}

bool MeStackProtect::MayWriteStack() const {
  auto *cfg = f->GetCfg();
  for (BB *bb: cfg->GetAllBBs()) {
    if (bb == nullptr || bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB()) {
      continue;
    }
    for (auto &stmt: bb->GetMeStmts()) {
      if (!IsMeStmtSafe(stmt)) {
        f->GetMirFunc()->SetMayWriteToAddrofStack();
        return true;
      }
    }
  }
  return false;
}

bool FuncMayWriteStack(MeFunction &func) {
  MeStackProtect checker(func);
  return checker.MayWriteStack();
}
}

