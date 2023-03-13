/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "fe_utils.h"
#include <sstream>
#include "mpl_logging.h"
#include "mir_type.h"
#include "mir_builder.h"
#include "fe_manager.h"
namespace maple {
// ---------- FEUtils ----------
const std::string FEUtils::kBoolean = "Z";
const std::string FEUtils::kByte = "B";
const std::string FEUtils::kShort = "S";
const std::string FEUtils::kChar = "C";
const std::string FEUtils::kInt = "I";
const std::string FEUtils::kLong = "J";
const std::string FEUtils::kFloat = "F";
const std::string FEUtils::kDouble = "D";
const std::string FEUtils::kVoid = "V";
const std::string FEUtils::kThis = "_this";
const std::string FEUtils::kDotDot = "/..";
const std::string FEUtils::kMCCStaticFieldGetBool   = "MCC_StaticFieldGetBool";
const std::string FEUtils::kMCCStaticFieldGetByte   = "MCC_StaticFieldGetByte";
const std::string FEUtils::kMCCStaticFieldGetShort  = "MCC_StaticFieldGetShort";
const std::string FEUtils::kMCCStaticFieldGetChar   = "MCC_StaticFieldGetChar";
const std::string FEUtils::kMCCStaticFieldGetInt    = "MCC_StaticFieldGetInt";
const std::string FEUtils::kMCCStaticFieldGetLong   = "MCC_StaticFieldGetLong";
const std::string FEUtils::kMCCStaticFieldGetFloat  = "MCC_StaticFieldGetFloat";
const std::string FEUtils::kMCCStaticFieldGetDouble = "MCC_StaticFieldGetDouble";
const std::string FEUtils::kMCCStaticFieldGetObject = "MCC_StaticFieldGetObject";

const std::string FEUtils::kMCCStaticFieldSetBool   = "MCC_StaticFieldSetBool";
const std::string FEUtils::kMCCStaticFieldSetByte   = "MCC_StaticFieldSetByte";
const std::string FEUtils::kMCCStaticFieldSetShort  = "MCC_StaticFieldSetShort";
const std::string FEUtils::kMCCStaticFieldSetChar   = "MCC_StaticFieldSetChar";
const std::string FEUtils::kMCCStaticFieldSetInt    = "MCC_StaticFieldSetInt";
const std::string FEUtils::kMCCStaticFieldSetLong   = "MCC_StaticFieldSetLong";
const std::string FEUtils::kMCCStaticFieldSetFloat  = "MCC_StaticFieldSetFloat";
const std::string FEUtils::kMCCStaticFieldSetDouble = "MCC_StaticFieldSetDouble";
const std::string FEUtils::kMCCStaticFieldSetObject = "MCC_StaticFieldSetObject";

const std::string FEUtils::kCondGoToStmtLabelNamePrefix = "shortCircuit_";

std::vector<std::string> FEUtils::Split(const std::string &str, char delim) {
  std::vector<std::string> ans;
  std::stringstream ss;
  ss.str(str);
  std::string item;
  while (std::getline(ss, item, delim)) {
    ans.push_back(item);
  }
  return ans;
}

uint8 FEUtils::GetWidth(PrimType primType) {
  switch (primType) {
    case PTY_u1:
      return 1;
    case PTY_i8:
    case PTY_u8:
      return 8;
    case PTY_i16:
    case PTY_u16:
      return 16;
    case PTY_i32:
    case PTY_u32:
    case PTY_f32:
      return 32;
    case PTY_i64:
    case PTY_u64:
    case PTY_f64:
      return 64;
    default:
      CHECK_FATAL(false, "unsupported type %d", primType);
      return 0;
  }
}

bool FEUtils::IsInteger(PrimType primType) {
  return (primType == PTY_i8) || (primType == PTY_u8) ||
         (primType == PTY_i16) || (primType == PTY_u16) ||
         (primType == PTY_i32) || (primType == PTY_u32) ||
         (primType == PTY_i64) || (primType == PTY_u64);
}

bool FEUtils::IsSignedInteger(PrimType primType) {
  return (primType == PTY_i8) || (primType == PTY_i16) || (primType == PTY_i32) || (primType == PTY_i64);
}

bool FEUtils::IsUnsignedInteger(PrimType primType) {
  return (primType == PTY_u8) || (primType == PTY_u16) || (primType == PTY_u32) || (primType == PTY_u64);
}

PrimType FEUtils::MergePrimType(PrimType primType1, PrimType primType2) {
  if (primType1 == primType2) {
    return primType1;
  }
  // merge signed integer
  CHECK_FATAL(!LogicXOR(IsSignedInteger(primType1), IsSignedInteger(primType2)),
              "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  if (IsSignedInteger(primType1)) {
    return GetPrimTypeSize(primType1) >= GetPrimTypeSize(primType2) ? primType1 : primType2;
  }

  // merge unsigned integer
  CHECK_FATAL(!LogicXOR(IsUnsignedInteger(primType1), IsUnsignedInteger(primType2)),
              "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  if (IsUnsignedInteger(primType1)) {
    if (GetPrimTypeSize(primType1) == GetPrimTypeSize(primType2) && GetPrimTypeSize(primType1) == 1) {
      return PTY_u8;
    } else {
      return GetPrimTypeSize(primType1) >= GetPrimTypeSize(primType2) ? primType1 : primType2;
    }
  }

  // merge float
  CHECK_FATAL(!LogicXOR(IsPrimitiveFloat(primType1), IsPrimitiveFloat(primType2)),
              "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  if (IsPrimitiveFloat(primType1)) {
    return GetPrimTypeSize(primType1) >= GetPrimTypeSize(primType2) ? primType1 : primType2;
  }

  CHECK_FATAL(false, "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  return PTY_unknown;
}

uint8 FEUtils::GetDim(const std::string &typeName) {
  uint8 dim = 0;
  for (size_t i = 0; i < typeName.length(); ++i) {
    if (typeName.at(i) == 'A') {
      dim++;
    } else {
      break;
    }
  }
  return dim;
}

std::string FEUtils::GetBaseTypeName(const std::string &typeName) {
  return typeName.substr(GetDim(typeName));
}

PrimType FEUtils::GetPrimType(const GStrIdx &typeNameIdx) {
  if (typeNameIdx == GetBooleanIdx()) {
    return PTY_u1;
  }
  if (typeNameIdx == GetByteIdx()) {
    return PTY_i8;
  }
  if (typeNameIdx == GetShortIdx()) {
    return PTY_i16;
  }
  if (typeNameIdx == GetCharIdx()) {
    return PTY_u16;
  }
  if (typeNameIdx == GetIntIdx()) {
    return PTY_i32;
  }
  if (typeNameIdx == GetLongIdx()) {
    return PTY_i64;
  }
  if (typeNameIdx == GetFloatIdx()) {
    return PTY_f32;
  }
  if (typeNameIdx == GetDoubleIdx()) {
    return PTY_f64;
  }
  if (typeNameIdx == GetVoidIdx()) {
    return PTY_void;
  }
  return PTY_ref;
}

std::string FEUtils::GetSequentialName0(const std::string &prefix, uint32_t num) {
  std::stringstream ss;
  ss << prefix << num;
  return ss.str();
}

uint32 FEUtils::GetSequentialNumber() {
  static uint32 unnamedSymbolIdx = 1;
  return unnamedSymbolIdx++;
}

// erase ".." in fileName
std::string FEUtils::NormalizeFileName(std::string fileName) {
  auto locOfDotDot = fileName.find(kDotDot);
  if (locOfDotDot == std::string::npos) {
    return fileName;
  }

  constexpr char forwardSlash = '/';
  // slash, '/', before ".." must not be the first character.
  if (locOfDotDot <= 1 || fileName[locOfDotDot] != forwardSlash) {
    return fileName;
  }

  // calculate position of the slash before the backed-up directory.
  auto locOfBackedDir = fileName.rfind(forwardSlash, locOfDotDot - 1);
  if (locOfBackedDir == std::string::npos) {
    return fileName;
  }

  // erase the backed-up directory and ".." from fileName.
  // For example,
  // /home/usrname/backed-up-dir/../nextDir/*.c  ==>
  // /home/usrname/nextDir/*.c
  constexpr size_t charNumNeedSkip = 3U;
  (void) fileName.erase(fileName.begin() + static_cast<long>(locOfBackedDir),
                        fileName.begin() + static_cast<long>(locOfDotDot + charNumNeedSkip));
  return NormalizeFileName(fileName);
}

// struct __pthread_cond_s::(anonymous at /opt/RTOS/208.2.0//bits/thread-shared-types.h:97:5)
// ==> struct __pthread_cond_s
void FEUtils::EraseFileNameforClangTypeStr(std::string &typeStr) {
  auto start = typeStr.find(':');
  auto end = typeStr.find(')');
  if (start == std::string::npos || end == std::string::npos) {
    return;
  }
  (void) typeStr.erase(start, (end - start) + 1U);
}

std::string FEUtils::GetHashStr(const std::string &str, uint32 seed) {
  const char *name = str.c_str();
  uint32 hash = 0;
  while (*name) {
    uint8_t uName = *name++;
    hash = hash * seed + uName;
  }
  return std::to_string(hash);
}

std::string FEUtils::GetFileNameHashStr(const std::string &fileName, uint32 seed) {
  std::string result = kRenameKeyWord + GetHashStr(fileName, seed);
  return result;
}

std::string FEUtils::GetSequentialName(const std::string &prefix) {
  std::string name = GetSequentialName0(prefix, GetSequentialNumber());
  return name;
}

std::string FEUtils::CreateLabelName() {
  static uint32 unnamedSymbolIdx = 1;
  return "L." + std::to_string(unnamedSymbolIdx++);
}

bool FEUtils::TraverseToNamedField(MIRStructType &structType, const GStrIdx &nameIdx, FieldID &fieldID,
                                   bool isTopLevel) {
  for (uint32 fieldIdx = 0; fieldIdx < structType.GetFieldsSize(); ++fieldIdx) {
    ++fieldID;
    TyIdx fieldTyIdx = structType.GetFieldsElemt(fieldIdx).second.first;
    MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
    ASSERT(fieldType != nullptr, "fieldType is null");
    if (isTopLevel && structType.GetFieldsElemt(fieldIdx).first == nameIdx) {
      return true;
    }
    // The fields of an embedded structure array are assigned fieldIDs
    if (fieldType->GetKind() == kTypeArray) {
      fieldType = fieldType->EmbeddedStructType();
    }
    if (fieldType != nullptr && fieldType->IsStructType()) {
      auto *subStructType = static_cast<MIRStructType *>(fieldType);
      if (TraverseToNamedField(*subStructType, nameIdx, fieldID, false)) {
        return true;
      }
    }
  }
  return false;
}

FieldID FEUtils::GetStructFieldID(MIRStructType *base, const std::string &fieldName) {
  MIRStructType *type = base;
  std::vector<std::string> fieldNames = FEUtils::Split(fieldName, '$');
  std::reverse(fieldNames.begin(), fieldNames.end());
  FieldID fieldID = 0;
  for (const auto &f: fieldNames) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(f);
    CHECK_FATAL(type->IsStructType(), "Must be struct type!");
    if (TraverseToNamedField(*type, strIdx, fieldID)) {
      type = static_cast<MIRStructType *>(base->GetFieldType(fieldID));
    }
  }
  return fieldID;
}

MIRType *FEUtils::GetStructFieldType(const MIRStructType *type, FieldID fieldID) {
  FieldID tmpID = fieldID;
  FieldPair fieldPair = type->TraverseToFieldRef(tmpID);
  MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
  return fieldType;
}

const MIRFuncType *FEUtils::GetFuncPtrType(const MIRType &type) {
  const MIRType *mirType = &type;
  if (mirType->GetKind() != kTypePointer) {
    return nullptr;
  }
  mirType = static_cast<const MIRPtrType*>(mirType)->GetPointedType();
  if (mirType->GetKind() != kTypePointer) {
    return nullptr;
  }
  mirType = static_cast<const MIRPtrType*>(mirType)->GetPointedType();
  if (mirType->GetKind() != kTypeFunction) {
    return nullptr;
  }
  return static_cast<const MIRFuncType*>(mirType);
}

MIRConst *FEUtils::CreateImplicitConst(MIRType *type) {
  switch (type->GetPrimType()) {
    case PTY_u1: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_u1));
    }
    case PTY_u8: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_u8));
    }
    case PTY_u16: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_u16));
    }
    case PTY_u32: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_u32));
    }
    case PTY_u64: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
    }
    case PTY_i8: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_i8));
    }
    case PTY_i16: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_i16));
    }
    case PTY_i32: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
    }
    case PTY_i64: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    }
    case PTY_f32: {
      return FEManager::GetModule().GetMemPool()->New<MIRFloatConst>(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
    }
    case PTY_f64: {
      return FEManager::GetModule().GetMemPool()->New<MIRDoubleConst>(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    }
    case PTY_f128: {
      return FEManager::GetModule().GetMemPool()->New<MIRFloat128Const>(
          nullptr, *GlobalTables::GetTypeTable().GetPrimType(PTY_f128));
    }
    case PTY_ptr: {
      return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          0, *GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    }
    case PTY_agg: {
      auto *aggConst = FEManager::GetModule().GetMemPool()->New<MIRAggConst>(FEManager::GetModule(), *type);
      if (type->IsStructType()) {
        auto structType = static_cast<MIRStructType*>(type);
        FieldID fieldID = 0;
        if (type->GetKind() == kTypeUnion) {
          fieldID++;
          auto &f = structType->GetFields().front();
          auto fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(f.second.first);
          aggConst->AddItem(CreateImplicitConst(fieldType), fieldID);
        } else {
          for (auto &f : structType->GetFields()) {
            fieldID++;
            auto fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(f.second.first);
            aggConst->AddItem(CreateImplicitConst(fieldType), fieldID);
          }
        }
      } else if (type->GetKind() == kTypeArray) {
        auto arrayType = static_cast<MIRArrayType*>(type);
        MIRConst *elementConst;
        if (arrayType->GetDim() > 1) {
          uint32 subSizeArray[arrayType->GetDim()];
          for (int dim = 1; dim < arrayType->GetDim(); ++dim) {
            subSizeArray[dim - 1] = arrayType->GetSizeArrayItem(dim);
          }
          auto subArrayType = GlobalTables::GetTypeTable().GetOrCreateArrayType(
              *arrayType->GetElemType(), static_cast<uint8>(arrayType->GetDim() - 1), subSizeArray);
          elementConst = CreateImplicitConst(subArrayType);
        } else {
          elementConst = CreateImplicitConst(arrayType->GetElemType());
        }
        for (uint32 i = 0; i < arrayType->GetSizeArrayItem(0); ++i) {
          aggConst->AddItem(elementConst, 0);
        }
      }
      return aggConst;
    }
    default: {
      CHECK_FATAL(false, "Unsupported Primitive type: %d", type->GetPrimType());
    }
  }
}

PrimType FEUtils::GetVectorElementPrimType(PrimType vectorPrimType) {
  switch (vectorPrimType) {
    case PTY_v2i64:
      return PTY_i64;
    case PTY_v4i32:
    case PTY_v2i32:
      return PTY_i32;
    case PTY_v8i16:
    case PTY_v4i16:
      return PTY_i16;
    case PTY_v16i8:
    case PTY_v8i8:
      return PTY_i8;
    case PTY_v2u64:
      return PTY_u64;
    case PTY_v4u32:
    case PTY_v2u32:
      return PTY_u32;
    case PTY_v8u16:
    case PTY_v4u16:
      return PTY_u16;
    case PTY_v16u8:
    case PTY_v8u8:
      return PTY_u8;
    case PTY_v2f64:
      return PTY_f64;
    case PTY_v4f32:
    case PTY_v2f32:
      return PTY_f32;
    default:
      return PTY_unknown;
  }
}

bool FEUtils::EndsWith(const std::string &value, const std::string &ending) {
  if (ending.size() > value.size()) {
    return false;
  }
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

// in the search, curfieldid is being decremented until it reaches 1
MIRConst *FEUtils::TraverseToMIRConst(MIRAggConst *aggConst, const MIRStructType &structType, FieldID &fieldID) {
  if (aggConst == nullptr || aggConst->GetConstVec().size() == 0) {
    return nullptr;
  }
  uint32 fieldIdx = 0;
  FieldPair curPair = structType.GetFields()[0];
  MIRConst *curConst = aggConst->GetConstVec()[0];
  while (fieldID > 1) {
    --fieldID;
    MIRType *curFieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(curPair.second.first);
    MIRStructType *subStructTy = curFieldType->EmbeddedStructType();
    if (subStructTy != nullptr && curConst->GetKind() == kConstAggConst) {
      curConst = TraverseToMIRConst(static_cast<MIRAggConst*>(curConst), *subStructTy, fieldID);
        if (fieldID == 1 && curConst != nullptr) {
          return curConst;
        }
    }
    ++fieldIdx;
    if (fieldIdx == structType.GetFields().size()) {
      return nullptr;
    }
    if (structType.GetKind() == kTypeUnion) {
      curConst = aggConst->GetConstVec()[0];  // union only is one element
    } else {
      curConst = aggConst->GetConstVec()[fieldIdx];
    }
    curPair = structType.GetFields()[fieldIdx];
  }
  return curConst;
}

Loc FEUtils::GetSrcLocationForMIRSymbol(const MIRSymbol &symbol) {
  return Loc(symbol.GetSrcPosition().FileNum(), symbol.GetSrcPosition().LineNum(), symbol.GetSrcPosition().Column());
}

void FEUtils::InitPrimTypeFuncNameIdxMap(std::map<PrimType, GStrIdx> &primTypeFuncNameIdxMap) {
  primTypeFuncNameIdxMap = {
      { PTY_u1, GetMCCStaticFieldSetBoolIdx() },
      { PTY_i8, GetMCCStaticFieldSetByteIdx() },
      { PTY_i16, GetMCCStaticFieldSetShortIdx() },
      { PTY_u16, GetMCCStaticFieldSetCharIdx() },
      { PTY_i32, GetMCCStaticFieldSetIntIdx() },
      { PTY_i64, GetMCCStaticFieldSetLongIdx() },
      { PTY_f32, GetMCCStaticFieldSetFloatIdx() },
      { PTY_f64, GetMCCStaticFieldSetDoubleIdx() },
      { PTY_ref, GetMCCStaticFieldSetObjectIdx() },
  };
}

MIRAliasVars FEUtils::AddAlias(const GStrIdx &mplNameIdx, const MIRType *sourceType, const TypeAttrs &attrs,
                               bool isLocal) {
  MIRAliasVars aliasVar;
  aliasVar.mplStrIdx = mplNameIdx;
  aliasVar.isLocal = isLocal;
  aliasVar.attrs = attrs;
  if (sourceType != nullptr) {
    aliasVar.atk = kATKType;
    aliasVar.index = sourceType->GetTypeIndex();
  } else {
    CHECK_FATAL(false, "unknown source type of %s",
                GlobalTables::GetStrTable().GetStringFromStrIdx(mplNameIdx).c_str());
  }
  return aliasVar;
};

void FEUtils::AddAliasInMIRScope(MIRScope &scope, const std::string &srcVarName, const MIRSymbol &symbol,
                                 const MIRType *sourceType) {
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(srcVarName);
  MIRAliasVars aliasVar = FEUtils::AddAlias(symbol.GetNameStrIdx(), sourceType, symbol.GetAttrs(), symbol.IsLocal());
  scope.SetAliasVarMap(nameIdx, aliasVar);
};

SrcPosition FEUtils::CvtLoc2SrcPosition(const Loc &loc) {
  SrcPosition srcPos;
  srcPos.SetFileNum(static_cast<uint16>(loc.fileIdx));
  srcPos.SetLineNum(loc.line);
  srcPos.SetColumn(static_cast<uint16>(loc.column));
  return srcPos;
}
// ---------- FELinkListNode ----------
FELinkListNode::FELinkListNode()
    : prev(nullptr), next(nullptr) {}

FELinkListNode::~FELinkListNode() {
  prev = nullptr;
  next = nullptr;
}

void FELinkListNode::InsertBefore(FELinkListNode *ins) {
  InsertBefore(ins, this);
}

void FELinkListNode::InsertAfter(FELinkListNode *ins) {
  InsertAfter(ins, this);
}

void FELinkListNode::InsertBefore(FELinkListNode *ins, FELinkListNode *pos) {
  // pos_p -- ins -- pos
  if (pos == nullptr || pos->prev == nullptr || ins == nullptr) {
    CHECK_FATAL(false, "invalid input");
  }
  FELinkListNode *posPrev = pos->prev;
  posPrev->next = ins;
  pos->prev = ins;
  ins->prev = posPrev;
  ins->next = pos;
}

void FELinkListNode::InsertAfter(FELinkListNode *ins, FELinkListNode *pos) {
  // pos -- ins -- pos_n
  if (pos == nullptr || pos->next == nullptr || ins == nullptr) {
    CHECK_FATAL(false, "invalid input");
  }
  FELinkListNode *posNext = pos->next;
  pos->next = ins;
  posNext->prev = ins;
  ins->prev = pos;
  ins->next = posNext;
}

void FELinkListNode::SpliceNodes(FELinkListNode *head, FELinkListNode *tail, FELinkListNode *newTail) {
  FELinkListNode *stmt = head->GetNext();
  FELinkListNode *nextStmt = stmt;
  do {
    stmt = nextStmt;
    nextStmt = stmt->GetNext();
    newTail->InsertBefore(stmt);
  } while (nextStmt != nullptr && nextStmt != tail);
}

uint32_t AstSwitchUtil::tempVarNo = 0;
const char *AstSwitchUtil::cleanLabel = "clean";
const char *AstSwitchUtil::exitLabel = "exit";
const char *AstSwitchUtil::blockLabel = "blklbl";
const char *AstSwitchUtil::caseLabel = "case";
const char *AstSwitchUtil::catchLabel = "catch";
const char *AstSwitchUtil::endehLabel = "endeh";

std::string AstSwitchUtil::CreateEndOrExitLabelName() const {
  std::string labelName = FEUtils::GetSequentialName0(blockLabel, tempVarNo);
  ++tempVarNo;
  return labelName;
}

void AstSwitchUtil::MarkLabelUsed(const std::string &label) {
  labelUsed[label] = true;
}

void AstSwitchUtil::MarkLabelUnUsed(const std::string &label) {
  labelUsed[label] = false;
}

void AstSwitchUtil::PushNestedBreakLabels(const std::string &label) {
  nestedBreakLabels.push(label);
}

void AstSwitchUtil::PopNestedBreakLabels() {
  nestedBreakLabels.pop();
}

void AstSwitchUtil::PushNestedCaseVectors(const std::pair<CaseVector*, LabelIdx> &caseVec) {
  nestedCaseVectors.push(caseVec);
}

void AstSwitchUtil::PopNestedCaseVectors() {
  nestedCaseVectors.pop();
}

bool AstSwitchUtil::CheckLabelUsed(const std::string &label) {
  return labelUsed[label];
}

const std::pair<CaseVector*, LabelIdx> &AstSwitchUtil::GetTopOfNestedCaseVectors() const {
  return nestedCaseVectors.top();
}

const std::string &AstSwitchUtil::GetTopOfBreakLabels() const {
  return nestedBreakLabels.top();
}

void AstLoopUtil::PushBreak(std::string label) {
  breakLabels.push(std::make_pair(label, false));
}

std::string AstLoopUtil::GetCurrentBreak() {
  breakLabels.top().second = true;
  return breakLabels.top().first;
}

bool AstLoopUtil::IsBreakLabelsEmpty() const {
  return breakLabels.empty();
}

bool AstLoopUtil::IsNestContinueLabelsEmpty() const {
  return nestContinueLabels.empty();
}

void AstLoopUtil::PopCurrentBreak() {
  breakLabels.pop();
}

void AstLoopUtil::PushContinue(std::string label) {
  continueLabels.push(std::make_pair(label, false));
}

void AstLoopUtil::PushNestContinue(std::string label) {
  nestContinueLabels.push(std::make_pair(label, false));
}

std::string AstLoopUtil::GetCurrentContinue() {
  continueLabels.top().second = true;
  return continueLabels.top().first;
}

std::string AstLoopUtil::GetNestContinue() {
  nestContinueLabels.top().second = true;
  return nestContinueLabels.top().first;
}

bool AstLoopUtil::IsContinueLabelsEmpty() const {
  return continueLabels.empty();
}

void AstLoopUtil::PopCurrentContinue() {
  continueLabels.pop();
}

void AstLoopUtil::PopNestContinue() {
  nestContinueLabels.pop();
}
}  // namespace maple
