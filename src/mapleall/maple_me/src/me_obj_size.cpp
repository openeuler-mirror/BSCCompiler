/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_obj_size.h"
#include "me_irmap_build.h"
#include "me_dominance.h"

namespace maple {
constexpr int64 kBitsPerByte = 8; // 8 bits per byte
constexpr size_t kNumOperands = 2;
constexpr uint64 kInvalidDestSize = static_cast<uint64>(-1);
constexpr char kMalloc[] = "malloc";
constexpr char kSprintfChk[] = "__sprintf_chk";
constexpr char kVsprintfChk[] = "__vsprintf_chk";

const std::map<std::string, std::string> kReplacedFuncPairs {
    {"__memcpy_chk", "memcpy"},
    {"__memset_chk", "memset"},
    {"__strncpy_chk", "strncpy"},
    {"__strcpy_chk", "strcpy"},
    {"__stpncpy_chk", "stpncpy"},
    {"__stpcpy_chk", "stpcpy"},
    {"__strcat_chk", "strcat"},
    {"__strncat_chk", "strncat"},
    {"__mempcpy_chk", "mempcpy"},
    {"__memmove_chk", "memmove"},
    {"__sprintf_chk", "sprintf"},
    {"__snprintf_chk", "snprintf"},
    {"__vsprintf_chk", "vsprintf"},
    {"__vsnprintf_chk", "vsnprintf"}
};

// Pair.first is the position of the src length information in the parameter list. If the position is -1,
// no such information. Pair.second is the position of the dest length information in the parameter list.
const std::map<std::string, std::pair<size_t, size_t>> kIndexOfSrcLength {
    // Function definition reference :
    // http://refspecs.linux-foundation.org/LSB_4.0.0/LSB-Core-generic/LSB-Core-generic/libcman.html
    //
    // void * memcpy (void * dest, const void * src, size_t len)
    // void * __memcpy_chk(void * dest, const void * src, size_t len, size_t destlen)
    // The parameter destlen specifies the size of the object dest. If len exceeds destlen, the function shall abort,
    // and the program calling it shall exit.
    {"__memcpy_chk", {2, 3}},
    // void * memset (void * dest, int c, size_t len)
    // void * __memset_chk(void * dest, int c, size_t len, size_t destlen)
    // The parameter destlen specifies the size of the object dest. If len exceeds destlen, the function shall abort,
    // and the program calling it shall exit.
    {"__memset_chk", {2, 3}},
    // char * strncpy (char * s1, const char * s2, size_t n)
    // char * __strncpy_chk(char * s1, const char * s2, size_t n, size_t s1len)
    // The parameter s1len specifies the size of the object pointed to by s1.
    {"__strncpy_chk", {2, 3}},
    // char* strcpy(char * dest, const char * src)
    // char * __strcpy_chk(char * dest, const char * src, size_t destlen)
    // The parameter destlen specifies the size of the object pointed to by dest.
    {"__strcpy_chk", {1, 2}},
    // char *stpcpy(char * dest, const char * src)
    // char * __stpcpy_chk(char * dest, const char * src, size_t destlen)
    // The parameter destlen specifies the size of the object pointed to by dest.
    {"__stpcpy_chk", {1, 2}},
    // int vsprintf(char *s, const char *format, va_list arg)
    // int __vsprintf_chk(char * s, int flag, size_t slen, const char * format, va_list args)
    // The parameter slen specifies the size of the object pointed to by s. If its value is zero,
    // the function shall abort and the program calling it shall exit.
    {"__vsprintf_chk", {-1, 2}},
    // int vsnprintf (char * s, size_t slen, const char * format, va_list arg)
    // int __vsnprintf_chk(char * s, size_t maxlen, int flag, size_t slen, const char * format, va_list args)
    // The parameter slen specifies the size of the object pointed to by s. If slen is less than maxlen,
    // the function shall abort and the program calling it shall exit.
    {"__vsnprintf_chk", {1, 3}},
    // int snprintf (char * str, size_t strlen, const char * format, ...)
    // int __snprintf_chk(char * str, size_t maxlen, int flag, size_t strlen, const char * format, ...)
    // The parameter strlen specifies the size of the buffer str. If strlen is less than maxlen,
    // the function shall abort, and the program calling it shall exit.
    {"__snprintf_chk", {1, 3}},
    // int sprintf(char *str, const char *format, ...)
    // int __sprintf_chk(char * str, int flag, size_t strlen, const char * format, ...)
    // The parameter strlen specifies the size of the string str. If strlen is zero,
    // the function shall abort, and the program calling it shall exit.
    {"__sprintf_chk", {-1, 2}},
    // char *strcat(char * dest, const char * src)
    // char * __strcat_chk(char * dest, const char * src, size_t destlen)
    // The parameter destlen specifies the size of the object pointed to by dest
    {"__strcat_chk", {1, 2}},
    // char * strncat (char * s1, const char * s2, size_t n)
    // char * __strncat_chk(char * s1, const char * s2, size_t n, size_t s1len)
    // The parameter s1len specifies the size of the object pointed to by s1.
    // N is maximum number of source to be appended.
    {"__strncat_chk", {2, 3}},
    // void *mempcpy(void * dest, const void * src, size_t len)
    // void * __mempcpy_chk(void * dest, const void * src, size_t len, size_t destlen)
    // The parameter destlen specifies the size of the object dest. If len exceeds destlen, the function shall abort,
    // and the program calling it shall exit.
    {"__mempcpy_chk", {2, 3}},
    // void * memmove(void * dest, const void * src, size_t len)
    // void * __memmove_chk(void * dest, const void * src, size_t len, size_t destlen)
    // The parameter destlen specifies the size of the object dest. If len exceeds destlen,
    // the function shall abort, and the program calling it shall exit.
    {"__memmove_chk", {2, 3}},
    // char *stpncpy(char * dest, const char * src, size_t n)
    // char * __stpncpy_chk(char * dest, const char * src, size_t n, size_t destlen)
    // The parameter destlen specifies the size of the object pointed to by dest. If n exceeds destlen,
    // the function shall abort, and the program calling it shall exit.
    {"__stpncpy_chk", {2, 3}}
};

void OBJSize::Execute() {
  auto cfg = func.GetCfg();
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (*bIt == nullptr) {
      continue;
    }
    for (auto &meStmt : (*bIt)->GetMeStmts()) {
      if (meStmt.GetOp() == OP_intrinsiccallassigned) {
        DealWithBuiltinObjectSize(**bIt, meStmt);
      } else {
        ComputeObjectSize(meStmt);
      }
    }
  }
}

void OBJSize::ERRWhenSizeTypeIsInvalid(const MeStmt &meStmt) const {
  auto srcPosition = meStmt.GetSrcPosition();
  bool inlined = (srcPosition.GetInlinedLineNum() != 0);
  auto fileNum = inlined ? srcPosition.GetInlinedFileNum() : srcPosition.FileNum();
  const std::string &currFile = func.GetMIRModule().GetFileNameFromFileNum(fileNum);
  auto lineNum = inlined ? srcPosition.GetInlinedLineNum() : srcPosition.LineNum();
  FATAL(kLncFatal, "%s:%d: error: last argument of __builtin_object_size is not integer constant between 0 and 3",
        currFile.c_str(), lineNum);
}

void OBJSize::DealWithBuiltinObjectSize(BB &bb, MeStmt &meStmt) {
  auto &intrinsiccall = static_cast<IntrinsiccallMeStmt&>(meStmt);
  if (intrinsiccall.GetIntrinsic() != INTRN_C___builtin_object_size) {
    return;
  }
  auto *opnd1 = intrinsiccall.GetOpnd(1);
  if (opnd1->GetMeOp() != kMeOpConst) {
    ERRWhenSizeTypeIsInvalid(meStmt);
  }
  // Type is an integer constant from 0 to 3.
  auto type = static_cast<ConstMeExpr*>(opnd1)->GetExtIntValue();
  if (type != kTypeZero && type != kTypeOne && type != kTypeTwo && type != kTypeThree) {
    ERRWhenSizeTypeIsInvalid(meStmt);
  }
  auto *opnd0 = intrinsiccall.GetOpnd(0);
  // If the least significant bit is clear, objects are whole variables, if it is set,
  // a closest surrounding subobject is considered the object a pointer points to.
  bool getSizeOfWholeVar = (type == kTypeZero || type == kTypeTwo);
  // If the pointer points to multiple objects at compile time,
  // The second bit determines whether computes the maximum or minimum of the remaining byte counts in those objects.
  bool getMaxSizeOfObjs = (type == kTypeZero || type == kTypeOne);
  auto size = ComputeObjectSizeWithType(*opnd0, getSizeOfWholeVar, getMaxSizeOfObjs);
  if (IsInvalidSize(size)) {
    // __builtin_object_size
    // inputType = 0 or 1 return -1
    // inputType = 2 or 3 return 0
    size = (type == kTypeZero || type == kTypeOne) ? -1 : 0;
  }
  // Replace callstmt ___builtin_object_size with dassign:
  // ||MEIR|| intrinsiccallassigned TYIDX:0C___builtin_object_size
  //          opnd[0] = VAR %arg{offset:0}<0>[idx:2] (field)0 mx2<Z>
  //          opnd[1] = CONST i32 0 mx3
  //    assignedpart: { VAR %_64__retVar_6_0{offset:0}<0>[idx:3] (field)0 mx4}
  // ====>
  // ||MEIR|| dassign VAR %_64__retVar_6_0{offset:0}<0>[idx:3] (field)0 mx4
  //          rhs = CONST u64 0 mx6
  MeExpr *constExpr = irMap.CreateIntConstMeExpr(static_cast<int64>(size), PTY_u64);
  auto *newStmt = irMap.CreateAssignMeStmt(*intrinsiccall.GetAssignedLHS(), *constExpr, bb);
  bb.ReplaceMeStmt(&intrinsiccall, newStmt);
}

// Recursively find whether the definition point is a string.
MeExpr *OBJSize::GetStrMeExpr(MeExpr &expr) {
  if (expr.GetMeOp() == kMeOpConststr) {
    return &expr;
  }
  if (expr.IsScalar() && static_cast<const ScalarMeExpr&>(expr).GetDefBy() == kDefByStmt) {
    return GetStrMeExpr(*static_cast<const ScalarMeExpr&>(expr).GetDefStmt()->GetRHS());
  }
  return nullptr;
}

size_t OBJSize::DealWithSprintfAndVsprintf(const CallMeStmt &callMeStmt, const MIRFunction &calleeFunc) {
  if (calleeFunc.GetName() != kSprintfChk && calleeFunc.GetName() != kVsprintfChk) {
    return kInvalidDestSize;
  }
  auto *fmt = GetStrMeExpr(*callMeStmt.GetOpnd(3));
  if (fmt == nullptr || fmt->GetMeOp() != kMeOpConststr) {
    return kInvalidDestSize;
  }
  auto *constStrMeExpr = static_cast<ConststrMeExpr*>(fmt);
  auto &fmtStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(constStrMeExpr->GetStrIdx());
  if (strchr(fmtStr.c_str(), '%') == nullptr) {
    // If the format doesn't contain % args or %%, the size is the length of fmtStr.
    return strlen(fmtStr.c_str()) + 1;
  } else if (calleeFunc.GetName() == kSprintfChk && strcmp (fmtStr.c_str(), "%s") == 0) {
    // If the format is "%s" and first ... argument is a string literal, the size is the length of string literal.
    if (callMeStmt.GetOpnds().size() < 5) {
      return kInvalidDestSize;
    }
    auto *src = callMeStmt.GetOpnd(4);
    src = GetStrMeExpr(*src);
    if (src != nullptr) {
      return ComputeObjectSizeWithType(*src, true, true);
    }
  }
  return kInvalidDestSize;
}

const std::map<std::string, std::pair<size_t, size_t>> kIndexOfParam {
    {"sprintf", {1, 2}},
    {"snprintf", {2, 3}},
    {"vsprintf", {1, 2}},
    {"vsnprintf", {2, 3}}
};

// If do not know the size of the buffer, replace the function without check. Such as:
// __memcpy_chk => memcpy
void OBJSize::ReplaceStmt(CallMeStmt &callMeStmt, const std::string &str) {
  auto *mirFunc = func.GetMIRModule().GetMIRBuilder()->GetOrCreateFunction(str, TyIdx(PTY_void));
  auto *newCallMeStmt = irMap.NewInPool<CallMeStmt>(callMeStmt.GetOp());
  const std::set<std::string> kReplacedExceptArgs {"sprintf", "snprintf", "vsprintf", "vsnprintf"};
  if (kReplacedExceptArgs.find(str) != kReplacedExceptArgs.end()) {
    for (size_t i = 0; i < callMeStmt.GetOpnds().size(); ++i) {
      // When replacing with a function without check, the second and third parameters need to be deleted.
      auto itOfIndex = kReplacedExceptArgs.find(str);
      if (itOfIndex == kReplacedExceptArgs.end()) {
        return;
      }
      auto itOfIndexParam = kIndexOfParam.find(str);
      if (i == itOfIndexParam->second.first || i == itOfIndexParam->second.second) {
        continue;
      }
      newCallMeStmt->PushBackOpnd(callMeStmt.GetOpnd(i));
    }
  } else {
    for (size_t i = 0; i < callMeStmt.GetOpnds().size() - 1; ++i) {
      newCallMeStmt->PushBackOpnd(callMeStmt.GetOpnd(i));
    }
  }
  newCallMeStmt->SetPUIdx(mirFunc->GetPuidx());
  for (auto &mu : *callMeStmt.GetMuList()) {
    (void)newCallMeStmt->GetMuList()->emplace(mu.first, mu.second);
  }
  newCallMeStmt->SetMustDefListAndUpdateBase(*callMeStmt.GetMustDefList());
  newCallMeStmt->SetChiListAndUpdateBase(*callMeStmt.GetChiList());
  callMeStmt.GetBB()->ReplaceMeStmt(&callMeStmt, newCallMeStmt);
}

void OBJSize::ComputeObjectSize(MeStmt &meStmt) {
  if (meStmt.GetOp() != OP_callassigned) {
    return;
  }
  auto &callMeStmt = static_cast<CallMeStmt&>(meStmt);
  MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callMeStmt.GetPUIdx());
  auto it = kReplacedFuncPairs.find(calleeFunc->GetName());
  if (it == kReplacedFuncPairs.end()) {
    return;
  }
  auto itOfIndex = kIndexOfSrcLength.find(calleeFunc->GetName());
  if (itOfIndex == kIndexOfSrcLength.end()) {
    return;
  }
  auto *destSizeOpnd = callMeStmt.GetOpnd(itOfIndex->second.second);
  size_t destSize = 0;
  destSize = ComputeObjectSizeWithType(*destSizeOpnd, true, true);
  if (destSize == 0 || destSize == kInvalidDestSize) {
    // The length information of dest is unknown during compilation.
    ReplaceStmt(callMeStmt, it->second);
    return;
  }
  size_t srcSize = 0;
  if (itOfIndex->second.first == kInvalidDestSize) {
    srcSize = DealWithSprintfAndVsprintf(callMeStmt, *calleeFunc);
  } else {
    MeExpr *srcSizeOpnd = callMeStmt.GetOpnd(itOfIndex->second.first);
    srcSize = ComputeObjectSizeWithType(*srcSizeOpnd, true, true);
  }
  if (IsInvalidSize(srcSize)) {
    // The length information of src is unknown during compilation.
    return;
  }
  if (srcSize <= destSize) {
    ReplaceStmt(callMeStmt, it->second);
    return;
  }
  auto srcPosition = callMeStmt.GetSrcPosition();
  SrcPosition pos = (srcPosition.GetInlinedFileNum() != 0) ?
      SrcPosition(srcPosition.GetInlinedFileNum(), srcPosition.GetInlinedLineNum(), 0, 0) : srcPosition;
  WARN_USER(kLncWarn, pos, func.GetMIRModule(), "‘__builtin_%s’ will always overflow; "
      "destination buffer has size %lu, but size argument is %lu", calleeFunc->GetName().c_str(), destSize, srcSize);
}

size_t OBJSize::DealWithAddrof(const MeExpr &opnd, bool getSizeOfWholeVar) const {
  auto &addrofMeExpr = static_cast<const AddrofMeExpr&>(opnd);
  auto *ost = addrofMeExpr.GetOst();
  auto fieldID = ost->GetFieldID();
  auto *typeOfBase = ost->GetMIRSymbol()->GetType();
  if (fieldID == 0) {
    return typeOfBase->GetSize();
  }
  if (typeOfBase->GetKind() != kTypeStruct) {
    return kInvalidDestSize;
  }
  auto *structMIRType = static_cast<MIRStructType*>(typeOfBase);
  if (!getSizeOfWholeVar) {
    return structMIRType->GetFieldType(fieldID)->GetSize();
  }
  auto offset = structMIRType->GetBitOffsetFromBaseAddr(fieldID);
  if (offset == kOffsetUnknown) {
    return kInvalidDestSize;
  }
  return structMIRType->GetSize() - static_cast<uint64>(offset / kBitsPerByte);
}

size_t OBJSize::DealWithIaddrof(const MeExpr &opnd, bool getSizeOfWholeVar, bool getMaxSizeOfObjs) const {
  auto &iaddrofMeExpr = static_cast<const OpMeExpr&>(opnd);
  auto fieldIDOfIaddrof = iaddrofMeExpr.GetFieldID();
  auto *typeOfBase = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrofMeExpr.GetTyIdx());
  if (fieldIDOfIaddrof == 0) {
    return typeOfBase->GetSize();
  }
  if (typeOfBase->GetKind() == kTypePointer) {
    typeOfBase = static_cast<MIRPtrType*>(typeOfBase)->GetPointedType();
  }
  if (typeOfBase->GetKind() != kTypeStruct && typeOfBase->GetKind() != kTypeUnion) {
    return kInvalidDestSize;
  }
  if (!getSizeOfWholeVar) {
    // When an array is the last field of a struct, when calculating the size,
    // you cannot only calculate the length of the array, but use the array as a pointer to calculate the size
    // between the start address of the array and the end address of the struct object.
    if (fieldIDOfIaddrof != static_cast<FieldID>(static_cast<MIRStructType*>(typeOfBase)->NumberOfFieldIDs()) ||
        static_cast<MIRStructType*>(typeOfBase)->GetFieldType(fieldIDOfIaddrof)->GetKind() != kTypeArray ||
        !iaddrofMeExpr.GetOpnd(0)->IsLeaf()) {
      return static_cast<MIRStructType*>(typeOfBase)->GetFieldType(fieldIDOfIaddrof)->GetSize();
    }
  }
  if (iaddrofMeExpr.GetNumOpnds() != 1) {
    return kInvalidDestSize;
  }
  auto *iaddrofOpnd0 = iaddrofMeExpr.GetOpnd(0);
  auto sizeOfIaddrofOpnd0 = ComputeObjectSizeWithType(*iaddrofOpnd0, true, getMaxSizeOfObjs);
  if (IsInvalidSize(sizeOfIaddrofOpnd0)) {
    return kInvalidDestSize;
  }
  return sizeOfIaddrofOpnd0 -
      static_cast<uint64>(typeOfBase->GetBitOffsetFromBaseAddr(fieldIDOfIaddrof) / kBitsPerByte);
}

bool OBJSize::DealWithOpnd(MeExpr &opnd, std::set<MePhiNode*> &visitedPhi) const {
  if (PhiOpndIsDefPointOfOtherPhi(opnd, visitedPhi)) {
    return true;
  }
  for (uint8 i = 0; i < opnd.GetNumOpnds(); ++i) {
    if (PhiOpndIsDefPointOfOtherPhi(*opnd.GetOpnd(i), visitedPhi)) {
      return true;
    }
  }
  return false;
}

// If phi opnd is def point of other phi, return true. Such as meir:
// VAR:%pu8Buf{offset:0}<0>[idx:16] mx94 = MEPHI{mx93,mx361}
// VAR:%pu8Buf{offset:0}<0>[idx:16] mx93 = MEPHI{mx53,mx94}
bool OBJSize::PhiOpndIsDefPointOfOtherPhi(MeExpr &expr, std::set<MePhiNode*> &visitedPhi) const {
  if (!expr.IsScalar()) {
    return false;
  }
  auto *var = static_cast<ScalarMeExpr*>(&expr);
  if (var->GetDefBy() == kDefByStmt) {
    auto defStmt = var->GetDefStmt();
    for (size_t i = 0; i < defStmt->NumMeStmtOpnds(); ++i) {
      if (DealWithOpnd(*defStmt->GetOpnd(i), visitedPhi)) {
        return true;
      }
    }
  }
  if (var->GetDefBy() == kDefByChi) {
    auto *rhs = var->GetDefChi().GetRHS();
    if (rhs == nullptr) {
      return false;
    }
    return PhiOpndIsDefPointOfOtherPhi(*rhs, visitedPhi);
  }
  if (var->GetDefBy() == kDefByPhi) {
    MePhiNode *phi = &(var->GetDefPhi());
    if (visitedPhi.find(phi) != visitedPhi.end()) {
      return true;
    }
    (void)visitedPhi.insert(phi);
    for (auto *phiOpnd : phi->GetOpnds()) {
      if (PhiOpndIsDefPointOfOtherPhi(*phiOpnd, visitedPhi)) {
        return true;
      }
    }
  }
  return false;
}

size_t OBJSize::DealWithDread(MeExpr &opnd, bool getSizeOfWholeVar, bool getMaxSizeOfObjs) const {
  if (!opnd.IsScalar()) {
    return kInvalidDestSize;
  }
  auto &scalarMeExpr = static_cast<ScalarMeExpr&>(opnd);
  if (scalarMeExpr.GetDefBy() == kDefByPhi) {
    std::set<MePhiNode*> visitedPhi;
    if (PhiOpndIsDefPointOfOtherPhi(scalarMeExpr, visitedPhi)) {
      return kInvalidDestSize;
    }
    auto &phi = scalarMeExpr.GetDefPhi();
    size_t size = getMaxSizeOfObjs ? 0 : std::numeric_limits<uint64_t>::max();
    for (size_t i = 0; i < phi.GetOpnds().size(); ++i) {
      size = getMaxSizeOfObjs ?
          std::max(ComputeObjectSizeWithType(*phi.GetOpnd(i), getSizeOfWholeVar, getMaxSizeOfObjs), size) :
          std::min(ComputeObjectSizeWithType(*phi.GetOpnd(i), getSizeOfWholeVar, getMaxSizeOfObjs), size);
    }
    return size;
  }
  if (scalarMeExpr.GetDefBy() == kDefByMustDef) {
    auto *defStmt = scalarMeExpr.GetDefMustDef().GetBase();
    if (defStmt->GetOp() != OP_callassigned) {
      return kInvalidDestSize;
    }
    MIRFunction *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(
        static_cast<const CallMeStmt*>(defStmt)->GetPUIdx());
    if (mirFunc->GetName() != kMalloc) {
      return kInvalidDestSize;
    }
    auto *opnd0 = defStmt->GetOpnd(0);
    if (IsConstIntExpr(*opnd0)) {
      return static_cast<ConstMeExpr*>(opnd0)->GetZXTIntValue();
    }
  }
  if (scalarMeExpr.GetDefBy() == kDefByStmt) {
    return ComputeObjectSizeWithType(*scalarMeExpr.GetDefStmt()->GetRHS(), getSizeOfWholeVar, getMaxSizeOfObjs);
  }
  auto *mirType = scalarMeExpr.GetOst()->GetType();
  if (mirType->IsMIRPtrType()) {
    // If the type is pointer type, can not compute the size of obj.
    // For example: int *b, memcpy(b, buf4, 1000).
    return kInvalidDestSize;
  }
  auto size = mirType->GetSize();
  if (size == kOffsetUnknown) {
    return kInvalidDestSize;
  }
  return size;
}

size_t OBJSize::DealWithArray(const MeExpr &opnd, bool getSizeOfWholeVar, bool getMaxSizeOfObjs) const {
  if (opnd.GetNumOpnds() != kNumOperands) {
    return kInvalidDestSize;
  }
  auto opnd0 = opnd.GetOpnd(0);
  auto opnd1 = opnd.GetOpnd(1);
  MIRType *elemType = IRMap::GetArrayElemType(*opnd0);
  if (elemType == nullptr) {
    return kInvalidDestSize;
  }
  auto size1 = ComputeObjectSizeWithType(*opnd0, getSizeOfWholeVar, getMaxSizeOfObjs);
  auto size2 = ComputeObjectSizeWithType(*opnd1, getSizeOfWholeVar, getMaxSizeOfObjs);
  if (IsInvalidSize(size1) || IsInvalidSize(size2)) {
    return kInvalidDestSize;
  }
  return size1 - size2 * elemType->GetSize();
}

// This function returns the size of object and
// If it is not possible to determine which objects ptr points to at compile time,
// should return (size_t) -1 for type 0 or 1 and (size_t) 0 for type 2 or 3.
size_t OBJSize::ComputeObjectSizeWithType(MeExpr &opnd, bool getSizeOfWholeVar, bool getMaxSizeOfObjs) const {
  switch (opnd.GetOp()) {
    case OP_addrof: {
      return DealWithAddrof(opnd, getSizeOfWholeVar);
    }
    case OP_iaddrof: {
      return DealWithIaddrof(opnd, getSizeOfWholeVar, getMaxSizeOfObjs);
    }
    case OP_dread: {
      return DealWithDread(opnd, getSizeOfWholeVar, getMaxSizeOfObjs);
    }
    case OP_add:
    case OP_sub: {
      if (opnd.GetNumOpnds() != kNumOperands) {
        return kInvalidDestSize;
      }
      auto size1 = ComputeObjectSizeWithType(*opnd.GetOpnd(0), getSizeOfWholeVar, getMaxSizeOfObjs);
      auto size2 = ComputeObjectSizeWithType(*opnd.GetOpnd(1), getSizeOfWholeVar, getMaxSizeOfObjs);
      if (IsInvalidSize(size1) || IsInvalidSize(size2)) {
        return kInvalidDestSize;
      }
      return (opnd.GetOp() == OP_add || opnd.GetOp() == OP_array) ? (size1 - size2) : (size1 + size2);
    }
    case OP_array: {
      return DealWithArray(opnd, getSizeOfWholeVar, getMaxSizeOfObjs);
    }
    case OP_constval: {
      auto &constMeExpr = static_cast<ConstMeExpr&>(opnd);
      if (constMeExpr.GetConstVal()->GetKind() != kConstInt) {
        return kInvalidDestSize;
      }
      if (IsPrimitivePoint(opnd.GetPrimType()) && opnd.IsZero()) {
        // Can not compute the size of null, return kInvalidDestSize.
        return kInvalidDestSize;
      }
      return static_cast<ConstMeExpr&>(opnd).GetIntValue().GetZXTValue();
    }
    case OP_conststr: {
      auto &constStrMeExpr = static_cast<ConststrMeExpr&>(opnd);
      auto &str = GlobalTables::GetUStrTable().GetStringFromStrIdx(constStrMeExpr.GetStrIdx());
      return strlen(str.c_str()) + 1;
    }
    default:
      break;
  }
  return kInvalidDestSize;
}

void MEOBJSize::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MEOBJSize::PhaseRun(maple::MeFunction &f) {
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  OBJSize objSize(f, *irMap);
  objSize.Execute();
  return false;
}
}  // namespace maple
