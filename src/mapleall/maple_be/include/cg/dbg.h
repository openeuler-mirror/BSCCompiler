/*
 * Copyright (c) [2020-2022] Futurewei Technologies, Inc. All rights reverved.
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
#ifndef MAPLEBE_INCLUDE_CG_DBG_H
#define MAPLEBE_INCLUDE_CG_DBG_H

#include "insn.h"
#include "mempool_allocator.h"
#include "mir_symbol.h"
#include "debug_info.h"

namespace mpldbg {
using namespace maple;

/* https://sourceware.org/binutils/docs-2.28/as/Loc.html */
enum LocOpt { kBB, kProEnd, kEpiBeg, kIsStmt, kIsa, kDisc };

enum DbgOpcode : uint8 {
#define DBG_DEFINE(k, sub, n, o0, o1, o2) OP_DBG_##k##sub,
#define ARM_DIRECTIVES_DEFINE(k, sub, n, o0, o1, o2) OP_ARM_DIRECTIVES_##k##sub,
#include "dbg.def"
#undef DBG_DEFINE
#undef ARM_DIRECTIVES_DEFINE
  kOpDbgLast
};

class DbgInsn : public maplebe::Insn {
 public:
  DbgInsn(MemPool &memPool, maplebe::MOperator op) : Insn(memPool, op) {}

  DbgInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0) : Insn(memPool, op, opnd0) {}

  DbgInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0, maplebe::Operand &opnd1)
      : Insn(memPool, op, opnd0, opnd1) {}

  DbgInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0, maplebe::Operand &opnd1,
      maplebe::Operand &opnd2)
      : Insn(memPool, op, opnd0, opnd1, opnd2) {}

  ~DbgInsn() = default;

  bool IsMachineInstruction() const override {
    return false;
  }

  void Dump() const override;

#if defined(DEBUG) && DEBUG
  void Check() const override;
#endif

  bool IsTargetInsn() const override{
    return false;
  }

  bool IsDbgInsn() const override {
    return true;
  }

  bool IsRegDefined(maplebe::regno_t regNO) const override {
    CHECK_FATAL(false, "dbg insn do not def regs");
    return false;
  }

  std::set<uint32> GetDefRegs() const override{
    CHECK_FATAL(false, "dbg insn do not def regs");
    return std::set<uint32>();
  }

  uint32 GetBothDefUseOpnd() const override {
    return maplebe::kInsnMaxOpnd;
  }

  uint32 GetLoc() const;

 private:
  DbgInsn &operator=(const DbgInsn&);
};

class ImmOperand : public maplebe::OperandVisitable<ImmOperand> {
 public:
  explicit ImmOperand(int64 val) : OperandVisitable(kOpdImmediate, 32), val(val) {}

  ~ImmOperand() = default;
  using OperandVisitable<ImmOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    Operand *opnd =  memPool.Clone<ImmOperand>(*this);
    return opnd;
  }

  void Dump() const override;

  bool Less(const Operand &right) const override {
    (void)right;
    return false;
  }

  int64 GetVal() const {
    return val;
  }

 private:
  int64 val;
};

class DBGOpndEmitVisitor : public maplebe::OperandVisitorBase,
                           public maplebe::OperandVisitor<ImmOperand> {
 public:
  explicit DBGOpndEmitVisitor(maplebe::Emitter &asmEmitter): emitter(asmEmitter) {}
  virtual ~DBGOpndEmitVisitor() = default;
 protected:
  maplebe::Emitter &emitter;
 private:
  void Visit(ImmOperand *v) final;
};

}  /* namespace mpldbg */

#endif  /* MAPLEBE_INCLUDE_CG_DBG_H */
