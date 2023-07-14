/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_INTRINSICS_H
#define MAPLE_IR_INCLUDE_INTRINSICS_H
#include <vector>
#include "prim_types.h"
#include "intrinsic_op.h"
#include "side_effect_info.h"
#include "utils.h"

namespace maple {
enum IntrinProperty : uint32 {
  kIntrnUndef,
  kIntrnIsJs,
  kIntrnIsJsUnary,
  kIntrnIsJsBinary,
  kIntrnIsJava,
  kIntrnIsCBuiltin,
  kIntrnIsJavaUnary,
  kIntrnIsJavaBinary,
  kIntrnIsReturnStruct,
  kIntrnNoSideEffect, // read only
  kIntrnIsLoadMem,
  kIntrnIsPure,
  kIntrnNeverReturn,
  kIntrnIsAtomic,
  kIntrnIsRC,
  kIntrnIsSpecial,
  kIntrnIsVector,
  kIntrnMembarrier,
  // the opnd is marked as "WRITE" but not "READ" => write only
  // the opnd is marked as "READ" but not "WRITE" => read only
  // the opnd is marked with nothing but has side effect => write & read
  // the opnd is marked as "WRITE" and "READ" => write & read
  kIntrnWriteFirstOpnd,
  kIntrnWriteSecondOpnd,
  kIntrnWriteThirdOpnd,
  kIntrnWriteFourthOpnd,
  kIntrnWriteFifthOpnd,
  kIntrnWriteSixthOpnd,
  kIntrnReadFirstOpnd,
  kIntrnReadSecondOpnd,
  kIntrnReadThirdOpnd,
  kIntrnReadFourthOpnd,
  kIntrnReadFifthOpnd,
  kIntrnReadSixthOpnd,
};

enum IntrinArgType : uint32 {
  kArgTyUndef,
  kArgTyVoid,
  kArgTyI8,
  kArgTyI16,
  kArgTyI32,
  kArgTyI64,
  kArgTyU8,
  kArgTyU16,
  kArgTyU32,
  kArgTyU64,
  kArgTyU1,
  kArgTyPtr,
  kArgTyRef,
  kArgTyA32,
  kArgTyA64,
  kArgTyF32,
  kArgTyF64,
  kArgTyF128,
  kArgTyC64,
  kArgTyC128,
  kArgTyAgg,
  kArgTyV2I64,
  kArgTyV4I32,
  kArgTyV8I16,
  kArgTyV16I8,
  kArgTyV2U64,
  kArgTyV4U32,
  kArgTyV8U16,
  kArgTyV16U8,
  kArgTyV2F64,
  kArgTyV4F32,
  kArgTyV1I64,
  kArgTyV2I32,
  kArgTyV4I16,
  kArgTyV8I8,
  kArgTyV1U64,
  kArgTyV2U32,
  kArgTyV4U16,
  kArgTyV8U8,
  kArgTyV1F64,
  kArgTyV2F32,
#ifdef DYNAMICLANG
  kArgTyDynany,
  kArgTyDynu32,
  kArgTyDyni32,
  kArgTyDynundef,
  kArgTyDynnull,
  kArgTyDynhole,
  kArgTyDynbool,
  kArgTyDynf64,
  kArgTyDynf32,
  kArgTySimplestr,
  kArgTyDynstr,
  kArgTySimpleobj,
  kArgTyDynobj
#endif
};

constexpr uint32 INTRNISJS = 1U << kIntrnIsJs;
constexpr uint32 INTRNISJSUNARY = 1U << kIntrnIsJsUnary;
constexpr uint32 INTRNISJSBINARY = 1U << kIntrnIsJsBinary;
constexpr uint32 INTRNISJAVA = 1U << kIntrnIsJava;
constexpr uint32 INTRNISCBUILTIN = 1U << kIntrnIsCBuiltin;
constexpr uint32 INTRNNOSIDEEFFECT = 1U << kIntrnNoSideEffect;
constexpr uint32 INTRNRETURNSTRUCT = 1U << kIntrnIsReturnStruct;
constexpr uint32 INTRNLOADMEM = 1U << kIntrnIsLoadMem;
constexpr uint32 INTRNISPURE = 1U << kIntrnIsPure;
constexpr uint32 INTRNNEVERRETURN = 1U << kIntrnNeverReturn;
constexpr uint32 INTRNATOMIC = 1U << kIntrnIsAtomic;
constexpr uint32 INTRNISRC = 1U << kIntrnIsRC;
constexpr uint32 INTRNISSPECIAL = 1U << kIntrnIsSpecial;
constexpr uint32 INTRNISVECTOR = 1U << kIntrnIsVector;
constexpr uint32 INTRNMEMBARRIER = 1U << kIntrnMembarrier;
constexpr uint32 INTRNWRITEFIRSTOPND = 1U << kIntrnWriteFirstOpnd;
constexpr uint32 INTRNWRITESECONDOPND = 1U << kIntrnWriteSecondOpnd;
constexpr uint32 INTRNWRITETHIRDOPND = 1U << kIntrnWriteThirdOpnd;
constexpr uint32 INTRNWRITEFOURTHOPND = 1U << kIntrnWriteFourthOpnd;
constexpr uint32 INTRNWRITEFIFTHOPND = 1U << kIntrnWriteFifthOpnd;
constexpr uint32 INTRNWRITESIXTHOPND = 1U << kIntrnWriteSixthOpnd;
constexpr uint32 INTRNREADFIRSTOPND = 1U << kIntrnReadFirstOpnd;
constexpr uint32 INTRNREADSECONDOPND = 1U << kIntrnReadSecondOpnd;
constexpr uint32 INTRNREADTHIRDOPND = 1U << kIntrnReadThirdOpnd;
constexpr uint32 INTRNREADFOURTHOPND = 1U << kIntrnReadFourthOpnd;
constexpr uint32 INTRNREADFIFTHOPND = 1U << kIntrnReadFifthOpnd;
constexpr uint32 INTRNREADSIXTHOPND = 1U << kIntrnReadSixthOpnd;
class MIRType;    // circular dependency exists, no other choice
class MIRModule;  // circular dependency exists, no other choice

struct IntrinArgInfo {
  IntrinArgInfo(IntrinArgType type) : argType(type) {}

  IntrinArgInfo(IntrinArgType type, MemEffect effect) : argType(type) {
    SetMemEffectAttr(effect);
  }

  IntrinArgInfo(IntrinArgType type, std::vector<MemEffect> effects) : argType(type) {
    for (auto effect : effects) {
      SetMemEffectAttr(effect);
    }
  }

  void SetMemEffectAttr(MemEffect effect) {
    argSideEffectAttr[static_cast<size_t>(effect)] = true;
  }

  bool GetMemEffectAttr(MemEffect effect) const {
    return argSideEffectAttr[static_cast<size_t>(effect)];
  }

  IntrinArgType argType;
  MemEffectAttr argSideEffectAttr = { 0 };
};

struct IntrinReturnInfo {
  IntrinReturnInfo(IntrinArgType type) : returnType(type) {}
  IntrinReturnInfo(IntrinArgType type, AliasLevelInfo info) : returnType(type), returnAliasInfo(info) {}
  IntrinArgType returnType;
  AliasLevelInfo returnAliasInfo = AliasLevelInfo::kNoAlias;
};

struct IntrinDesc {
  static constexpr int kMaxArgsNum = 7;
  const char *name;
  size_t numInsn;
  uint32 properties;
  IntrinReturnInfo returnInfo;
  std::vector<IntrinArgInfo> argInfo;
  bool IsJS() const {
    return static_cast<bool>(properties & INTRNISJS);
  }

  bool IsJava() const {
    return static_cast<bool>(properties & INTRNISJAVA);
  }

  bool IsCBuiltIn() const {
    return static_cast<bool>(properties & INTRNISCBUILTIN);
  }

  bool IsJsUnary() const {
    return static_cast<bool>(properties & INTRNISJSUNARY);
  }

  bool IsJsBinary() const {
    return static_cast<bool>(properties & INTRNISJSBINARY);
  }

  bool IsJsOp() const {
    return static_cast<bool>(properties & INTRNISJSUNARY) || static_cast<bool>(properties & INTRNISJSBINARY);
  }

  bool IsLoadMem() const {
    return static_cast<bool>(properties & INTRNLOADMEM);
  }

  bool IsJsReturnStruct() const {
    return static_cast<bool>(properties & INTRNRETURNSTRUCT);
  }

  bool IsPure() const {
    return static_cast<bool>(properties & INTRNISPURE);
  }

  bool IsNeverReturn() const {
    return static_cast<bool>(properties & INTRNNEVERRETURN);
  }

  bool IsAtomic() const {
    return static_cast<bool>(properties & INTRNATOMIC);
  }

  bool IsRC() const {
    return static_cast<bool>(properties & INTRNISRC);
  }

  bool IsSpecial() const {
    return static_cast<bool>(properties & INTRNISSPECIAL);
  }

  bool HasNoSideEffect() const {
    return static_cast<bool>(properties & INTRNNOSIDEEFFECT);
  }

  bool IsVectorOp() const {
    return static_cast<bool>(properties & INTRNISVECTOR);
  }

  bool IsMemoryBarrier() const {
    return static_cast<bool>(properties & INTRNMEMBARRIER);
  }

  size_t GetNumInsn() const {
    return numInsn;
  }

  bool ReadNthOpnd(uint32 opndIdx) const;
  bool WriteNthOpnd(uint32 opndIdx) const;
  bool ReturnNthOpnd(uint32 opndIdx) const;
  MIRType *GetReturnType() const;
  MIRType *GetArgType(uint32 index) const;
  MIRType *GetTypeFromArgTy(IntrinArgType argType) const;
  static MIRType *jsValueType;
  static MIRModule *mirModule;
  static void InitMIRModule(MIRModule *mod);
  static MIRType *GetOrCreateJSValueType();
  static IntrinDesc intrinTable[INTRN_LAST + 1];
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_INTRINSICS_H
