/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mir_const.h"
#include "mir_function.h"
#include "global_tables.h"
#include "printing.h"
#if MIR_FEATURE_FULL

namespace maple {
void MIRIntConst::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  LogInfo::MapleLogger() << value;
}

bool MIRIntConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &intConst = static_cast<const MIRIntConst&>(rhs);
  return ((&intConst.GetType() == &GetType()) && (intConst.value == value));
}

uint16 MIRIntConst::GetActualBitWidth() const {
  return value.CountSignificantBits();
}

void MIRAddrofConst::Dump(const MIRSymbolTable *localSymTab) const {
  LogInfo::MapleLogger() << "addrof " << GetPrimTypeName(PTY_ptr);
  const MIRSymbol *sym = stIdx.IsGlobal() ? GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx())
                                          : localSymTab->GetSymbolFromStIdx(stIdx.Idx());
  CHECK_NULL_FATAL(sym);
  ASSERT(stIdx.IsGlobal() || sym->GetStorageClass() == kScPstatic || sym->GetStorageClass() == kScFstatic,
         "MIRAddrofConst can only point to a global symbol");
  LogInfo::MapleLogger() << (stIdx.IsGlobal() ? " $" : " %") << sym->GetName();
  if (fldID > 0) {
    LogInfo::MapleLogger() << " " << fldID;
  }
  if (offset != 0) {
    LogInfo::MapleLogger() << " (" << offset << ")";
  }
}

bool MIRAddrofConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &rhsA = static_cast<const MIRAddrofConst&>(rhs);
  if (&GetType() != &rhs.GetType()) {
    return false;
  }
  return (stIdx == rhsA.stIdx) && (fldID == rhsA.fldID);
}

void MIRAddroffuncConst::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  LogInfo::MapleLogger() << "addroffunc " << GetPrimTypeName(PTY_ptr);
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  CHECK_NULL_FATAL(func);
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
  CHECK_NULL_FATAL(sym);
  LogInfo::MapleLogger() << " &" << sym->GetName();
}

bool MIRAddroffuncConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &rhsAf = static_cast<const MIRAddroffuncConst&>(rhs);
  return (&GetType() == &rhs.GetType()) && (puIdx == rhsAf.puIdx);
}

void MIRLblConst::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  LogInfo::MapleLogger() << "addroflabel " << GetPrimTypeName(PTY_ptr);
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  LogInfo::MapleLogger() << " @" << func->GetLabelName(value);
}

bool MIRLblConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &lblConst = static_cast<const MIRLblConst&>(rhs);
  return (lblConst.value == value);
}

bool MIRFloatConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &floatConst = static_cast<const MIRFloatConst&>(rhs);
  if (std::isnan(floatConst.value.floatValue)) {
    return std::isnan(value.floatValue);
  }
  if (std::isnan(value.floatValue)) {
    return std::isnan(floatConst.value.floatValue);
  }
  if (floatConst.value.floatValue == 0.0 && value.floatValue == 0.0) {
    return floatConst.IsNeg() == IsNeg();
  }
  // Use bitwise comparison instead of approximate comparison for FP to avoid treating 0.0 and FLT_MIN as equal
  return (floatConst.value.intValue == value.intValue);
}

bool MIRDoubleConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &floatConst = static_cast<const MIRDoubleConst&>(rhs);
  if (std::isnan(floatConst.value.dValue)) {
    return std::isnan(value.dValue);
  }
  if (std::isnan(value.dValue)) {
    return std::isnan(floatConst.value.dValue);
  }
  if (floatConst.value.dValue == 0.0 && value.dValue == 0.0) {
    return floatConst.IsNeg() == IsNeg();
  }
  // Use bitwise comparison instead of approximate comparison for FP to avoid treating 0.0 and DBL_MIN as equal
  return (floatConst.value.intValue == value.intValue);
}

bool MIRFloat128Const::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &floatConst = static_cast<const MIRFloat128Const&>(rhs);
  if ((val[0] == floatConst.val[0]) && (val[1] == floatConst.val[1])) {
    return true;
  }
  return false;
}

bool MIRAggConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &aggregateConst = static_cast<const MIRAggConst&>(rhs);
  if (aggregateConst.constVec.size() != constVec.size()) {
    return false;
  }
  for (size_t i = 0; i < constVec.size(); ++i) {
    if (!(*aggregateConst.constVec[i] == *constVec[i])) {
      return false;
    }
  }
  return true;
}

void MIRFloatConst::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  LogInfo::MapleLogger() << std::setprecision(std::numeric_limits<float>::max_digits10) << value.floatValue << "f";
}

void MIRDoubleConst::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  LogInfo::MapleLogger() << std::setprecision(std::numeric_limits<double>::max_digits10) << value.dValue;
}

void MIRFloat128Const::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  constexpr int fieldWidth = 16;
  std::ios::fmtflags f(LogInfo::MapleLogger().flags());
  LogInfo::MapleLogger().setf(std::ios::uppercase);
  LogInfo::MapleLogger() << "0xL" << std::hex << std::setfill('0') << std::setw(fieldWidth) << val[0]
                         << std::setfill('0') << std::setw(fieldWidth) << val[1];
  LogInfo::MapleLogger().flags(f);
}

void MIRAggConst::Dump(const MIRSymbolTable *localSymTab) const {
  LogInfo::MapleLogger() << "[";
  size_t size = constVec.size();
  for (size_t i = 0; i < size; ++i) {
    if (fieldIdVec[i] != 0) {
      LogInfo::MapleLogger() << fieldIdVec[i] << "= ";
    }
    constVec[i]->Dump(localSymTab);
    if (i != size - 1) {
      LogInfo::MapleLogger() << ", ";
    }
  }
  LogInfo::MapleLogger() << "]";
}

MIRStrConst::MIRStrConst(const std::string &str, MIRType &type)
    : MIRConst(type, kConstStrConst), value(GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str)) {}

void MIRStrConst::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  LogInfo::MapleLogger() << "conststr " << GetPrimTypeName(GetType().GetPrimType());
  const std::string &dumpStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(value);
  PrintString(dumpStr);
}

bool MIRStrConst::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &rhsCs = static_cast<const MIRStrConst&>(rhs);
  return (&rhs.GetType() == &GetType()) && (value == rhsCs.value);
}

MIRStr16Const::MIRStr16Const(const std::u16string &str, MIRType &type)
    : MIRConst(type, kConstStr16Const),
      value(GlobalTables::GetU16StrTable().GetOrCreateStrIdxFromName(str)) {}

void MIRStr16Const::Dump(const MIRSymbolTable *localSymTab [[maybe_unused]]) const {
  LogInfo::MapleLogger() << "conststr16 " << GetPrimTypeName(GetType().GetPrimType());
  std::u16string str16 = GlobalTables::GetU16StrTable().GetStringFromStrIdx(value);
  // UTF-16 string are dumped as UTF-8 string in mpl to keep the printable chars in ascii form
  std::string str;
  (void)namemangler::UTF16ToUTF8(str, str16);
  PrintString(str);
}

bool MIRStr16Const::operator==(const MIRConst &rhs) const {
  if (&rhs == this) {
    return true;
  }
  if (GetKind() != rhs.GetKind()) {
    return false;
  }
  const auto &rhsCs = static_cast<const MIRStr16Const&>(rhs);
  return (&GetType() == &rhs.GetType()) && (value == rhsCs.value);
}

bool IsDivSafe(const MIRIntConst &dividend, const MIRIntConst &divisor, PrimType pType) {
  if (IsUnsignedInteger(pType)) {
    return divisor.GetValue() != 0;
  }

  return divisor.GetValue() != 0 && (!dividend.GetValue().IsMinValue() || !divisor.GetValue().AreAllBitsOne());
}

}  // namespace maple
#endif  // MIR_FEATURE_FULL
