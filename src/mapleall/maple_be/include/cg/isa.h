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
#include "operand.h"

namespace maplebe {
enum MopProperty : maple::uint8 {
  kInsnIsAbstract,
  kInsnIsMove,
  kInsnIsLoad,
  kInsnIsLoadPair,
  kInsnIsStore,
  kInsnIsStorePair,
  kInsnIsLoadAddress,
  kInsnIsAtomic,
  kInsnIsCall,
  kInsnIsConversion,
  kInsnIsConditionalSet,
  kInsnUseSpecReg,
  kInsnIsCondDef,
  kInsnHasAcqure,
  kInsnHasAcqureRCpc,
  kInsnHasLOAcqure,
  kInsnHasRelease,
  kInsnHasLORelease,
  kInsnCanThrow,
  kInsnIsPartDefine,
  kInsnIsDMB,
  kInsnIsUnCondBr,
  kInsnIsCondBr,
  kInsnHasLoop,
  kInsnIsVectorOp,
  kInsnIsBinaryOp,
  kInsnIsPhi,
  kInsnIsUnaryOp,
  kInsnIsShift,
  kInsnIsNop,
};
using regno_t = uint32_t;
#define ISABSTRACT 1ULL
#define ISMOVE (1ULL << kInsnIsMove)
#define ISLOAD (1ULL << kInsnIsLoad)
#define ISLOADPAIR (1ULL << kInsnIsLoadPair)
#define ISSTORE (1ULL << kInsnIsStore)
#define ISSTOREPAIR (1ULL << kInsnIsStorePair)
#define ISLOADADDR (1ULL << kInsnIsLoadAddress)
#define ISATOMIC (1ULL << kInsnIsAtomic)
#define ISCALL (1ULL << kInsnIsCall)
#define ISCONVERSION (1ULL << kInsnIsConversion)
#define ISCONDSET (1ULL << kInsnIsConditionalSet)
#define USESPECREG (1ULL << kInsnUseSpecReg)
#define ISCONDDEF (1ULL << kInsnIsCondDef)
#define HASACQUIRE (1ULL << kInsnHasAcqure)
#define HASACQUIRERCPC (1ULL << kInsnHasAcqureRCpc)
#define HASLOACQUIRE (1ULL << kInsnHasLOAcqure)
#define HASRELEASE (1ULL << kInsnHasRelease)
#define HASLORELEASE (1ULL << kInsnHasLORelease)
#define CANTHROW (1ULL << kInsnCanThrow)
#define ISPARTDEF (1ULL << kInsnIsPartDefine)
#define ISDMB (1ULL << kInsnIsDMB)
#define ISUNCONDBRANCH (1ULL << kInsnIsUnCondBr)
#define ISCONDBRANCH (1ULL << kInsnIsCondBr)
#define HASLOOP (1ULL << kInsnHasLoop)
#define ISVECTOR (1ULL << kInsnIsVectorOp)
#define ISBASICOP (1ULL << kInsnIsBinaryOp)
#define ISPHI (1ULL << kInsnIsPhi)
#define ISUNARYOP (1ULL << kInsnIsUnaryOp)
#define ISSHIFT (1ULL << kInsnIsShift)
#define ISNOP (1ULL << kInsnIsNop)
constexpr maplebe::regno_t kInvalidRegNO = 0;

/*
 * ARM64 has 32 int registes and 32 FP registers.
 * AMD64/X86_64 has 16 int registes, and 16 FP registers.
 * In either case, the corresponding calling conventions use
 * the smaller number of caller-saved registers.
 * 64 bit is not large enough?
 */
using CsrBitset = uint64_t;

template <typename ParaType>
class ConstraintFunction {
 public:
  using CfPointer = bool (*) (ParaType);
  bool CheckConstraint(CfPointer ccfunc, ParaType a) const {
    return (*ccfunc)(a);
  }
};

/*
 * abstract machine instruction
 * a lower-level maple IR which is aimed to represent general machine instruction for extreme cpus
 * 1. Support conversion between all types and registers
 * 2. Support conversion between memory and registers
 * 3. Support three address basic operations
 *
 */
namespace abstract {
#define DEFINE_MOP(op, ...) op,
enum AbstractMOP_t : maple::uint32 {
#include "abstract_mmir.def"
  kMopLast
};
#undef DEFINE_MOP
}

struct InsnDescription {
  MOperator opc;
  std::vector<const OpndDescription*> opndMD;
  uint64 properties;
  uint32 latencyType;
  const std::string &name;
  const std::string &format;
  uint32 atomicNum; /* indicate how many asm instructions it will emit. */
  std::function<bool(int64)> validFunc = nullptr; /* If insn has immOperand, this function needs to be implemented. */

  bool IsSame(const InsnDescription &left,
      std::function<bool (const InsnDescription &left, const InsnDescription &right)> cmp) const;

  bool IsCall() const {
    return (properties & ISCALL) != 0;
  }
  bool IsPhi() const {
    return (properties & ISPHI) != 0;
  }
  bool IsPhysicalInsn() const {
    return (properties & ISABSTRACT) == 0;
  }
  bool IsStore() const {
    return (properties & ISSTORE) != 0;
  }
  bool IsLoad() const {
    return (properties & ISLOAD) != 0;
  }
  bool IsConversion() const {
    return (properties & ISCONVERSION) != 0;
  }
  bool IsLoadPair() const {
    return (properties & (ISLOADPAIR)) != 0;
  }
  bool IsStorePair() const {
    return (properties & (ISSTOREPAIR)) != 0;
  }
  bool IsLoadStorePair() const {
    return (properties & (ISLOADPAIR | ISSTOREPAIR)) != 0;
  }
  bool IsMove() const {
    return (properties & ISMOVE) != 0;
  }
  bool IsDMB() const {
    return (properties & (ISDMB)) != 0;
  }
  bool IsBasicOp() const {
    return (properties & ISBASICOP) != 0;
  }
  bool IsCondBranch() const {
    return (properties & (ISCONDBRANCH)) != 0;
  }
  bool IsUnCondBranch() const {
    return (properties & (ISUNCONDBRANCH)) != 0;
  }
  bool IsLoadAddress() const {
    return (properties & (ISLOADADDR)) != 0;
  }
  bool IsAtomic() const {
    return (properties & ISATOMIC) != 0;
  }
  bool IsCondDef() const {
    return (properties & ISCONDDEF) != 0;
  }
  bool IsPartDef() const {
    return (properties & ISPARTDEF) != 0;
  }
  bool IsVectorOp() const {
    return (properties & ISVECTOR) != 0;
  }
  bool IsVolatile() const {
    return ((properties & HASRELEASE) != 0) || ((properties & HASACQUIRE) != 0);
  }
  bool IsMemAccessBar() const {
    return (properties & (HASRELEASE | HASACQUIRE | HASACQUIRERCPC | HASLOACQUIRE | HASLORELEASE)) != 0;
  }
  bool IsMemAccess() const {
    return (properties & (ISLOAD | ISSTORE | ISLOADPAIR | ISSTOREPAIR)) != 0;
  }
  bool IsBranch() const {
    return (properties & (ISCONDBRANCH | ISUNCONDBRANCH)) != 0;
  }
  bool HasLoop() const {
    return (properties & HASLOOP) != 0;
  }
  bool CanThrow() const {
    return (properties & CANTHROW) != 0;
  }
  MOperator GetOpc() const {
    return opc;
  }
  const OpndDescription *GetOpndDes(size_t index) const {
    return opndMD[index];
  }
  uint32 GetLatencyType() const {
    return latencyType;
  }
  bool IsUnaryOp() const {
    return (properties & ISUNARYOP) != 0;
  }
  bool IsShift() const {
    return (properties & ISSHIFT) != 0;
  }
  const std::string &GetName() const {
    return name;
  }
  const std::string &GetFormat() const {
    return format;
  }
  uint32 GetAtomicNum() const {
    return atomicNum;
  }
  static const InsnDescription &GetAbstractId(MOperator mOperator) {
    ASSERT(mOperator < abstract::kMopLast, "op must be lower than kMopLast");
    return abstractId[mOperator];
  }
  static const InsnDescription abstractId[abstract::kMopLast];
};

enum RegPropState : uint32 {
  kRegPropUndef = 0,
  kRegPropDef = 0x1,
  kRegPropUse = 0x2
};
enum RegAddress : uint32 {
  kRegHigh = 0x4,
  kRegLow = 0x8
};
constexpr uint32 kMemLow12 = 0x10;
constexpr uint32 kLiteralLow12 = kMemLow12;
constexpr uint32 kPreInc = 0x20;
constexpr uint32 kPostInc = 0x40;
constexpr uint32 kLoadLiteral = 0x80;
constexpr uint32 kVector = 0x100;

class RegProp {
 public:
  RegProp(RegType t, regno_t r, uint32 d) : regType(t), physicalReg(r), defUse(d) {}
  virtual ~RegProp() = default;
  const RegType &GetRegType() const {
    return regType;
  }
  const regno_t &GetPhysicalReg() const {
    return physicalReg;
  }
  uint32 GetDefUse() const {
    return defUse;
  }

 private:
  RegType regType;
  regno_t physicalReg;
  uint32 defUse; /* used for register use/define and other properties of other operand */
};

class OpndProp {
 public:
  OpndProp(Operand::OperandType t, const RegProp &p, uint8 s) : opndType(t), regProp(p), size(s) {}
  virtual ~OpndProp() = default;
  Operand::OperandType GetOperandType() const {
    return opndType;
  }

  const RegProp &GetRegProp() const {
    return regProp;
  }

  bool IsRegister() const {
    return opndType == Operand::kOpdRegister;
  }

  bool IsRegDef() const {
    return opndType == Operand::kOpdRegister && (regProp.GetDefUse() & kRegPropDef);
  }

  bool IsRegUse() const {
    return opndType == Operand::kOpdRegister && (regProp.GetDefUse() & kRegPropUse);
  }

  bool IsMemLow12() const {
    return opndType == Operand::kOpdMem && (regProp.GetDefUse() & kMemLow12);
  }

  bool IsLiteralLow12() const {
    return opndType == Operand::kOpdStImmediate && (regProp.GetDefUse() & kLiteralLow12);
  }

  bool IsDef() const {
    return (regProp.GetDefUse() & kRegPropDef) != 0;
  }

  bool IsUse() const {
    return (regProp.GetDefUse() & kRegPropUse) != 0;
  }

  bool IsLoadLiteral() const {
    return (regProp.GetDefUse() & kLoadLiteral) != 0;
  }

  uint8 GetSize() const {
    return size;
  }

  uint32 GetOperandSize() const {
    return static_cast<uint32>(size);
  }

  bool IsVectorOperand() const {
    return (regProp.GetDefUse() & kVector) != 0;
  }

  void SetContainImm() {
    isContainImm = true;
  }

  bool IsContainImm() const {
    return isContainImm;
  }

 protected:
  bool isContainImm = false;

 private:
  Operand::OperandType opndType;
  RegProp regProp;
  uint8 size;
};

/*
 * Operand which might include immediate value.
 * function ptr returns whether a immediate is legal in specific target
 */
class ImmOpndProp : public OpndProp {
 public:
  ImmOpndProp(Operand::OperandType t, const RegProp &p, uint8 s, const std::function<bool(int64)> f)
      : OpndProp(t, p, s),
        validFunc(f) {
    SetContainImm();
  }
  virtual ~ImmOpndProp() = default;

  bool IsValidImmOpnd(int64 value) const {
    CHECK_FATAL(validFunc, " Have not set valid function yet in ImmOpndProp");
    return validFunc(value);
  }

 private:
  std::function<bool(int64)> validFunc;
};

} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ISA_H */
