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
#include "verify_memorder.h"
#include "mpl_logging.h"
#include "maple_phase_manager.h"

namespace maple {
// storage location of the memorder param in atomic func
constexpr int32 kNodeSecondOpnd = 1;
constexpr int32 kNodeThirdOpnd = 2;
constexpr int32 kNodeFourthOpnd = 3;
constexpr int32 kNodeFifthOpnd = 4;

int64 VerifyMemorder::GetMemorderValue(const ConstvalNode &memorderNode) const {
  int64 value = std::memory_order_seq_cst;
  const MIRConst *mirConst = memorderNode.GetConstVal();
  if (mirConst->GetKind() == kConstInt) {
    const MIRIntConst *intConst = static_cast<const MIRIntConst*>(mirConst);
    const IntVal &intval = intConst->GetValue();
    value = intval.GetExtValue();
  } else {
    CHECK_FATAL(false, "memorder param is not integer");
  }

  return value;
}

int64 VerifyMemorder::HandleMemorder(IntrinsiccallNode &intrn, size_t opnd) {
  const SrcPosition &srcPosition = intrn.GetSrcPos();
  int64 value = 0;
  BaseNode *baseNode = intrn.GetNopndAt(opnd);
  // memorder param is variable
  if (!baseNode->IsConstval()) {
    WARN_USER(kLncWarn, srcPosition, mod, "variable memorder in memory model to builtin");
    return std::memory_order_seq_cst;
  }

  // memorder param is const
  if (baseNode->IsConstval()) {
    ConstvalNode *memorderNode = static_cast<ConstvalNode *>(baseNode);
    value = GetMemorderValue(*memorderNode);
    if (value > static_cast<int64>(std::memory_order_seq_cst) ||
        value < static_cast<int64>(std::memory_order_relaxed)) {
      value = std::memory_order_seq_cst;
    }
  }

  return value;
}

void VerifyMemorder::SetMemorderNode(IntrinsiccallNode &intrn, int64 value, size_t opnd) {
  BaseNode *oldMemorderNode = intrn.GetNopndAt(opnd);
  ConstvalNode *newMemorderNode = mod.GetMIRBuilder()->CreateIntConst(static_cast<uint64>(value), PTY_u64);
  if (!oldMemorderNode->IsConstval() || GetMemorderValue(*static_cast<ConstvalNode *>(oldMemorderNode)) != value) {
    intrn.SetOpnd(newMemorderNode, opnd);
  }
}

void VerifyMemorder::HandleCompareExchangeMemorder(IntrinsiccallNode &intrn, size_t opnd) {
  int64 success = HandleMemorder(intrn, opnd);
  int64 failure = HandleMemorder(intrn, opnd + 1);
  if (failure > success) {
    success = std::memory_order_seq_cst;
  }

  if (failure == std::memory_order_release || failure == std::memory_order_acq_rel) {
    failure = std::memory_order_seq_cst;
    success = std::memory_order_seq_cst;
  }

  SetMemorderNode(intrn, success, opnd);
  SetMemorderNode(intrn, failure, opnd + 1);
}

void VerifyMemorder::HandleLoadMemorder(IntrinsiccallNode &intrn, size_t opnd) {
  int64 memorder = HandleMemorder(intrn, opnd);
  if (memorder == std::memory_order_release || memorder == std::memory_order_acq_rel) {
    memorder = std::memory_order_seq_cst;
  }
  SetMemorderNode(intrn, memorder, opnd);
}

void VerifyMemorder::HandleStoreMemorder(IntrinsiccallNode &intrn, size_t opnd) {
  int64 memorder = HandleMemorder(intrn, opnd);
  if (memorder == std::memory_order_consume || memorder == std::memory_order_acquire ||
      memorder == std::memory_order_acq_rel) {
    memorder = std::memory_order_seq_cst;
  }
  SetMemorderNode(intrn, memorder, opnd);
}

void VerifyMemorder::ProcessStmt(StmtNode &stmt) {
  if (!(stmt.GetOpCode() == OP_intrinsiccallwithtypeassigned || stmt.GetOpCode() == OP_intrinsiccallwithtype)) {
    return;
  }
  IntrinsiccallNode &intrn = static_cast<IntrinsiccallNode&>(stmt);
  switch (intrn.GetIntrinsic()) {
    case INTRN_C___atomic_load_n: {
      HandleLoadMemorder(intrn, kNodeSecondOpnd);
      break;
    }
    case INTRN_C___atomic_test_and_set: {
      SetMemorderNode(intrn, HandleMemorder(intrn, kNodeSecondOpnd), kNodeSecondOpnd);
      break;
    }
    case INTRN_C___atomic_clear: {
      HandleStoreMemorder(intrn, kNodeSecondOpnd);
      break;
    }
    case INTRN_C___atomic_load: {
      HandleLoadMemorder(intrn, kNodeThirdOpnd);
      break;
    }
    case INTRN_C___atomic_store_n:
    case INTRN_C___atomic_store: {
      HandleStoreMemorder(intrn, kNodeThirdOpnd);
      break;
    }
    case INTRN_C___atomic_exchange_n:
    case INTRN_C___atomic_add_fetch:
    case INTRN_C___atomic_sub_fetch:
    case INTRN_C___atomic_and_fetch:
    case INTRN_C___atomic_xor_fetch:
    case INTRN_C___atomic_or_fetch:
    case INTRN_C___atomic_nand_fetch:
    case INTRN_C___atomic_fetch_add:
    case INTRN_C___atomic_fetch_sub:
    case INTRN_C___atomic_fetch_and:
    case INTRN_C___atomic_fetch_xor:
    case INTRN_C___atomic_fetch_or:
    case INTRN_C___atomic_fetch_nand: {
      SetMemorderNode(intrn, HandleMemorder(intrn, kNodeThirdOpnd), kNodeThirdOpnd);
      break;
    }
    case INTRN_C___atomic_exchange: {
      SetMemorderNode(intrn, HandleMemorder(intrn, kNodeFourthOpnd), kNodeFourthOpnd);
      break;
    }
    case INTRN_C___atomic_compare_exchange_n:
    case INTRN_C___atomic_compare_exchange: {
      HandleCompareExchangeMemorder(intrn, kNodeFifthOpnd);
      break;
    }
    default:
      break;
  }
}

void M2MVerifyMemorder::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}

bool M2MVerifyMemorder::PhaseRun(MIRModule &m) {
  OPT_TEMPLATE_NEWPM(VerifyMemorder, m);

  return true;
}
} // namespace maple
