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
#ifndef MAPLEBE_INCLUDE_CG_OPERAND_H
#define MAPLEBE_INCLUDE_CG_OPERAND_H

#include "becommon.h"
#include "cg_option.h"
#include "visitor_common.h"

/* maple_ir */
#include "types_def.h"  /* need uint8 etc */
#include "prim_types.h" /* for PrimType */
#include "mir_symbol.h"

/* Mempool */
#include "mempool_allocator.h" /* MapleList */
#include "memlayout.h"

namespace maplebe {
class OpndProp;
class Emitter;

bool IsBitSizeImmediate(maple::uint64 val, maple::uint32 bitLen, maple::uint32 nLowerZeroBits);
bool IsBitmaskImmediate(maple::uint64 val, maple::uint32 bitLen);
bool IsMoveWidableImmediate(uint64 val, uint32 bitLen);
bool BetterUseMOVZ(uint64 val);


using MOperator = uint32;
enum RegType : maple::uint8 {
  kRegTyUndef,
  kRegTyInt,
  kRegTyFloat,
  kRegTyCc,
  kRegTyX87,
  kRegTyVary,
  kRegTyFpsc,
  kRegTyIndex,
  kRegTyLast,
};

class Operand {
 public:
  enum OperandType : uint8 {
    kOpdRegister,
    kOpdImmediate,
    kOpdMem,
    kOpdCond,     /*  for condition code */
    kOpdPhi,      /*  for phi operand */
    kOpdFPImmediate,
    kOpdFPZeroImmediate,
    kOpdStImmediate, /* use the symbol name as the offset */
    kOpdOffset,      /* for the offset operand in MemOperand */
    kOpdBBAddress,
    kOpdList,     /*  for list operand */
    kOpdShift,    /*  for imm shift operand */
    kOpdRegShift, /*  for reg shift operand */
    kOpdExtend,   /*  for extend operand */
    kOpdString,   /*  for comments */
    kOpdUndef
  };

  Operand(OperandType type, uint32 size) : opndKind(type), size(size) {}
  virtual ~Operand() = default;

  uint32 GetSize() const {
    return size;
  }

  void SetSize(uint32 sz) {
    size = sz;
  }

  OperandType GetKind() const {
    return opndKind;
  }

  bool IsIntImmediate() const {
    return opndKind == kOpdImmediate || opndKind == kOpdOffset;
  }

  bool IsConstImmediate() const {
    return opndKind == kOpdImmediate || opndKind == kOpdOffset || opndKind == kOpdFPImmediate ||
           opndKind == kOpdFPZeroImmediate;
  }

  bool IsOfstImmediate() const {
    return opndKind == kOpdOffset;
  }

  bool IsStImmediate() const {
    return opndKind == kOpdStImmediate;
  }

  bool IsImmediate() const {
    return (kOpdFPImmediate <= opndKind && opndKind <= kOpdOffset) || opndKind == kOpdImmediate;
  }

  bool IsRegister() const {
    return opndKind == kOpdRegister;
  }

  bool IsList() const {
    return opndKind == kOpdList;
  }

  bool IsPhi() const {
    return opndKind == kOpdPhi;
  }

  bool IsMemoryAccessOperand() const {
    return opndKind == kOpdMem;
  }

  bool IsLabel() const {
    return opndKind == kOpdBBAddress;
  }

  bool IsConditionCode() const {
    return opndKind == kOpdCond;
  }

  bool IsOpdShift() const {
    return opndKind == kOpdShift;
  }

  bool IsRegShift() const {
    return opndKind == kOpdRegShift;
  }

  bool IsOpdExtend() const {
    return opndKind == kOpdExtend;
  }

  virtual bool IsLabelOpnd() const {
    return false;
  }

  virtual bool IsFuncNameOpnd() const {
    return false;
  }

  virtual bool IsCommentOpnd() const {
    return false;
  }

  virtual Operand *Clone(MemPool &memPool) const = 0;

  /*
   * A simple implementation here.
   * Each subclass can elaborate on demand.
   */
  virtual bool Equals(Operand &op) const {
    return BasicEquals(op) && (&op == this);
  }

  bool BasicEquals(const Operand &op) const {
    return opndKind == op.GetKind() && size == op.GetSize();
  }

  virtual void Emit(Emitter&, const OpndProp*) const = 0;

  virtual void Dump() const = 0;


  virtual bool Less(const Operand &right) const = 0;

  virtual void Accept(OperandVisitorBase &v) = 0;

 protected:
  OperandType opndKind; /* operand type */
  uint32 size;          /* size in bits */
  uint64 flag = 0;      /* operand property*/
};

/* RegOperand */
enum RegOperandState : uint32 {
  kRegOpndNone = 0,
  kRegOpndSetLow32 = 0x1,
  kRegOpndSetHigh32 = 0x2
};

template<typename VisitableTy>
class OperandVisitable : public Operand {
 public:
  using Operand::Operand;
  void Accept(OperandVisitorBase &v) override {
    if (OperandVisitor<VisitableTy>* typeV = dynamic_cast<OperandVisitor<VisitableTy>*>(&v)) {
      typeV->Visit(static_cast<VisitableTy*>(this));
    } else {
      /* the type which has no implements */
    }
  }
};

class RegOperand : public OperandVisitable<RegOperand> {
 public:
  RegOperand(regno_t regNum, uint32 size, RegType type, uint32 flg = 0)
      : OperandVisitable(kOpdRegister, size),
        regNO(regNum),
        regType(type),
        validBitsNum(size),
        regFlag(flg) {}

  ~RegOperand() override = default;
  using OperandVisitable<RegOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<RegOperand>(*this);
  }

  void SetValidBitsNum(uint32 validNum) {
    validBitsNum = validNum;
  }

  uint32 GetValidBitsNum() const {
    return validBitsNum;
  }

  bool IsOfIntClass() const {
    return regType == kRegTyInt;
  }

  bool IsOfFloatOrSIMDClass() const {
    return regType == kRegTyFloat;
  }

  bool IsOfCC() const {
    return regType == kRegTyCc;
  }

  bool IsOfVary() const {
    return regType == kRegTyVary;
  }

  RegType GetRegisterType() const {
    return regType;
  }

  void SetRegisterType(RegType newTy) {
    regType = newTy;
  }

  virtual bool IsBBLocalReg() const {
    return isBBLocal;
  }

  void SetRegNotBBLocal() {
    isBBLocal = false;
  }

  regno_t GetRegisterNumber() const {
    return regNO;
  }

  void SetRegisterNumber(regno_t regNum) {
    regNO = regNum;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    CHECK_FATAL(false, "do not run here");
  };
  void Dump() const override {
    CHECK_FATAL(false, "do not run here");
  };

  bool Less(const Operand &right) const override {
    if (&right == this) {
      return false;
    }

    /* For different type. */
    if (opndKind != right.GetKind()) {
      return opndKind < right.GetKind();
    }

    auto *rightOpnd = static_cast<const RegOperand*>(&right);

    /* The same type. */
    return regNO < rightOpnd->regNO;
  }

  bool Less(const RegOperand &right) const {
    return regNO < right.regNO;
  }

  bool RegNumEqual(const RegOperand &right) const {
    return regNO == right.GetRegisterNumber();
  }

  int32 RegCompare(const RegOperand &right) const {
    return (regNO - right.GetRegisterNumber());
  }

  bool Equals(Operand &operand) const override {
    if (!operand.IsRegister()) {
      return false;
    }
    auto &op = static_cast<RegOperand&>(operand);
    if (&op == this) {
      return true;
    }
    return (BasicEquals(op) && regNO == op.GetRegisterNumber() && regType == op.GetRegisterType() &&
            IsBBLocalReg() == op.IsBBLocalReg());
  }

  static bool IsSameRegNO(const Operand &firstOpnd, const Operand &secondOpnd) {
    if (!firstOpnd.IsRegister() || !secondOpnd.IsRegister()) {
      return false;
    }
    auto &firstReg = static_cast<const RegOperand&>(firstOpnd);
    auto &secondReg = static_cast<const RegOperand&>(secondOpnd);
    return firstReg.RegNumEqual(secondReg);
  }

  static bool IsSameReg(const Operand &firstOpnd, const Operand &secondOpnd) {
    if (firstOpnd.GetSize() != secondOpnd.GetSize()) {
      return false;
    }
    return IsSameRegNO(firstOpnd, secondOpnd);
  }

  void SetOpndSSAForm() {
    isSSAForm = true;
  }

  void SetOpndOutOfSSAForm() {
    isSSAForm = false;
  }

  bool IsSSAForm() const {
    return isSSAForm;
  }

  void SetRefField(bool newIsRefField) {
    isRefField = newIsRefField;
  }

  bool IsPhysicalRegister() const {
    return GetRegisterNumber() < 100 && !IsOfCC();
  }

  bool IsVirtualRegister() const {
    return !IsPhysicalRegister();
  }

  bool IsBBLocalVReg() const {
    return IsVirtualRegister() && IsBBLocalReg();
  }

  void SetIF64Vec() {
    if64Vec = true;
  }

  bool GetIF64Vec() const {
    return if64Vec;
  }

  void SetVecLanePosition(int32 pos) {
    vecLane = static_cast<int16>(pos);
  }

  int32 GetVecLanePosition() const {
    return vecLane;
  }

  void SetVecLaneSize(uint32 size) {
    vecLaneSize = static_cast<uint16>(size);
  }

  uint32 GetVecLaneSize() const {
    return vecLaneSize;
  }

  void SetVecElementSize(uint32 size) {
    vecElementSize = size;
  }

  uint64 GetVecElementSize() const {
    return vecElementSize;
  }

  bool operator==(const RegOperand &o) const;

  bool operator<(const RegOperand &o) const;

 protected:
  regno_t regNO;
  RegType regType;

  /*
   * used for EBO(-O1), it can recognize the registers whose use and def are in different BB. It is
   * true by default. Sometime it should be false such as when handle intrinsiccall for target
   * aarch64(AArch64CGFunc::SelectIntrinCall).
   */
  bool isBBLocal = true;
  uint32 validBitsNum;
  /* use for SSA analysis */
  bool isSSAForm = false;
  bool isRefField = false;
  uint32 regFlag = 0;
  int16 vecLane = -1;     /* -1 for whole reg, 0 to 15 to specify each lane one at a time */
  uint16 vecLaneSize = 0; /* Number of lanes */
  uint64 vecElementSize = 0;  /* size of vector element in each lane */
  bool if64Vec = false;   /* operand returning 64x1's int value in FP/Simd register */
}; /* class RegOperand */

enum VaryType : uint8 {
  kNotVary = 0,
  kUnAdjustVary,
  kAdjustVary,
};

class ImmOperand : public OperandVisitable<ImmOperand> {
 public:
  ImmOperand(int64 val, uint32 size, bool isSigned, VaryType isVar = kNotVary, bool isFloat = false)
      : OperandVisitable(kOpdImmediate, size), value(val), isSigned(isSigned), isVary(isVar), isFmov(isFloat) {}
  ImmOperand(OperandType type, int64 val, uint32 size, bool isSigned, VaryType isVar = kNotVary, bool isFloat = false)
      : OperandVisitable(type, size), value(val), isSigned(isSigned), isVary(isVar), isFmov(isFloat) {}

  ~ImmOperand() override = default;
  using OperandVisitable<ImmOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<ImmOperand>(*this);
  }

  bool IsInBitSize(uint8 size, uint8 nLowerZeroBits) const {
    return maplebe::IsBitSizeImmediate(static_cast<uint64>(value), size, nLowerZeroBits);
  }

  bool IsBitmaskImmediate() const {
    ASSERT(!IsZero(), " 0 is reserved for bitmask immediate");
    ASSERT(!IsAllOnes(), " -1 is reserved for bitmask immediate");
    return maplebe::IsBitmaskImmediate(static_cast<uint64>(value), static_cast<uint32>(size));
  }

  bool IsBitmaskImmediate(uint32 destSize) const {
    ASSERT(!IsZero(), " 0 is reserved for bitmask immediate");
    ASSERT(!IsAllOnes(), " -1 is reserved for bitmask immediate");
    return maplebe::IsBitmaskImmediate(static_cast<uint64>(value), static_cast<uint32>(destSize));
  }

  bool IsSingleInstructionMovable() const {
    return (IsMoveWidableImmediate(static_cast<uint64>(value), static_cast<uint32>(size)) ||
            IsMoveWidableImmediate(~static_cast<uint64>(value), static_cast<uint32>(size)) ||
            IsBitmaskImmediate());
  }

  bool IsSingleInstructionMovable(uint32 destSize) const {
    return (IsMoveWidableImmediate(static_cast<uint64>(value), static_cast<uint32>(destSize)) ||
            IsMoveWidableImmediate(~static_cast<uint64>(value), static_cast<uint32>(destSize)) ||
            IsBitmaskImmediate(destSize));
  }

  int64 GetValue() const {
    return value;
  }

  void SetValue(int64 val) {
    value = val;
  }

  void SetVary(VaryType flag) {
    isVary = flag;
  }

  bool IsZero() const {
    return value == 0;
  }

  VaryType GetVary() const {
    return isVary;
  }

  bool IsOne() const {
    return value == 1;
  }

  bool IsSignedValue() const {
    return isSigned;
  }

  void SetSigned() {
    isSigned = true;
  }

  void SetSigned(bool flag) {
    isSigned = flag;
  }

  bool IsInBitSizeRot(uint8 size) const {
    return IsInBitSizeRot(size, value);
  }

  static bool IsInBitSizeRot(uint8 size, int64 val) {
    /* to tell if the val is in a rotate window of size */
#if __GNU_C__ || __clang__
    if (val == 0) {
      return true;
    }
    int32 start = __builtin_ctzll(static_cast<uint64>(val));
    int32 end = static_cast<int32>(sizeof(val) * kBitsPerByte - __builtin_clzll(static_cast<uint64>(val)) - 1);
    return (size >= end - start + 1);
#else
    uint8 start = 0;
    uint8 end = 0;
    bool isFound = false;
    CHECK_FATAL(val > 0, "do not perform bit operator operations on signed integers");
    for (uint32 i = 0; i < k64BitSize; ++i) {
      /* check whether the ith bit of val is 1 or not */
      if (((static_cast<uint64>(val) >> i) & 0x1) == 0x1) {
        if (!isFound) {
          start = i;
          end = i;
          isFound = true;
        } else {
          end = i;
        }
      }
    }
    return !isFound || (size >= (end - start) + 1);
#endif
  }

  static bool IsInValueRange(int32 lowVal, int32 highVal, int32 val) {
    return val >= lowVal && val <= highVal;
  }

  bool IsNegative() const {
    return isSigned && value < 0;
  }

  void Add(int64 delta) {
    value += delta;
  }

  void Negate() {
    value = -value;
  }

  void BitwiseNegate() {
    value = ~(static_cast<uint64>(value)) & ((1ULL << size) - 1UL);
  }

  void DivideByPow2(int32 shift) {
    value = (static_cast<uint64>(value)) >> shift;
  }

  void ModuloByPow2(int32 shift) {
    value = (static_cast<uint64>(value)) & ((1ULL << shift) - 1UL);
  }

  bool IsAllOnes() const {
    return value == -1;
  }

  bool IsAllOnes32bit() const {
    return value == 0x0ffffffffLL;
  }

  bool operator<(const ImmOperand &iOpnd) const {
    return value < iOpnd.value || (value == iOpnd.value && isSigned < iOpnd.isSigned) ||
           (value == iOpnd.value && isSigned == iOpnd.isSigned && size < iOpnd.GetSize());
  }

  bool operator==(const ImmOperand &iOpnd) const {
    return (value == iOpnd.value && isSigned == iOpnd.isSigned && size == iOpnd.GetSize());
  }

  void Emit(Emitter &emitter, const OpndProp *prop) const override {
    CHECK_FATAL(false, "do not run here");
  }

  void Dump() const override;

  bool Less(const Operand &right) const override {
    if (&right == this) {
      return false;
    }

    /* For different type. */
    if (opndKind != right.GetKind()) {
      return opndKind < right.GetKind();
    }

    auto *rightOpnd = static_cast<const ImmOperand*>(&right);

    /* The same type. */
    if (isSigned != rightOpnd->isSigned) {
      return isSigned;
    }

    if (isVary != rightOpnd->isVary) {
      return isVary;
    }

    return value < rightOpnd->value;
  }

  bool Equals(Operand &operand) const override {
    if (!operand.IsImmediate()) {
      return false;
    }
    auto &op = static_cast<ImmOperand&>(operand);
    if (&op == this) {
      return true;
    }
    return (BasicEquals(op) && value == op.GetValue() && isSigned == op.IsSignedValue());
  }

  bool ValueEquals(const ImmOperand &op) const {
    if (&op == this) {
      return true;
    }
    return (value == op.GetValue() && isSigned == op.IsSignedValue());
  }
  bool IsFmov() const {
    return isFmov;
  }

 protected:
  int64 value;
  bool isSigned;
  VaryType isVary;
  bool isFmov = false;
};

class OfstOperand : public ImmOperand {
 public:
  enum OfstType : uint8 {
    kSymbolOffset,
    kImmediateOffset,
    kSymbolImmediateOffset,
  };

  /* only for symbol offset */
  OfstOperand(const MIRSymbol &mirSymbol, uint32 size, int32 relocs)
      : ImmOperand(kOpdOffset, 0, size, true, kNotVary, false),
        offsetType(kSymbolOffset), symbol(&mirSymbol), relocs(relocs) {}
  /* only for Immediate offset */
  OfstOperand(int64 val, uint32 size, VaryType isVar = kNotVary)
      : ImmOperand(kOpdOffset, static_cast<int64>(val), size, true, isVar, false),
        offsetType(kImmediateOffset), symbol(nullptr), relocs(0) {}
  /* for symbol and Immediate offset */
  OfstOperand(const MIRSymbol &mirSymbol, int64 val, uint32 size, int32 relocs, VaryType isVar = kNotVary)
      : ImmOperand(kOpdOffset, val, size, true, isVar, false),
        offsetType(kSymbolImmediateOffset),
        symbol(&mirSymbol),
        relocs(relocs) {}

  ~OfstOperand() override {
    symbol = nullptr;
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<OfstOperand>(*this);
  }

  bool IsSymOffset() const {
    return offsetType == kSymbolOffset;
  }
  bool IsImmOffset() const {
    return offsetType == kImmediateOffset;
  }
  bool IsSymAndImmOffset() const {
    return offsetType == kSymbolImmediateOffset;
  }

  const MIRSymbol *GetSymbol() const {
    return symbol;
  }

  const std::string &GetSymbolName() const {
    return symbol->GetName();
  }

  int64 GetOffsetValue() const {
    return GetValue();
  }

  void SetOffsetValue(int32 offVal) {
    SetValue(static_cast<int64>(offVal));
  }

  void AdjustOffset(int32 delta) {
    Add(static_cast<int64>(delta));
  }

  bool operator==(const OfstOperand &opnd) const {
    return (offsetType == opnd.offsetType && symbol == opnd.symbol &&
            ImmOperand::operator==(opnd) && relocs == opnd.relocs);
  }

  bool operator<(const OfstOperand &opnd) const {
    return (offsetType < opnd.offsetType ||
            (offsetType == opnd.offsetType && symbol < opnd.symbol) ||
            (offsetType == opnd.offsetType && symbol == opnd.symbol && GetValue() < opnd.GetValue()));
  }

  void Emit(Emitter &emitter, const OpndProp *prop) const override {
    CHECK_FATAL(false, "dont run here");
  };

  void Dump() const override {
    if (IsImmOffset()) {
      LogInfo::MapleLogger() << "ofst:" << GetValue();
    } else {
      LogInfo::MapleLogger() << GetSymbolName();
      LogInfo::MapleLogger() << "+offset:" << GetValue();
    }
  }

 private:
  OfstType offsetType;
  const MIRSymbol *symbol;
  int32 relocs;
};

/*
 * Table C1-6 A64 Load/Store addressing modes
 * |         Offset
 * Addressing Mode    | Immediate     | Register             | Extended Register
 *
 * Base register only | [base{,#0}]   | -                    | -
 * (no offset)        | B_OI_NONE     |                      |
 *                   imm=0
 *
 * Base plus offset   | [base{,#imm}] | [base,Xm{,LSL #imm}] | [base,Wm,(S|U)XTW {#imm}]
 *                  B_OI_NONE     | B_OR_X               | B_OR_X
 *                                   imm=0,1 (0,3)        | imm=00,01,10,11 (0/2,s/u)
 *
 * Pre-indexed        | [base, #imm]! | -                    | -
 *
 * Post-indexed       | [base], #imm  | [base], Xm(a)        | -
 *
 * Literal            | label         | -                    | -
 * (PC-relative)
 *
 * a) The post-indexed by register offset mode can be used with the SIMD Load/Store
 * structure instructions described in Load/Store Vector on page C3-154. Otherwise
 * the post-indexed by register offset mode is not available.
 */
class MemOperand : public OperandVisitable<MemOperand> {
 public:
  enum AArch64AddressingMode : uint8 {
    kAddrModeUndef,
    /* AddrMode_BO, base, offset. EA = [base] + offset; */
    kAddrModeBOi,  /* INTACT: EA = [base]+immediate */
    /*
     * PRE: base += immediate, EA = [base]
     * POST: EA = [base], base += immediate
     */
    kAddrModeBOrX,  /* EA = [base]+Extend([offreg/idxreg]), OR=Wn/Xn */
    kAddrModeLiteral,  /* AArch64 insruction LDR takes literal and */
    /*
     * "calculates an address from the PC value and an immediate offset,
     * loads a word from memory, and writes it to a register."
     */
    kAddrModeLo12Li  // EA = [base] + #:lo12:Label+immediate. (Example: [x0, #:lo12:__Label300+456]
  };
  /*
   * ARMv8-A A64 ISA Overview by Matteo Franchin @ ARM
   * (presented at 64-bit Android on ARM. Sep. 2015) p.14
   * o Address to load from/store to is a 64-bit base register + an optional offset
   *   LDR X0, [X1] ; Load from address held in X1
   *   STR X0, [X1] ; Store to address held in X1
   *
   * o Offset can be an immediate or a register
   *   LDR X0, [X1, #8]  ; Load from address [X1 + 8 bytes]
   *   LDR X0, [X1, #-8] ; Load with negative offset
   *   LDR X0, [X1, X2]  ; Load from address [X1 + X2]
   *
   * o A Wn register offset needs to be extended to 64 bits
   *  LDR X0, [X1, W2, SXTW] ; Sign-extend offset in W2
   *   LDR X0, [X1, W2, UXTW] ; Zero-extend offset in W2
   *
   * o Both Xn and Wn register offsets can include an optional left-shift
   *   LDR X0, [X1, W2, UXTW #2] ; Zero-extend offset in W2 & left-shift by 2
   *   LDR X0, [X1, X2, LSL #2]  ; Left-shift offset in X2 by 2
   *
   * p.15
   * Addressing Modes                       Analogous C Code
   *                                       int *intptr = ... // X1
   *                                       int out; // W0
   * o Simple: X1 is not changed
   *   LDR W0, [X1]                        out = *intptr;
   * o Offset: X1 is not changed
   *   LDR W0, [X1, #4]                    out = intptr[1];
   * o Pre-indexed: X1 changed before load
   *   LDR W0, [X1, #4]! =|ADD X1,X1,#4    out = *(++intptr);
   * |LDR W0,[X1]
   * o Post-indexed: X1 changed after load
   *   LDR W0, [X1], #4  =|LDR W0,[X1]     out = *(intptr++);
   * |ADD X1,X1,#4
   */
  enum ExtendInfo : uint8 {
    kShiftZero = 0x1,
    kShiftOne = 0x2,
    kShiftTwo = 0x4,
    kShiftThree = 0x8,
    kUnsignedExtend = 0x10,
    kSignExtend = 0x20
  };

  enum IndexingOption : uint8 {
    kIntact,     /* base register stays the same */
    kPreIndex,   /* base register gets changed before load */
    kPostIndex,  /* base register gets changed after load */
  };

  MemOperand(uint32 size, const MIRSymbol &mirSymbol) :
      OperandVisitable(Operand::kOpdMem, size), symbol(&mirSymbol) {}

  MemOperand(uint32 size, RegOperand *baseOp, RegOperand *indexOp, ImmOperand *ofstOp, const MIRSymbol *mirSymbol,
             Operand *scaleOp = nullptr)
      : OperandVisitable(Operand::kOpdMem, size),
        baseOpnd(baseOp),
        indexOpnd(indexOp),
        offsetOpnd(ofstOp),
        scaleOpnd(scaleOp),
        symbol(mirSymbol) {}

  MemOperand(RegOperand *base, OfstOperand *offset, uint32 size, IndexingOption idxOpt = kIntact)
      : OperandVisitable(Operand::kOpdMem, size),
        baseOpnd(base),
        indexOpnd(nullptr),
        offsetOpnd(offset),
        symbol(nullptr),
        addrMode(kAddrModeBOi),
        extend(0),
        idxOpt(idxOpt),
        noExtend(false),
        isStackMem(false) {}

  MemOperand(AArch64AddressingMode mode, uint32 size, RegOperand &base, RegOperand *index,
             ImmOperand *offset, const MIRSymbol *sym)
      : OperandVisitable(Operand::kOpdMem, size),
        baseOpnd(&base),
        indexOpnd(index),
        offsetOpnd(offset),
        symbol(sym),
        addrMode(mode),
        extend(0),
        idxOpt(kIntact),
        noExtend(false),
        isStackMem(false) {}

  MemOperand(AArch64AddressingMode mode, uint32 size, RegOperand &base, RegOperand &index,
             ImmOperand *offset, const MIRSymbol &sym, bool noExtend)
      : OperandVisitable(Operand::kOpdMem, size),
        baseOpnd(&base),
        indexOpnd(&index),
        offsetOpnd(offset),
        symbol(&sym),
        addrMode(mode),
        extend(0),
        idxOpt(kIntact),
        noExtend(noExtend),
        isStackMem(false) {}

  MemOperand(AArch64AddressingMode mode, uint32 dSize, RegOperand &baseOpnd, RegOperand &indexOpnd,
             uint32 shift, bool isSigned = false)
      : OperandVisitable(Operand::kOpdMem, dSize),
        baseOpnd(&baseOpnd),
        indexOpnd(&indexOpnd),
        offsetOpnd(nullptr),
        symbol(nullptr),
        addrMode(mode),
        extend((isSigned ? kSignExtend : kUnsignedExtend) | (1U << shift)),
        idxOpt(kIntact),
        noExtend(false),
        isStackMem(false) {}

  MemOperand(AArch64AddressingMode mode, uint32 dSize, const MIRSymbol &sym)
      : OperandVisitable(Operand::kOpdMem, dSize),
        baseOpnd(nullptr),
        indexOpnd(nullptr),
        offsetOpnd(nullptr),
        symbol(&sym),
        addrMode(mode),
        extend(0),
        idxOpt(kIntact),
        noExtend(false),
        isStackMem(false) {
    ASSERT(mode == kAddrModeLiteral, "This constructor version is supposed to be used with AddrMode_Literal only");
  }

  /* Copy constructor */
  explicit MemOperand(const MemOperand &memOpnd)
      : OperandVisitable(Operand::kOpdMem, memOpnd.GetSize()),
        baseOpnd(memOpnd.baseOpnd),
        indexOpnd(memOpnd.indexOpnd),
        offsetOpnd(memOpnd.offsetOpnd),
        scaleOpnd(memOpnd.scaleOpnd),
        symbol(memOpnd.symbol),
        memoryOrder(memOpnd.memoryOrder),
        addrMode(memOpnd.addrMode),
        extend(memOpnd.extend),
        idxOpt(memOpnd.idxOpt),
        noExtend(memOpnd.noExtend),
        isStackMem(memOpnd.isStackMem),
        isStackArgMem(memOpnd.isStackArgMem){}

  MemOperand &operator=(const MemOperand &memOpnd) = default;

  ~MemOperand() override = default;
  using OperandVisitable<MemOperand>::OperandVisitable;

  MemOperand *Clone(MemPool &memPool) const override {
    return memPool.Clone<MemOperand>(*this);
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {};
  void Dump() const override {};

  RegOperand *GetBaseRegister() const {
    return baseOpnd;
  }

  void SetBaseRegister(RegOperand &regOpnd) {
    baseOpnd = &regOpnd;
  }

  RegOperand *GetIndexRegister() const {
    return indexOpnd;
  }

  void SetIndexRegister(RegOperand &regOpnd) {
    indexOpnd = &regOpnd;
  }

  ImmOperand *GetOffsetOperand() const {
    return offsetOpnd;
  }

  void SetOffsetOperand(ImmOperand &oftOpnd) {
    offsetOpnd = &oftOpnd;
  }

  const Operand *GetScaleOperand() const {
    return scaleOpnd;
  }

  const MIRSymbol *GetSymbol() const {
    return symbol;
  }

  void SetMemoryOrdering(uint32 memOrder) {
    memoryOrder |= memOrder;
  }

  bool HasMemoryOrdering(uint32 memOrder) const {
    return (memoryOrder & memOrder) != 0;
  }

  void SetAccessSize(uint8 size) {
    accessSize = size;
  }

  uint8 GetAccessSize() const {
    return accessSize;
  }

  AArch64AddressingMode GetAddrMode() const {
    return addrMode;
  }

  const std::string &GetSymbolName() const {
    return GetSymbol()->GetName();
  }

  bool IsStackMem() const {
    return isStackMem;
  }

  void SetStackMem(bool isStack) {
    isStackMem = isStack;
  }

  bool IsStackArgMem() const {
    return isStackArgMem;
  }

  void SetStackArgMem(bool isStackArg) {
    isStackArgMem = isStackArg;
  }

  Operand *GetOffset() const;

  OfstOperand *GetOffsetImmediate() const {
    return static_cast<OfstOperand*>(GetOffsetOperand());
  }

  /* Returns N where alignment == 2^N */
  static int32 GetImmediateOffsetAlignment(uint32 dSize) {
    ASSERT(dSize >= k8BitSize, "error val:dSize");
    ASSERT(dSize <= k128BitSize, "error val:dSize");
    ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
    /* dSize==8: 0, dSize==16 : 1, dSize==32: 2, dSize==64: 3 */
    return __builtin_ctz(dSize) - static_cast<int32>(kBaseOffsetAlignment);
  }

  static int32 GetMaxPIMM(uint32 dSize) {
    dSize = dSize > k64BitSize ? k64BitSize : dSize;
    ASSERT(dSize >= k8BitSize, "error val:dSize");
    ASSERT(dSize <= k128BitSize, "error val:dSize");
    ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
    int32 alignment = GetImmediateOffsetAlignment(dSize);
    /* alignment is between kAlignmentOf8Bit and kAlignmentOf64Bit */
    ASSERT(alignment >= kOffsetAlignmentOf8Bit, "error val:alignment");
    ASSERT(alignment <= kOffsetAlignmentOf128Bit, "error val:alignment");
    return (kMaxPimm[alignment]);
  }

  static int32 GetMaxPairPIMM(uint32 dSize) {
    ASSERT(dSize >= k32BitSize, "error val:dSize");
    ASSERT(dSize <= k128BitSize, "error val:dSize");
    ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
    int32 alignment = GetImmediateOffsetAlignment(dSize);
    /* alignment is between kAlignmentOf8Bit and kAlignmentOf64Bit */
    ASSERT(alignment >= kOffsetAlignmentOf32Bit, "error val:alignment");
    ASSERT(alignment <= kOffsetAlignmentOf128Bit, "error val:alignment");
    return (kMaxPairPimm[static_cast<uint32>(alignment) - k2BitSize]);
  }

  bool IsOffsetMisaligned(uint32 dSize) const {
    ASSERT(dSize >= k8BitSize, "error val:dSize");
    ASSERT(dSize <= k128BitSize, "error val:dSize");
    ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
    if (dSize == k8BitSize || addrMode != kAddrModeBOi) {
      return false;
    }
    OfstOperand *ofstOpnd = GetOffsetImmediate();
    if (ofstOpnd->GetOffsetValue() >= -256 && ofstOpnd->GetOffsetValue() <= 255) {
      return false;
    }
    return ((static_cast<uint32>(ofstOpnd->GetOffsetValue()) &
             static_cast<uint32>((1U << static_cast<uint32>(GetImmediateOffsetAlignment(dSize))) - 1)) != 0);
  }

  static bool IsSIMMOffsetOutOfRange(int64 offset, bool is64bit, bool isLDSTPair) {
    if (!isLDSTPair) {
      return (offset < kMinSimm32 || offset > kMaxSimm32);
    }
    if (is64bit) {
      return (offset < kMinSimm64 || offset > kMaxSimm64Pair) || (static_cast<uint64>(offset) & k7BitSize) ;
    }
    return (offset < kMinSimm32 || offset > kMaxSimm32Pair) || (static_cast<uint64>(offset) & k3BitSize);
  }

  static bool IsPIMMOffsetOutOfRange(int32 offset, uint32 dSize) {
    ASSERT(dSize >= k8BitSize, "error val:dSize");
    ASSERT(dSize <= k128BitSize, "error val:dSize");
    ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
    return (offset < 0 || offset > GetMaxPIMM(dSize));
  }

  bool operator<(const MemOperand &opnd) const {
    return addrMode < opnd.addrMode ||
           (addrMode == opnd.addrMode && GetBaseRegister() < opnd.GetBaseRegister()) ||
           (addrMode == opnd.addrMode && GetBaseRegister() == opnd.GetBaseRegister() &&
            GetIndexRegister() < opnd.GetIndexRegister()) ||
           (addrMode == opnd.addrMode && GetBaseRegister() == opnd.GetBaseRegister() &&
            GetIndexRegister() == opnd.GetIndexRegister() && GetOffsetOperand() < opnd.GetOffsetOperand()) ||
           (addrMode == opnd.addrMode && GetBaseRegister() == opnd.GetBaseRegister() &&
            GetIndexRegister() == opnd.GetIndexRegister() && GetOffsetOperand() == opnd.GetOffsetOperand() &&
            GetSymbol() < opnd.GetSymbol()) ||
           (addrMode == opnd.addrMode && GetBaseRegister() == opnd.GetBaseRegister() &&
            GetIndexRegister() == opnd.GetIndexRegister() && GetOffsetOperand() == opnd.GetOffsetOperand() &&
            GetSymbol() == opnd.GetSymbol() && GetSize() < opnd.GetSize()) ||
           (addrMode == opnd.addrMode && GetBaseRegister() == opnd.GetBaseRegister() &&
            GetIndexRegister() == opnd.GetIndexRegister() && GetOffsetOperand() == opnd.GetOffsetOperand() &&
            GetSymbol() == opnd.GetSymbol() && GetSize() == opnd.GetSize() && extend < opnd.extend);
  }

  bool operator==(const MemOperand &opnd) const {
    return  (GetSize() == opnd.GetSize()) && (addrMode == opnd.addrMode) && (extend == opnd.extend) &&
            (GetBaseRegister() == opnd.GetBaseRegister()) &&
            (GetIndexRegister() == opnd.GetIndexRegister()) &&
            (GetSymbol() == opnd.GetSymbol()) &&
            (GetOffsetOperand() == opnd.GetOffsetOperand()) ;
  }

  VaryType GetMemVaryType() const {
    Operand *ofstOpnd = GetOffsetOperand();
    if (ofstOpnd != nullptr) {
      auto *opnd = static_cast<OfstOperand*>(ofstOpnd);
      return opnd->GetVary();
    }
    return kNotVary;
  }

  void SetAddrMode(AArch64AddressingMode val) {
    addrMode = val;
  }

  bool IsExtendedRegisterMode() const {
    return addrMode == kAddrModeBOrX;
  }

  void UpdateExtend(ExtendInfo flag) {
    extend = flag | (1U << ShiftAmount());
  }

  bool SignedExtend() const {
    return IsExtendedRegisterMode() && ((extend & kSignExtend) != 0);
  }

  bool UnsignedExtend() const {
    return IsExtendedRegisterMode() && !SignedExtend();
  }

  uint32 ShiftAmount() const {
    uint32 scale = extend & 0xF;
    /* 8 is 1 << 3, 4 is 1 << 2, 2 is 1 << 1, 1 is 1 << 0; */
    return (scale == 8) ? 3 : ((scale == 4) ? 2 : ((scale == 2) ? 1 : 0));
  }

  bool ShouldEmitExtend() const {
    return !noExtend && ((extend & 0x3F) != 0);
  }

  IndexingOption GetIndexOpt() const {
    return idxOpt;
  }

  void SetIndexOpt(IndexingOption newidxOpt) {
    idxOpt = newidxOpt;
  }

  bool GetNoExtend() const {
    return noExtend;
  }

  void SetNoExtend(bool val) {
    noExtend = val;
  }

  uint32 GetExtend() const {
    return extend;
  }

  void SetExtend(uint32 val) {
    extend = val;
  }

  bool IsIntactIndexed() const {
    return idxOpt == kIntact;
  }

  bool IsPostIndexed() const {
    return idxOpt == kPostIndex;
  }

  bool IsPreIndexed() const {
    return idxOpt == kPreIndex;
  }

  std::string GetExtendAsString() const {
    if (GetIndexRegister()->GetSize() == k64BitSize) {
      return std::string("LSL");
    }
    return ((extend & kSignExtend) != 0) ? std::string("SXTW") : std::string("UXTW");
  }

  /* Return true if given operand has the same base reg and offset with this. */
  bool Equals(Operand &op) const override;
  bool Equals(const MemOperand &op) const;
  bool Less(const Operand &right) const override;

 private:
  RegOperand *baseOpnd = nullptr;    /* base register */
  RegOperand *indexOpnd = nullptr;   /* index register */
  ImmOperand *offsetOpnd = nullptr; /* offset immediate */
  Operand *scaleOpnd = nullptr;
  const MIRSymbol *symbol; /* AddrMode_Literal */
  uint32 memoryOrder = 0;
  uint8 accessSize = 0; /* temp, must be set right before use everytime. */
  AArch64AddressingMode addrMode = kAddrModeBOi;
  uint32 extend = false;        /* used with offset register ; AddrMode_B_OR_X */
  IndexingOption idxOpt = kIntact;  /* used with offset immediate ; AddrMode_B_OI */
  bool noExtend = false;
  bool isStackMem = false;
  bool isStackArgMem = false;
};

class LabelOperand : public OperandVisitable<LabelOperand> {
 public:
  LabelOperand(const char *parent, LabelIdx labIdx)
      : OperandVisitable(kOpdBBAddress, 0), labelIndex(labIdx), parentFunc(parent), orderID(-1u) {}

  ~LabelOperand() override = default;
  using OperandVisitable<LabelOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<LabelOperand>(*this);
  }

  bool IsLabelOpnd() const override {
    return true;
  }

  LabelIdx GetLabelIndex() const {
    return labelIndex;
  }

  const std::string GetParentFunc() const {
    return parentFunc;
  }

  LabelIDOrder GetLabelOrder() const {
    return orderID;
  }

  void SetLabelOrder(LabelIDOrder idx) {
    orderID = idx;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    CHECK_FATAL(false, "do not run here");
  }

  void Dump() const override;

  bool Less(const Operand &right) const override {
    if (&right == this) {
      return false;
    }

    /* For different type. */
    if (opndKind != right.GetKind()) {
      return opndKind < right.GetKind();
    }

    auto *rightOpnd = static_cast<const LabelOperand*>(&right);

    int32 nRes = strcmp(parentFunc, rightOpnd->parentFunc);
    if (nRes == 0) {
      return labelIndex < rightOpnd->labelIndex;
    } else {
      return nRes < 0;
    }
  }

  bool Equals(Operand &operand) const override {
    if (!operand.IsLabel()) {
      return false;
    }
    auto &op = static_cast<LabelOperand&>(operand);
    return ((&op == this) || (op.GetLabelIndex() == labelIndex));
  }

 protected:
  LabelIdx labelIndex;
  const char *parentFunc;

 private:
  /* this index records the order this label is defined during code emit. */
  LabelIDOrder orderID = -1u;
};

class ListOperand : public OperandVisitable<ListOperand> {
 public:
  explicit ListOperand(MapleAllocator &allocator) :
      OperandVisitable(Operand::kOpdList, 0),
      opndList(allocator.Adapter()) {}

  ~ListOperand() override = default;

  using OperandVisitable<ListOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<ListOperand>(*this);
  }

  void PushOpnd(RegOperand &opnd) {
    opndList.push_back(&opnd);
  }

  MapleList<RegOperand*> &GetOperands() {
    return opndList;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {
    CHECK_FATAL(false, "do not run here");
  }

  void Dump() const override {
    for (auto it = opndList.begin(); it != opndList.end();) {
      (*it)->Dump();
      LogInfo::MapleLogger() << (++it == opndList.end() ? "" : " ,");
    }
  }

  bool Less(const Operand &right) const override {
    /* For different type. */
    if (opndKind != right.GetKind()) {
      return opndKind < right.GetKind();
    }

    ASSERT(false, "We don't need to compare list operand.");
    return false;
  }

  bool Equals(Operand &operand) const override {
    if (!operand.IsList()) {
      return false;
    }
    auto &op = static_cast<ListOperand&>(operand);
    return (&op == this);
  }

 protected:
  MapleList<RegOperand*> opndList;
};

/* for cg ssa analysis */
class PhiOperand : public OperandVisitable<PhiOperand> {
 public:
  explicit PhiOperand(MapleAllocator &allocator)
      : OperandVisitable(Operand::kOpdPhi, 0),
        phiList(allocator.Adapter()) {}

  ~PhiOperand() override = default;
  using OperandVisitable<PhiOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<PhiOperand>(*this);
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override  {
    CHECK_FATAL(false, "a CPU support phi?");
  }

  void Dump() const override {
    CHECK_FATAL(false, "NIY");
  }

  void InsertOpnd(uint32 bbId, RegOperand &phiParam) {
    ASSERT(!phiList.count(bbId), "cannot insert duplicate operand");
    (void)phiList.emplace(std::pair(bbId, &phiParam));
  }

  void UpdateOpnd(uint32 bbId, uint32 newId, RegOperand &phiParam) {
    (void)phiList.emplace(std::pair(newId, &phiParam));
    phiList.erase(bbId);
  }

  MapleMap<uint32, RegOperand*> &GetOperands() {
    return phiList;
  }

  uint32 GetLeastCommonValidBit() {
    uint32 leastCommonVb = 0;
    for (auto phiOpnd : phiList) {
      uint32 curVb = phiOpnd.second->GetValidBitsNum();
      if (curVb > leastCommonVb) {
        leastCommonVb = curVb;
      }
    }
    return leastCommonVb;
  }

  bool IsRedundancy() {
    uint32 srcSsaIdx = 0;
    for (auto phiOpnd : phiList) {
      if (srcSsaIdx == 0) {
        srcSsaIdx = phiOpnd.second->GetRegisterNumber();
      }
      if (srcSsaIdx != phiOpnd.second->GetRegisterNumber()) {
        return false;
      }
    }
    return true;
  }

  bool Less(const Operand &right) const override {
    /* For different type. */
    if (opndKind != right.GetKind()) {
      return opndKind < right.GetKind();
    }
    ASSERT(false, "We don't need to compare list operand.");
    return false;
  }

  bool Equals(Operand &operand) const override {
    if (!operand.IsPhi()) {
      return false;
    }
    auto &op = static_cast<PhiOperand&>(operand);
    return (&op == this);
  }

 protected:
  MapleMap<uint32, RegOperand*> phiList; /* ssa-operand && BBId */
};

class CGRegOperand : public OperandVisitable<CGRegOperand> {
 public:
  CGRegOperand(regno_t regId, uint32 sz, RegType type) : OperandVisitable(kOpdRegister, sz),
      regNO(regId),
      regType(type) {}
  ~CGRegOperand() override = default;
  using OperandVisitable<CGRegOperand>::OperandVisitable;

  regno_t GetRegisterNumber() const {
    return regNO;
  }
  bool IsOfIntClass() const {
    return regType == kRegTyInt;
  }

  bool IsOfFloatOrSIMDClass() const {
    return regType == kRegTyFloat;
  }

  bool IsOfCC() const {
    return regType == kRegTyCc;
  }

  bool IsOfVary() const {
    return regType == kRegTyVary;
  }

  RegType GetRegisterType() const {
    return regType;
  }

  void SetRegisterType(RegType newTy) {
    regType = newTy;
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.New<CGRegOperand>(*this);
  }
  bool Less(const Operand &right) const override {
    return GetKind() < right.GetKind();
  }
  /* delete soon */
  void Emit(Emitter&, const OpndProp*) const override {}

  void Dump() const override {
    LogInfo::MapleLogger() << "reg ";
    LogInfo::MapleLogger() << "size : " << GetSize();
    LogInfo::MapleLogger() << " NO_" << GetRegisterNumber();
  }
 private:
  regno_t regNO;
  RegType regType;
};

class CGImmOperand : public OperandVisitable<CGImmOperand> {
 public:
  CGImmOperand(uint32 sz, int64 value) : OperandVisitable(kOpdImmediate, sz), val(value), symbol(nullptr), relocs(0) {}
  CGImmOperand(const MIRSymbol &symbol, int64 value, int32 relocs)
      : OperandVisitable(kOpdStImmediate, 0), val(value), symbol(&symbol), relocs(relocs) {}
  ~CGImmOperand() override {
    symbol = nullptr;
  }
  using OperandVisitable<CGImmOperand>::OperandVisitable;

  int64 GetValue() const {
    return val;
  }
  const MIRSymbol *GetSymbol() const {
    return symbol;
  }
  const std::string &GetName() const {
    return symbol->GetName();
  }
  int32 GetRelocs() const {
    return relocs;
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.New<CGImmOperand>(*this);
  }
  bool Less(const Operand &right) const override {
    return GetKind() < right.GetKind();
  }

  /* delete soon */
  void Emit(Emitter&, const OpndProp*) const override {}

  void Dump() const override {
    if (GetSymbol() != nullptr && GetSize() == 0) {
      /* for symbol form */
      LogInfo::MapleLogger() << "symbol : " << GetName();
    } else {
      LogInfo::MapleLogger() << "imm ";
      LogInfo::MapleLogger() << "size : " << GetSize();
      LogInfo::MapleLogger() << " value : " << GetValue();
    }
  }
 private:
  int64 val;
  const MIRSymbol *symbol; /* for Immediate in symbol form */
  int32 relocs;
};

class CGMemOperand : public OperandVisitable<CGMemOperand> {
 public:
  explicit CGMemOperand(uint32 sz) : OperandVisitable(kOpdMem, sz) {}
  ~CGMemOperand() override {
    baseReg = nullptr;
    indexReg = nullptr;
    baseOfst = nullptr;
    scaleFactor = nullptr;
  }
  using OperandVisitable<CGMemOperand>::OperandVisitable;

  void Dump() const override {
    LogInfo::MapleLogger() << "mem ";
    LogInfo::MapleLogger() << "size : " << GetSize();
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.New<CGMemOperand>(*this);
  }

  bool Less(const Operand &right) const override {
    return GetKind() < right.GetKind();
  }
  /* delete soon */
  void Emit(Emitter&, const OpndProp*) const override {}

  CGRegOperand *GetBaseRegister() const {
    return baseReg;
  }

  void SetBaseRegister(CGRegOperand &newReg) {
    baseReg = &newReg;
  }

  CGRegOperand *GetIndexRegister() const {
    return indexReg;
  }

  void SetIndexRegister(CGRegOperand &newIndex) {
    indexReg = &newIndex;
  }

  CGImmOperand *GetBaseOfst() const {
    return baseOfst;
  }

  void SetBaseOfst(CGImmOperand &newOfst) {
    baseOfst = &newOfst;
  }

  CGImmOperand *GetScaleFactor() const {
    return scaleFactor;
  }

  void SetScaleFactor(CGImmOperand &scale) {
    scaleFactor = &scale;
  }

 private:
  CGRegOperand *baseReg = nullptr;
  CGRegOperand *indexReg = nullptr;
  CGImmOperand *baseOfst = nullptr;
  CGImmOperand *scaleFactor = nullptr;
};

class CGListOperand : public OperandVisitable<CGListOperand> {
 public:
  explicit CGListOperand(MapleAllocator &allocator) :
      OperandVisitable(Operand::kOpdList, 0),
      opndList(allocator.Adapter()) {}

  ~CGListOperand() override = default;

  using OperandVisitable<CGListOperand>::OperandVisitable;

  void PushOpnd(CGRegOperand &opnd) {
    opndList.push_back(&opnd);
  }

  MapleList<CGRegOperand*> &GetOperands() {
    return opndList;
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.New<CGListOperand>(*this);
  }

  bool Less(const Operand &right) const override {
    return GetKind() < right.GetKind();
  }
  /* delete soon */
  void Emit(Emitter&, const OpndProp*) const override {}

  void Dump() const override {
    for (auto it = opndList.begin(); it != opndList.end();) {
      (*it)->Dump();
      LogInfo::MapleLogger() << (++it == opndList.end() ? "" : " ,");
    }
  }

 protected:
  MapleList<CGRegOperand*> opndList;
};

class CGFuncNameOperand : public OperandVisitable<CGFuncNameOperand> {
 public:
  explicit CGFuncNameOperand(const MIRSymbol &fsym) : OperandVisitable(kOpdBBAddress, 0),
      symbol(&fsym) {}

  ~CGFuncNameOperand() override {
    symbol = nullptr;
  }
  using OperandVisitable<CGFuncNameOperand>::OperandVisitable;

  const std::string &GetName() const {
    return symbol->GetName();
  }

  const MIRSymbol *GetFunctionSymbol() const {
    return symbol;
  }

  void SetFunctionSymbol(const MIRSymbol &fsym) {
    symbol = &fsym;
  }

  Operand *Clone(MemPool &memPool) const override {
    return memPool.New<CGFuncNameOperand>(*this);
  }

  bool Less(const Operand &right) const override {
    return GetKind() < right.GetKind();
  }

  /* delete soon */
  void Emit(Emitter&, const OpndProp*) const override {}

  void Dump() const override {
    LogInfo::MapleLogger() << GetName();
  }

 private:
  const MIRSymbol *symbol;
};

class CGLabelOperand : public OperandVisitable<CGLabelOperand> {
 public:
  CGLabelOperand(const char *parent, LabelIdx labIdx)
      : OperandVisitable(kOpdBBAddress, 0), labelIndex(labIdx), parentFunc(parent) {}

  ~CGLabelOperand() override = default;
  using OperandVisitable<CGLabelOperand>::OperandVisitable;

  Operand *Clone(MemPool &memPool) const override {
    return memPool.Clone<CGLabelOperand>(*this);
  }

  LabelIdx GetLabelIndex() const {
    return labelIndex;
  }

  const std::string &GetParentFunc() const {
    return parentFunc;
  }

  void Emit(Emitter &emitter, const OpndProp *opndProp) const override {}

  void Dump() const override {
    LogInfo::MapleLogger() << "label ";
    LogInfo::MapleLogger() << "name : " << GetParentFunc();
    LogInfo::MapleLogger() << " Idx: " << GetLabelIndex();
  }

  bool Less(const Operand &right) const override {
    if (&right == this) {
      return false;
    }

    /* For different type. */
    if (opndKind != right.GetKind()) {
      return opndKind < right.GetKind();
    }

    auto *rightOpnd = static_cast<const CGLabelOperand*>(&right);

    int32 nRes = strcmp(parentFunc.c_str(), rightOpnd->parentFunc.c_str());
    if (nRes == 0) {
      return labelIndex < rightOpnd->labelIndex;
    } else {
      return nRes < 0;
    }
  }

 protected:
  LabelIdx labelIndex;
  const std::string parentFunc;

};

namespace operand {
/* bit 0-7 for common */
enum CommOpndDescProp : maple::uint64 {
  kIsDef = 1ULL,
  kIsUse = (1ULL << 1),
  kIsVector = (1ULL << 2)

};

/* bit 8-15 for reg */
enum RegOpndDescProp : maple::uint64 {
  kInt = (1ULL << 8),
  kFloat = (1ULL << 9),
  kRegTyCc = (1ULL << 10),
  kRegTyVary = (1ULL << 11),
};

/* bit 16-23 for imm */
enum ImmOpndDescProp : maple::uint64 {

};

/* bit 24-31 for mem */
enum MemOpndDescProp : maple::uint64 {
  kMemLow12 = (1ULL << 24),
  kLiteralLow12 = kMemLow12,
  kIsLoadLiteral = (1ULL << 25)

};
}

class OpndDescription {
 public:
  OpndDescription(Operand::OperandType t, maple::uint64 p, maple::uint32 s) :
      opndType(t), property(p), size(s) {}
  virtual ~OpndDescription() = default;

  Operand::OperandType GetOperandType() const {
    return opndType;
  }

  maple::uint32 GetSize() const {
    return size;
  }

  bool IsImm() const {
    return opndType == Operand::kOpdImmediate;
  }

  bool IsRegister() const {
    return opndType == Operand::kOpdRegister;
  }

  bool IsMem() const {
    return opndType == Operand::kOpdMem;
  }

  bool IsRegDef() const {
    return opndType == Operand::kOpdRegister && (property & operand::kIsDef);
  }

  bool IsRegUse() const {
    return opndType == Operand::kOpdRegister && (property & operand::kIsUse);
  }

  bool IsDef() const {
    return (property & operand::kIsDef) != 0;
  }

  bool IsUse() const {
    return (property & operand::kIsUse) != 0;
  }

  bool IsMemLow12() const {
    return IsMem() && (property & operand::kMemLow12);
  }

  bool IsLiteralLow12() const {
    return opndType == Operand::kOpdStImmediate && (property & operand::kLiteralLow12);
  }

  bool IsLoadLiteral() const {
    return (property & operand::kIsLoadLiteral) != 0;
  }

#define DEFINE_MOP(op, ...) static const OpndDescription op;
#include "operand.def"
#undef DEFINE_MOP

 private:
  Operand::OperandType opndType;
  maple::uint64 property;
  maple::uint32 size;
};

class OpndDumpVisitor : public OperandVisitorBase,
                        public OperandVisitors<CGRegOperand,
                                               CGImmOperand,
                                               CGMemOperand,
                                               CGFuncNameOperand,
                                               CGListOperand,
                                               CGLabelOperand> {
 public:
  explicit OpndDumpVisitor(const OpndDescription &operandDesc) : opndDesc(&operandDesc) {}
  virtual ~OpndDumpVisitor() {
    opndDesc = nullptr;
  }

 protected:
  virtual void DumpOpndPrefix() {
    LogInfo::MapleLogger() << " (opnd:";
  }
  virtual void DumpOpndSuffix() {
    LogInfo::MapleLogger() << " )";
  }
  void DumpSize(const Operand &opnd) const {
    LogInfo::MapleLogger() << " [size:" << opnd.GetSize() << "]";
  }
  const OpndDescription *GetOpndDesc() const {
    return opndDesc;
  }

 private:
  const OpndDescription *opndDesc;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_OPERAND_H */
