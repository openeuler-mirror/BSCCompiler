/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H

#include <limits>
#include <string>
#include <iomanip>
#include "aarch64_isa.h"
#include "operand.h"
#include "cg.h"
#include "emit.h"
#include "common_utils.h"

namespace maplebe {
using namespace maple;

/*
 * http://stackoverflow.com/questions/30904718/range-of-immediate-values-in-armv8-a64-assembly
 *
 * Unlike A32's "flexible second operand", there is no common
 * immediate format in A64. For immediate-operand data-processing
 * instructions (ignoring the boring and straightforward ones like shifts),
 *
 * 1. Arithmetic instructions (add{s}, sub{s}, cmp, cmn) take
 *    a 12-bit unsigned immediate with an optional 12-bit left shift.
 * 2. Move instructions (movz, movn, movk) take a 16-bit immediate
 *    optionally shifted to any 16-bit-aligned position within the register.
 * 3. Address calculations (adr, adrp) take a 21-bit signed immediate,
 *    although there's no actual syntax to specify it directly - to do
 *    so you'd have to resort to assembler expression trickery to generate
 *    an appropriate "label".
 * 4. Logical instructions (and{s}, orr, eor, tst) take a "bitmask immediate",
 *    which I'm not sure I can even explain, so I'll just quote the
 *    mind-bogglingly complicated definition:
 *    "Such an immediate is a 32-bit or 64-bit pattern viewed as a vector of
 *    identical elements of size e = 2, 4, 8, 16, 32, or 64 bits. Each element
 *    contains the same sub-pattern: a single run of 1 to e-1 non-zero bits,
 *    rotated by 0 to e-1 bits. This mechanism can generate 5,334 unique
 *    64-bit patterns (as 2,667 pairs of pattern and their bitwise inverse)."
 */

class ImmFPZeroOperand : public OperandVisitable<ImmFPZeroOperand> {
 public:
  explicit ImmFPZeroOperand(uint32 sz) : OperandVisitable(kOpdFPZeroImmediate, uint8(sz)) {}

  ~ImmFPZeroOperand() override = default;
  using OperandVisitable<ImmFPZeroOperand>::OperandVisitable;

  static ImmFPZeroOperand *Allocate(uint8 sz) {
    CHECK_FATAL((sz == k32BitSize || sz == k64BitSize), "half-precession is yet to be supported");
    auto *memPool = static_cast<MemPool*>(CG::GetCurCGFuncNoConst()->GetMemoryPool());
    ImmFPZeroOperand *inst = memPool->New<ImmFPZeroOperand>(static_cast<uint32>(sz));
    return inst;
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<ImmFPZeroOperand>(*this);
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    (void)opndProp;
    emitter.Emit("#0.0");
  }

  bool Less(const Operand &right) const override {
    /* For different type. */
    return GetKind() < right.GetKind();
  }

  void Dump() const override {
    LogInfo::MapleLogger() << "imm fp" << size << ": 0.0";
  }
};

/* representing for global variables address */
class StImmOperand : public OperandVisitable<StImmOperand> {
 public:
  StImmOperand(const MIRSymbol &symbol, int64 offset, int32 relocs)
      : OperandVisitable(kOpdStImmediate, 0), symbol(&symbol), offset(offset), relocs(relocs) {}

  ~StImmOperand() override = default;
  using OperandVisitable<StImmOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<StImmOperand>(*this);
  }

  const MIRSymbol *GetSymbol() const {
    return symbol;
  }

  const std::string &GetName() const {
    return symbol->GetName();
  }

  int64 GetOffset() const {
    return offset;
  }

  void SetOffset(int64 newOffset) {
    offset = newOffset;
  }

  int32 GetRelocs() const {
    return relocs;
  }

  bool operator==(const StImmOperand &opnd) const {
    return (symbol == opnd.symbol && offset == opnd.offset && relocs == opnd.relocs);
  }

  bool operator<(const StImmOperand &opnd) const {
    return (symbol < opnd.symbol || (symbol == opnd.symbol && offset < opnd.offset) ||
            (symbol == opnd.symbol && offset == opnd.offset && relocs < opnd.relocs));
  }

  bool Less(const Operand &right) const override;

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override;

  void Dump() const override {
    CHECK_FATAL(false, "dont run here");
  }

 private:
  const MIRSymbol *symbol;
  int64 offset;
  int32 relocs;
};

/* Use StImmOperand instead? */
class FuncNameOperand : public OperandVisitable<FuncNameOperand> {
 public:
  explicit FuncNameOperand(const MIRSymbol &fsym) : OperandVisitable(kOpdBBAddress, 0), symbol(&fsym) {}

  ~FuncNameOperand() override = default;
  using OperandVisitable<FuncNameOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<FuncNameOperand>(*this);
  }

  const std::string &GetName() const {
    return symbol->GetName();
  }

  const MIRSymbol *GetFunctionSymbol() const {
    return symbol;
  }

  bool IsFuncNameOpnd() const override {
    return true;
  }

  void SetFunctionSymbol(const MIRSymbol &fsym) {
    symbol = &fsym;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    (void)opndProp;
    emitter.Emit(GetName());
  }

  bool Less(const Operand &right) const override {
    if (&right == this) {
      return false;
    }
    /* For different type. */
    if (GetKind() != right.GetKind()) {
      return GetKind() < right.GetKind();
    }

    auto *rightOpnd = static_cast<const FuncNameOperand*>(&right);

    return static_cast<const void*>(symbol) < static_cast<const void*>(rightOpnd->symbol);
  }

  void Dump() const override {
    LogInfo::MapleLogger() << GetName();
  }

 private:
  const MIRSymbol *symbol;
};

class CondOperand : public OperandVisitable<CondOperand> {
 public:
  explicit CondOperand(AArch64CC_t cc) : OperandVisitable(Operand::kOpdCond, k4ByteSize), cc(cc) {}

  ~CondOperand() override = default;
  using OperandVisitable<CondOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.New<CondOperand>(cc);
  }

  AArch64CC_t GetCode() const {
    return cc;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    (void)opndProp;
    emitter.Emit(ccStrs[cc]);
  }

  bool Less(const Operand &right) const override;

  void Dump() const override {
    CHECK_FATAL(false, "dont run here");
  }

  static const char *ccStrs[kCcLast];

 private:
  AArch64CC_t cc;
};

/* used with MOVK */
class LogicalShiftLeftOperand : public OperandVisitable<LogicalShiftLeftOperand> {
 public:
  /*
   * Do not make the constructor public unless you are sure you know what you are doing.
   * Only AArch64CGFunc is supposed to create LogicalShiftLeftOperand objects
   * as part of initialization
   */
  LogicalShiftLeftOperand(uint32 amt, int32 bitLen)
      : OperandVisitable(Operand::kOpdShift, bitLen), shiftAmount(amt) {} /* bitlength is equal to 4 or 6 */

  ~LogicalShiftLeftOperand() override = default;
  using OperandVisitable<LogicalShiftLeftOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<LogicalShiftLeftOperand>(*this);
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    (void)opndProp;
    emitter.Emit(" LSL #").Emit(shiftAmount);
  }

  bool Less(const Operand &right) const override {
    if (&right == this) {
      return false;
    }

    /* For different type. */
    if (GetKind() != right.GetKind()) {
      return GetKind() < right.GetKind();
    }

    auto *rightOpnd = static_cast<const LogicalShiftLeftOperand*>(&right);

    /* The same type. */
    return shiftAmount < rightOpnd->shiftAmount;
  }

  uint32 GetShiftAmount() const {
    return shiftAmount;
  }

  void Dump() const override {
    CHECK_FATAL(false, "dont run here");
  }

 private:
  uint32 shiftAmount;
};

class ExtendShiftOperand : public OperandVisitable<ExtendShiftOperand> {
 public:
  /* if and only if at least one register is WSP, ARM Recommends use of the LSL operator name rathe than UXTW */
  enum ExtendOp : uint8 {
    kUndef,
    kUXTB,
    kUXTH,
    kUXTW, /* equal to lsl in 32bits */
    kUXTX, /* equal to lsl in 64bits */
    kSXTB,
    kSXTH,
    kSXTW,
    kSXTX,
  };

  ExtendShiftOperand(ExtendOp op, uint32 amt, int32 bitLen)
      : OperandVisitable(Operand::kOpdExtend, bitLen), extendOp(op), shiftAmount(amt) {}

  ~ExtendShiftOperand() override = default;
  using OperandVisitable<ExtendShiftOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<ExtendShiftOperand>(*this);
  }

  uint32 GetShiftAmount() const {
    return shiftAmount;
  }

  ExtendOp GetExtendOp() const {
    return extendOp;
  }

  void Emit(Emitter &emitter, const OpndProp *prop) const override;

  bool Less(const Operand &right) const override;

  void Dump() const override {
    CHECK_FATAL(false, "dont run here");
  }

  bool Equals(Operand &operand) const override {
    if (!operand.IsOpdExtend()) {
      return false;
    }
    auto &op = static_cast<ExtendShiftOperand&>(operand);
    return ((&op == this) || (op.GetExtendOp() == extendOp && op.GetShiftAmount() == shiftAmount));
  }

 private:
  ExtendOp extendOp;
  uint32 shiftAmount;
};

class BitShiftOperand : public OperandVisitable<BitShiftOperand> {
 public:
  enum ShiftOp : uint8 {
    kUndef,
    kLSL, /* logical shift left */
    kLSR, /* logical shift right */
    kASR, /* arithmetic shift right */
  };

  /* bitlength is equal to 5 or 6 */
  BitShiftOperand(ShiftOp op, uint32 amt, int32 bitLen)
      : OperandVisitable(Operand::kOpdShift, bitLen), shiftOp(op), shiftAmount(amt) {}

  ~BitShiftOperand() override = default;
  using OperandVisitable<BitShiftOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<BitShiftOperand>(*this);
  }

  void Emit(Emitter &emitter, const OpndProp *prop) const override {
    (void)prop;
    emitter.Emit((shiftOp == kLSL) ? "LSL #" : ((shiftOp == kLSR) ? "LSR #" : "ASR #")).Emit(shiftAmount);
  }

  bool Less(const Operand &right) const override;

  uint32 GetShiftAmount() const {
    return shiftAmount;
  }

  ShiftOp GetShiftOp() const {
    return shiftOp;
  }

  void Dump() const override {
    CHECK_FATAL(false, "dont run here");
  }

  bool Equals(Operand &operand) const override {
    if (!operand.IsOpdShift()) {
      return false;
    }
    auto &op = static_cast<BitShiftOperand&>(operand);
    return ((&op == this) || (op.GetShiftOp() == shiftOp && op.GetShiftAmount() == shiftAmount));
  }

 private:
  ShiftOp shiftOp;
  uint32 shiftAmount;
};

class CommentOperand : public OperandVisitable<CommentOperand> {
 public:
  CommentOperand(const char *str, MemPool &memPool)
      : OperandVisitable(Operand::kOpdString, 0), comment(str, &memPool) {}

  CommentOperand(const std::string &str, MemPool &memPool)
      : OperandVisitable(Operand::kOpdString, 0), comment(str, &memPool) {}

  ~CommentOperand() override = default;
  using OperandVisitable<CommentOperand>::OperandVisitable;

  const MapleString &GetComment() const {
    return comment;
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<CommentOperand>(*this);
  }

  bool IsCommentOpnd() const override {
    return true;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    (void)opndProp;
    emitter.Emit(comment);
  }

  bool Less(const Operand &right) const override {
    /* For different type. */
    return GetKind() < right.GetKind();
  }

  void Dump() const override {
    LogInfo::MapleLogger() << "# ";
    if (!comment.empty()) {
      LogInfo::MapleLogger() << comment;
    }
  }

 private:
  const MapleString comment;
};

using StringOperand = CommentOperand;

class ListConstraintOperand : public OperandVisitable<ListConstraintOperand> {
 public:
  explicit ListConstraintOperand(MapleAllocator &allocator)
      : OperandVisitable(Operand::kOpdString, 0),
        stringList(allocator.Adapter()) {};

  ~ListConstraintOperand() override = default;
  using OperandVisitable<ListConstraintOperand>::OperandVisitable;

  void Dump() const override {
    for (auto *str : stringList) {
      LogInfo::MapleLogger() << "(" << str->GetComment().c_str() << ")";
    }
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<ListConstraintOperand>(*this);
  }

  bool Less(const Operand &right) const override {
    /* For different type. */
    if (opndKind != right.GetKind()) {
      return opndKind < right.GetKind();
    }

    ASSERT(false, "We don't need to compare list operand.");
    return false;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override;

  MapleVector<StringOperand*> stringList;
};
}  /* namespace maplebe */
namespace std {
template<> /* function-template-specialization */
class std::hash<maplebe::MemOperand> {
 public:
  size_t operator()(const maplebe::MemOperand &x) const {
    std::size_t seed = 0;
    hash_combine<uint8_t>(seed, x.GetAddrMode());
    hash_combine<uint32_t>(seed, x.GetSize());
    maplebe::RegOperand *xb = x.GetBaseRegister();
    maplebe::RegOperand *xi = x.GetIndexRegister();
    if (xb != nullptr) {
      hash_combine<uint32_t>(seed, xb->GetRegisterNumber());
      hash_combine<uint32_t>(seed, xb->GetSize());
    }
    if (xi != nullptr) {
      hash_combine<uint32_t>(seed, xi->GetRegisterNumber());
      hash_combine<uint32_t>(seed, xi->GetSize());
    }
    return seed;
  }
};
}
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H */
