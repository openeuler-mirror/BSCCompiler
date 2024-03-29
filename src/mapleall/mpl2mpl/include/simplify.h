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

const std::map<std::string, std::string> kAsmMap = {
#include "asm_map.def"
};

enum ErrorNumber : int32 {
  ERRNO_OK = EOK,
  ERRNO_INVAL = EINVAL,
  ERRNO_RANGE = ERANGE,
  ERRNO_INVAL_AND_RESET = EINVAL_AND_RESET,
  ERRNO_RANGE_AND_RESET = ERANGE_AND_RESET,
  ERRNO_OVERLAP_AND_RESET = EOVERLAP_AND_RESET
};

enum OpKind {
  kMemOpUnknown,
  kMemOpMemset,
  kMemOpMemcpy,
  kMemOpMemsetS,
  kMemOpMemcpyS,
  kSprintfOpSprintf,
  kSprintfOpSprintfS,
  kSprintfOpSnprintfS,
  kSprintfOpVsnprintfS
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

  BaseNode *BuildAsRhsExpr(MIRFunction &func, MIRType *accessType = nullptr) const;
  bool ExpandMemset(int64 byte, uint64 size, MIRFunction &func,
                    StmtNode &stmt, BlockNode &block, bool isLowLevel, bool debug, ErrorNumber errorNumber) const;
  void ExpandMemsetLowLevel(int64 byte, uint64 size, MIRFunction &func, StmtNode &stmt, BlockNode &block,
                            OpKind memOpKind, bool debug, ErrorNumber errorNumber) const;
  bool ExpandMemcpy(const MemEntry &srcMem, uint64 copySize, MIRFunction &func,
                    StmtNode &stmt, BlockNode &block, bool isLowLevel, bool debug, ErrorNumber errorNumber) const;
  void ExpandMemcpyLowLevel(const MemEntry &srcMem, uint64 copySize, MIRFunction &func, StmtNode &stmt,
                            BlockNode &block, OpKind memOpKind, bool debug, ErrorNumber errorNumber) const;
  static StmtNode *GenRetAssign(StmtNode &stmt, MIRFunction &func, bool isLowLevel, OpKind opKind,
                                int32 returnVal = ERRNO_OK);
  StmtNode *GenEndZeroAssign(const StmtNode &stmt, MIRFunction &func, bool isLowLevel, uint64 count) const;
  BaseNode *addrExpr = nullptr;   // memory address
  MIRType *memType = nullptr;     // memory type, this may be nullptr for low level memory entry
};

class ProxyMemOp {
 public:
  ProxyMemOp() = default;
  virtual ~ProxyMemOp() = default;
  virtual bool SimplifyMemcpy(StmtNode &stmt, BlockNode &block, bool isLowLevel) = 0;
  virtual MIRFunction *GetFunction() = 0;
  virtual bool IsDebug() const = 0;
};

class SprintfBaseOper {
 public:
  explicit SprintfBaseOper(ProxyMemOp &op) : op(op) {}
  void ProcessRetValue(StmtNode &stmt, BlockNode &block, OpKind opKind, int32 retVal, bool isLowLevel);
  bool DealWithFmtConstStr(StmtNode &stmt, const BaseNode *fmt, BlockNode &block, bool isLowLevel);
  static bool CheckCondIfNeedReplace(const StmtNode &stmt, uint32_t opIdx);
  static bool IsCountConst(StmtNode &stmt, uint64 &count, uint32_t opndIdx);
  StmtNode *InsertMemcpyCallStmt(const MapleVector<BaseNode *> &args, StmtNode &stmt,
                                 BlockNode &block, int32 retVal, bool isLowLevel);
  virtual bool GetDstMaxOrCountSize(StmtNode &stmt, uint64 &dstMax, uint64 &count) {
    CHECK_FATAL(false, "NEVER REACH");
  };
  bool ReplaceSprintfWithMemcpy(StmtNode &stmt, BlockNode &block, uint32 opndIdx, uint64 copySize, bool isLowLevel);
  bool CompareDstMaxSrcSize(StmtNode &stmt, BlockNode &block, uint64 dstMax, uint64 srcSize, bool isLowLevel);
  bool CompareCountSrcSize(StmtNode &stmt, BlockNode &block, uint64 count, uint64 srcSize, bool isLowLevel);
  bool DealWithDstOrEndZero(const StmtNode &stmt, BlockNode &block, bool isLowLevel, uint64 count);
  static bool CheckInvalidPara(uint64 count, uint64 dstMax, uint64 srcSize);
  virtual bool ReplaceSprintfIfNeeded(StmtNode &stmt, BlockNode &block, bool isLowLevel, const OpKind &opKind) {
    CHECK_FATAL(false, "NEVER REACH");
  };
  virtual ~SprintfBaseOper() = default;
 protected:
  ProxyMemOp &op;
};

class SimplifySprintf : public SprintfBaseOper {
 public:
  explicit SimplifySprintf(ProxyMemOp &op) : SprintfBaseOper(op) {}
  ~SimplifySprintf() override = default;
  bool ReplaceSprintfIfNeeded(StmtNode &stmt, BlockNode &block, bool isLowLevel, const OpKind &opKind) override;
};

class SimplifySprintfS : public SprintfBaseOper {
 public:
  explicit SimplifySprintfS(ProxyMemOp &op) : SprintfBaseOper(op) {}
  ~SimplifySprintfS() override = default;
  bool ReplaceSprintfIfNeeded(StmtNode &stmt, BlockNode &block, bool isLowLevel, const OpKind &opKind) override;
  bool GetDstMaxOrCountSize(StmtNode &stmt, uint64 &dstMax, uint64 &count) override;
};

class SimplifySnprintfS : public SprintfBaseOper {
 public:
  explicit SimplifySnprintfS(ProxyMemOp &op) : SprintfBaseOper(op) {}
  ~SimplifySnprintfS() override = default;
  bool ReplaceSprintfIfNeeded(StmtNode &stmt, BlockNode &block, bool isLowLevel, const OpKind &opKind) override;
  bool GetDstMaxOrCountSize(StmtNode &stmt, uint64 &dstMax, uint64 &count) override;
};

// For simplifying operation.
class SimplifyOp : public ProxyMemOp {
 public:
  static OpKind ComputeOpKind(StmtNode &stmt);
  ~SimplifyOp() override {
    func = nullptr;
  }

  explicit SimplifyOp(MemPool &memPool) : sprintfAlloc(&memPool), sprintfMap(sprintfAlloc.Adapter()) {
    auto simplifySnprintfs = sprintfAlloc.New<SimplifySnprintfS>(*this);
    (void)sprintfMap.emplace(kSprintfOpSprintf, sprintfAlloc.New<SimplifySprintf>(*this));
    (void)sprintfMap.emplace(kSprintfOpSprintfS, sprintfAlloc.New<SimplifySprintfS>(*this));
    (void)sprintfMap.emplace(kSprintfOpSnprintfS, simplifySnprintfs);
    (void)sprintfMap.emplace(kSprintfOpVsnprintfS, simplifySnprintfs);
  }
  void SetFunction(MIRFunction *f) {
    func = f;
  }

  MIRFunction *GetFunction() override {
    return func;
  }

  bool IsDebug() const override {
    return debug;
  }

  void SetDebug(bool dbg) {
    debug = dbg;
  }

  bool AutoSimplify(StmtNode &stmt, BlockNode &block, bool isLowLevel);
  bool SimplifyMemset(StmtNode &stmt, BlockNode &block, bool isLowLevel) const;
  bool SimplifyMemcpy(StmtNode &stmt, BlockNode &block, bool isLowLevel) override;
 private:
  void FoldMemsExpr(StmtNode &stmt, uint64 &srcSize, bool &isSrcSizeConst, uint64 &dstSize,
                    bool &isDstSizeConst) const;
  StmtNode *PartiallyExpandMemsetS(StmtNode &stmt, BlockNode &block) const;
  StmtNode *PartiallyExpandMemcpyS(StmtNode &stmt, BlockNode &block);

  static const uint32 thresholdMemsetExpand;
  static const uint32 thresholdMemsetSExpand;
  static const uint32 thresholdMemcpyExpand;
  static const uint32 thresholdMemcpySExpand;
  MIRFunction *func = nullptr;
  bool debug = false;
  MapleAllocator sprintfAlloc;
  MapleMap<OpKind, SprintfBaseOper*> sprintfMap;
};

class Simplify : public FuncOptimizeImpl {
 public:
  Simplify(MIRModule &mod, KlassHierarchy *kh, MemPool &memPool, bool dump)
      : FuncOptimizeImpl(mod, kh, dump), mirMod(mod), simplifyMemOp(memPool) {}
  Simplify(const Simplify &other) = delete;
  Simplify &operator=(const Simplify &other) = delete;
  ~Simplify() override = default;
  FuncOptimizeImpl *Clone() override {
    CHECK_FATAL(false, "Simplify has pointer, should not be Cloned");
  }

  void Finish() override;

 protected:
  void ProcessStmt(StmtNode &stmt) override;

 private:
  MIRModule &mirMod;
  SimplifyOp simplifyMemOp;
  bool IsMathSqrt(const std::string funcName);
  bool IsMathAbs(const std::string funcName);
  bool IsMathMin(const std::string funcName);
  bool IsMathMax(const std::string funcName);
  bool IsSymbolReplaceableWithConst(const MIRSymbol &symbol) const;
  bool IsConstRepalceable(const MIRConst &mirConst) const;
  bool SimplifyMathMethod(const StmtNode &stmt, BlockNode &block);
  void SimplifyCallAssigned(StmtNode &stmt, BlockNode &block);
  StmtNode *SimplifyBitFieldWrite(const IassignNode &iass) const;
  BaseNode *SimplifyBitFieldRead(IreadNode &iread) const;
  StmtNode *SimplifyToSelect(MIRFunction &func, IfStmtNode *ifNode, BlockNode *block) const;
  BaseNode *SimplifyExpr(BaseNode &expr);
  BaseNode *ReplaceExprWithConst(DreadNode &dread) const;
  MIRConst *GetElementConstFromFieldId(FieldID fieldId, MIRConst &mirConst) const;
};

MAPLE_MODULE_PHASE_DECLARE(M2MSimplify)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_SIMPLIFY_H
