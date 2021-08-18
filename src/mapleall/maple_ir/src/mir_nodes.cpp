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
#include "mir_nodes.h"
#include <algorithm>
#include <stack>
#include "mir_function.h"
#include "printing.h"
#include "maple_string.h"
#include "opcode_info.h"
#include "namemangler.h"
#include "utils.h"
#include "verification.h"

namespace maple {
MIRModule *theMIRModule = nullptr;
std::atomic<uint32> StmtNode::stmtIDNext(1);  // 0 is reserved
uint32 StmtNode::lastPrintedLineNum = 0;

const char *GetIntrinsicName(MIRIntrinsicID intrn) {
  switch (intrn) {
    default:
#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...) \
    case INTRN_##STR:                                                 \
      return #STR;
#include "intrinsics.def"
#undef DEF_MIR_INTRINSIC
  }
}

const char *BaseNode::GetOpName() const {
  return kOpcodeInfo.GetTableItemAt(GetOpCode()).name.c_str();
}

bool BaseNode::MayThrowException() {
  if (kOpcodeInfo.MayThrowException(GetOpCode())) {
    if (GetOpCode() != OP_array) {
      return true;
    }
    auto *arry = static_cast<ArrayNode*>(this);
    if (arry->GetBoundsCheck()) {
      return true;
    }
  } else if (GetOpCode() == OP_intrinsicop) {
    auto *inNode = static_cast<IntrinsicopNode*>(this);
    if (inNode->GetIntrinsic() == INTRN_JAVA_ARRAY_LENGTH) {
      return true;
    }
  }
  for (size_t i = 0; i < NumOpnds(); ++i) {
    if (Opnd(i)->MayThrowException()) {
      return true;
    }
  }
  return false;
}

bool AddrofNode::CheckNode(const MIRModule &mod) const {
  const MIRSymbol *st = mod.CurFunction()->GetLocalOrGlobalSymbol(GetStIdx());
  MIRType *ty = st->GetType();
  switch (ty->GetKind()) {
    case kTypeScalar: {
#ifdef DYNAMICLANG
      if (GetPrimType() == PTY_dynany) {
        return true;
      }
      return IsPrimitiveScalar(GetPrimType());
#else
      return IsPrimitiveScalar(GetPrimType());
#endif
    }
    case kTypeArray: {
      return GetPrimType() == PTY_agg;
    }
    case kTypeUnion:
    case kTypeStruct:
    case kTypeStructIncomplete: {
      if (GetFieldID() == 0) {
        return GetPrimType() == PTY_agg;
      }
      auto *structType = static_cast<MIRStructType*>(ty);
      TyIdx fTyIdx = structType->GetFieldTyIdx(fieldID);
      MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fTyIdx);
      MIRTypeKind subKind = subType->GetKind();
      return (subKind == kTypeBitField && VerifyPrimType(subType->GetPrimType(), GetPrimType())) ||
             (subKind == kTypeScalar && IsPrimitiveScalar(GetPrimType())) ||
             (subKind == kTypePointer && IsPrimitivePoint(GetPrimType())) ||
             (subKind == kTypeStruct && GetPrimType() == PTY_agg) || (fTyIdx != 0u && GetPrimType() == PTY_agg);
    }
    case kTypeClass:
    case kTypeClassIncomplete: {
      if (fieldID == 0) {
        return GetPrimType() == PTY_agg;
      }
      auto *classType = static_cast<MIRClassType*>(ty);
      MIRType *subType = classType->GetFieldType(fieldID);
      MIRTypeKind subKind = subType->GetKind();
      return (subKind == kTypeBitField && VerifyPrimType(subType->GetPrimType(), GetPrimType())) ||
             (subKind == kTypeScalar && IsPrimitiveScalar(GetPrimType())) ||
             (subKind == kTypePointer && IsPrimitivePoint(GetPrimType())) ||
             (subKind == kTypeStruct && GetPrimType() == PTY_agg);
    }
    case kTypeInterface:
    case kTypeInterfaceIncomplete: {
      if (fieldID == 0) {
        return GetPrimType() == PTY_agg;
      }
      auto *interfaceType = static_cast<MIRInterfaceType*>(ty);
      MIRType *subType = interfaceType->GetFieldType(fieldID);
      MIRTypeKind subKind = subType->GetKind();
      return (subKind == kTypeBitField && VerifyPrimType(subType->GetPrimType(), GetPrimType())) ||
             (subKind == kTypeScalar && IsPrimitiveScalar(GetPrimType())) ||
             (subKind == kTypePointer && IsPrimitivePoint(GetPrimType())) ||
             (subKind == kTypeStruct && GetPrimType() == PTY_agg);
    }
    case kTypePointer:
      return IsPrimitivePoint(GetPrimType());
    case kTypeParam:
    case kTypeGenericInstant:
      return true;
    default:
      return false;
  }
}

bool IreadNode::IsVolatile() const {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  ASSERT(type != nullptr, "null ptr check");
  ASSERT(type->IsMIRPtrType(), "type of iread should be pointer type");
  return static_cast<MIRPtrType*>(type)->IsPointedTypeVolatile(fieldID);
}

bool AddrofNode::IsVolatile(const MIRModule &mod) const {
  auto *symbol = mod.CurFunction()->GetLocalOrGlobalSymbol(stIdx);
  ASSERT(symbol != nullptr, "null ptr check on symbol");
  return symbol->IsVolatile();
}

bool DreadoffNode::IsVolatile(const MIRModule &mod) const {
  auto *symbol = mod.CurFunction()->GetLocalOrGlobalSymbol(stIdx);
  ASSERT(symbol != nullptr, "null ptr check on symbol");
  return symbol->IsVolatile();
}

bool DassignNode::AssigningVolatile(const MIRModule &mod) const {
  auto *symbol = mod.CurFunction()->GetLocalOrGlobalSymbol(stIdx);
  ASSERT(symbol != nullptr, "null ptr check on symbol");
  return symbol->IsVolatile();
}

bool IassignNode::AssigningVolatile() const {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  ASSERT(type != nullptr, "null ptr check");
  ASSERT(type->IsMIRPtrType(), "type of iassign should be pointer type");
  return static_cast<MIRPtrType*>(type)->IsPointedTypeVolatile(fieldID);
}

void BlockNode::AddStatement(StmtNode *stmt) {
  ASSERT(stmt != nullptr, "null ptr check");
  stmtNodeList.push_back(stmt);
}

void BlockNode::AppendStatementsFromBlock(BlockNode &blk) {
  if (blk.GetStmtNodes().empty()) {
    return;
  }
  stmtNodeList.splice(stmtNodeList.end(), blk.GetStmtNodes());
}

/// Insert stmt as the first
void BlockNode::InsertFirst(StmtNode *stmt) {
  ASSERT(stmt != nullptr, "stmt is null");
  stmtNodeList.push_front(stmt);
}

/// Insert stmt as the last
void BlockNode::InsertLast(StmtNode *stmt) {
  ASSERT(stmt != nullptr, "stmt is null");
  stmtNodeList.push_back(stmt);
}

void BlockNode::ReplaceStmtWithBlock(StmtNode &stmtNode, BlockNode &blk) {
  stmtNodeList.splice(&stmtNode, blk.GetStmtNodes());
  stmtNodeList.erase(&stmtNode);
  stmtNode.SetNext(blk.GetLast()->GetNext());
}

void BlockNode::ReplaceStmt1WithStmt2(const StmtNode *stmtNode1, StmtNode *stmtNode2) {
  if (stmtNode2 == stmtNode1) {
    // do nothing
  } else if (stmtNode2 == nullptr) {
    // delete stmtNode1
    stmtNodeList.erase(stmtNode1);
  } else {
    // replace stmtNode1 with stmtNode2
    stmtNodeList.insert(stmtNode1, stmtNode2);
    (void)stmtNodeList.erase(stmtNode1);
  }
}

// remove sstmtNode1 from block
void BlockNode::RemoveStmt(StmtNode *stmtNode1) {
  ASSERT(stmtNode1 != nullptr, "delete a null stmtment");
  (void)stmtNodeList.erase(stmtNode1);
}

/// Insert stmtNode2 before stmtNode1 in current block.
void BlockNode::InsertBefore(const StmtNode *stmtNode1, StmtNode *stmtNode2) {
  stmtNodeList.insert(stmtNode1, stmtNode2);
}

/// Insert stmtNode2 after stmtNode1 in current block.
void BlockNode::InsertAfter(StmtNode *stmtNode1, StmtNode *stmtNode2) {
  stmtNodeList.insertAfter(stmtNode1, stmtNode2);
}

// insert all the stmts in inblock to the current block after stmt1
void BlockNode::InsertBlockAfter(BlockNode &inblock, StmtNode *stmt1) {
  ASSERT(stmt1 != nullptr, "null ptr check");
  ASSERT(!inblock.IsEmpty(), "NYI");
  stmtNodeList.splice(stmt1, inblock.GetStmtNodes());
}

void BaseNode::DumpBase(int32 indent) const {
  PrintIndentation(indent);
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
}

void CatchNode::Dump(int32 indent) const {
  PrintIndentation(indent);
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " {";
  size_t size = exceptionTyIdxVec.size();
  for (size_t i = 0; i < size; ++i) {
    LogInfo::MapleLogger() << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(exceptionTyIdxVec[i])->Dump(indent + 1);
  }
  LogInfo::MapleLogger() << " }\n";
}

void CatchNode::Dump() const {
  this->BaseNode::Dump();
}

void CppCatchNode::Dump(int32 indent) const {
  PrintIndentation(indent);
  LogInfo::MapleLogger() << kOpcodeInfo.GetName(op);
  if (exceptionTyIdx.GetIdx() != 0) {
    LogInfo::MapleLogger() << " { ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(exceptionTyIdx)->Dump(indent + 1);
    LogInfo::MapleLogger() << " }";
  }
  LogInfo::MapleLogger() << std::endl;
}

void UnaryNode::DumpOpnd(const MIRModule&, int indent) const {
  LogInfo::MapleLogger() << " (";
  uOpnd->Dump(indent);
  LogInfo::MapleLogger() << ")";
}

void UnaryNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  DumpOpnd(*theMIRModule, indent);
}

void TypeCvtNode::Dump(int32 indent) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " ";
  LogInfo::MapleLogger() << GetPrimTypeName(GetPrimType()) << " " << GetPrimTypeName(FromType());
  DumpOpnd(*theMIRModule, indent);
}

void RetypeNode::Dump(int32 indent) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " ";
  LogInfo::MapleLogger() << GetPrimTypeName(GetPrimType()) << " ";
  MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (ty->GetKind() == kTypeScalar) {
    LogInfo::MapleLogger() << "<";
    ty->Dump(indent + 1);
    LogInfo::MapleLogger() << ">";
  } else {
    ty->Dump(indent + 1);
  }
  DumpOpnd(*theMIRModule, indent);
}

void ExtractbitsNode::Dump(int32 indent) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  if (GetOpCode() == OP_extractbits) {
    LogInfo::MapleLogger() << " " << static_cast<int32>(bitsOffset) << " " << static_cast<int32>(bitsSize);
  } else {
    LogInfo::MapleLogger() << " " << static_cast<int32>(bitsSize);
  }
  DumpOpnd(*theMIRModule, indent);
}

void IreadNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
  LogInfo::MapleLogger() << " " << fieldID;
  DumpOpnd(*theMIRModule, indent);
}

void IreadoffNode::Dump(int32 indent) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " " << offset;
  DumpOpnd(*theMIRModule, indent);
}

void IreadFPoffNode::Dump(int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " " << offset;
}

void BinaryNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  BinaryOpnds::Dump(indent);
}

void BinaryNode::Dump() const {
  this->BaseNode::Dump();
}

void BinaryOpnds::Dump(int32 indent) const {
  LogInfo::MapleLogger() << " (";
  if (bOpnd[0]->IsLeaf() && bOpnd[1]->IsLeaf()) {
    bOpnd[0]->Dump(0);
    LogInfo::MapleLogger() << ", ";
    bOpnd[1]->Dump(0);
  } else {
    LogInfo::MapleLogger() << '\n';
    PrintIndentation(indent + 1);
    bOpnd[0]->Dump(indent + 1);
    LogInfo::MapleLogger() << ",\n";
    PrintIndentation(indent + 1);
    bOpnd[1]->Dump(indent + 1);
  }
  LogInfo::MapleLogger() << ")";
}

void ResolveFuncNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  LogInfo::MapleLogger() << " &" << func->GetName();
  BinaryOpnds::Dump(indent);
}

void CompareNode::Dump(int32 indent) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " " << GetPrimTypeName(opndType);
  BinaryOpnds::Dump(indent);
}

void CompareNode::Dump() const {
  this->BaseNode::Dump();
}

void DepositbitsNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  LogInfo::MapleLogger() << " " << static_cast<int32>(bitsOffset) << " " << static_cast<int32>(bitsSize) << " (";
  if (GetBOpnd(0)->IsLeaf() && GetBOpnd(1)->IsLeaf()) {
    GetBOpnd(0)->Dump(0);
    LogInfo::MapleLogger() << ", ";
    GetBOpnd(1)->Dump(0);
  } else {
    LogInfo::MapleLogger() << '\n';
    PrintIndentation(indent + 1);
    GetBOpnd(0)->Dump(indent + 1);
    LogInfo::MapleLogger() << ",\n";
    PrintIndentation(indent + 1);
    GetBOpnd(1)->Dump(indent + 1);
  }
  LogInfo::MapleLogger() << ")";
}

void TernaryNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  LogInfo::MapleLogger() << " (";
  if (topnd[0]->IsLeaf() && topnd[1]->IsLeaf() && topnd[2]->IsLeaf()) {
    topnd[0]->Dump(0);
    LogInfo::MapleLogger() << ", ";
    topnd[1]->Dump(0);
    LogInfo::MapleLogger() << ", ";
    topnd[2]->Dump(0);
  } else {
    LogInfo::MapleLogger() << '\n';
    PrintIndentation(indent + 1);
    topnd[0]->Dump(indent + 1);
    LogInfo::MapleLogger() << ",\n";
    PrintIndentation(indent + 1);
    topnd[1]->Dump(indent + 1);
    LogInfo::MapleLogger() << ",\n";
    PrintIndentation(indent + 1);
    topnd[2]->Dump(indent + 1);
  }
  LogInfo::MapleLogger() << ")";
}

void NaryOpnds::Dump(int32 indent) const {
  LogInfo::MapleLogger() << " (";
  if (GetNopndSize() == 0) {
    LogInfo::MapleLogger() << ")";
    return;
  }
  if (GetNopndSize() == 1) {
    GetNopndAt(0)->Dump(indent);
  } else {
    bool allisLeaf = true;
    for (size_t i = 0; i < GetNopndSize(); ++i)
      if (!GetNopndAt(i)->IsLeaf()) {
        allisLeaf = false;
        break;
      }
    if (allisLeaf) {
      GetNopndAt(0)->Dump(0);
      for (size_t i = 1; i < GetNopndSize(); ++i) {
        LogInfo::MapleLogger() << ", ";
        GetNopndAt(i)->Dump(0);
      }
    } else {
      LogInfo::MapleLogger() << '\n';
      PrintIndentation(indent + 1);
      GetNopndAt(0)->Dump(indent + 1);
      for (size_t i = 1; i < GetNopndSize(); ++i) {
        LogInfo::MapleLogger() << ",\n";
        PrintIndentation(indent + 1);
        GetNopndAt(i)->Dump(indent + 1);
      }
    }
  }
  LogInfo::MapleLogger() << ")";
}

bool NaryOpnds::VerifyOpnds() const {
  bool nOpndsVerify = true;
  for (size_t i = 0; i < GetNopndSize(); ++i) {
    if (!GetNopndAt(i)->Verify()) {
      nOpndsVerify = false;
      break;
    }
  }
  return nOpndsVerify;
}

void NaryNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  NaryOpnds::Dump(indent);
}

const MIRType *ArrayNode::GetArrayType(const TypeTable &tt) const {
  const MIRType *type = tt.GetTypeFromTyIdx(tyIdx);
  CHECK_FATAL(type->GetKind() == kTypePointer, "expect array type pointer");
  const auto *pointType = static_cast<const MIRPtrType*>(type);
  return tt.GetTypeFromTyIdx(pointType->GetPointedTyIdx());
}
MIRType *ArrayNode::GetArrayType(const TypeTable &tt) {
  return const_cast<MIRType*>(const_cast<const ArrayNode*>(this)->GetArrayType(tt));
}

const BaseNode *ArrayNode::GetDim(const MIRModule &mod, TypeTable &tt, int i) const {
  const auto *arrayType = static_cast<const MIRArrayType*>(GetArrayType(tt));
  auto *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
      i, *tt.GetTypeFromTyIdx(arrayType->GetElemTyIdx()));
  return mod.CurFuncCodeMemPool()->New<ConstvalNode>(mirConst);
}
BaseNode *ArrayNode::GetDim(const MIRModule &mod, TypeTable &tt, int i) {
  return const_cast<BaseNode*>(const_cast<const ArrayNode*>(this)->GetDim(mod, tt, i));
}

void ArrayNode::Dump(int32 indent) const {
  PrintIndentation(0);
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " ";
  if (boundsCheck) {
    LogInfo::MapleLogger() << "1 ";
  } else {
    LogInfo::MapleLogger() << "0 ";
  }
  LogInfo::MapleLogger() << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
  NaryOpnds::Dump(indent);
}

bool ArrayNode::IsSameBase(ArrayNode *arry) {
  ASSERT(arry != nullptr, "null ptr check");
  if (arry == this) {
    return true;
  }
  BaseNode *curBase = this->GetBase();
  BaseNode *otherBase = arry->GetBase();
  if (curBase->GetOpCode() != OP_addrof || otherBase->GetOpCode() != OP_addrof) {
    return false;
  }
  return static_cast<AddrofNode*>(curBase)->GetStIdx() == static_cast<AddrofNode*>(otherBase)->GetStIdx();
}

void IntrinsicopNode::Dump(int32 indent) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  if (GetOpCode() == OP_intrinsicopwithtype) {
    LogInfo::MapleLogger() << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(indent + 1);
  }
  LogInfo::MapleLogger() << " " << GetIntrinsicName(GetIntrinsic());
  NaryOpnds::Dump(indent);
}

void ConstvalNode::Dump(int32) const {
  if (GetConstVal()->GetType().GetKind() != kTypePointer) {
    BaseNode::DumpBase(0);
    LogInfo::MapleLogger() << " ";
  }
  GetConstVal()->Dump();
}

void ConststrNode::Dump(int32) const {
  BaseNode::DumpBase(0);
  const std::string kStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(UStrIdx(strIdx));
  PrintString(kStr);
}

void Conststr16Node::Dump(int32) const {
  BaseNode::DumpBase(0);
  const std::u16string kStr16 = GlobalTables::GetU16StrTable().GetStringFromStrIdx(U16StrIdx(strIdx));
  // UTF-16 string are dumped as UTF-8 string in mpl to keep the printable chars in ascii form
  std::string str;
  (void)namemangler::UTF16ToUTF8(str, kStr16);
  PrintString(str);
}

void SizeoftypeNode::Dump(int32) const {
  BaseNode::DumpBase(0);
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
}

void FieldsDistNode::Dump(int32) const {
  BaseNode::DumpBase(0);
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
  LogInfo::MapleLogger() << " " << fieldID1 << " " << fieldID2;
}

void AddrofNode::Dump(int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(GetStIdx());
  LogInfo::MapleLogger() << (GetStIdx().Islocal() ? " %" : " $");
  LogInfo::MapleLogger() << st->GetName();
  if (fieldID != 0) {
    LogInfo::MapleLogger() << " " << fieldID;
  }
}

void DreadoffNode::Dump(int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(stIdx);
  LogInfo::MapleLogger() << (stIdx.Islocal() ? " %" : " $");
  LogInfo::MapleLogger() << st->GetName();
  LogInfo::MapleLogger() << " " << offset;
}

void RegreadNode::Dump(int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  if (regIdx >= 0) {
    LogInfo::MapleLogger()
        << " %" << theMIRModule->CurFunction()->GetPregTab()->PregFromPregIdx(static_cast<uint32>(regIdx))->GetPregNo();
    return;
  }
  LogInfo::MapleLogger() << " %%";
  switch (regIdx) {
    case -kSregSp:
      LogInfo::MapleLogger() << "SP";
      break;
    case -kSregFp:
      LogInfo::MapleLogger() << "FP";
      break;
    case -kSregGp:
      LogInfo::MapleLogger() << "GP";
      break;
    case -kSregThrownval:
      LogInfo::MapleLogger() << "thrownval";
      break;
    case -kSregMethodhdl:
      LogInfo::MapleLogger() << "methodhdl";
      break;
    default:
      int32 retValIdx = (-regIdx) - kSregRetval0;
      LogInfo::MapleLogger() << "retval" << retValIdx;
      break;
  }
}

void AddroffuncNode::Dump(int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  LogInfo::MapleLogger() << " &" << GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx())->GetName();
}

void AddroflabelNode::Dump(int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName((LabelIdx)offset);
}

void StmtNode::DumpBase(int32 indent) const {
  if (srcPosition.FileNum() != 0 && srcPosition.LineNum() != 0 && srcPosition.LineNum() != lastPrintedLineNum &&
      theMIRModule->CurFunction()->WithLocInfo()) {
    LogInfo::MapleLogger() << "LOC " << srcPosition.FileNum() << " " << srcPosition.LineNum() << '\n';
    lastPrintedLineNum = srcPosition.LineNum();
  }
  PrintIndentation(indent);
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name;
}

void StmtNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << '\n';
}

void StmtNode::Dump() const {
  this->BaseNode::Dump();
}

// Get the next stmt skip the comment stmt.
StmtNode *StmtNode::GetRealNext() const {
  StmtNode *stmt = this->GetNext();
  while (stmt != nullptr) {
    if (stmt->GetOpCode() != OP_comment) {
      break;
    }
    stmt = stmt->GetNext();
  }
  return stmt;
}

// insert this before pos
void StmtNode::InsertAfterThis(StmtNode &pos) {
  this->SetNext(&pos);
  if (pos.GetPrev()) {
    this->SetPrev(pos.GetPrev());
    pos.GetPrev()->SetNext(this);
  }
  pos.SetPrev(this);
}

// insert stmtnode after pos
void StmtNode::InsertBeforeThis(StmtNode &pos) {
  this->SetPrev(&pos);
  if (pos.GetNext()) {
    this->SetNext(pos.GetNext());
    pos.GetNext()->SetPrev(this);
  }
  pos.SetNext(this);
}

void DassignNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(stIdx);
  LogInfo::MapleLogger() << (st->IsLocal() ? " %" : " $");
  LogInfo::MapleLogger() << st->GetName() << " " << fieldID;
  LogInfo::MapleLogger() << " (";
  if (GetRHS() != nullptr) {
    GetRHS()->Dump(indent + 1);
  } else {
    LogInfo::MapleLogger() << "/*empty-rhs*/";
  }
  LogInfo::MapleLogger() << ")\n";
}

void DassignoffNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " " << GetPrimTypeName(GetPrimType());
  const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(stIdx);
  LogInfo::MapleLogger() << (st->IsLocal() ? " %" : " $");
  LogInfo::MapleLogger() << st->GetName() << " " << offset;
  LogInfo::MapleLogger() << " (";
  if (GetRHS() != nullptr) {
    GetRHS()->Dump(indent + 1);
  } else {
    LogInfo::MapleLogger() << "/*empty-rhs*/";
  }
  LogInfo::MapleLogger() << ")\n";
}

void RegassignNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " " << GetPrimTypeName(GetPrimType());
  if (regIdx >= 0) {
    LogInfo::MapleLogger()
        << " %" << theMIRModule->CurFunction()->GetPregTab()->PregFromPregIdx(static_cast<uint32>(regIdx))->GetPregNo();
  } else {
    LogInfo::MapleLogger() << " %%";
    switch (regIdx) {
      case -kSregSp:
        LogInfo::MapleLogger() << "SP";
        break;
      case -kSregFp:
        LogInfo::MapleLogger() << "FP";
        break;
      case -kSregGp:
        LogInfo::MapleLogger() << "GP";
        break;
      case -kSregThrownval:
        LogInfo::MapleLogger() << "thrownval";
        break;
      case -kSregMethodhdl:
        LogInfo::MapleLogger() << "methodhdl";
        break;
      case -kSregRetval0:
        LogInfo::MapleLogger() << "retval0";
        break;
      // no default
      default:
        break;
    }
  }
  LogInfo::MapleLogger() << " (";
  UnaryStmtNode::Opnd(0)->Dump(indent + 1);
  LogInfo::MapleLogger() << ")\n";
}

void IassignNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
  LogInfo::MapleLogger() << " " << fieldID;
  LogInfo::MapleLogger() << " (";
  if (addrExpr->IsLeaf() && rhs->IsLeaf()) {
    addrExpr->Dump(0);
    LogInfo::MapleLogger() << ", ";
    rhs->Dump(0);
  } else {
    LogInfo::MapleLogger() << '\n';
    PrintIndentation(indent + 1);
    addrExpr->Dump(indent + 1);
    LogInfo::MapleLogger() << ", \n";
    PrintIndentation(indent + 1);
    rhs->Dump(indent + 1);
  }
  LogInfo::MapleLogger() << ")\n";
}

void IassignoffNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " " << GetPrimTypeName(GetPrimType()) << " " << offset;
  BinaryOpnds::Dump(indent);
  LogInfo::MapleLogger() << '\n';
}

void IassignFPoffNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " " << GetPrimTypeName(GetPrimType()) << " " << offset;
  DumpOpnd(*theMIRModule, indent);
  LogInfo::MapleLogger() << '\n';
}

void GotoNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  if (offset == 0) {
    LogInfo::MapleLogger() << '\n';
  } else {
    LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName((LabelIdx)offset) << '\n';
  }
}

void JsTryNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  if (catchOffset == 0) {
    LogInfo::MapleLogger() << " 0";
  } else {
    LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName((LabelIdx)catchOffset);
  }
  if (finallyOffset == 0) {
    LogInfo::MapleLogger() << " 0\n";
  } else {
    LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName((LabelIdx)finallyOffset) << '\n';
  }
}

void TryNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " {";
  for (size_t i = 0; i < offsets.size(); ++i) {
    uint32 offset = offsets[i];
    LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName((LabelIdx)offset);
  }
  LogInfo::MapleLogger() << " }\n";
}

void TryNode::Dump() const {
  this->BaseNode::Dump();
}

void CondGotoNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName((LabelIdx)offset);
  LogInfo::MapleLogger() << " (";
  Opnd(0)->Dump(indent);
  LogInfo::MapleLogger() << ")\n";
}

void SwitchNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " (";
  switchOpnd->Dump(indent);
  if (defaultLabel == 0) {
    LogInfo::MapleLogger() << ") 0 {";
  } else {
    LogInfo::MapleLogger() << ") @" << theMIRModule->CurFunction()->GetLabelName(defaultLabel) << " {";
  }
  for (auto it = switchTable.begin(); it != switchTable.end(); it++) {
    LogInfo::MapleLogger() << '\n';
    PrintIndentation(indent + 1);
    LogInfo::MapleLogger() << std::hex << "0x" << it->first << std::dec;
    LogInfo::MapleLogger() << ": goto @" << theMIRModule->CurFunction()->GetLabelName(it->second);
  }
  LogInfo::MapleLogger() << " }\n";
}

void RangeGotoNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " (";
  Opnd(0)->Dump(indent);
  LogInfo::MapleLogger() << ") " << tagOffset << " {";
  for (auto it = rangegotoTable.begin(); it != rangegotoTable.end(); it++) {
    LogInfo::MapleLogger() << '\n';
    PrintIndentation(indent + 1);
    LogInfo::MapleLogger() << std::hex << "0x" << it->first << std::dec;
    LogInfo::MapleLogger() << ": goto @" << theMIRModule->CurFunction()->GetLabelName(it->second);
  }
  LogInfo::MapleLogger() << " }\n";
}

void MultiwayNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " (";
  multiWayOpnd->Dump(indent);
  if (defaultLabel == 0) {
    LogInfo::MapleLogger() << ") 0 {";
  } else {
    LogInfo::MapleLogger() << ") @" << theMIRModule->CurFunction()->GetLabelName(defaultLabel) << " {";
  }
  for (auto it = multiWayTable.begin(); it != multiWayTable.end(); it++) {
    LogInfo::MapleLogger() << '\n';
    PrintIndentation(indent);
    LogInfo::MapleLogger() << " (";
    it->first->Dump(indent + 1);
    LogInfo::MapleLogger() << "): goto @" << theMIRModule->CurFunction()->GetLabelName(it->second);
  }
  LogInfo::MapleLogger() << " }\n";
}

void UnaryStmtNode::DumpOpnd(const MIRModule&, int indent) const {
  LogInfo::MapleLogger() << " (";
  uOpnd->Dump(indent);
  LogInfo::MapleLogger() << ")";
}

void UnaryStmtNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " (";
  if (uOpnd) {
    uOpnd->Dump(indent);
  }
  LogInfo::MapleLogger() << ")\n";
}

void UnaryStmtNode::Dump() const {
  this->BaseNode::Dump();
}

void GCMallocNode::Dump(int32) const {
  BaseNode::DumpBase(0);
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
}

void JarrayMallocNode::Dump(int32 indent) const {
  BaseNode::DumpBase(0);
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0, false);
  DumpOpnd(*theMIRModule, indent);
}

void IfStmtNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  LogInfo::MapleLogger() << " (";
  Opnd()->Dump(indent);
  LogInfo::MapleLogger() << ")";
  thenPart->Dump(indent);
  if (elsePart) {
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "else {\n";
    for (auto &stmt : elsePart->GetStmtNodes()) {
      stmt.Dump(indent + 1);
    }
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "}\n";
  }
}

void WhileStmtNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  if (GetOpCode() == OP_while) {
    LogInfo::MapleLogger() << " (";
    Opnd(0)->Dump(indent);
    LogInfo::MapleLogger() << ")";
    body->Dump(indent);
  } else {  // OP_dowhile
    LogInfo::MapleLogger() << " {\n";
    for (auto &stmt : body->GetStmtNodes()) {
      stmt.Dump(indent + 1);
    }
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "} (";
    Opnd(0)->Dump(indent);
    LogInfo::MapleLogger() << ")\n";
  }
}

void DoloopNode::DumpDoVar(const MIRModule &mod) const {
  if (isPreg) {
    LogInfo::MapleLogger() << " %"
                           << mod.CurFunction()->GetPregTab()->PregFromPregIdx(doVarStIdx.FullIdx())->GetPregNo()
                           << " (\n";
  } else {
    const MIRSymbol *st = mod.CurFunction()->GetLocalOrGlobalSymbol(doVarStIdx);
    LogInfo::MapleLogger() << " %" << st->GetName() << " (\n";
  }
}

void DoloopNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  DumpDoVar(*theMIRModule);
  PrintIndentation(indent + 1);
  startExpr->Dump(indent + 1);
  LogInfo::MapleLogger() << ",\n";
  PrintIndentation(indent + 1);
  condExpr->Dump(indent + 1);
  LogInfo::MapleLogger() << ",\n";
  PrintIndentation(indent + 1);
  incrExpr->Dump(indent + 1);
  LogInfo::MapleLogger() << ")";
  doBody->Dump(indent + 1);
}

void ForeachelemNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(elemStIdx);
  ASSERT(st != nullptr, "null ptr check");
  LogInfo::MapleLogger() << " %" << st->GetName();
  st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(arrayStIdx);
  ASSERT(st != nullptr, "null ptr check");
  LogInfo::MapleLogger() << (arrayStIdx.Islocal() ? " %" : " $");
  LogInfo::MapleLogger() << st->GetName();
  loopBody->Dump(indent + 1);
}

void BinaryStmtNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  BinaryOpnds::Dump(indent);
  LogInfo::MapleLogger() << '\n';
}

void AssertStmtNode::Dump(int32 indent) const {
  BinaryStmtNode::Dump(indent);
}

void NaryStmtNode::Dump(int32 indent) const {
  StmtNode::DumpBase(indent);
  NaryOpnds::Dump(indent);
  LogInfo::MapleLogger() << '\n';
}

void DumpCallReturns(const MIRModule &mod, CallReturnVector nrets, int32 indent) {
  const MIRFunction *mirFunc = mod.CurFunction();
  if (nrets.empty()) {
    LogInfo::MapleLogger() << " {}\n";
    return;
  } else if (nrets.size() == 1) {
    StIdx stIdx = nrets.begin()->first;
    RegFieldPair regFieldPair = nrets.begin()->second;
    if (!regFieldPair.IsReg()) {
      const MIRSymbol *st = mirFunc->GetLocalOrGlobalSymbol(stIdx);
      ASSERT(st != nullptr, "st is null");
      FieldID fieldID = regFieldPair.GetFieldID();
      LogInfo::MapleLogger() << " { dassign ";
      LogInfo::MapleLogger() << (stIdx.Islocal() ? "%" : "$");
      LogInfo::MapleLogger() << st->GetName() << " " << fieldID << " }\n";
      return;
    } else {
      PregIdx regIdx = regFieldPair.GetPregIdx();
      const MIRPreg *mirPreg = mirFunc->GetPregItem(static_cast<PregIdx>(regIdx));
      ASSERT(mirPreg != nullptr, "mirPreg is null");
      LogInfo::MapleLogger() << " { regassign";
      LogInfo::MapleLogger() << " " << GetPrimTypeName(mirPreg->GetPrimType());
      LogInfo::MapleLogger() << " %" << mirPreg->GetPregNo() << "}\n";
      return;
    }
  }
  LogInfo::MapleLogger() << " {\n";
  for (auto it = nrets.begin(); it != nrets.end(); it++) {
    PrintIndentation(indent + 2);
    StIdx stIdx = (it)->first;
    RegFieldPair regFieldPair = it->second;
    if (!regFieldPair.IsReg()) {
      FieldID fieldID = regFieldPair.GetFieldID();
      LogInfo::MapleLogger() << "dassign";
      const MIRSymbol *st = mirFunc->GetLocalOrGlobalSymbol(stIdx);
      ASSERT(st != nullptr, "st is null");
      LogInfo::MapleLogger() << (stIdx.Islocal() ? " %" : " $");
      LogInfo::MapleLogger() << st->GetName() << " " << fieldID << '\n';
    } else {
      PregIdx regIdx = regFieldPair.GetPregIdx();
      const MIRPreg *mirPreg = mirFunc->GetPregItem(static_cast<PregIdx>(regIdx));
      ASSERT(mirPreg != nullptr, "mirPreg is null");
      LogInfo::MapleLogger() << "regassign"
                             << " " << GetPrimTypeName(mirPreg->GetPrimType());
      LogInfo::MapleLogger() << " %" << mirPreg->GetPregNo() << '\n';
    }
  }
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << "}\n";
}

// iread expr has sideeffect, may cause derefference error
bool HasIreadExpr(const BaseNode *expr) {
  if (expr->GetOpCode() == OP_iread) {
    return true;
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    if (HasIreadExpr(expr->Opnd(i))) {
      return true;
    }
  }
  return false;
}

MIRType *CallNode::GetCallReturnType() {
  if (!kOpcodeInfo.IsCallAssigned(GetOpCode())) {
    return nullptr;
  }
  ASSERT(GlobalTables::GetFunctionTable().GetFuncTable().empty() == false, "container check");
  MIRFunction *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  return mirFunc->GetReturnType();
}
const MIRSymbol *CallNode::GetCallReturnSymbol(const MIRModule &mod) const {
  if (!kOpcodeInfo.IsCallAssigned(GetOpCode())) {
    return nullptr;
  }
  const CallReturnVector &nRets = this->GetReturnVec();
  if (nRets.size() == 1) {
    StIdx stIdx = nRets.begin()->first;
    RegFieldPair regFieldPair = nRets.begin()->second;
    if (!regFieldPair.IsReg()) {
      const MIRFunction *mirFunc = mod.CurFunction();;
      const MIRSymbol *st = mirFunc->GetLocalOrGlobalSymbol(stIdx);
      return st;
    }
  }
  return nullptr;
}


void CallNode::Dump(int32 indent, bool newline) const {
  StmtNode::DumpBase(indent);
  if (tyIdx != 0u) {
    LogInfo::MapleLogger() << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(indent + 1);
  }
  CHECK(puIdx < GlobalTables::GetFunctionTable().GetFuncTable().size(), "index out of range in CallNode::Dump");
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  LogInfo::MapleLogger() << " &" << func->GetName();
  NaryOpnds::Dump(indent);
  if (kOpcodeInfo.IsCallAssigned(GetOpCode())) {
    DumpCallReturns(*theMIRModule, this->GetReturnVec(), indent);
  } else if (newline) {
    LogInfo::MapleLogger() << '\n';
  }
}

MIRType *IcallNode::GetCallReturnType() {
  return GlobalTables::GetTypeTable().GetTypeFromTyIdx(retTyIdx);
}

void IcallNode::Dump(int32 indent, bool newline) const {
  StmtNode::DumpBase(indent);
  NaryOpnds::Dump(indent);
  if (kOpcodeInfo.IsCallAssigned(GetOpCode())) {
    DumpCallReturns(*theMIRModule, this->returnValues, indent);
  } else if (newline) {
    LogInfo::MapleLogger() << '\n';
  }
}

MIRType *IntrinsiccallNode::GetCallReturnType() {
  CHECK_FATAL(intrinsic < INTRN_LAST, "Index out of bound in IntrinsiccallNode::GetCallReturnType");
  IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrinsic];
  return intrinDesc->GetReturnType();
}

void IntrinsiccallNode::Dump(int32 indent, bool newline) const {
  StmtNode::DumpBase(indent);
  if (tyIdx != 0u) {
    LogInfo::MapleLogger() << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(indent + 1);
  }
  if (GetOpCode() == OP_intrinsiccall || GetOpCode() == OP_intrinsiccallassigned ||
      GetOpCode() == OP_intrinsiccallwithtype || GetOpCode() == OP_intrinsiccallwithtypeassigned) {
    LogInfo::MapleLogger() << " " << GetIntrinsicName(intrinsic);
  } else {
    LogInfo::MapleLogger() << " " << intrinsic;
  }
  NaryOpnds::Dump(indent);
  if (kOpcodeInfo.IsCallAssigned(GetOpCode())) {
    DumpCallReturns(*theMIRModule, this->GetReturnVec(), indent);
  } else if (newline) {
    LogInfo::MapleLogger() << '\n';
  }
}

void CallinstantNode::Dump(int32 indent, bool newline) const {
  StmtNode::DumpBase(indent);
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(GetPUIdx());
  LogInfo::MapleLogger() << " &" << func->GetName();
  MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(instVecTyIdx);
  LogInfo::MapleLogger() << "<";
  auto *instVecType = static_cast<MIRInstantVectorType*>(ty);
  instVecType->Dump(indent);
  LogInfo::MapleLogger() << ">";
  NaryOpnds::Dump(indent);
  if (kOpcodeInfo.IsCallAssigned(GetOpCode())) {
    DumpCallReturns(*theMIRModule, this->GetReturnVec(), indent);
  } else if (newline) {
    LogInfo::MapleLogger() << '\n';
  }
}

void BlockNode::Dump(int32 indent, const MIRSymbolTable *theSymTab, MIRPregTable *thePregTab,
                     bool withInfo, bool isFuncbody) const {
  if (!withInfo) {
    LogInfo::MapleLogger() << " {\n";
  }
  // output puid for debugging purpose
  if (isFuncbody) {
    theMIRModule->CurFunction()->DumpFuncBody(indent);
    if (theSymTab != nullptr || thePregTab != nullptr) {
      // print the locally declared type names
      if (theMIRModule->CurFunction()->HaveTypeNameTab()) {
        for (auto it : theMIRModule->CurFunction()->GetGStrIdxToTyIdxMap()) {
          const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(it.first);
          MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second);
          PrintIndentation(indent + 1);
          LogInfo::MapleLogger() << "type %" << name << " ";
          if (type->GetKind() != kTypeByName) {
            type->Dump(indent + 2, true);
          } else {
            type->Dump(indent + 2);
          }
          LogInfo::MapleLogger() << '\n';
        }
      }
      // print the locally declared variables
      theSymTab->Dump(true, indent + 1);
      thePregTab->DumpPregsWithTypes(indent + 1);
    }
    LogInfo::MapleLogger() << '\n';
    if (theMIRModule->CurFunction()->NeedEmitAliasInfo()) {
      for (std::pair<GStrIdx, MIRAliasVars> it : theMIRModule->CurFunction()->GetAliasVarMap()) {
        LogInfo::MapleLogger() << "ALIAS %" << GlobalTables::GetStrTable().GetStringFromStrIdx(it.first) << " %"
                               << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.memPoolStrIdx) << " ";
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second.tyIdx)->Dump(0);
        if (it.second.sigStrIdx) {
          LogInfo::MapleLogger() << " \""
                                 << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.sigStrIdx) << "\"";
        }
        LogInfo::MapleLogger() << '\n';
      }
    }
  }
  if (srcPosition.FileNum() != 0 && srcPosition.LineNum() != 0 && srcPosition.LineNum() != lastPrintedLineNum &&
      theMIRModule->CurFunction()->WithLocInfo()) {
    LogInfo::MapleLogger() << "LOC " << srcPosition.FileNum() << " " << srcPosition.LineNum() << '\n';
    lastPrintedLineNum = srcPosition.LineNum();
  }
  for (auto &stmt : GetStmtNodes()) {
    stmt.Dump(indent + 1);
  }
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "}\n";
}

void LabelNode::Dump(int32) const {
  if (srcPosition.FileNum() != 0 && srcPosition.LineNum() != 0 && srcPosition.LineNum() != lastPrintedLineNum &&
      theMIRModule->CurFunction()->WithLocInfo()) {
    LogInfo::MapleLogger() << "LOC " << srcPosition.FileNum() << " " << srcPosition.LineNum() << '\n';
    lastPrintedLineNum = srcPosition.LineNum();
  }
  LogInfo::MapleLogger() << "@" << theMIRModule->CurFunction()->GetLabelName(labelIdx) << " ";
}

void CommentNode::Dump(int32 indent) const {
  if (srcPosition.FileNum() != 0 && srcPosition.LineNum() != 0 && srcPosition.LineNum() != lastPrintedLineNum &&
      theMIRModule->CurFunction()->WithLocInfo()) {
    LogInfo::MapleLogger() << "LOC " << srcPosition.FileNum() << " " << srcPosition.LineNum() << '\n';
    lastPrintedLineNum = srcPosition.LineNum();
  }
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "#" << comment << '\n';
}

void EmitStr(const MapleString &mplStr) {
  const char *str = mplStr.c_str();
  size_t len = mplStr.length();
  LogInfo::MapleLogger() << "\"";

  // don't expand special character; convert all \s to \\s in string
  for (size_t i = 0; i < len; ++i) {
    /* Referred to GNU AS: 3.6.1.1 Strings */
    constexpr int kBufSize = 5;
    constexpr int kFirstChar = 0;
    constexpr int kSecondChar = 1;
    constexpr int kThirdChar = 2;
    constexpr int kLastChar = 4;
    char buf[kBufSize];
    if (isprint(*str)) {
      buf[kFirstChar] = *str;
      buf[kSecondChar] = 0;
      if (*str == '\\' || *str == '\"') {
        buf[kFirstChar] = '\\';
        buf[kSecondChar] = *str;
        buf[kThirdChar] = 0;
      }
      LogInfo::MapleLogger() << buf;
    } else if (*str == '\b') {
      LogInfo::MapleLogger() << "\\b";
    } else if (*str == '\n') {
      LogInfo::MapleLogger() << "\\n";
    } else if (*str == '\r') {
      LogInfo::MapleLogger() << "\\r";
    } else if (*str == '\t') {
      LogInfo::MapleLogger() << "\\t";
    } else if (*str == '\0') {
      buf[kFirstChar] = '\\';
      buf[kSecondChar] = '0';
      buf[kThirdChar] = 0;
      LogInfo::MapleLogger() << buf;
    } else {
      /* all others, print as number */
      int ret = snprintf_s(buf, sizeof(buf), kBufSize - 1, "\\%03o", (*str) & 0xFF);
      if (ret < 0) {
        FATAL(kLncFatal, "snprintf_s failed");
      }
      buf[kLastChar] = '\0';
      LogInfo::MapleLogger() << buf;
    }
    str++;
  }

  LogInfo::MapleLogger() << "\"\n";
}

AsmNode *AsmNode::CloneTree(MapleAllocator &allocator) const {
  auto *node = allocator.GetMemPool()->New<AsmNode>(allocator, *this);
  for (size_t i = 0; i < GetNopndSize(); ++i) {
    node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
  }
  for (size_t i = 0; i < inputConstraints.size(); ++i) {
    node->inputConstraints.push_back(inputConstraints[i]);
  }
  for (size_t i = 0; i < asmOutputs.size(); ++i) {
    node->asmOutputs.push_back(asmOutputs[i]);
  }
  for (size_t i = 0; i < outputConstraints.size(); ++i) {
    node->outputConstraints.push_back(outputConstraints[i]);
  }
  for (size_t i = 0; i < clobberList.size(); ++i) {
    node->clobberList.push_back(clobberList[i]);
  }
  for (size_t i = 0; i < gotoLabels.size(); ++i) {
    node->gotoLabels.push_back(gotoLabels[i]);
  }
  node->SetNumOpnds(GetNopndSize());
  return node;
}

void AsmNode::DumpOutputs(int32 indent, std::string &uStr) const {
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << " :";
  size_t numOutputs = asmOutputs.size();

  const MIRFunction *mirFunc = theMIRModule->CurFunction();
  if (numOutputs == 0) {
    LogInfo::MapleLogger() << '\n';
  } else {
    for (size_t i = 0; i < numOutputs; i++) {
      if (i != 0) {
        PrintIndentation(indent + 2); // Increase the indent by 2 bytes.
      }
      uStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(outputConstraints[i]);
      PrintString(uStr);
      LogInfo::MapleLogger() << " ";
      StIdx stIdx = asmOutputs[i].first;
      RegFieldPair regFieldPair = asmOutputs[i].second;
      if (!regFieldPair.IsReg()) {
        FieldID fieldID = regFieldPair.GetFieldID();
        LogInfo::MapleLogger() << "dassign";
        const MIRSymbol *st = mirFunc->GetLocalOrGlobalSymbol(stIdx);
        ASSERT(st != nullptr, "st is null");
        LogInfo::MapleLogger() << (stIdx.Islocal() ? " %" : " $");
        LogInfo::MapleLogger() << st->GetName() << " " << fieldID;
      } else {
        PregIdx regIdx = regFieldPair.GetPregIdx();
        const MIRPreg *mirPreg = mirFunc->GetPregItem(static_cast<PregIdx>(regIdx));
        ASSERT(mirPreg != nullptr, "mirPreg is null");
        LogInfo::MapleLogger() << "regassign"
                               << " " << GetPrimTypeName(mirPreg->GetPrimType());
        LogInfo::MapleLogger() << " %" << mirPreg->GetPregNo();
      }
      if (i != numOutputs - 1) {
        LogInfo::MapleLogger() << ',';
      }
      LogInfo::MapleLogger() << '\n';
    }
  }
}

void AsmNode::DumpInputOperands(int32 indent, std::string &uStr) const {
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << " :";
  if (numOpnds == 0) {
    LogInfo::MapleLogger() << '\n';
  } else {
    for (size_t i = 0; i < numOpnds; i++) {
      if (i != 0) {
        PrintIndentation(indent + 2); // Increase the indent by 2 bytes.
      }
      uStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(inputConstraints[i]);
      PrintString(uStr);
      LogInfo::MapleLogger() << " (";
      GetNopndAt(i)->Dump(indent + 4); // Increase the indent by 4 bytes.
      LogInfo::MapleLogger() << ")";
      if (i != numOpnds - 1) {
        LogInfo::MapleLogger() << ',';
      }
      LogInfo::MapleLogger() << "\n";
    }
  }
}

void AsmNode::Dump(int32 indent) const {
  if (srcPosition.FileNum() != 0 && srcPosition.LineNum() != 0 && srcPosition.LineNum() != lastPrintedLineNum &&
      theMIRModule->CurFunction()->WithLocInfo()) {
    LogInfo::MapleLogger() << "LOC " << srcPosition.FileNum() << " " << srcPosition.LineNum() << '\n';
    lastPrintedLineNum = srcPosition.LineNum();
  }
  PrintIndentation(indent);
  LogInfo::MapleLogger() << kOpcodeInfo.GetName(op);
  if (GetQualifier(kASMvolatile))
    LogInfo::MapleLogger() << " volatile";
  if (GetQualifier(kASMinline))
    LogInfo::MapleLogger() << " inline";
  if (GetQualifier(kASMgoto))
    LogInfo::MapleLogger() << " goto";
  LogInfo::MapleLogger() << " { ";
  EmitStr(asmString);
  // print outputs
  std::string uStr;
  DumpOutputs(indent, uStr);
  // print input operands
  DumpInputOperands(indent, uStr);
  // print clobber list
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << " :";
  for (size_t i = 0; i < clobberList.size(); i++) {
    uStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(clobberList[i]);
    PrintString(uStr);
    if (i != clobberList.size() - 1) {
      LogInfo::MapleLogger() << ',';
    }
  }
  LogInfo::MapleLogger() << '\n';
  // print labels
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << " :";
  for (size_t i = 0; i < gotoLabels.size(); i++) {
    LabelIdx offset = gotoLabels[i];
    LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName(offset);
    if (i != gotoLabels.size() - 1) {
      LogInfo::MapleLogger() << ',';
    }
  }
  LogInfo::MapleLogger() << " }\n";
}

inline bool IntTypeVerify(PrimType pTyp) {
  return pTyp == PTY_i32 || pTyp == PTY_u32 || pTyp == PTY_i64 || pTyp == PTY_u64;
}

inline bool UnaryTypeVerify0(PrimType pTyp) {
  bool verifyResult = IntTypeVerify(pTyp);
  if (!verifyResult) {
    LogInfo::MapleLogger() << "\n#Error:result type of bnot,extractbits,sext,zext must be in [i32,u32,i64,u64]\n";
  }
  return verifyResult;
}

bool ArithResTypeVerify(PrimType pTyp) {
  switch (pTyp) {
    case PTY_i32:
    case PTY_u32:
    case PTY_i64:
    case PTY_u64:
    case PTY_f32:
    case PTY_f64:
      return true;
    case PTY_a32:
    case PTY_a64:
    case PTY_ptr:
      return theMIRModule->IsCModule();
    default:
      break;
  }

  // Arithmetic operations on all vector types are allowed
  PrimitiveType pt(pTyp);
  if (pt.IsVector()) return true;

  return false;
}

inline bool UnaryTypeVerify1(PrimType pType) {
  bool verifyResult = ArithResTypeVerify(pType);
  if (!verifyResult) {
    LogInfo::MapleLogger() << "\n#Error:result type of abs,neg must be in [i32,u32,i64,u64,f32,f64]\n";
  }
  return verifyResult;
}

inline bool UnaryTypeVerify2(PrimType pType) {
  bool verifyResult = IsPrimitiveFloat(pType);
  if (!verifyResult) {
    LogInfo::MapleLogger() << "\n#Error:result-type of recip,sqrt must be in [f32,f64]\n";
  }
  return verifyResult;
}

inline bool BinaryTypeVerify(PrimType pType) {
  return ArithResTypeVerify(pType) || IsPrimitiveDynType(pType);
}

inline bool BinaryGenericVerify(const BaseNode &bOpnd0, const BaseNode &bOpnd1) {
  return bOpnd0.Verify() && bOpnd1.Verify();
}

inline bool CompareTypeVerify(PrimType pType) {
  bool verifyResult = IsPrimitiveInteger(pType);
  if (!verifyResult) {
    LogInfo::MapleLogger() << "\n#Error:result type of eq,ge,gt,le,lt,ne must be primitive integer\n";
  }
  return verifyResult;
}

enum PTYGroup {
  kPTYGi32u32a32,
  kPTYGi32u32a32PtrRef,
  kPTYGi64u64a64,
  kPTYGPtrRef,
  kPTYGDynall,
  kPTYGu1,
  kPTYGSimpleObj,
  kPTYGSimpleStr,
  kPTYGOthers
};

uint8 GetPTYGroup(PrimType primType) {
  switch (primType) {
    case PTY_i32:
    case PTY_u32:
    case PTY_a32:
      return kPTYGi32u32a32;
    case PTY_i64:
    case PTY_u64:
    case PTY_a64:
      return kPTYGi64u64a64;
    case PTY_ref:
    case PTY_ptr:
      return kPTYGPtrRef;
    case PTY_dynany:
    case PTY_dyni32:
    case PTY_dynf64:
    case PTY_dynstr:
    case PTY_dynobj:
    case PTY_dynundef:
    case PTY_dynbool:
    case PTY_dynf32:
    case PTY_dynnone:
    case PTY_dynnull:
      return kPTYGDynall;
    case PTY_u1:
      return kPTYGu1;
    case PTY_simpleobj:
      return kPTYGSimpleObj;
    case PTY_simplestr:
      return kPTYGSimpleStr;
    default:
      return kPTYGOthers;
  }
}

uint8 GetCompGroupID(const BaseNode &opnd) {
  return GetPTYGroup(opnd.GetPrimType());
}

/*
  Refer to C11 Language Specification.
  $ 6.3.1.8 Usual arithmetic conversions
 */
bool CompatibleTypeVerify(const BaseNode &opnd1, const BaseNode &opnd2) {
  uint8 groupID1 = GetCompGroupID(opnd1);
  uint8 groupID2 = GetCompGroupID(opnd2);
  Opcode opCode2 = opnd2.GetOpCode();
  bool verifyResult = (groupID1 == groupID2);
  if (opCode2 == OP_gcmallocjarray || opCode2 == OP_gcpermallocjarray) {
    verifyResult = (groupID1 == kPTYGi32u32a32);
  }
  if (!verifyResult) {
    LogInfo::MapleLogger() << "\n#Error:incompatible operand types :\n";
    opnd1.Dump();
    opnd2.Dump();
  }
  return verifyResult;
}

bool FloatIntCvtTypeVerify(PrimType resPType, PrimType opndPType) {
  bool resTypeVerf = resPType == PTY_i32 || resPType == PTY_u32 || resPType == PTY_i64 || resPType == PTY_u64;
  if (!resTypeVerf) {
    LogInfo::MapleLogger() << "\n#Error:result-type of ceil,floor,round,trunc must be in [i32,u32,i64,u64]\n";
  }
  bool opndTypeVerf = opndPType == PTY_f32 || opndPType == PTY_f64;
  if (!opndTypeVerf) {
    LogInfo::MapleLogger() << "\n#Error:oerand-type of ceil,floor,round,trunc must be in [f32,f64]\n";
  }
  return resTypeVerf && opndTypeVerf;
}

inline MIRTypeKind GetTypeKind(StIdx stIdx) {
  const MIRSymbol *var = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(stIdx);
  ASSERT(var != nullptr, "null ptr check");
  MIRType *type = var->GetType();
  ASSERT(type != nullptr, "null ptr check");
  return type->GetKind();
}

inline MIRTypeKind GetTypeKind(TyIdx tyIdx) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  ASSERT(type != nullptr, "null ptr check");
  return type->GetKind();
}

inline MIRType *GetPointedMIRType(TyIdx tyIdx) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  CHECK_FATAL(type->GetKind() == kTypePointer, "TyIdx: %d is not pointer type", static_cast<uint32>(tyIdx));
  auto *ptrType = static_cast<MIRPtrType*>(type);
  return ptrType->GetPointedType();
}

inline MIRTypeKind GetPointedTypeKind(TyIdx tyIdx) {
  MIRType *pointedType = GetPointedMIRType(tyIdx);
  ASSERT(pointedType != nullptr, "null ptr check");
  return pointedType->GetKind();
}

MIRTypeKind GetFieldTypeKind(MIRStructType *structType, FieldID fieldId) {
  TyIdx fieldTyIdx;
  if (fieldId > 0) {
    MIRType *mirType = structType->GetFieldType(fieldId);
    fieldTyIdx = mirType->GetTypeIndex();
  } else {
    ASSERT(static_cast<unsigned>(-fieldId) < structType->GetParentFieldsSize() + 1, "array index out of range");
    fieldTyIdx = structType->GetParentFieldsElemt(-fieldId - 1).second.first;
  }
  return GetTypeKind(fieldTyIdx);
}

inline bool IsStructureTypeKind(MIRTypeKind kind) {
  return kind == kTypeStruct || kind == kTypeStructIncomplete || kind == kTypeUnion || kind == kTypeClass ||
         kind == kTypeClassIncomplete || kind == kTypeInterface || kind == kTypeInterfaceIncomplete;
}

inline bool IsStructureVerify(FieldID fieldID, StIdx stIdx) {
  if ((fieldID != 0) && (!IsStructureTypeKind(GetTypeKind(stIdx)))) {
    LogInfo::MapleLogger() << "\n#Error:if fieldID is not 0, the variable must be a structure\n";
    return false;
  }
  return true;
}

inline bool IsStructureVerify(FieldID fieldID, TyIdx tyIdx) {
  if ((fieldID != 0) && (!IsStructureTypeKind(GetTypeKind(tyIdx)))) {
    LogInfo::MapleLogger() << "\n#Error:if fieldID is not 0, the variable must be a structure\n";
    return false;
  }
  return true;
}

bool IsSignedType(const BaseNode *opnd) {
  switch (opnd->GetPrimType()) {
    case PTY_i32:
    case PTY_i64:
    case PTY_f32:
    case PTY_f64:
    case PTY_dyni32:
    case PTY_dynf32:
    case PTY_dynf64:
      return true;
    default:
      break;
  }
  return false;
}

inline bool BinaryStrictSignVerify0(const BaseNode *bOpnd0, const BaseNode *bOpnd1) {
  ASSERT(bOpnd0 != nullptr, "bOpnd0 is null");
  ASSERT(bOpnd1 != nullptr, "bOpnd1 is null");
  bool isDynany = (bOpnd0->GetPrimType() == PTY_dynany || bOpnd1->GetPrimType() == PTY_dynany);
  return isDynany || (IsSignedType(bOpnd0) && IsSignedType(bOpnd1)) || (!IsSignedType(bOpnd0) && !IsSignedType(bOpnd1));
}

bool BinaryStrictSignVerify1(const BaseNode *bOpnd0, const BaseNode *bOpnd1, const BaseNode *res) {
  if (GetCompGroupID(*res) == kPTYGDynall) {
    return BinaryStrictSignVerify0(bOpnd0, res) && BinaryStrictSignVerify0(bOpnd1, res) &&
           BinaryStrictSignVerify0(bOpnd0, bOpnd1);
  }
  return (IsSignedType(bOpnd0) && IsSignedType(bOpnd1) && IsSignedType(res)) ||
         (!IsSignedType(bOpnd0) && !IsSignedType(bOpnd1) && !IsSignedType(res));
}

bool UnaryNode::Verify() const {
  bool resTypeVerf = true;
  if (GetOpCode() == OP_bnot) {
    resTypeVerf = UnaryTypeVerify0(GetPrimType());
  } else if (GetOpCode() == OP_lnot) {
    if (!IsPrimitiveInteger(GetPrimType())) {
      resTypeVerf = false;
      LogInfo::MapleLogger() << "\n#Error:result-type of lnot must be primitive integer\n";
    }
  } else if (GetOpCode() == OP_abs || GetOpCode() == OP_neg) {
    resTypeVerf = UnaryTypeVerify1(GetPrimType());
  } else if (GetOpCode() == OP_recip || GetOpCode() == OP_sqrt) {
    resTypeVerf = UnaryTypeVerify2(GetPrimType());
  }

  // When an opcode only specifies one type, check for compatibility
  // between the operands and the result-type.
  bool compVerf = true;
  // op_alloca : return type is not compatible with operand, skip
  if (GetOpCode() != OP_alloca) {
    compVerf = CompatibleTypeVerify(*uOpnd, *this);
  }
  bool opndExprVerf = uOpnd->Verify();
  return resTypeVerf && compVerf && opndExprVerf;
}

bool TypeCvtNode::Verify() const {
  bool opndTypeVerf = true;
  bool opndSizeVerf = true;
  if (GetOpCode() == OP_ceil || GetOpCode() == OP_floor || GetOpCode() == OP_round || GetOpCode() == OP_trunc) {
    opndTypeVerf = FloatIntCvtTypeVerify(GetPrimType(), Opnd(0)->GetPrimType());
  } else if (GetOpCode() == OP_retype) {
    if (GetPrimTypeSize(GetPrimType()) != GetPrimTypeSize(Opnd(0)->GetPrimType())) {
      opndSizeVerf = false;
      LogInfo::MapleLogger() << "\n#Error:The size of opnd0 and prim-type must be the same\n";
    }
  }
  bool opndExprVerf = Opnd(0)->Verify();
  return opndTypeVerf && opndSizeVerf && opndExprVerf;
}

void AddRuntimeVerifyError(std::string errMsg, VerifyResult &verifyResult) {
  LogInfo::MapleLogger() << "\n#Error: " << errMsg << '\n';
  // Throw Verify Error
  verifyResult.AddPragmaVerifyError(verifyResult.GetCurrentClassName(), std::move(errMsg));
}

bool RetypeNode::VerifyPrimTypesAndOpnd() const {
  PrimType toPrimType = GetPrimType();
  PrimType fromPrimType = Opnd(0)->GetPrimType();
  if (GetPrimTypeSize(toPrimType) != GetPrimTypeSize(fromPrimType)) {
    LogInfo::MapleLogger() << "\n#Error: The size of opnd0 and prim-type must be the same\n";
    return false;
  }

  if (!IsPrimitivePoint(toPrimType) || !IsPrimitivePoint(fromPrimType)) {
    LogInfo::MapleLogger() << "\n#Error: Wrong prim-type in retype node, should be ref or ptr\n";
    return false;
  }
  return Opnd(0)->Verify();
}

bool RetypeNode::CheckFromJarray(const MIRType &from, const MIRType &to, VerifyResult &verifyResult) const {
  // Array types are subtypes of Object.
  // The intent is also that array types are subtypes of Cloneable and java.io.Serializable.
  if (IsInterfaceOrClass(to)) {
    Klass &toKlass = utils::ToRef(verifyResult.GetKlassHierarchy().GetKlassFromStrIdx(to.GetNameStrIdx()));
    const std::string &toKlassName = toKlass.GetKlassName();
    const std::string &javaLangObject = namemangler::kJavaLangObjectStr;
    const std::string javaLangCloneable = "Ljava_2Flang_2FCloneable_3B";
    const std::string javaIoSerializable = "Ljava_2Fio_2FSerializable_3B";
    if (toKlassName == javaLangObject || toKlassName == javaIoSerializable || toKlassName == javaLangCloneable) {
      return true;
    }
  }

  AddRuntimeVerifyError("Java array " + from.GetName() + " is not assignable to " + to.GetName(), verifyResult);
  return false;
}

bool RetypeNode::IsJavaAssignable(const MIRType &from, const MIRType &to, VerifyResult &verifyResult) const {
  // isJavaAssignable(arrayOf(X), arrayOf(Y)) :- compound(X), compound(Y), isJavaAssignable(X, Y).
  // arrayOf(X), arrayOf(Y) should already be X, Y here
  if (from.IsMIRJarrayType()) {
    return CheckFromJarray(from, to, verifyResult);
  }
  // isJavaAssignable(arrayOf(X), arrayOf(Y)) :- atom(X), atom(Y), X = Y.
  // This rule is not applicable to Maple IR
  if (from.IsScalarType() && to.IsScalarType()) {
    return true;
  }

  if (IsInterfaceOrClass(from) && IsInterfaceOrClass(to)) {
    const KlassHierarchy &klassHierarchy = verifyResult.GetKlassHierarchy();
    const std::string javaLangObject = namemangler::kJavaLangObjectStr;
    Klass &fromKlass = utils::ToRef(klassHierarchy.GetKlassFromStrIdx(from.GetNameStrIdx()));
    Klass &toKlass = utils::ToRef(klassHierarchy.GetKlassFromStrIdx(to.GetNameStrIdx()));
    // We can cast everything to java.lang.Object, but interface isn't subclass of that, so we need this branch
    if (toKlass.GetKlassName() == javaLangObject) {
      return true;
    }
    // isJavaAssignable(class(_, _), class(To, L)) :- loadedClass(To, L, ToClass), classIsInterface(ToClass).
    // isJavaAssignable(From, To) :- isJavaSubclassOf(From, To).
    bool isAssignableKlass = klassHierarchy.IsSuperKlass(&toKlass, &fromKlass) ||
                             klassHierarchy.IsSuperKlassForInterface(&toKlass, &fromKlass) ||
                             klassHierarchy.IsInterfaceImplemented(&toKlass, &fromKlass);
    if (isAssignableKlass) {
      return true;
    }
    AddRuntimeVerifyError("Java type " + fromKlass.GetKlassName() + " is NOT assignable to " + toKlass.GetKlassName(),
                          verifyResult);
    return false;
  }
  AddRuntimeVerifyError(from.GetName() + " is NOT assignable to " + to.GetName(), verifyResult);
  return false;
}

bool RetypeNode::VerifyCompleteMIRType(const MIRType &from, const MIRType &to, bool isJavaRefType,
                                       VerifyResult &verifyResult) const {
  if (from.IsScalarType() && to.IsScalarType() && !isJavaRefType) {
    if (GetPTYGroup(from.GetPrimType()) == GetPTYGroup(to.GetPrimType())) {
      return true;
    }
    LogInfo::MapleLogger() << "\n#Error: retype scalar type failed\n";
    return false;
  }
  if (!verifyResult.GetMIRModule().IsJavaModule()) {
    return true;
  }
  isJavaRefType |= IsJavaRefType(from) && IsJavaRefType(to);
  if (isJavaRefType) {
    return IsJavaAssignable(from, to, verifyResult);
  }

  if (from.GetKind() != to.GetKind()) {
    if (from.GetPrimType() == PTY_void || to.GetPrimType() == PTY_void) {
      return true;
    }
    LogInfo::MapleLogger() << "\n#Error: Retype different kind: from " << from.GetKind() <<
                              " to " << to.GetKind() << "\n";
    return false;
  }
  return true;
}

bool RetypeNode::VerifyJarrayDimention(const MIRJarrayType &from, const MIRJarrayType &to,
                                       VerifyResult &verifyResult) const {
  int fromDim = const_cast<MIRJarrayType&>(from).GetDim();
  int toDim = const_cast<MIRJarrayType&>(to).GetDim();
  if (fromDim == toDim) {
    return true;
  } else if (fromDim > toDim) {
    const MIRType *toElemType = to.GetElemType();
    while (toElemType != nullptr && (toElemType->IsMIRJarrayType() || toElemType->IsMIRPtrType())) {
      toElemType = toElemType->IsMIRJarrayType() ?
                   static_cast<const MIRJarrayType*>(toElemType)->GetElemType() :
                   static_cast<const MIRPtrType*>(toElemType)->GetPointedType();
    }
    if (toElemType != nullptr && CheckFromJarray(from, *toElemType, verifyResult)) {
      return true;
    }
  }
  Dump(0);
  std::string errorMsg = "Arrays have different dimentions: from " + std::to_string(fromDim) +
                         " to " + std::to_string(toDim);
  AddRuntimeVerifyError(std::move(errorMsg), verifyResult);
  return false;
}

bool RetypeNode::Verify(VerifyResult& verifyResult) const {
  // If RetypeNode::Verify return false, Dump this node to show the wrong IR
  if (!VerifyPrimTypesAndOpnd()) {
    Dump(0);
    LogInfo::MapleLogger() << "\n#Error: Verify PrimTypes and Opnd failed in retype node\n";
    return false;
  }
  bool isJavaRefType = false;
  const MIRType *fromMIRType = verifyResult.GetCurrentFunction()->GetNodeType(*Opnd(0));
  const MIRType *toMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  while (fromMIRType != nullptr && toMIRType != nullptr && BothPointerOrJarray(*fromMIRType, *toMIRType)) {
    if (fromMIRType->IsMIRJarrayType()) {
      isJavaRefType = true;
      if (!VerifyJarrayDimention(static_cast<const MIRJarrayType&>(*fromMIRType),
                                 static_cast<const MIRJarrayType&>(*toMIRType), verifyResult)) {
        return false;
      }
      fromMIRType = static_cast<const MIRJarrayType*>(fromMIRType)->GetElemType();
      toMIRType = static_cast<const MIRJarrayType*>(toMIRType)->GetElemType();
    } else {
      fromMIRType = static_cast<const MIRPtrType*>(fromMIRType)->GetPointedType();
      toMIRType = static_cast<const MIRPtrType*>(toMIRType)->GetPointedType();
    }
  }
  if (fromMIRType == nullptr || toMIRType == nullptr) {
    Dump(0);
    LogInfo::MapleLogger() << "\n#Error: MIRType is nullptr in retype node\n";
    return false;
  }

  if (fromMIRType->IsIncomplete() || toMIRType->IsIncomplete()) {
    // Add Deferred Check
    const std::string &currentClassName = verifyResult.GetCurrentClassName();
    LogInfo::MapleLogger(kLlDbg) << "Add AssignableCheck from " << fromMIRType->GetName() <<
                                    " to " << toMIRType->GetName() <<
                                    " in class " << currentClassName << '\n';
    verifyResult.AddPragmaAssignableCheck(currentClassName, fromMIRType->GetName(), toMIRType->GetName());
    // Deferred Assignable Check returns true because we should collect all the deferred checks for runtime
    return true;
  }

  if (VerifyCompleteMIRType(*fromMIRType, *toMIRType, isJavaRefType, verifyResult)) {
    return true;
  }
  Dump(0);
  LogInfo::MapleLogger() << "\n#Error: Verify Complete MIRType failed in retype node\n";
  return false;
}

bool UnaryStmtNode::VerifyThrowable(VerifyResult &verifyResult) const {
  const BaseNode *rhs = GetRHS();
  if (rhs == nullptr) {
    return true;
  }

  const MIRType *mirType = verifyResult.GetCurrentFunction()->GetNodeType(*rhs);
  if (mirType != nullptr && mirType->IsMIRPtrType()) {
    mirType = static_cast<const MIRPtrType*>(mirType)->GetPointedType();
  }
  if (mirType != nullptr) {
    if (mirType->GetPrimType() == PTY_void) {
      return true;
    }
    if (mirType->IsIncomplete()) {
      // Add Deferred Check
      const std::string &currentClassName = verifyResult.GetCurrentClassName();
      std::string throwableName = "Ljava_2Flang_2FThrowable_3B";
      LogInfo::MapleLogger(kLlDbg) << "Add AssignableCheck from " << mirType->GetName() <<
                                      " to " << throwableName <<
                                      " in class " << currentClassName << '\n';
      verifyResult.AddPragmaAssignableCheck(currentClassName, mirType->GetName(), std::move(throwableName));
      // Deferred Assignable Check returns true because we should collect all the deferred checks for runtime
      return true;
    }
    if (mirType->IsMIRClassType() && static_cast<const MIRClassType*>(mirType)->IsExceptionType()) {
      return true;
    }
  }
  Dump(0);
  std::string errMsg = (mirType == nullptr ? "nullptr" : mirType->GetName());
  errMsg += " is NOT throwable.";
  AddRuntimeVerifyError(std::move(errMsg), verifyResult);
  return false;
}

bool IntrinsicopNode::Verify(VerifyResult &verifyResult) const {
  if (GetIntrinsic() == INTRN_JAVA_ARRAY_LENGTH && !VerifyJArrayLength(verifyResult)) {
    return false;
  }
  return VerifyOpnds();
}

bool IntrinsicopNode::VerifyJArrayLength(VerifyResult &verifyResult) const {
  BaseNode &val = utils::ToRef(Opnd(0));
  const MIRType *valType = verifyResult.GetCurrentFunction()->GetNodeType(val);
  if (valType != nullptr && valType->IsMIRPtrType()) {
    valType = static_cast<const MIRPtrType*>(valType)->GetPointedType();
    if (valType != nullptr && !valType->IsMIRJarrayType()) {
      Dump(0);
      AddRuntimeVerifyError("Operand of array length is not array", verifyResult);
      return false;
    }
  }
  return true;
}

bool IreadNode::Verify() const {
  bool addrExprVerf = Opnd(0)->Verify();
  bool pTypeVerf = true;
  bool structVerf = true;
  if (GetTypeKind(tyIdx) != kTypePointer) {
    LogInfo::MapleLogger() << "\n#Error:<type> must be a pointer type\n";
    return false;
  }
  if (GetOpCode() == OP_iaddrof) {
    pTypeVerf = IsAddress(GetPrimType());
    if (!pTypeVerf) {
      LogInfo::MapleLogger() << "\n#Error:prim-type must be either ptr, ref, a32 or a64\n";
    }
  } else {
    if (fieldID == 0 && IsStructureTypeKind(GetPointedTypeKind(tyIdx))) {
      if (GetPrimType() != PTY_agg) {
        pTypeVerf = false;
        LogInfo::MapleLogger()
            << "\n#Error:If the content dereferenced is a structure, then <prim-type> should specify agg\n";
      }
    }
  }
  if (fieldID != 0) {
    if (!IsStructureTypeKind(GetPointedTypeKind(tyIdx))) {
      structVerf = false;
      LogInfo::MapleLogger() << "\n#Error:If field-id is not 0, then type must specify pointer to a structure\n";
    } else {
      MIRType *type = GetPointedMIRType(tyIdx);
      auto *stTy = static_cast<MIRStructType*>(type);
      if (GetOpCode() == OP_iread && stTy->GetFieldsSize() != 0) {
        if (IsStructureTypeKind(GetFieldTypeKind(stTy, fieldID))) {
          if (GetPrimType() != PTY_agg) {
            pTypeVerf = false;
            LogInfo::MapleLogger() << "\n#Error:If the field itself is a structure, prim-type should specify agg\n";
          }
        }
      }
    }
  }
  return addrExprVerf && pTypeVerf && structVerf;
}

bool RegreadNode::Verify() const {
  return true;
}

bool IreadoffNode::Verify() const {
  return true;
}

bool IreadFPoffNode::Verify() const {
  return true;
}

bool ExtractbitsNode::Verify() const {
  bool opndExprVerf = Opnd(0)->Verify();
  bool compVerf = CompatibleTypeVerify(*Opnd(0), *this);
  bool resTypeVerf = UnaryTypeVerify0(GetPrimType());
  constexpr int numBitsInByte = 8;
  bool opnd0SizeVerf = (numBitsInByte * GetPrimTypeSize(Opnd(0)->GetPrimType()) >= bitsSize);
  if (!opnd0SizeVerf) {
    LogInfo::MapleLogger()
        << "\n#Error: The operand of extractbits must be large enough to contain the specified bitfield\n";
  }
  return opndExprVerf && compVerf && resTypeVerf && opnd0SizeVerf;
}

bool BinaryNode::Verify() const {
  bool opndsVerf = BinaryGenericVerify(*GetBOpnd(0), *GetBOpnd(1));
  bool resTypeVerf = BinaryTypeVerify(GetPrimType());
  if (!resTypeVerf && theMIRModule->IsCModule()) {
    if ((IsAddress(GetBOpnd(0)->GetPrimType()) && !IsAddress(GetBOpnd(1)->GetPrimType())) ||
        (!IsAddress(GetBOpnd(0)->GetPrimType()) && IsAddress(GetBOpnd(1)->GetPrimType()))) {
      resTypeVerf = true;  // don't print the same kind of error message twice
      if (GetOpCode() != OP_add && GetOpCode() != OP_sub && GetOpCode() != OP_CG_array_elem_add) {
        LogInfo::MapleLogger() << "\n#Error: Only add and sub are allowed for pointer arithemetic\n";
        this->Dump();
      } else if (!IsAddress(GetPrimType())) {
        LogInfo::MapleLogger()
            << "\n#Error: Adding an offset to a pointer or subtracting one from a pointer should result in a pointer "
               "value\n";
        this->Dump();
      }
    }
  }
  if (!resTypeVerf) {
    LogInfo::MapleLogger()
        << "\n#Error:result type of [add,div,sub,mul,max,min] and [ashr,band,bior,bxor,land,lior,lshr,shl,rem] must "
           "be in [i32,u32,i64,u64,f32,f64,dynamic-type]\n";
    this->Dump();
  }
  bool comp0Verf = CompatibleTypeVerify(*GetBOpnd(0), *this);
  bool comp1Verf = true;
  // Shift operations do not require same-type operands
  if (GetOpCode() < OP_ashr || GetOpCode() > OP_shl) {
    comp1Verf = CompatibleTypeVerify(*GetBOpnd(1), *this);
  }
  bool signVerf = true;
  bool typeVerf = resTypeVerf && comp0Verf && comp1Verf;
  if (typeVerf) {
    if (GetOpCode() == OP_div || GetOpCode() == OP_mul || GetOpCode() == OP_rem || GetOpCode() == OP_max ||
        GetOpCode() == OP_min) {
      signVerf = BinaryStrictSignVerify1(GetBOpnd(0), GetBOpnd(1), this);
      if (!signVerf) {
        LogInfo::MapleLogger()
            << "\n#Error:the result and operands of [div,mul,rem,max,min] must be of the same sign\n";
      }
    }
  }
  return opndsVerf && typeVerf && signVerf;
}

bool CompareNode::Verify() const {
  bool opndsVerf = BinaryGenericVerify(*GetBOpnd(0), *GetBOpnd(1));
  bool compVerf = CompatibleTypeVerify(*GetBOpnd(0), *GetBOpnd(1));
  bool resTypeVerf = CompareTypeVerify(GetPrimType());
  if (!resTypeVerf) {
    this->Dump();
  }
  bool signVerf = true;
  bool typeVerf = compVerf && resTypeVerf;
  if (typeVerf && GetOpCode() != OP_eq && GetOpCode() != OP_ne) {
    signVerf = BinaryStrictSignVerify0(GetBOpnd(0), GetBOpnd(1));
    if (!signVerf) {
      LogInfo::MapleLogger() << "\n#Error:the operands of [ge,gt,le,lt] must be of the same sign\n";
    }
  }
  return opndsVerf && typeVerf && signVerf;
}

bool DepositbitsNode::Verify() const {
  bool opndsVerf = BinaryGenericVerify(*GetBOpnd(0), *GetBOpnd(1));
  bool resTypeVerf = IntTypeVerify(GetPrimType());
  constexpr int numBitsInByte = 8;
  bool opnd0SizeVerf = (numBitsInByte * GetPrimTypeSize(GetBOpnd(0)->GetPrimType()) >= bitsSize);
  if (!opnd0SizeVerf) {
    LogInfo::MapleLogger() << "\n#Error:opnd0 of depositbits must be large enough to contain the specified bitfield\n";
  }
  return opndsVerf && resTypeVerf && opnd0SizeVerf;
}

bool IntrinsicopNode::Verify() const {
  return VerifyOpnds();
}

bool TernaryNode::Verify() const {
  bool comp1Verf = CompatibleTypeVerify(*topnd[1], *this);
  bool comp2Verf = CompatibleTypeVerify(*topnd[2], *this);
  bool opnd0TypeVerf = IsPrimitiveInteger(topnd[0]->GetPrimType());
  if (!opnd0TypeVerf) {
    LogInfo::MapleLogger() << "\n#Error:select-opnd0 must be of integer type\n";
  }
  return comp1Verf && comp2Verf && opnd0TypeVerf;
}

bool SizeoftypeNode::Verify() const {
  return true;
}

bool ArrayNode::Verify() const {
  bool opndsVerf = VerifyOpnds();
  bool resTypeVerf = IsAddress(GetPrimType());
  bool opndsTypeVerf = true;
  if (!resTypeVerf) {
    LogInfo::MapleLogger() << "\n#Error:result-type of array must be in [ptr,ref,a32,a64]\n";
  }
  bool opnd0TypeVerf = IsAddress(GetNopndAt(0)->GetPrimType());
  if (!opnd0TypeVerf) {
    LogInfo::MapleLogger() << "\n#Error:result-type of array-opnd0 must be in [ptr,ref,a32,a64]\n";
  }
  for (size_t i = 1; i < NumOpnds(); ++i) {
    if (!IntTypeVerify(GetNopndAt(i)->GetPrimType())) {
      opndsTypeVerf = false;
      LogInfo::MapleLogger() << "\n#Error:result of the array index operands must be in [i32,u32,i64,u64]\n";
    }
  }
  return opndsVerf && resTypeVerf && opnd0TypeVerf && opndsTypeVerf;
}

bool DassignNode::Verify() const {
  bool structVerf = IsStructureVerify(fieldID, stIdx);
  bool rhsVerf = GetRHS()->Verify();
  return structVerf && rhsVerf;
}

bool AddrofNode::Verify() const {
  bool pTypeVerf = true;
  bool structVerf = IsStructureVerify(fieldID, GetStIdx());
  if (GetOpCode() == OP_dread) {
    if (fieldID == 0 && IsStructureTypeKind(GetTypeKind(GetStIdx()))) {
      if (GetPrimType() != PTY_agg) {
        pTypeVerf = false;
        LogInfo::MapleLogger() << "\n#Error:if variable is a structure, prim-type should specify agg\n";
      }
    }
    if (fieldID != 0 && structVerf) {
      const MIRSymbol *var = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(GetStIdx());
      MIRType *type = var->GetType();
      auto *stTy = static_cast<MIRStructType*>(type);
      if (IsStructureTypeKind(GetFieldTypeKind(stTy, fieldID))) {
        if (GetPrimType() != PTY_agg) {
          pTypeVerf = false;
          LogInfo::MapleLogger() << "\n#Error:if the field itself is a structure, prim-type should specify agg\n";
        }
      }
    }
  } else {
    pTypeVerf = IsAddress(GetPrimType());
    if (!pTypeVerf) {
      LogInfo::MapleLogger()
          << "\n#Error:result-type of addrof,addroflabel,addroffunc,iaddrof must be in [ptr,ref,a32,a64]\n";
    }
  }
  return pTypeVerf && structVerf;
}

bool AddroffuncNode::Verify() const {
  bool addrTypeVerf = IsAddress(GetPrimType());
  if (!addrTypeVerf) {
    LogInfo::MapleLogger()
        << "\n#Error:result-type of addrof,addroflabel,addroffunc,iaddrof must be in [ptr,ref,a32,a64]\n";
  }
  return addrTypeVerf;
}

bool AddroflabelNode::Verify() const {
  bool addrTypeVerf = IsAddress(GetPrimType());
  if (!addrTypeVerf) {
    LogInfo::MapleLogger()
        << "\n#Error:result-type of addrof,addroflabel,addroffunc,iaddrof must be in [ptr,ref,a32,a64]\n";
  }
  return addrTypeVerf;
}

bool IassignNode::Verify() const {
  bool addrExpVerf = addrExpr->Verify();
  bool rhsVerf = rhs->Verify();
  bool structVerf = true;
  if (GetTypeKind(tyIdx) != kTypePointer) {
    LogInfo::MapleLogger() << "\n#Error:<type> must be a pointer type\n";
    return false;
  }
  if (fieldID != 0) {
    if (!IsStructureTypeKind(GetPointedTypeKind(tyIdx))) {
      structVerf = false;
      LogInfo::MapleLogger() << "\n#Error:If field-id is not 0, the computed address must correspond to a structure\n";
    }
  }
  return addrExpVerf && rhsVerf && structVerf;
}

bool IassignoffNode::Verify() const {
  bool addrVerf = GetBOpnd(0)->Verify();
  bool rhsVerf = GetBOpnd(1)->Verify();
  bool compVerf = CompatibleTypeVerify(*this, *GetBOpnd(1));
  return addrVerf && rhsVerf && compVerf;
}

bool IassignFPoffNode::Verify() const {
  bool rhsVerf = Opnd(0)->Verify();
  bool compVerf = CompatibleTypeVerify(*this, *Opnd(0));
  return rhsVerf && compVerf;
}

bool RegassignNode::Verify() const {
  bool rhsVerf = Opnd(0)->Verify();
  bool compVerf = CompatibleTypeVerify(*this, *Opnd(0));
  return rhsVerf && compVerf;
}

bool CondGotoNode::Verify() const {
  bool opndExprVerf = UnaryStmtNode::Opnd(0)->Verify();
  bool opndTypeVerf = true;
  if (!IsPrimitiveInteger(UnaryStmtNode::Opnd(0)->GetPrimType())) {
    opndTypeVerf = false;
    LogInfo::MapleLogger() << "\n#Error:the operand of brfalse and trfalse must be primitive integer\n";
  }
  return opndExprVerf && opndTypeVerf;
}

bool SwitchNode::Verify() const {
  bool opndExprVerf = switchOpnd->Verify();
  bool opndTypeVerf = IntTypeVerify(switchOpnd->GetPrimType());
  if (!opndTypeVerf) {
    LogInfo::MapleLogger() << "\n#Error: the operand of switch must be in [i32,u32,i64,u64]\n";
  }
  return opndExprVerf && opndTypeVerf;
}

bool BinaryStmtNode::Verify() const {
  return GetBOpnd(0)->Verify() && GetBOpnd(1)->Verify() && CompatibleTypeVerify(*GetBOpnd(0), *GetBOpnd(1)) &&
         BinaryStrictSignVerify0(GetBOpnd(0), GetBOpnd(1));
}

bool RangeGotoNode::Verify() const {
  bool opndExprVerf = Opnd(0)->Verify();
  bool opndTypeVerf = IntTypeVerify(Opnd(0)->GetPrimType());
  if (!opndTypeVerf) {
    LogInfo::MapleLogger() << "\n#Error: the operand of rangegoto must be in [i32,u32,i64,u64]\n";
  }
  return opndExprVerf && opndTypeVerf;
}

bool BlockNode::Verify() const {
  for (auto &stmt : GetStmtNodes()) {
    if (!stmt.Verify()) {
      return false;
    }
  }
  return true;
}

bool BlockNode::Verify(VerifyResult &verifyResult) const {
  auto &nodes = GetStmtNodes();
  return !std::any_of(nodes.begin(), nodes.end(), [&verifyResult](auto &stmt){
    return !stmt.Verify(verifyResult);
  });
}

bool DoloopNode::Verify() const {
  bool startVerf = startExpr->Verify();
  bool contVerf = condExpr->Verify();
  bool incrVerf = incrExpr->Verify();
  bool doBodyVerf = true;
  if (doBody) {
    doBodyVerf = doBody->Verify();
  }
  return startVerf && contVerf && incrVerf && doBodyVerf;
}

bool IfStmtNode::Verify() const {
  bool condVerf = Opnd()->Verify();
  bool thenVerf = true;
  bool elseVerf = true;
  if (thenPart != nullptr) {
    thenVerf = thenPart->Verify();
  }
  if (elsePart != nullptr) {
    elseVerf = elsePart->Verify();
  }
  return condVerf && thenVerf && elseVerf;
}

bool WhileStmtNode::Verify() const {
  bool condVerf = Opnd(0)->Verify();
  bool bodyVerf = true;
  if (body != nullptr) {
    bodyVerf = body->Verify();
  }
  return condVerf && bodyVerf;
}

bool NaryStmtNode::Verify() const {
  return VerifyOpnds();
}

bool CallNode::Verify() const {
  return VerifyOpnds();
}

bool IcallNode::Verify() const {
  bool nOpndsVerf = true;
  for (size_t i = 0; i < NumOpnds(); ++i) {
    if (!GetNopndAt(i)->Verify()) {
      nOpndsVerf = false;
      break;
    }
  }
  return nOpndsVerf;
}

bool IntrinsiccallNode::Verify() const {
  return VerifyOpnds();
}
}  // namespace maple
