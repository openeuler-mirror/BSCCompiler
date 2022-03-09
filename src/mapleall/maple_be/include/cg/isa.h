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
};
using regno_t = uint32_t;
#define ISABSTRACT (1ULL << kInsnIsAbstract)
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
  typedef bool (*cfPointer)(ParaType);
  bool CheckConstraint(cfPointer ccfunc, ParaType a) {
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

  const OpndDescription* GetOpndDes(size_t index) const {
    return opndMD[index];
  }
  bool IsSame(const InsnDescription &left,
      std::function<bool (const InsnDescription &left, const InsnDescription &right)> cmp) const;
  MOperator GetOpc() const {
    return opc;
  }
  bool IsPhysicalInsn() const {
    return !(properties & ISABSTRACT);
  }
  bool IsStore() const {
    return (properties & ISSTORE);
  }
  bool IsLoad() const {
    return (properties & ISLOAD);
  }
  bool IsMove() const {
    return (properties & ISMOVE);
  }
  bool IsBasicOp() const {
    return (properties & ISBASICOP);
  }
  const std::string &GetName() const {
    return name;
  }
  const std::string &GetFormat() const {
    return format;
  }
  uint32 GetAtomicNum() {
    return atomicNum;
  }
  static const InsnDescription &GetAbstractId(MOperator opc) {
    return abstractId[opc];
  }
  static const InsnDescription abstractId[abstract::kMopLast];
};

/* empty class; just for parameter passing */
class OpndProp {};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ISA_H */
