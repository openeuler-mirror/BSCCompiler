/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MPL2MPL_INCLUDE_SIMPLIFY_H
#define MPL2MPL_INCLUDE_SIMPLIFY_H
#include "phase_impl.h"
#include "factory.h"
#include "maple_phase_manager.h"
namespace maple {
enum ErrorNumber {
  ERRNO_OK = EOK,
  ERRNO_INVAL = EINVAL,
  ERRNO_RANGE = ERANGE,
  ERRNO_RANGE_AND_RESET = ERANGE_AND_RESET
};

enum MemOpKind {
  MEM_OP_unknown,
  MEM_OP_memset,
  MEM_OP_memcpy,
  MEM_OP_memset_s
};

// MemEntry models a memory entry with high level type information.
struct MemEntry {
  enum MemEntryKind {
    kMemEntryUnknown,
    kMemEntryPrimitive,
    kMemEntryStruct,
    kMemEntryArray
  };

  static bool ComputeMemEntry(BaseNode &expr, MIRFunction &func, MemEntry &memEntry, bool isLowLevel);
  MemEntry() = default;
  MemEntry(BaseNode *addrExpr, MIRType *memType) : addrExpr(addrExpr), memType(memType) {}

  MemEntryKind GetKind() const {
    if (memType == nullptr) {
      return kMemEntryUnknown;
    }
    auto typeKind = memType->GetKind();
    if (typeKind == kTypeScalar || typeKind == kTypePointer) {
      return kMemEntryPrimitive;
    } else if (typeKind == kTypeArray) {
      return kMemEntryArray;
    } else if (memType->IsStructType()) {
      return kMemEntryStruct;
    }
    return kMemEntryUnknown;
  }

  BaseNode *BuildAsRhsExpr(MIRFunction &func) const;
  bool ExpandMemset(int64 byte, int64 size, MIRFunction &func,
                    CallNode &callStmt, BlockNode &block, bool isLowLevel, bool debug, ErrorNumber errorNumber) const;
  bool ExpandMemcpy(const MemEntry &srcMem, int64 copySize, MIRFunction &func,
                    CallNode &callStmt, BlockNode &block, bool isLowLevel, bool debug) const;
  static StmtNode *GenMemopRetAssign(CallNode &callStmt, MIRFunction &func, bool isLowLevel, MemOpKind memOpKind,
                                     ErrorNumber errorNumber = ERRNO_OK);

  BaseNode *addrExpr = nullptr;   // memory address
  MIRType *memType = nullptr;     // memory type, this may be nullptr for low level memory entry
};

// For simplifying memory operation, either memset or memcpy/memmove.
class SimplifyMemOp {
 public:
  static MemOpKind ComputeMemOpKind(StmtNode &stmt);
  SimplifyMemOp() = default;
  virtual ~SimplifyMemOp() = default;
  explicit SimplifyMemOp(MIRFunction *func, bool debug = false) : func(func), debug(debug) {}
  void SetFunction(MIRFunction *f) {
    func = f;
  }
  void SetDebug(bool dbg) {
    debug = dbg;
  }

  bool AutoSimplify(StmtNode &stmt, BlockNode &block, bool isLowLevel) const;
  bool SimplifyMemset(StmtNode &stmt, BlockNode &block, bool isLowLevel) const;
  bool SimplifyMemcpy(StmtNode &stmt, BlockNode &block, bool isLowLevel) const;
 private:
  static const uint32 thresholdMemsetExpand;
  static const uint32 thresholdMemsetSExpand;
  static const uint32 thresholdMemcpyExpand;
  MIRFunction *func = nullptr;
  bool debug = false;
};

class Simplify : public FuncOptimizeImpl {
 public:
  Simplify(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump), mirMod(mod) {
  }
  Simplify(const Simplify &other) = delete;
  Simplify &operator=(const Simplify &other) = delete;
  ~Simplify() = default;
  FuncOptimizeImpl *Clone() override {
    CHECK_FATAL(false, "Simplify has pointer, should not be Cloned");
  }

  void ProcessFunc(MIRFunction *func) override;
  void ProcessFuncStmt(MIRFunction &func, StmtNode *stmt = nullptr, BlockNode *block = nullptr);
  void Finish() override;

 private:
  MIRModule &mirMod;
  SimplifyMemOp simplifyMemOp;
  bool IsMathSqrt(const std::string funcName);
  bool IsMathAbs(const std::string funcName);
  bool IsMathMin(const std::string funcName);
  bool IsMathMax(const std::string funcName);
  bool SimplifyMathMethod(const StmtNode &stmt, BlockNode &block);
  void SimplifyCallAssigned(StmtNode &stmt, BlockNode &block);
  StmtNode *SimplifyToSelect(MIRFunction *func, IfStmtNode *ifNode, BlockNode *block);
};

MAPLE_MODULE_PHASE_DECLARE(M2MSimplify)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_SIMPLIFY_H
