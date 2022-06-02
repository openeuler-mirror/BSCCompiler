/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_operand.h"
#include <fstream>
#include <string>
#include "aarch64_abi.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

namespace maplebe {

const char *CondOperand::ccStrs[kCcLast] = {
#define CONDCODE(a) #a,
#include "aarch64_cc.def"
#undef CONDCODE
};

bool StImmOperand::Less(const Operand &right) const{
  if (&right == this) {
    return false;
  }

  /* For different type. */
  if (GetKind() != right.GetKind()) {
    return GetKind() < right.GetKind();
  }

  const StImmOperand *rightOpnd = static_cast<const StImmOperand*>(&right);
  if (symbol != rightOpnd->symbol) {
    return symbol < rightOpnd->symbol;
  }
  if (offset != rightOpnd->offset) {
    return offset < rightOpnd->offset;
  }
  return relocs < rightOpnd->relocs;
}

void StImmOperand::Emit(Emitter &emitter, const OpndProp *opndProp) const {
  CHECK_FATAL(opndProp != nullptr, "opndProp is nullptr in  StImmOperand::Emit");
  bool isLiteralLow12 = opndProp->IsLiteralLow12();
  if (CGOptions::IsPIC() && symbol->NeedPIC()) {
    std::string gotEntry = symbol->IsThreadLocal() ? ":tlsdesc" : ":got";
    gotEntry += isLiteralLow12 ? "_lo12:" : ":";
    (void)emitter.Emit(gotEntry + GetName());
    return;
  }
  if (isLiteralLow12) {
    (void)emitter.Emit("#:lo12:");
  }
  if (symbol->GetStorageClass() == kScPstatic && symbol->GetSKind() != kStConst && symbol->IsLocal()) {
    (void)emitter.Emit(symbol->GetName() +
        std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()));
  } else {
    (void)emitter.Emit(GetName());
  }
  if (offset != 0) {
    (void)emitter.Emit("+" + std::to_string(offset));
  }
}

void ListConstraintOperand::Emit(Emitter &emitter, const OpndProp *opndProp) const {
  /* nothing emitted for inline asm constraints */
  (void)emitter;
  (void)opndProp;
}

bool CondOperand::Less(const Operand &right) const {
  if (&right == this) {
    return false;
  }

  /* For different type. */
  if (GetKind() != right.GetKind()) {
    return GetKind() < right.GetKind();
  }

  const CondOperand *rightOpnd = static_cast<const CondOperand*>(&right);

  /* The same type. */
  if (cc == CC_AL || rightOpnd->cc == CC_AL) {
    return false;
  }
  return cc < rightOpnd->cc;
}

bool ExtendShiftOperand::Less(const Operand &right) const {
  if (&right == this) {
    return false;
  }
  /* For different type. */
  if (GetKind() != right.GetKind()) {
    return GetKind() < right.GetKind();
  }

  const ExtendShiftOperand *rightOpnd = static_cast<const ExtendShiftOperand*>(&right);

  /* The same type. */
  if (extendOp != rightOpnd->extendOp) {
    return extendOp < rightOpnd->extendOp;
  }
  return shiftAmount < rightOpnd->shiftAmount;
}

void ExtendShiftOperand::Emit(Emitter &emitter, const OpndProp *prop) const {
  (void)prop;
  ASSERT(shiftAmount <= k4BitSize && shiftAmount >= 0,
         "shift amount out of range in ExtendShiftOperand");
  auto emitExtendShift = [&emitter, this](const std::string &extendKind)->void {
    (void)emitter.Emit(extendKind);
    if (shiftAmount != 0) {
      (void)emitter.Emit(" #").Emit(shiftAmount);
    }
  };
  switch (extendOp) {
    case kUXTB:
      emitExtendShift("UXTB");
      break;
    case kUXTH:
      emitExtendShift("UXTH");
      break;
    case kUXTW:
      emitExtendShift("UXTW");
      break;
    case kUXTX:
      emitExtendShift("UXTX");
      break;
    case kSXTB:
      emitExtendShift("SXTB");
      break;
    case kSXTH:
      emitExtendShift("SXTH");
      break;
    case kSXTW:
      emitExtendShift("SXTW");
      break;
    case kSXTX:
      emitExtendShift("SXTX");
      break;
    default:
      ASSERT(false, "should not be here");
      break;
  }
}

bool BitShiftOperand::Less(const Operand &right) const {
  if (&right == this) {
    return false;
  }

  /* For different type. */
  if (GetKind() != right.GetKind()) {
    return GetKind() < right.GetKind();
  }

  const BitShiftOperand *rightOpnd = static_cast<const BitShiftOperand*>(&right);

  /* The same type. */
  if (shiftOp != rightOpnd->shiftOp) {
    return shiftOp < rightOpnd->shiftOp;
  }
  return shiftAmount < rightOpnd->shiftAmount;
}
}  /* namespace maplebe */
