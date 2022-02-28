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
#ifndef MAPLEBE_INCLUDE_CG_ISA_H
#define MAPLEBE_INCLUDE_CG_ISA_H

#include <cstdint>
#include "types_def.h"
#include "cgoperand.h"

namespace maplebe {
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

enum MopProperty : maple::uint8 {
  kPropIsMove,
  kPropIsLoad,
  kPropIsLoadPair,
  kPropIsStore,
  kPropIsStorePair,
  kPropIsLoadAddress,
  kPropIsAtomic,
  kPropIsCall,
  kPropIsConversion,
  kPropIsConditionalSet,
  kPropUseSpecReg,
  kPropIsCondDef,
  kPropHasAcqure,
  kPropHasAcqureRCpc,
  kPropHasLOAcqure,
  kPropHasRelease,
  kPropHasLORelease,
  kPropCanThrow,
  kPropIsPartDefine,
  kPropIsDMB,
  kPropIsUnCondBr,
  kPropIsCondBr,
  kPropHasLoop,
  kPropIsVectorOp,
  kPropIsPhi,
};

#define ISMOVE (1ULL << kPropIsMove)
#define ISLOAD (1ULL << kPropIsLoad)
#define ISLOADPAIR (1ULL << kPropIsLoadPair)
#define ISSTORE (1ULL << kPropIsStore)
#define ISSTOREPAIR (1ULL << kPropIsStorePair)
#define ISLOADADDR (1ULL << kPropIsLoadAddress)
#define ISATOMIC (1ULL << kPropIsAtomic)
#define ISCALL (1ULL << kPropIsCall)
#define ISCONVERSION (1ULL << kPropIsConversion)
#define ISCONDSET (1ULL << kPropIsConditionalSet)
#define USESPECREG (1ULL << kPropUseSpecReg)
#define ISCONDDEF (1ULL << kPropIsCondDef)
#define HASACQUIRE (1ULL << kPropHasAcqure)
#define HASACQUIRERCPC (1ULL << kPropHasAcqureRCpc)
#define HASLOACQUIRE (1ULL << kPropHasLOAcqure)
#define HASRELEASE (1ULL << kPropHasRelease)
#define HASLORELEASE (1ULL << kPropHasLORelease)
#define CANTHROW (1ULL << kPropCanThrow)
#define ISPARTDEF (1ULL << kPropIsPartDefine)
#define ISDMB (1ULL << kPropIsDMB)
#define ISUNCONDBRANCH (1ULL << kPropIsUnCondBr)
#define ISCONDBRANCH (1ULL << kPropIsCondBr)
#define HASLOOP (1ULL << kPropHasLoop)
#define ISVECTOR (1ULL << kPropIsVectorOp)
#define ISPHI (1ULL << kPropIsPhi)

using regno_t = uint32_t;

constexpr regno_t kInvalidRegNO = 0;

/*
 * ARM64 has 32 int registes and 32 FP registers.
 * AMD64/X86_64 has 16 int registes, and 16 FP registers.
 * In either case, the corresponding calling conventions use
 * the smaller number of caller-saved registers.
 * 64 bit is not large enough?
 */
using CsrBitset = uint64_t;

// bit 0-7 for common
enum CommOpndDescProp : maple::uint64 {
  kOpndIsDef = (1ULL << 0),
  kOpndIsUse = (1ULL << 1),
  kOpndIsVector = (1ULL << 2)

};

// bit 8-15 for reg
enum RegOpndDescProp : maple::uint64 {
  kOpndPreInc = (1ULL << 8),
  kOpndPostInc = (1ULL << 9),

};

// bit 16-23 for imm
enum ImmOpndDescProp : maple::uint64 {

};

// bit 24-31 for mem
enum MemOpndDescProp : maple::uint64 {

  kMemOpndLow12 = (1ULL << 24),
  kOpndLiteralLow12 = kMemOpndLow12,
  kOpndIsLoadLiteral = (1ULL << 25)

};

class OpndDescription {
 public:
  OpndDescription(CGOperand::OperandType t, maple::uint64 p, maple::uint32 s) :
      opndType(t), property(p), size(s) {}
  OpndDescription(CGOperand::OperandType t, maple::uint64 p, maple::uint32 s,
      std::function<bool(int64)> f) : opndType(t), property(p), size(s), validFunc(f) {}
  virtual ~OpndDescription() = default;

  CGOperand::OperandType GetOperandType() const {
    return opndType;
  }

  maple::uint32 GetSize() const {
    return size;
  }

  maple::uint64 GetProperty() const {
    return property;
  }

  bool IsRegister() const {
    return opndType == CGOperand::kOpdRegister;
  }

  bool IsRegDef() const {
    return opndType == CGOperand::kOpdRegister && (property & kOpndIsDef);
  }

  bool IsRegUse() const {
    return opndType == CGOperand::kOpdRegister && (property & kOpndIsUse);
  }

  bool IsDef() const {
    return (property & kOpndIsDef);
  }

  bool IsUse() const {
    return (property & kOpndIsUse);
  }

 private:
  CGOperand::OperandType opndType;
  maple::uint64 property;
  maple::uint32 size;
  std::function<bool(int64)> validFunc;
};

template <typename ParaType>
class ConstraintFunction {
 public:
  typedef bool (*cfPointer)(ParaType);
  bool CheckConstraint(cfPointer ccfunc, ParaType a) {
    return (*ccfunc)(a);
  }
};

/* empty class; just for parameter passing */
class OpndProp {};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ISA_H */
