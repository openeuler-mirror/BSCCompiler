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
#include "aarch64_cg.h"
#include "common_utils.h"
#include "insn.h"
#include "metadata_layout.h"
#include <fstream>

namespace maplebe {

void A64OpndEmitVisitor::EmitIntReg(const RegOperand &v, int32 opndSz) {
  CHECK_FATAL(v.GetRegisterType() == kRegTyInt, "wrong Type");
  int32 opndSize = (opndSz == kMaxSimm32) ? static_cast<int32>(v.GetSize()) : opndSz;
  ASSERT((opndSize == k32BitSizeInt || opndSize == k64BitSizeInt), "illegal register size");
#ifdef USE_32BIT_REF
  bool r32 = (opndSize == k32BitSizeInt) || isRefField;
#else
  bool r32 = (opndSize == k32BitSizeInt);
#endif  /* USE_32BIT_REF */
  (void)emitter.Emit(AArch64CG::intRegNames[(r32 ? AArch64CG::kR32List : AArch64CG::kR64List)][v.GetRegisterNumber()]);
}

void A64OpndEmitVisitor::Visit(maplebe::RegOperand *v) {
  ASSERT(opndProp == nullptr || opndProp->IsRegister(),
         "operand type doesn't match");
  uint32 size = v->GetSize();
  regno_t regNO = v->GetRegisterNumber();
  uint32 opndSize = (opndProp != nullptr) ? opndProp->GetSize() : size;
  switch (v->GetRegisterType()) {
    case kRegTyInt: {
      EmitIntReg(*v, static_cast<int32>(opndSize));
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
  bool isNegative = (value < 0);
  if (!v->IsFmov()) {
    value = (v->GetSize() == k64BitSize ? value : (isNegative ?
      static_cast<int64>(static_cast<int32>(value)) : static_cast<int64>(static_cast<uint32>(value))));
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
#if DEBUG
  const InsnDesc *md = &AArch64CG::kMd[emitter.GetCurrentMOP()];
  bool isLDSTpair = md->IsLoadStorePair();
  ASSERT(md->Is64Bit() || md->GetOperandSize() <= k32BitSize || md->GetOperandSize() == k128BitSize,
         "unexpected opnd size");
#endif
  if (addressMode == MemOperand::kAddrModeBOi) {
    (void)emitter.Emit("[");
    auto *baseReg = v->GetBaseRegister();
    ASSERT(baseReg != nullptr, "expect an RegOperand here");
    uint32 baseSize = baseReg->GetSize();
    if (baseSize != k64BitSize) {
      baseReg->SetSize(k64BitSize);
    }
    EmitIntReg(*baseReg);
    baseReg->SetSize(baseSize);
    OfstOperand *offset = a64v->GetOffsetImmediate();
    if (offset != nullptr) {
#ifndef USE_32BIT_REF  /* can be load a ref here */
      /*
       * Cortex-A57 Software Optimization Guide:
       * The ARMv8-A architecture allows many types of load and store accesses to be arbitrarily aligned.
       * The Cortex- A57 processor handles most unaligned accesses without performance penalties.
       */
#if DEBUG
      if (a64v->IsOffsetMisaligned(md->GetOperandSize())) {
        INFO(kLncInfo, "The Memory operand's offset is misaligned:", "");

      }
#endif
#endif  /* USE_32BIT_REF */
      if (a64v->IsPostIndexed()) {
        ASSERT(!a64v->IsSIMMOffsetOutOfRange(offset->GetOffsetValue(), md->Is64Bit(), isLDSTpair),
               "should not be SIMMOffsetOutOfRange");
        (void)emitter.Emit("]");
        if (!offset->IsZero()) {
          (void)emitter.Emit(", ");
          Visit(offset);
        }
      } else if (a64v->IsPreIndexed()) {
        ASSERT(!a64v->IsSIMMOffsetOutOfRange(offset->GetOffsetValue(), md->Is64Bit(), isLDSTpair),
               "should not be SIMMOffsetOutOfRange");
        if (!offset->IsZero()) {
          (void)emitter.Emit(",");
          Visit(offset);
        }
        (void)emitter.Emit("]!");
      } else {
        if (CGOptions::IsPIC() && (offset->IsSymOffset() || offset->IsSymAndImmOffset()) &&
            (offset->GetSymbol()->NeedPIC() || offset->GetSymbol()->IsThreadLocal())) {
          std::string gotEntry = offset->GetSymbol()->IsThreadLocal() ? ", #:tlsdesc_lo12:" : ", #:got_lo12:";
          std::string symbolName = offset->GetSymbolName();
          symbolName += offset->GetSymbol()->GetStorageClass() == kScPstatic && !offset->GetSymbol()->IsConst() ?
              std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()) : "";
          (void)emitter.Emit(gotEntry + symbolName);
        } else {
          if (!offset->IsZero()) {
            (void)emitter.Emit(",");
            Visit(offset);
          }
        }
        (void)emitter.Emit("]");
      }
    } else {
      (void)emitter.Emit("]");
    }
  } else if (addressMode == MemOperand::kAddrModeBOrX) {
    /*
     * Base plus offset   | [base{, #imm}]  [base, Xm{, LSL #imm}]   [base, Wm, (S|U)XTW {#imm}]
     *                      offset_opnds=nullptr
     *                                      offset_opnds=64          offset_opnds=32
     *                                      imm=0 or 3               imm=0 or 2, s/u
     */
    (void)emitter.Emit("[");
    auto *baseReg = v->GetBaseRegister();
    // After ssa version support different size, the value is changed back
    baseReg->SetSize(k64BitSize);

    EmitIntReg(*baseReg);
    (void)emitter.Emit(",");
    EmitIntReg(*a64v->GetIndexRegister());
    if (a64v->ShouldEmitExtend() || v->GetBaseRegister()->GetSize() > a64v->GetIndexRegister()->GetSize()) {
      (void)emitter.Emit(",");
      /* extend, #0, of #3/#2 */
      (void)emitter.Emit(a64v->GetExtendAsString());
      if (a64v->GetExtendAsString() == "LSL" || a64v->ShiftAmount() != 0) {
        (void)emitter.Emit(" #");
        (void)emitter.Emit(a64v->ShiftAmount());
      }
    }
    (void)emitter.Emit("]");
  } else if (addressMode == MemOperand::kAddrModeLiteral) {
    CHECK_FATAL(opndProp != nullptr, "prop is nullptr in  MemOperand::Emit");
    if (opndProp->IsMemLow12()) {
      (void)emitter.Emit("#:lo12:");
    }
    (void)emitter.Emit(v->GetSymbol()->GetName());
  } else if (addressMode == MemOperand::kAddrModeLo12Li) {
    (void)emitter.Emit("[");
    EmitIntReg(*v->GetBaseRegister());

    OfstOperand *offset = a64v->GetOffsetImmediate();
    ASSERT(offset != nullptr, "nullptr check");

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
    if (!offset->IsZero()) {
      (void)emitter.Emit("+");
      (void)emitter.Emit(std::to_string(offset->GetOffsetValue()));
    }
    (void)emitter.Emit("]");
  } else {
    ASSERT(false, "nyi");
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
    case BitShiftOperand::kLSL:
      shiftOp = "LSL #";
      break;
    case BitShiftOperand::kLSR:
      shiftOp = "LSR #";
      break;
    case BitShiftOperand::kASR:
      shiftOp = "ASR #";
      break;
    case BitShiftOperand::kROR:
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
  const bool hasGotEntry = CGOptions::IsPIC() && symbol.NeedPIC();
  bool hasPrefix = false;
  if (isThreadLocal) {
    (void)emitter.Emit(":tlsdesc");
    hasPrefix = true;
  }
  if (!hasPrefix && hasGotEntry) {
    (void)emitter.Emit(":got");
    hasPrefix = true;
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
  if (CGOptions::IsPIC() && symbol->NeedPIC()) {
    (void)emitter.Emit(":got:" + symbol->GetName());
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
  uint32 vb = v->GetValidBitsNum();
  LogInfo::MapleLogger() << (v->IsVirtualRegister() ? "vreg:" : " reg:") << prims[regType];
  if (reg + 1 == RSP && v->IsPhysicalRegister()) {
    LogInfo::MapleLogger() << "SP";
  } else if (reg + 1 == RZR && v->IsPhysicalRegister()) {
    LogInfo::MapleLogger() << "ZR";
  } else {
    LogInfo::MapleLogger() << reg ;
  }
  LogInfo::MapleLogger() << " " << classes[regType];
  if (v->GetValidBitsNum() != v->GetSize()) {
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
  }
}

void A64OpndDumpVisitor::Visit(MemOperand *a64v) {
  LogInfo::MapleLogger() << "Mem:";
  LogInfo::MapleLogger() << " size:" << a64v->GetSize() << " ";
  LogInfo::MapleLogger() << " isStack:" << a64v->IsStackMem() << "-" << a64v->IsStackArgMem() << " ";
  switch (a64v->GetAddrMode()) {
    case MemOperand::kAddrModeBOi: {
      LogInfo::MapleLogger() << "base:";
      Visit(a64v->GetBaseRegister());
      LogInfo::MapleLogger() << "offset:";
      Visit(a64v->GetOffsetOperand());
      switch (a64v->GetIndexOpt()) {
        case MemOperand::kIntact:
          LogInfo::MapleLogger() << "  intact";
          break;
        case MemOperand::kPreIndex:
          LogInfo::MapleLogger() << "  pre-index";
          break;
        case MemOperand::kPostIndex:
          LogInfo::MapleLogger() << "  post-index";
          break;
        default:
          break;
      }
      break;
    }
    case MemOperand::kAddrModeBOrX: {
      LogInfo::MapleLogger() << "base:";
      Visit(a64v->GetBaseRegister());
      LogInfo::MapleLogger() << "offset:";
      Visit(a64v->GetIndexRegister());
      LogInfo::MapleLogger() << " " << a64v->GetExtendAsString();
      LogInfo::MapleLogger() << " shift: " << a64v->ShiftAmount();
      LogInfo::MapleLogger() << " extend: " << a64v->GetExtendAsString();
      break;
    }
    case MemOperand::kAddrModeLiteral:
      LogInfo::MapleLogger() << "literal: " << a64v->GetSymbolName();
      break;
    case MemOperand::kAddrModeLo12Li: {
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
  LogInfo::MapleLogger() << ((shiftOp == BitShiftOperand::kLSL) ? "LSL: " :
      ((shiftOp == BitShiftOperand::kLSR) ? "LSR: " : "ASR: "));
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
