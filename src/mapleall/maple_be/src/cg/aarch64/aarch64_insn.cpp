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
#include "aarch64_insn.h"
#include <fstream>
#include "aarch64_cg.h"
#include "common_utils.h"
#include "insn.h"
#include "metadata_layout.h"


namespace maplebe {

void A64OpndEmitVisitor::EmitIntReg(const RegOperand &v, uint32 opndSz) {
  CHECK_FATAL(v.GetRegisterType() == kRegTyInt, "wrong Type");
  ASSERT((opndSz == k32BitSizeInt || opndSz == k64BitSizeInt), "illegal register size");
#ifdef USE_32BIT_REF
  bool r32 = (opndSz == k32BitSizeInt) || isRefField;
#else
  bool r32 = (opndSz == k32BitSizeInt);
#endif  /* USE_32BIT_REF */
  (void)emitter.Emit(AArch64CG::intRegNames[(r32 ? AArch64CG::kR32List : AArch64CG::kR64List)][v.GetRegisterNumber()]);
}

void A64OpndEmitVisitor::Visit(maplebe::RegOperand *v) {
  CHECK_NULL_FATAL(opndProp);
  CHECK_FATAL(opndProp->IsRegister(), "opnd is not register!");
  regno_t regNO = v->GetRegisterNumber();
  uint32 opndSize = opndProp->GetSize();
  switch (v->GetRegisterType()) {
    case kRegTyInt: {
      EmitIntReg(*v, opndSize);
      break;
    }
    case kRegTyFloat: {
      ASSERT((opndSize == k8BitSize || opndSize == k16BitSize || opndSize == k32BitSize ||
          opndSize == k64BitSize || opndSize == k128BitSize), "illegal register size");
      if (opndProp->IsVectorOperand() && v->GetVecLaneSize() != 0) {
        EmitVectorOperand(*v);
      } else {
        /* FP reg cannot be reffield. 8~0, 16~1, 32~2, 64~3. 8 is 1000b, has 3 zero. */
        int32 regSet = __builtin_ctz(static_cast<uint32>(opndSize)) - 3;
        (void)emitter.Emit(AArch64CG::intRegNames[static_cast<uint32>(regSet)][regNO]);
      }
      break;
    }
    default:
      ASSERT(false, "NYI");
      break;
  }
}

void A64OpndEmitVisitor::Visit(maplebe::ImmOperand *v) {
  if (v->IsOfstImmediate()) {
    Visit(static_cast<OfstOperand*>(v));
    return;
  }

  if (v->IsStImmediate()) {
    Visit(*v->GetSymbol(), v->GetValue());
    return;
  }

  int64 value = v->GetValue();
  if (!v->IsFmov()) {
    (void)emitter.Emit((opndProp != nullptr && opndProp->IsLoadLiteral()) ? "=" : "#").Emit(value);
    return;
  }
  if (v->GetKind() == Operand::kOpdFPImmediate) {
    CHECK_FATAL(value == 0, "NIY");
    (void)emitter.Emit("#0.0");
  }
  /*
   * compute float value
   * use top 4 bits expect MSB of value . then calculate its fourth power
   */
  int32 exp = static_cast<int32>((((static_cast<uint32>(value) & 0x70) >> 4) ^ 0x4) - 3);
  /* use the lower four bits of value in this expression */
  const float mantissa = 1.0 + (static_cast<float>(static_cast<uint64>(value) & 0xf) / 16.0);
  float result = static_cast<float>(std::pow(2, exp)) * mantissa;

  std::stringstream ss;
  ss << std::setprecision(10) << result;
  std::string res;
  ss >> res;
  size_t dot = res.find('.');
  if (dot == std::string::npos) {
    res += ".0";
    dot = res.find('.');
    CHECK_FATAL(dot != std::string::npos, "cannot find in string");
  }
  (void)res.erase(dot, 1);
  std::string integer(res, 0, 1);
  std::string fraction(res, 1);
  while (fraction.size() != 1 && fraction[fraction.size() - 1] == '0') {
    fraction.pop_back();
  }
  /* fetch the sign bit of this value */
  std::string sign = ((static_cast<uint64>(value) & 0x80) > 0) ? "-" : "";
  (void)emitter.Emit(sign + integer + "." + fraction + "e+").Emit(static_cast<int64>(dot) - 1);
}

void A64OpndEmitVisitor::Visit(maplebe::MemOperand *v) {
  auto a64v = static_cast<MemOperand*>(v);
  MemOperand::AArch64AddressingMode addressMode = a64v->GetAddrMode();
#if defined(DEBUG) && DEBUG
  const InsnDesc *md = &AArch64CG::kMd[emitter.GetCurrentMOP()];
  ASSERT(md->Is64Bit() || md->GetOperandSize() <= k32BitSize || md->GetOperandSize() == k128BitSize,
         "unexpected opnd size");
#endif
  switch (addressMode) {
    case MemOperand::kAddrModeUndef:
    case MemOperand::kScale: CHECK_FATAL(false, "undef mode in aarch64!");
    case MemOperand::kBOI: {
      (void)emitter.Emit("[");
      auto *baseReg = v->GetBaseRegister();
      EmitIntReg(*baseReg, k64BitSize);
      ImmOperand *offset = a64v->GetOffsetImmediate();
      if (offset != nullptr && !offset->IsZero()) {
        (void)emitter.Emit(",");
        Visit(offset);
      }
      (void)emitter.Emit("]");
      break;
    }
    case MemOperand::kBOR: {
      (void)emitter.Emit("[");
      auto *baseReg = v->GetBaseRegister();
      EmitIntReg(*baseReg, k64BitSize);
      (void)emitter.Emit(",");
      EmitIntReg(*a64v->GetIndexRegister(), a64v->GetIndexRegister()->GetSize());
      if (a64v->NeedFixIndex()) {
        (void)emitter.Emit(", UXTW");
      }
      (void)emitter.Emit("]");
      break;
    }
    case MemOperand::kBOE: {
      (void)emitter.Emit("[");
      auto *baseReg = v->GetBaseRegister();
      EmitIntReg(*baseReg, k64BitSize);
      (void)emitter.Emit(",");
      if (a64v->NeedFixIndex()) {
        EmitIntReg(*a64v->GetIndexRegister(), k32BitSize);
      } else {
        EmitIntReg(*a64v->GetIndexRegister(), a64v->GetIndexRegister()->GetSize());
      }
      (void)emitter.Emit(",");
      (void)emitter.Emit(a64v->GetExtendAsString());
      CHECK_FATAL(a64v->CheckAmount(), "check amount!");
      if (a64v->ShiftAmount() != 0) {
        (void)emitter.Emit(" #");
        (void)emitter.Emit(a64v->ShiftAmount());
      }
      (void)emitter.Emit("]");
      break;
    }
    case MemOperand::kBOL: {
      (void)emitter.Emit("[");
      auto *baseReg = v->GetBaseRegister();
      EmitIntReg(*baseReg, k64BitSize);
      (void)emitter.Emit(",");
      EmitIntReg(*a64v->GetIndexRegister(), a64v->GetIndexRegister()->GetSize());
      if (a64v->ShiftAmount() != 0) {
        CHECK_FATAL(a64v->GetExtendAsString() == "LSL", "must be lsl!");
        CHECK_FATAL(a64v->CheckAmount(), "check amount!");
        /* ImplicitCvt! eliminate in the future! */
        std::string shiftString = a64v->GetIndexRegister()->GetSize() == k32BitSize ? ",UXTW #" : ",LSL #";
        (void)emitter.Emit(shiftString);
        (void)emitter.Emit(a64v->ShiftAmount());
      }
      (void)emitter.Emit("]");
      break;
    }
    case MemOperand::kLo12Li: {
      (void)emitter.Emit("[");
      RegOperand *baseReg = v->GetBaseRegister();
      EmitIntReg(*baseReg, k64BitSize);
      CHECK_NULL_FATAL(v->GetSymbol());
      if ((CGOptions::IsPIC() && v->GetSymbol()->IsThreadLocal()) || 
          v->GetSymbol()->NeedGOT(CGOptions::IsPIC(), CGOptions::IsPIE())) {
        std::string gotEntry = "";
        if (v->GetSymbol()->IsThreadLocal()) {
          gotEntry = ", #:tlsdesc_lo12:";
        } else if (CGOptions::GetPICMode() == CGOptions::kLargeMode) {
          gotEntry = ", #:got_lo12:";
        } else if (CGOptions::GetPICMode() == CGOptions::kSmallMode) {
          if (CGOptions::IsArm64ilp32()) {
            gotEntry = ", #:gotpage_lo14:";
          } else {
            gotEntry = ", #:gotpage_lo15:";
          }
        }
        CHECK_FATAL(gotEntry != "", "A64OpndEmitVisitor::Visit(MemOperand): wrong entry for got.");
        std::string symbolName = v->GetSymbolName();
        symbolName += (v->GetSymbol()->GetStorageClass() == kScPstatic && v->GetSymbol()->GetSKind() != kStConst) ?
            std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()) : "";
        (void)emitter.Emit(gotEntry + symbolName);
        (void)emitter.Emit("]");
        break;
      }
      (void)emitter.Emit(", #:lo12:");
      if (v->GetSymbol()->GetAsmAttr() != UStrIdx(0) &&
          (v->GetSymbol()->GetStorageClass() == kScPstatic || v->GetSymbol()->GetStorageClass() == kScPstatic)) {
        std::string asmSection = GlobalTables::GetUStrTable().GetStringFromStrIdx(v->GetSymbol()->GetAsmAttr());
        (void)emitter.Emit(asmSection);
      } else {
        if (v->GetSymbol()->GetStorageClass() == kScPstatic && v->GetSymbol()->IsLocal()) {
          PUIdx pIdx = emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
          (void)emitter.Emit(a64v->GetSymbolName() + std::to_string(pIdx));
        } else {
          (void)emitter.Emit(a64v->GetSymbolName());
        }
      }
      OfstOperand *offset = a64v->GetOffsetImmediate();
      if (!offset->IsZero()) {
        (void)emitter.Emit("+");
        (void)emitter.Emit(std::to_string(offset->GetOffsetValue()));
      }
      (void)emitter.Emit("]");
      break;
    }
    case MemOperand::kPreIndex: {
      (void)emitter.Emit("[");
      auto *baseReg = v->GetBaseRegister();
      EmitIntReg(*baseReg, k64BitSize);
      ImmOperand *offset = a64v->GetOffsetImmediate();
      CHECK_NULL_FATAL(offset);
      if (!offset->IsZero()) {
        (void)emitter.Emit(",");
        Visit(offset);
      }
      (void)emitter.Emit("]!");
      break;
    }
    case MemOperand::kPostIndex: {
      (void)emitter.Emit("[");
      auto *baseReg = v->GetBaseRegister();
      EmitIntReg(*baseReg, k64BitSize);
      (void)emitter.Emit("]");
      ImmOperand *offset = a64v->GetOffsetImmediate();
      CHECK_NULL_FATAL(offset);
      if (!offset->IsZero()) {
        (void)emitter.Emit(", ");
        Visit(offset);
      }
      break;
    }
    case MemOperand::kLiteral: {
      CHECK_NULL_FATAL(opndProp);
      if (opndProp->IsMemLow12()) {
        (void)emitter.Emit("#:lo12:");
      }
      CHECK_NULL_FATAL(v->GetSymbol());
      PUIdx pIdx = emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
      (void)emitter.Emit(v->GetSymbol()->GetName() + std::to_string(pIdx));
    }
  }
}

void A64OpndEmitVisitor::Visit(LabelOperand *v) {
  emitter.EmitLabelRef(v->GetLabelIndex());
}

void A64OpndEmitVisitor::Visit(CondOperand *v) {
  (void)emitter.Emit(CondOperand::ccStrs[v->GetCode()]);
}

void A64OpndEmitVisitor::Visit(ExtendShiftOperand *v) {
  ASSERT(v->GetShiftAmount() <= k4BitSize && v->GetShiftAmount() >= 0,
         "shift amount out of range in ExtendShiftOperand");
  auto emitExtendShift = [this, v](const std::string &extendKind)->void {
    (void)emitter.Emit(extendKind);
    if (v->GetShiftAmount() != 0) {
      (void)emitter.Emit(" #").Emit(v->GetShiftAmount());
    }
  };
  switch (v->GetExtendOp()) {
    case ExtendShiftOperand::kUXTB:
      emitExtendShift("UXTB");
      break;
    case ExtendShiftOperand::kUXTH:
      emitExtendShift("UXTH");
      break;
    case ExtendShiftOperand::kUXTW:
      emitExtendShift("UXTW");
      break;
    case ExtendShiftOperand::kUXTX:
      emitExtendShift("UXTX");
      break;
    case ExtendShiftOperand::kSXTB:
      emitExtendShift("SXTB");
      break;
    case ExtendShiftOperand::kSXTH:
      emitExtendShift("SXTH");
      break;
    case ExtendShiftOperand::kSXTW:
      emitExtendShift("SXTW");
      break;
    case ExtendShiftOperand::kSXTX:
      emitExtendShift("SXTX");
      break;
    default:
      ASSERT(false, "should not be here");
      break;
  }
}

void A64OpndEmitVisitor::Visit(BitShiftOperand *v) {
  std::string shiftOp;
  switch (v->GetShiftOp()) {
    case BitShiftOperand::kShiftLSL:
      shiftOp = "LSL #";
      break;
    case BitShiftOperand::kShiftLSR:
      shiftOp = "LSR #";
      break;
    case BitShiftOperand::kShiftASR:
      shiftOp = "ASR #";
      break;
    case BitShiftOperand::kShiftROR:
      shiftOp = "ROR #";
      break;
    default:
      CHECK_FATAL(false, "check shiftOp");
  }
  (void)emitter.Emit(shiftOp).Emit(v->GetShiftAmount());
}

void A64OpndEmitVisitor::Visit(StImmOperand *v) {
  Visit(*v->GetSymbol(), v->GetOffset());
}

void A64OpndEmitVisitor::Visit(const MIRSymbol &symbol, int64 offset) {
  CHECK_FATAL(opndProp != nullptr, "opndProp is nullptr in  StImmOperand::Emit");
  const bool isThreadLocal = symbol.IsThreadLocal();
  const bool isLiteralLow12 = opndProp->IsLiteralLow12();
  const bool hasGotEntry = symbol.NeedGOT(CGOptions::IsPIC(), CGOptions::IsPIE());
  bool hasPrefix = false;
  if (isThreadLocal) {
    (void)emitter.Emit(":tlsdesc");
    hasPrefix = true;
  }
  if (!hasPrefix && hasGotEntry) {
    if (CGOptions::GetPICMode() == CGOptions::kLargeMode) {
      (void)emitter.Emit(":got");
      hasPrefix = true;
    } else if (CGOptions::GetPICMode() == CGOptions::kSmallMode) {
      (void)emitter.Emit("_GLOBAL_OFFSET_TABLE_");
      return;
    }
  }
  if (isLiteralLow12) {
    std::string lo12String = hasPrefix ? "_lo12" : ":lo12";
    (void)emitter.Emit(lo12String);
    hasPrefix = true;
  }
  if (hasPrefix) {
    (void)emitter.Emit(":");
  }
  if (symbol.GetAsmAttr() != UStrIdx(0) &&
      (symbol.GetStorageClass() == kScPstatic || symbol.GetStorageClass() == kScPstatic)) {
    std::string asmSection = GlobalTables::GetUStrTable().GetStringFromStrIdx(symbol.GetAsmAttr());
    (void)emitter.Emit(asmSection);
  } else {
    if (symbol.GetStorageClass() == kScPstatic && symbol.GetSKind() != kStConst) {
      (void)emitter.Emit(symbol.GetName() +
          std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()));
    } else {
      (void)emitter.Emit(symbol.GetName());
    }
  }
  if (!hasGotEntry && offset != 0) {
    (void)emitter.Emit("+" + std::to_string(offset));
  }
}

void A64OpndEmitVisitor::Visit(FuncNameOperand *v) {
  (void)emitter.Emit(v->GetName());
}

void A64OpndEmitVisitor::Visit(CommentOperand *v) {
  (void)emitter.Emit(v->GetComment());
}

void A64OpndEmitVisitor::Visit(ListOperand *v) {
  (void)opndProp;
  size_t nLeft = v->GetOperands().size();
  if (nLeft == 0) {
    return;
  }

  for (auto it = v->GetOperands().cbegin(); it != v->GetOperands().cend(); ++it) {
    Visit(*it);
    if (--nLeft >= 1) {
      (void)emitter.Emit(", ");
    }
  }
}

void A64OpndEmitVisitor::Visit(OfstOperand *v) {
  int64 value = v->GetValue();
  if (v->IsImmOffset()) {
    (void)emitter.Emit((opndProp != nullptr && opndProp->IsLoadLiteral()) ? "=" : "#")
        .Emit((v->GetSize() == k64BitSize) ? value : static_cast<int64>(static_cast<int32>(value)));
    return;
  }
  const MIRSymbol *symbol = v->GetSymbol();
  if (symbol->NeedGOT(CGOptions::IsPIC(), CGOptions::IsPIE())) {
    if (CGOptions::GetPICMode() == CGOptions::kLargeMode) {
      (void)emitter.Emit(":got:" + symbol->GetName());
    } else if (CGOptions::GetPICMode() == CGOptions::kSmallMode) {
      (void)emitter.Emit("_GLOBAL_OFFSET_TABLE_");
    }
  } else if (symbol->GetStorageClass() == kScPstatic && symbol->GetSKind() != kStConst && symbol->IsLocal()) {
    (void)emitter.Emit(symbol->GetName() +
        std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()));
  } else {
    (void)emitter.Emit(symbol->GetName());
  }
  if (value != 0) {
    (void)emitter.Emit("+" + std::to_string(value));
  }
}

void A64OpndEmitVisitor::EmitVectorOperand(const RegOperand &v) {
  std::string width;
  switch (v.GetVecElementSize()) {
    case k8BitSize:
      width = "b";
      break;
    case k16BitSize:
      width = "h";
      break;
    case k32BitSize:
      width = "s";
      break;
    case k64BitSize:
      width = "d";
      break;
    default:
      CHECK_FATAL(false, "unexpected value size for vector element");
      break;
  }
  (void)emitter.Emit(AArch64CG::vectorRegNames[v.GetRegisterNumber()]);
  int32 lanePos = v.GetVecLanePosition();
  if (lanePos == -1) {
    (void)emitter.Emit("." + std::to_string(v.GetVecLaneSize()) + width);
  } else {
    (void)emitter.Emit("." + width + "[" + std::to_string(lanePos) + "]");
  }
}

void A64OpndDumpVisitor::Visit(RegOperand *v) {
  std::array<const std::string, kRegTyLast> prims = { "U", "R", "V", "C", "X", "Vra" };
  std::array<const std::string, kRegTyLast> classes = { "[U]", "[I]", "[F]", "[CC]", "[X87]", "[Vra]" };
  uint32 regType = v->GetRegisterType();
  ASSERT(regType < kRegTyLast, "unexpected regType");

  regno_t reg = v->GetRegisterNumber();
  reg = v->IsVirtualRegister() ? reg : (reg - 1);
  LogInfo::MapleLogger() << (v->IsVirtualRegister() ? "vreg:" : " reg:") << prims[regType];
  if (reg + 1 == RSP && v->IsPhysicalRegister()) {
    LogInfo::MapleLogger() << "SP";
  } else if (reg + 1 == RZR && v->IsPhysicalRegister()) {
    LogInfo::MapleLogger() << "ZR";
  } else {
    LogInfo::MapleLogger() << reg;
  }
  LogInfo::MapleLogger() << " " << classes[regType];
  uint32 vb = v->GetValidBitsNum();
  if (vb != v->GetSize()) {
    LogInfo::MapleLogger() << " Vb: [" << vb << "]";
  }
  LogInfo::MapleLogger() << " Sz: [" << v->GetSize() << "]" ;
}

void A64OpndDumpVisitor::Visit(ImmOperand *v) {
  if (v->IsStImmediate()) {
    LogInfo::MapleLogger() << v->GetName();
    LogInfo::MapleLogger() << "+offset:" << v->GetValue();
  } else {
    LogInfo::MapleLogger() << "imm:" << v->GetValue();
    if (v->GetVary() == kNotVary) {
      LogInfo::MapleLogger() << " notVary";
    } else if (v->GetVary() == kUnAdjustVary) {
      LogInfo::MapleLogger() << " unVary";
    } else if (v->GetVary() == kAdjustVary) {
      LogInfo::MapleLogger() << " Varied";
    } else {
      CHECK_FATAL_FALSE("not expect");
    }
  }
}

void A64OpndDumpVisitor::Visit(MemOperand *a64v) {
  LogInfo::MapleLogger() << "Mem:";
  LogInfo::MapleLogger() << " size:" << a64v->GetSize() << " ";
  LogInfo::MapleLogger() << " isStack:" << a64v->IsStackMem() << "-" << a64v->IsStackArgMem() << " ";
  MemOperand::AArch64AddressingMode mode = a64v->GetAddrMode();
  switch (mode) {
    case MemOperand::kPreIndex:
    case MemOperand::kPostIndex:
    case MemOperand::kBOI: {
      LogInfo::MapleLogger() << "base:";
      ASSERT(a64v->GetBaseRegister(), " lack of base register");
      Visit(a64v->GetBaseRegister());
      LogInfo::MapleLogger() << "offset:";
      if (a64v->GetOffsetOperand()) {
        Visit(a64v->GetOffsetOperand());
      }
      if (mode == MemOperand::kPreIndex) {
        LogInfo::MapleLogger() << "  pre-index";
      } else if (mode == MemOperand::kPostIndex) {
        LogInfo::MapleLogger() << "  post-index";
      } else {
        LogInfo::MapleLogger() << "  intact";
      }
      break;
    }
    case MemOperand::kBOE:
    case MemOperand::kBOL:
    case MemOperand::kBOR: {
      LogInfo::MapleLogger() << "base:";
      Visit(a64v->GetBaseRegister());
      if (!a64v->GetIndexRegister()) {
        break;
      }
      LogInfo::MapleLogger() << "offset:";
      Visit(a64v->GetIndexRegister());
      if (mode == MemOperand::kBOE || mode == MemOperand::kBOL) {
        LogInfo::MapleLogger() << " " << a64v->GetExtendAsString();
        LogInfo::MapleLogger() << " shiftAmount: " << a64v->ShiftAmount();
      }
      break;
    }
    case MemOperand::kLiteral:
      LogInfo::MapleLogger() << "literal: " << a64v->GetSymbolName();
      break;
    case MemOperand::kLo12Li: {
      LogInfo::MapleLogger() << "base:";
      Visit(a64v->GetBaseRegister());
      LogInfo::MapleLogger() << "offset:";
      OfstOperand *offOpnd = a64v->GetOffsetImmediate();
      LogInfo::MapleLogger() << "#:lo12:";
      if (a64v->GetSymbol()->GetStorageClass() == kScPstatic && a64v->GetSymbol()->IsLocal()) {
        PUIdx pIdx = CG::GetCurCGFunc()->GetMirModule().CurFunction()->GetPuidx();
        LogInfo::MapleLogger() << a64v->GetSymbolName() << std::to_string(pIdx);
      } else {
        LogInfo::MapleLogger() << a64v->GetSymbolName();
      }
      LogInfo::MapleLogger() << "+" << std::to_string(offOpnd->GetOffsetValue());
      break;
    }
    default:
      ASSERT(false, "error memoperand dump");
      break;
  }
}

void A64OpndDumpVisitor::Visit(CondOperand *v) {
  LogInfo::MapleLogger() << "CC: " << CondOperand::ccStrs[v->GetCode()];
}
void A64OpndDumpVisitor::Visit(StImmOperand *v) {
  LogInfo::MapleLogger() << v->GetName();
  LogInfo::MapleLogger() << "+offset:" << v->GetOffset();
}
void A64OpndDumpVisitor::Visit(BitShiftOperand *v) {
  BitShiftOperand::ShiftOp shiftOp = v->GetShiftOp();
  uint32 shiftAmount = v->GetShiftAmount();
  LogInfo::MapleLogger() << ((shiftOp == BitShiftOperand::kShiftLSL) ? "LSL: " :
      ((shiftOp == BitShiftOperand::kShiftLSR) ? "LSR: " : "ASR: "));
  LogInfo::MapleLogger() << shiftAmount;
}
void A64OpndDumpVisitor::Visit(ExtendShiftOperand *v) {
  auto dumpExtendShift = [v](const std::string &extendKind)->void {
    LogInfo::MapleLogger() << extendKind;
    if (v->GetShiftAmount() != 0) {
      LogInfo::MapleLogger() << " : " << v->GetShiftAmount();
    }
  };
  switch (v->GetExtendOp()) {
    case ExtendShiftOperand::kUXTB:
      dumpExtendShift("UXTB");
      break;
    case ExtendShiftOperand::kUXTH:
      dumpExtendShift("UXTH");
      break;
    case ExtendShiftOperand::kUXTW:
      dumpExtendShift("UXTW");
      break;
    case ExtendShiftOperand::kUXTX:
      dumpExtendShift("UXTX");
      break;
    case ExtendShiftOperand::kSXTB:
      dumpExtendShift("SXTB");
      break;
    case ExtendShiftOperand::kSXTH:
      dumpExtendShift("SXTH");
      break;
    case ExtendShiftOperand::kSXTW:
      dumpExtendShift("SXTW");
      break;
    case ExtendShiftOperand::kSXTX:
      dumpExtendShift("SXTX");
      break;
    default:
      ASSERT(false, "should not be here");
      break;
  }
}
void A64OpndDumpVisitor::Visit(LabelOperand *v) {
  LogInfo::MapleLogger() << "label:" << v->GetLabelIndex();
}
void A64OpndDumpVisitor::Visit(FuncNameOperand *v) {
  LogInfo::MapleLogger() << "func :" << v->GetName();
}
void A64OpndDumpVisitor::Visit(CommentOperand *v) {
  LogInfo::MapleLogger() << " #" << v->GetComment();
}
void A64OpndDumpVisitor::Visit(PhiOperand *v) {
  auto &phiList = v->GetOperands();
  for (auto it = phiList.cbegin(); it != phiList.cend();) {
    Visit(it->second);
    LogInfo::MapleLogger() << " fBB<" << it->first << ">";
    LogInfo::MapleLogger() << (++it == phiList.end() ? "" : " ,");
  }
}
void A64OpndDumpVisitor::Visit(ListOperand *v) {
  auto &opndList = v->GetOperands();
  for (auto it = opndList.cbegin(); it != opndList.cend();) {
    Visit(*it);
    LogInfo::MapleLogger() << (++it == opndList.end() ? "" : " ,");
  }
}
}  /* namespace maplebe */
