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
#include "dex_op.h"
#include "dex_file_util.h"
#include "mpl_logging.h"
#include "namemangler.h"
#include "global_tables.h"
#include "bc_class.h"
#include "feir_builder.h"
#include "fe_utils_java.h"
#include "dex_util.h"
#include "feir_type_helper.h"
#include "dex_strfac.h"
#include "fe_options.h"
#include "feir_var_name.h"
#include "ark_annotation_processor.h"

namespace maple {
namespace bc {
// ========== DexOp ==========
DexOp::DexOp(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : BCInstruction(allocatorIn, pcIn, opcodeIn) {}

void DexOp::SetVA(uint32 num) {
  SetVAImpl(num);
}

void DexOp::SetVB(uint32 num) {
  SetVBImpl(num);
}

void DexOp::SetWideVB(uint64 num) {
  SetWideVBImpl(num);
}

void DexOp::SetArgs(const MapleList<uint32> &args) {
  SetArgsImpl(args);
}

void DexOp::SetVC(uint32 num) {
  SetVCImpl(num);
}

void DexOp::SetVH(uint32 num) {
  SetVHImpl(num);
}

std::string DexOp::GetArrayElementTypeFromArrayType(const std::string &typeName) {
  CHECK_FATAL(!typeName.empty() && typeName[0] == 'A', "Invalid array type name %s", typeName.c_str());
  return typeName.substr(1);
}

// ========== DexOpNop ==========
DexOpNop::DexOpNop(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn) : DexOp(allocatorIn, pcIn, opcodeIn) {}

// ========== DexOpMove ==========
DexOpMove::DexOpMove(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpMove::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  GStrIdx typeNameIdx;
  if (kDexOpMove <= opcode && opcode <= kDexOpMove16) {
    typeNameIdx = BCUtil::GetIntIdx();
  } else if (kDexOpMoveWide <= opcode && opcode <= kDexOpMoveWide16) {
    typeNameIdx = BCUtil::GetLongIdx();
  } else if (kDexOpMoveObject <= opcode && opcode <= kDexOpMoveObject16) {
    typeNameIdx = BCUtil::GetJavaObjectNameMplIdx();
  } else {
    CHECK_FATAL(false, "invalid opcode: %x in `DexOpMove`", opcode);
  }
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, typeNameIdx, true);
  defedRegs.emplace_back(&vA);
}

void DexOpMove::SetVBImpl(uint32 num) {
  vB.regNum = num;
  vB.regTypeItem = vA.regTypeItem;
  usedRegs.emplace_back(&vB);
}

void DexOpMove::SetRegTypeInTypeInferImpl() {
  vB.regType->RegisterRelatedBCRegType(vA.regType);
}

std::list<UniqueFEIRStmt> DexOpMove::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  std::unique_ptr<FEIRExpr> dreadExpr = std::make_unique<FEIRExprDRead>(vB.GenFEIRVarReg());
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtDAssign>(vA.GenFEIRVarReg(), std::move(dreadExpr));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpMoveResult =========
DexOpMoveResult::DexOpMoveResult(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {
  isReturn = true;
}

void DexOpMoveResult::SetVATypeNameIdx(const GStrIdx &idx) {
  vA.regType->SetTypeNameIdx(idx);
}

DexReg DexOpMoveResult::GetVA() const {
  return vA;
}

void DexOpMoveResult::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpMoveResult::SetBCRegTypeImpl(const BCInstruction &inst) {
  if ((kDexOpInvokeVirtual <= inst.GetOpcode() && inst.GetOpcode() <= kDexOpInvokeInterface) ||
      (kDexOpInvokeVirtualRange <= inst.GetOpcode() && inst.GetOpcode() <= kDexOpInvokeInterfaceRange) ||
      (kDexOpInvokePolymorphic <= inst.GetOpcode() && inst.GetOpcode() <= kDexOpInvokeCustomRange)) {
    GStrIdx retTyIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        namemangler::EncodeName(static_cast<const DexOpInvoke&>(inst).GetReturnType()));
    vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, retTyIdx);
  } else if (inst.GetOpcode() == kDexOpFilledNewArray || inst.GetOpcode() == kDexOpFilledNewArrayRange) {
    vA.regType = allocator.GetMemPool()->New<BCRegType>(
        allocator, vA, static_cast<const DexOpFilledNewArray&>(inst).GetReturnType());
  } else {
    CHECK_FATAL(false, "the reg of opcode %u should be inserted in move-result", inst.GetOpcode());
  }
}

// ========== DexOpMoveException ==========
DexOpMoveException::DexOpMoveException(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpMoveException::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpMoveException::SetBCRegTypeImpl(const BCInstruction &inst) {
  (void) inst;
  GStrIdx exceptionTypeNameIdx = *(catchedExTypeNamesIdx.begin());
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, exceptionTypeNameIdx);
  // If exception type is primitive type, it should be <* void>
  if (vA.GetBasePrimType() != PTY_ref) {
    vA.regTypeItem->typeNameIdx = BCUtil::GetJavaThrowableNameMplIdx();
  }
}

std::list<UniqueFEIRStmt> DexOpMoveException::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  std::unique_ptr<FEIRExpr> readExpr = std::make_unique<FEIRExprRegRead>(PTY_ref, -kSregThrownval);
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtDAssign>(vA.GenFEIRVarReg(), std::move(readExpr));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpReturn ==========
DexOpReturn::DexOpReturn(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpReturn::SetVAImpl(uint32 num) {
  if (opcode == kDexOpReturnVoid) {
    isReturnVoid = true;
    return;
  }
  vA.regNum = num;
  usedRegs.emplace_back(&vA);
}

void DexOpReturn::ParseImpl(BCClassMethod &method) {
  GStrIdx usedTypeNameIdx =
      GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(method.GetSigTypeNames().at(0)));
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(usedTypeNameIdx);
}

std::list<UniqueFEIRStmt> DexOpReturn::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> ans;
  if (isReturnVoid) {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtReturn>(UniqueFEIRExpr(nullptr));
    ans.emplace_back(std::move(stmt));
  } else {
    UniqueFEIRVar var = vA.GenFEIRVarReg();
    UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(var));
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtReturn>(std::move(expr));
    ans.emplace_back(std::move(stmt));
  }
  return ans;
}

// ========== DexOpConst ==========
DexOpConst::DexOpConst(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpConst::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  vA.regValue = allocator.GetMemPool()->New<BCRegValue>();
  defedRegs.emplace_back(&vA);
}

void DexOpConst::SetVBImpl(uint32 num) {
  if (opcode != kDexOpConstWide) {
    vA.regValue->primValue.raw32 = num;
  }
  if (!isWide) {
    vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetIntIdx());
  }
}

void DexOpConst::SetWideVBImpl(uint64 num) {
  if (opcode == kDexOpConstWide) {
    vA.regValue->primValue.raw64 = num;
  }
  if (isWide) {
    vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetLongIdx());
  }
}

std::list<UniqueFEIRStmt> DexOpConst::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> ans;
  UniqueFEIRExpr expr;
  DexReg vATmp;
  vATmp.regNum = vA.regNum;
  GStrIdx typeNameIdx;
  if (isWide) {
    typeNameIdx = BCUtil::GetLongIdx();
  } else {
    typeNameIdx = BCUtil::GetIntIdx();
  }
  vATmp.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vATmp, typeNameIdx, true);
  switch (opcode) {
    case kDexOpConst4: {
      expr = FEIRBuilder::CreateExprConstI8(static_cast<int8>(vA.regValue->primValue.raw32));
      expr = FEIRBuilder::CreateExprCvtPrim(std::move(expr), PTY_i32);
      break;
    }
    case kDexOpConst16: {
      expr = FEIRBuilder::CreateExprConstI16(static_cast<int16>(vA.regValue->primValue.raw32));
      expr = FEIRBuilder::CreateExprCvtPrim(std::move(expr), PTY_i32);
      break;
    }
    case kDexOpConst: {
      expr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(vA.regValue->primValue.raw32));
      break;
    }
    case kDexOpConstHigh16: {
      expr = FEIRBuilder::CreateExprConstI16(static_cast<int16>(vA.regValue->primValue.raw32));
      expr = FEIRBuilder::CreateExprCvtPrim(std::move(expr), PTY_i32);
      UniqueFEIRExpr exprBit = FEIRBuilder::CreateExprConstI32(16);  // 16 means right-zero-extended to 32 bits
      expr = FEIRBuilder::CreateExprMathBinary(OP_shl, std::move(expr), std::move(exprBit));
      break;
    }
    case kDexOpConstWide16: {
      expr = FEIRBuilder::CreateExprConstI16(static_cast<int16>(vA.regValue->primValue.raw64));
      expr = FEIRBuilder::CreateExprCvtPrim(std::move(expr), PTY_i64);
      break;
    }
    case kDexOpConstWide32: {
      expr = FEIRBuilder::CreateExprConstI32(static_cast<int32>(vA.regValue->primValue.raw64));
      expr = FEIRBuilder::CreateExprCvtPrim(std::move(expr), PTY_i64);
      break;
    }
    case kDexOpConstWide: {
      expr = FEIRBuilder::CreateExprConstI64(static_cast<int64>(vA.regValue->primValue.raw64));
      break;
    }
    case kDexOpConstWideHigh16: {
      expr = FEIRBuilder::CreateExprConstI16(static_cast<int16>(vA.regValue->primValue.raw64));
      expr = FEIRBuilder::CreateExprCvtPrim(std::move(expr), PTY_i64);
      UniqueFEIRExpr exprBit = FEIRBuilder::CreateExprConstI32(48);  // 48 means right-zero-extended to 64 bits
      expr = FEIRBuilder::CreateExprMathBinary(OP_shl, std::move(expr), std::move(exprBit));
      break;
    }
    default: {
      CHECK_FATAL(false, "Unsupported const-op: %u", opcode);
      break;
    }
  }
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vATmp.GenFEIRVarReg(), std::move(expr));
  ans.emplace_back(std::move(stmt));
  if (!(*vA.regTypeItem == *vATmp.regTypeItem)) {
    stmt = FEIRBuilder::CreateStmtRetype(vA.GenFEIRVarReg(), vATmp.GenFEIRVarReg());
    ans.emplace_back(std::move(stmt));
  }
  return ans;
}

// ========== DexOpConstString ==========
DexOpConstString::DexOpConstString(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn), strValue(allocator.GetMemPool()) {}

void DexOpConstString::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetJavaStringNameMplIdx());
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpConstString::SetVBImpl(uint32 num) {
  vA.dexLitStrIdx = num;
}

void DexOpConstString::ParseImpl(BCClassMethod &method) {
  fileIdx = method.GetBCClass().GetBCParser().GetReader()->GetFileIndex();
  strValue = method.GetBCClass().GetBCParser().GetReader()->GetStringFromIdx(vA.dexLitStrIdx);
  if (FEOptions::GetInstance().IsRC() && !method.IsRcPermanent() &&
      ArkAnnotation::GetInstance().IsPermanent(strValue.c_str())) {
    // "Lcom/huawei/ark/annotation/Permanent;" and "Lark/annotation/Permanent;"
    // indicates next new-instance / new-array Permanent object
    method.SetIsRcPermanent(true);
  }
}

std::list<UniqueFEIRStmt> DexOpConstString::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRVar dstVar = vA.GenFEIRVarReg();
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtJavaConstString>(std::move(dstVar), strValue.c_str(),
                                                                  fileIdx, vA.dexLitStrIdx);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpConstClass ==========
DexOpConstClass::DexOpConstClass(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpConstClass::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetJavaClassNameMplIdx());
  defedRegs.emplace_back(&vA);
}

void DexOpConstClass::SetVBImpl(uint32 num) {
  dexTypeIdx = num;
}

void DexOpConstClass::ParseImpl(BCClassMethod &method) {
  const std::string &typeName = method.GetBCClass().GetBCParser().GetReader()->GetTypeNameFromIdx(dexTypeIdx);
  const std::string &mplTypeName = namemangler::EncodeName(typeName);
  mplTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(mplTypeName);
}

std::list<UniqueFEIRStmt> DexOpConstClass::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRVar dstVar = vA.GenFEIRVarReg();
  UniqueFEIRType refType = std::make_unique<FEIRTypeDefault>(BCUtil::GetPrimType(mplTypeNameIdx), mplTypeNameIdx);
  UniqueFEIRExpr expr = std::make_unique<FEIRExprIntrinsicop>(std::move(refType), INTRN_JAVA_CONST_CLASS,
                                                              std::make_unique<FEIRTypeDefault>(PTY_ref), dexTypeIdx);
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtDAssign>(std::move(dstVar), std::move(expr));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpMonitor ==========
DexOpMonitor::DexOpMonitor(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpMonitor::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetJavaObjectNameMplIdx(), true);
  usedRegs.emplace_back(&vA);
}

std::list<UniqueFEIRStmt> DexOpMonitor::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> exprs;
  UniqueFEIRExpr expr = std::make_unique<FEIRExprDRead>(vA.GenFEIRVarReg());
  exprs.emplace_back(std::move(expr));
  if (opcode == kDexOpMonitorEnter) {
    expr = std::make_unique<FEIRExprConst>(static_cast<int64>(2), PTY_i32);
    exprs.emplace_back(std::move(expr));
  }
  UniqueFEIRStmt stmt =
      std::make_unique<FEIRStmtNary>(opcode == kDexOpMonitorEnter ? OP_syncenter : OP_syncexit, std::move(exprs));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpCheckCast =========
DexOpCheckCast::DexOpCheckCast(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpCheckCast::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetJavaObjectNameMplIdx(), true);
  usedRegs.emplace_back(&vA); // use vA to gen vDef
  vDef.regNum = num;
  vDef.isDef = true;
  defedRegs.emplace_back(&vDef);
}

void DexOpCheckCast::SetVBImpl(uint32 num) {
  targetDexTypeIdx = num;
}

void DexOpCheckCast::ParseImpl(BCClassMethod &method) {
  const std::string &typeName = method.GetBCClass().GetBCParser().GetReader()->GetTypeNameFromIdx(targetDexTypeIdx);
  targetTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(typeName));
  vDef.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vDef, targetTypeNameIdx);
}

std::list<UniqueFEIRStmt> DexOpCheckCast::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  if (vA.GetPrimType() == PTY_i32) {
    // only null ptr approach here
    UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtRetype(vDef.GenFEIRVarReg(), vA.GenFEIRVarReg());
    stmts.emplace_back(std::move(stmt));
    stmt = FEIRBuilder::CreateStmtJavaCheckCast(vDef.GenFEIRVarReg(), vDef.GenFEIRVarReg(),
        FEIRBuilder::CreateTypeByJavaName(GlobalTables::GetStrTable().GetStringFromStrIdx(targetTypeNameIdx), true),
        targetDexTypeIdx);
    stmts.emplace_back(std::move(stmt));
  } else {
    UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtJavaCheckCast(vDef.GenFEIRVarReg(), vA.GenFEIRVarReg(),
        FEIRBuilder::CreateTypeByJavaName(GlobalTables::GetStrTable().GetStringFromStrIdx(targetTypeNameIdx), true),
        targetDexTypeIdx);
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ========== DexOpInstanceOf =========
DexOpInstanceOf::DexOpInstanceOf(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpInstanceOf::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetBooleanIdx());
}

void DexOpInstanceOf::SetVBImpl(uint32 num) {
  vB.regNum = num;
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetJavaObjectNameMplIdx(), true);
  usedRegs.emplace_back(&vB);
}

void DexOpInstanceOf::SetVCImpl(uint32 num) {
  targetDexTypeIdx = num;
}

void DexOpInstanceOf::ParseImpl(BCClassMethod &method) {
  typeName = method.GetBCClass().GetBCParser().GetReader()->GetTypeNameFromIdx(targetDexTypeIdx);
}

std::list<UniqueFEIRStmt> DexOpInstanceOf::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  const std::string &targetTypeName = namemangler::EncodeName(typeName);
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtJavaInstanceOf(vA.GenFEIRVarReg(), vB.GenFEIRVarReg(),
                                                              FEIRBuilder::CreateTypeByJavaName(targetTypeName, true),
                                                              targetDexTypeIdx);
  stmts.emplace_back(std::move(stmt));
  typeName.clear();
  typeName.shrink_to_fit();
  return stmts;
}

// ========== DexOpArrayLength =========
DexOpArrayLength::DexOpArrayLength(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpArrayLength::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetIntIdx());
}

void DexOpArrayLength::SetVBImpl(uint32 num) {
  vB.regNum = num;
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetABooleanIdx(), true);
  usedRegs.emplace_back(&vB);
}

std::list<UniqueFEIRStmt> DexOpArrayLength::EmitToFEIRStmtsImpl() {
  CHECK_FATAL(BCUtil::IsArrayType(vB.regTypeItem->typeNameIdx),
      "Invalid array type: %s in DexOpArrayLength",
      GlobalTables::GetStrTable().GetStringFromStrIdx(vB.regTypeItem->typeNameIdx).c_str());
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtArrayLength(vA.GenFEIRVarReg(), vB.GenFEIRVarReg());
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpNewInstance =========
DexOpNewInstance::DexOpNewInstance(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpNewInstance::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
}

void DexOpNewInstance::SetVBImpl(uint32 num) {
  vA.dexTypeIdx = num;
}

void DexOpNewInstance::ParseImpl(BCClassMethod &method) {
  const std::string &typeName = method.GetBCClass().GetBCParser().GetReader()->GetTypeNameFromIdx(vA.dexTypeIdx);
  // we should register vA in defs, even if it was new-instance java.lang.String.
  // Though new-instance java.lang.String would be replaced by StringFactory at following java.lang.String.<init>,
  // and vA is defed by StringFactory.
  // In some cases, vA may be used between new-instance and its <init> invokation.
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA,
      GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(typeName)));
  defedRegs.emplace_back(&vA);
  if (method.IsRcPermanent()) {
    isRcPermanent = true;
    method.SetIsRcPermanent(false);
  }
}

std::list<UniqueFEIRStmt> DexOpNewInstance::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  if (isSkipNewString) {
    return stmts;
  }
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmtCall = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_JAVA_CLINIT_CHECK, std::make_unique<FEIRTypeDefault>(PTY_ref, vA.regTypeItem->typeNameIdx), nullptr,
                                                                 vA.dexTypeIdx);
  stmts.emplace_back(std::move(stmtCall));
  UniqueFEIRExpr exprJavaNewInstance = FEIRBuilder::CreateExprJavaNewInstance(
      vA.GenFEIRType(), vA.dexTypeIdx, isRcPermanent);
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vA.GenFEIRVarReg(), std::move(exprJavaNewInstance));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpNewArray =========
DexOpNewArray::DexOpNewArray(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpNewArray::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpNewArray::SetVBImpl(uint32 num) {
  vB.regNum = num;
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx());
  usedRegs.emplace_back(&vB);
}

void DexOpNewArray::SetVCImpl(uint32 num) {
  vA.dexTypeIdx = num;
}

void DexOpNewArray::ParseImpl(BCClassMethod &method) {
  const std::string &typeName = method.GetBCClass().GetBCParser().GetReader()->GetTypeNameFromIdx(vA.dexTypeIdx);
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA,
      GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(typeName)));
  if (method.IsRcPermanent()) {
    isRcPermanent = true;
    method.SetIsRcPermanent(false);
  }
}

std::list<UniqueFEIRStmt> DexOpNewArray::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr exprSize = FEIRBuilder::CreateExprDRead(vB.GenFEIRVarReg());
  UniqueFEIRExpr exprJavaNewArray = FEIRBuilder::CreateExprJavaNewArray(vA.GenFEIRType(), std::move(exprSize),
                                                                        vA.dexTypeIdx, isRcPermanent);
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vA.GenFEIRVarReg(), std::move(exprJavaNewArray));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpFilledNewArray =========
DexOpFilledNewArray::DexOpFilledNewArray(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn), argRegs(allocator.Adapter()), vRegs(allocator.Adapter()) {
  isRange = (opcode == kDexOpFilledNewArrayRange);
}

void DexOpFilledNewArray::SetVAImpl(uint32 num) {
  argsSize = num;
}

void DexOpFilledNewArray::SetVBImpl(uint32 num) {
  dexArrayTypeIdx = num;
}

void DexOpFilledNewArray::SetArgsImpl(const MapleList<uint32> &args) {
  argRegs = args;
}

GStrIdx DexOpFilledNewArray::GetReturnType() const {
  return arrayTypeNameIdx;
}

void DexOpFilledNewArray::ParseImpl(BCClassMethod &method) {
  const std::string &typeName = method.GetBCClass().GetBCParser().GetReader()->GetTypeNameFromIdx(dexArrayTypeIdx);
  const std::string &typeNameMpl = namemangler::EncodeName(typeName);
  arrayTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeNameMpl);
  const std::string &elemTypeName = GetArrayElementTypeFromArrayType(typeNameMpl);
  elemTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(elemTypeName);
  for (uint32 regNum : argRegs) {
    DexReg reg;
    reg.regNum = regNum;
    reg.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(elemTypeNameIdx);
    vRegs.emplace_back(reg);
  }
  for (auto &reg : vRegs) {
    usedRegs.emplace_back(&reg);
  }
}

std::list<UniqueFEIRStmt> DexOpFilledNewArray::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  auto exprRegList = std::make_unique<std::list<UniqueFEIRExpr>>();
  for (auto vReg : vRegs) {
    UniqueFEIRExpr exprReg = FEIRBuilder::CreateExprDRead(vReg.GenFEIRVarReg());
    exprRegList->emplace_back(std::move(exprReg));
  }
  UniqueFEIRVar var = nullptr;
  if (HasReturn()) {
    static_cast<DexOpMoveResult*>(returnInst)->SetVATypeNameIdx(arrayTypeNameIdx);
    var = static_cast<DexOpMoveResult*>(returnInst)->GetVA().GenFEIRVarReg();
  } else {
    return stmts;
  }
  const std::string &elemName = GlobalTables::GetStrTable().GetStringFromStrIdx(elemTypeNameIdx);
  UniqueFEIRType elemType = FEIRTypeHelper::CreateTypeByJavaName(elemName, true, false);
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_JAVA_FILL_NEW_ARRAY, std::move(elemType), std::move(var), std::move(exprRegList));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpFillArrayData =========
DexOpFillArrayData::DexOpFillArrayData(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpFillArrayData::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetVoidIdx(), true);
  usedRegs.emplace_back(&vA);
}

void DexOpFillArrayData::SetVBImpl(uint32 num) {
  offset = static_cast<int32>(num); // signed value in unsigned data buffer
}

void DexOpFillArrayData::ParseImpl(BCClassMethod &method) {
  const uint16 *data = method.GetInstPos() + pc + offset;
  size = *(reinterpret_cast<const uint32*>(&data[2]));
  arrayData = reinterpret_cast<const int8*>(&data[4]);
}

std::list<UniqueFEIRStmt> DexOpFillArrayData::EmitToFEIRStmtsImpl() {
  CHECK_FATAL(BCUtil::IsArrayType(vA.regTypeItem->typeNameIdx),
      "Invalid array type: %s in DexOpFillArrayData",
      GlobalTables::GetStrTable().GetStringFromStrIdx(vA.regTypeItem->typeNameIdx).c_str());
  std::list<UniqueFEIRStmt> stmts;
  thread_local static std::stringstream ss("");
  ss.str("");
  ss << "const_array_" << funcNameIdx << "_" << pc;
  UniqueFEIRVar arrayReg = vA.GenFEIRVarReg();
  const std::string &arrayTypeName = GlobalTables::GetStrTable().GetStringFromStrIdx(vA.regTypeItem->typeNameIdx);
  CHECK_FATAL(arrayTypeName.size() > 1 && arrayTypeName.at(0) == 'A' &&
      BCUtil::IsJavaPrimitveType(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrayTypeName.substr(1))),
      "Invalid type %s", arrayTypeName.c_str());
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtJavaFillArrayData(std::move(arrayReg), arrayData, size, ss.str());
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpThrow =========
DexOpThrow::DexOpThrow(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpThrow::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetJavaThrowableNameMplIdx(), true);
  usedRegs.emplace_back(&vA);
}

std::list<UniqueFEIRStmt> DexOpThrow::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  if (BCUtil::IsJavaReferenceType(vA.regTypeItem->typeNameIdx)) {
    UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(vA.GenFEIRVarReg());
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtUseOnly>(OP_throw, std::move(expr));
    stmts.emplace_back(std::move(stmt));
  } else {
    CHECK_FATAL(vA.regValue != nullptr && vA.regValue->primValue.raw32 == 0, "only null or ref can be thrown.");
    DexReg vAtmp;
    vAtmp.regNum = vA.regNum;
    vAtmp.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vAtmp, BCUtil::GetJavaThrowableNameMplIdx());
    UniqueFEIRStmt retypeStmt = FEIRBuilder::CreateStmtRetype(vAtmp.GenFEIRVarReg(), vA.GenFEIRVarReg());
    stmts.emplace_back(std::move(retypeStmt));
    UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(vAtmp.GenFEIRVarReg());
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtUseOnly>(OP_throw, std::move(expr));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ========== DexOpGoto =========
DexOpGoto::DexOpGoto(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpGoto::SetVAImpl(uint32 num) {
  offset = static_cast<int32>(num);
}

std::vector<uint32> DexOpGoto::GetTargetsImpl() const {
  std::vector<uint32> res;
  res.emplace_back(offset + pc);
  return res;
}

void DexOpGoto::ParseImpl(BCClassMethod &method) {
  (void) method;
  target = offset + pc;
}

std::list<UniqueFEIRStmt> DexOpGoto::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtGoto2>(funcNameIdx, target);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpSwitch =========
DexOpSwitch::DexOpSwitch(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn), keyTargetOPpcMap(allocator.Adapter()) {
  isPacked = (opcode == kDexOpPackedSwitch);
}

void DexOpSwitch::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx());
  usedRegs.emplace_back(&vA);
}

void DexOpSwitch::SetVBImpl(uint32 num) {
  offset = static_cast<int32>(num);
}

void DexOpSwitch::ParseImpl(BCClassMethod &method) {
  const uint16 *data = method.GetInstPos() + pc + offset;
  if (isPacked) {
    uint16 size = data[1];
    int32 key = *(reinterpret_cast<const int32*>(&data[2]));
    const int32 *targets = reinterpret_cast<const int32*>(&data[4]);
    for (uint16 i = 0; i < size; i++) {
      uint32 target = pc + targets[i];
      keyTargetOPpcMap.emplace(key, std::make_pair(target, pc));
      key = key + 1;
    }
  } else {
    uint16 size = data[1];
    const int32 *keys = reinterpret_cast<const int32*>(&data[2]);
    const int32 *targets = reinterpret_cast<const int32*>(&data[2 + size * 2]);
    for (uint16 i = 0; i < size; i++) {
      int32 key = keys[i];
      uint32 target = pc + targets[i];
      keyTargetOPpcMap.emplace(key, std::make_pair(target, pc));
    }
  }
}

std::vector<uint32> DexOpSwitch::GetTargetsImpl() const {
  std::vector<uint32> res;
  for (const auto &elem : keyTargetOPpcMap) {
    res.emplace_back(elem.second.first);
  }
  return res;
}

std::list<UniqueFEIRStmt> DexOpSwitch::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr expr = std::make_unique<FEIRExprDRead>(vA.GenFEIRVarReg());
  std::unique_ptr<FEIRStmtSwitch2> stmt = std::make_unique<FEIRStmtSwitch2>(funcNameIdx, std::move(expr));
  if (defaultTarget == nullptr) {
    stmt->SetDefaultLabelIdx(UINT32_MAX);  // no default target, switch is the last instruction of the file
  } else {
    stmt->SetDefaultLabelIdx(defaultTarget->GetPC());
  }
  for (auto &e : keyTargetOPpcMap) {
    stmt->AddTarget(e.first, e.second.first);
  }
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpCompare =========
DexOpCompare::DexOpCompare(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}
void DexOpCompare::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetIntIdx());
}

void DexOpCompare::SetVBImpl(uint32 num) {
  vB.regNum = num;
  GStrIdx typeNameIdx;
  if (opcode == kDexOpCmplFloat || opcode == kDexOpCmpgFloat) {
    typeNameIdx = BCUtil::GetFloatIdx();
  } else if (opcode == kDexOpCmplDouble || opcode == kDexOpCmpgDouble) {
    typeNameIdx = BCUtil::GetDoubleIdx();
  } else {
    // kDexOpCmpLong
    typeNameIdx = BCUtil::GetLongIdx();
  }
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(typeNameIdx);
  usedRegs.emplace_back(&vB);
}

void DexOpCompare::SetVCImpl(uint32 num) {
  vC = vB;
  vC.regNum = num;
  vC.regTypeItem = vB.regTypeItem;
  usedRegs.emplace_back(&vC);
}

Opcode DexOpCompare::GetOpcodeFromDexIns() const {
  auto mirOp = dexOpCmp2MIROp.find(opcode);
  CHECK_FATAL(mirOp != dexOpCmp2MIROp.end(), "Invalid opcode: 0x%x in dexOpCmp2MIROp", opcode);
  return mirOp->second;
}

std::list<UniqueFEIRStmt> DexOpCompare::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr cmpExpr =
      FEIRBuilder::CreateExprMathBinary(GetOpcodeFromDexIns(), vB.GenFEIRVarReg(), vC.GenFEIRVarReg());
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vA.GenFEIRVarReg(), std::move(cmpExpr));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpIfTest =========
DexOpIfTest::DexOpIfTest(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpIfTest::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx(), true);
  usedRegs.emplace_back(&vA);
}

void DexOpIfTest::SetVBImpl(uint32 num) {
  vB.regNum = num;
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx(), true);
  usedRegs.emplace_back(&vB);
}

void DexOpIfTest::SetVCImpl(uint32 num) {
  offset = static_cast<int32>(num);
}

std::vector<uint32> DexOpIfTest::GetTargetsImpl() const {
  std::vector<uint32> res;
  res.emplace_back(offset + pc);
  return res;
}

void DexOpIfTest::ParseImpl(BCClassMethod &method) {
  (void) method;
  target = offset + pc;
}

std::list<UniqueFEIRStmt> DexOpIfTest::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  DexReg vATmp;
  DexReg vBTmp;
  vATmp = vA;
  vBTmp = vB;
  // opnd of if-test should be same PTY type.
  // Or one opnd must be nullptr, such as const 0.
  if (vA.GetPrimType() == PTY_ref && vB.GetPrimType() != PTY_ref) {
    CHECK_FATAL(vB.regValue != nullptr && vB.regValue->primValue.raw32 == 0,
        "Cannot compare a ref var with an integer.");
    vBTmp.regTypeItem = vATmp.regTypeItem;
    UniqueFEIRStmt retypeStmt = FEIRBuilder::CreateStmtRetype(vBTmp.GenFEIRVarReg(), vB.GenFEIRVarReg());
    stmts.emplace_back(std::move(retypeStmt));
  } else if (vA.GetPrimType() != PTY_ref && vB.GetPrimType() == PTY_ref) {
    CHECK_FATAL(vA.regValue != nullptr && vA.regValue->primValue.raw32 == 0,
        "Cannot compare a ref var with an integer.");
    vATmp.regTypeItem = vBTmp.regTypeItem;
    UniqueFEIRStmt retypeStmt = FEIRBuilder::CreateStmtRetype(vATmp.GenFEIRVarReg(), vA.GenFEIRVarReg());
    stmts.emplace_back(std::move(retypeStmt));
  }
  UniqueFEIRExpr exprA = std::make_unique<FEIRExprDRead>(vATmp.GenFEIRVarReg());
  UniqueFEIRExpr exprB = std::make_unique<FEIRExprDRead>(vBTmp.GenFEIRVarReg());
  auto itor = dexOpConditionOp2MIROp.find(static_cast<DexOpCode>(opcode));
  CHECK_FATAL(itor != dexOpConditionOp2MIROp.end(), "Invalid opcode: %u in DexOpIfTest", opcode);
  // All primitive var should be compared in i32.
  if (vATmp.GetPrimType() != PTY_ref) {
    if (vATmp.GetPrimType() != PTY_i32) {
      exprA = std::make_unique<FEIRExprTypeCvt>(std::make_unique<FEIRTypeDefault>(PTY_i32, BCUtil::GetIntIdx()),
                                                OP_cvt, std::move(exprA));
    }
    if (vBTmp.GetPrimType() != PTY_i32) {
      exprB = std::make_unique<FEIRExprTypeCvt>(std::make_unique<FEIRTypeDefault>(PTY_i32, BCUtil::GetIntIdx()),
                                                OP_cvt, std::move(exprB));
    }
  }
  std::unique_ptr<FEIRExprBinary> expr =
      std::make_unique<FEIRExprBinary>(itor->second, std::move(exprA), std::move(exprB));
  UniqueFEIRStmt stmt =
      std::make_unique<FEIRStmtCondGoto2>(OP_brtrue, funcNameIdx, target, std::move(expr));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpIfTestZ =========
DexOpIfTestZ::DexOpIfTestZ(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpIfTestZ::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx(), true);
  usedRegs.emplace_back(&vA);
}

void DexOpIfTestZ::SetVBImpl(uint32 num) {
  offset = static_cast<int32>(num);
}

std::vector<uint32> DexOpIfTestZ::GetTargetsImpl() const {
  std::vector<uint32> res;
  res.emplace_back(offset + pc);
  return res;
}

void DexOpIfTestZ::ParseImpl(BCClassMethod &method) {
  (void) method;
  target = offset + pc;
}

std::list<UniqueFEIRStmt> DexOpIfTestZ::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRStmt stmt = nullptr;
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(vA.GenFEIRVarReg());
  if (vA.GetPrimType() == PTY_u1 && opcode == kDexOpIfEqZ) {
    stmt = std::make_unique<FEIRStmtCondGoto2>(OP_brfalse, funcNameIdx, target, std::move(expr));
  } else if (vA.GetPrimType() == PTY_u1 && opcode == kDexOpIfNeZ) {
    stmt = std::make_unique<FEIRStmtCondGoto2>(OP_brtrue, funcNameIdx, target, std::move(expr));
  } else {
    UniqueFEIRExpr constExpr = std::make_unique<FEIRExprConst>(static_cast<int64>(0), vA.GetPrimType());
    auto itor = dexOpConditionOp2MIROp.find(static_cast<DexOpCode>(opcode));
    CHECK_FATAL(itor != dexOpConditionOp2MIROp.end(), "Invalid opcode: %u in DexOpIfTestZ", opcode);
    expr = FEIRBuilder::CreateExprMathBinary(itor->second, std::move(expr), std::move(constExpr));
    stmt = std::make_unique<FEIRStmtCondGoto2>(OP_brtrue, funcNameIdx, target, std::move(expr));
  }
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpUnused =========
DexOpUnused::DexOpUnused(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

// ========== DexOpAget =========
DexOpAget::DexOpAget(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpAget::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpAget::SetVBImpl(uint32 num) {
  vB.regNum = num;
  usedRegs.emplace_back(&vB);
}

void DexOpAget::SetVCImpl(uint32 num) {
  vC.regNum = num;
  vC.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx());
  usedRegs.emplace_front(&vC);
}

void DexOpAget::ParseImpl(BCClassMethod &method) {
  (void) method;
  GStrIdx elemTypeNameIdx;
  GStrIdx usedTypeNameIdx;
  bool isIndeterminateType = false;
  switch (opcode) {
    case kDexOpAget: {
      // int or float
      elemTypeNameIdx = BCUtil::GetIntIdx();
      usedTypeNameIdx = BCUtil::GetAIntIdx();
      isIndeterminateType = true;
      break;
    }
    case kDexOpAgetWide: {
      elemTypeNameIdx = BCUtil::GetLongIdx();
      usedTypeNameIdx = BCUtil::GetALongIdx();
      isIndeterminateType = true;
      break;
    }
    case kDexOpAgetObject: {
      elemTypeNameIdx = BCUtil::GetJavaObjectNameMplIdx();
      usedTypeNameIdx = BCUtil::GetAJavaObjectNameMplIdx();
      isIndeterminateType = true;
      break;
    }
    case kDexOpAgetBoolean: {
      elemTypeNameIdx = BCUtil::GetBooleanIdx();
      usedTypeNameIdx = BCUtil::GetABooleanIdx();
      break;
    }
    case kDexOpAgetByte: {
      elemTypeNameIdx = BCUtil::GetByteIdx();
      usedTypeNameIdx = BCUtil::GetAByteIdx();
      break;
    }
    case kDexOpAgetChar: {
      elemTypeNameIdx = BCUtil::GetCharIdx();
      usedTypeNameIdx = BCUtil::GetACharIdx();
      break;
    }
    case kDexOpAgetShort: {
      elemTypeNameIdx = BCUtil::GetShortIdx();
      usedTypeNameIdx = BCUtil::GetAShortIdx();
      break;
    }
    default: {
      CHECK_FATAL(false, "Invalid opcode : 0x%x in DexOpAget", opcode);
      break;
    }
  }
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, elemTypeNameIdx, isIndeterminateType);
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(usedTypeNameIdx, isIndeterminateType);
}

void DexOpAget::SetRegTypeInTypeInferImpl() {
  if (vB.regType != nullptr && vC.regType != nullptr) {
    vB.regType->AddElemType(vA.regTypeItem);
  }
}

std::list<UniqueFEIRStmt> DexOpAget::EmitToFEIRStmtsImpl() {
  DexReg vAtmp;
  vAtmp.regNum = vA.regNum;
  const std::string arrayTypeName = GlobalTables::GetStrTable().GetStringFromStrIdx(vB.regTypeItem->typeNameIdx);
  if (arrayTypeName.size() > 1 && arrayTypeName.at(0) == 'A') {
    vAtmp.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrayTypeName.substr(1)));
  } else {
    CHECK_FATAL(false, "Invalid array type %s", arrayTypeName.c_str());
  }
  std::list<UniqueFEIRStmt> stmts = FEIRBuilder::CreateStmtArrayLoad(vAtmp.GenFEIRVarReg(), vB.GenFEIRVarReg(),
                                                                     vC.GenFEIRVarReg());

  if (!((*vAtmp.regTypeItem) == (*vA.regTypeItem))) {
    UniqueFEIRStmt retypeStmt =
        FEIRBuilder::CreateStmtRetype(vA.GenFEIRVarReg(), vAtmp.GenFEIRVarReg());
    stmts.emplace_back(std::move(retypeStmt));
  }
  return stmts;
}

// ========== DexOpAput =========
DexOpAput::DexOpAput(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpAput::SetVAImpl(uint32 num) {
  vA.regNum = num;
  usedRegs.emplace_back(&vA);
}

void DexOpAput::SetVBImpl(uint32 num) {
  vB.regNum = num;
  usedRegs.emplace_back(&vB);
}

void DexOpAput::SetVCImpl(uint32 num) {
  vC.regNum = num;
  vC.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx());
  usedRegs.emplace_back(&vC);
}

void DexOpAput::ParseImpl(BCClassMethod &method) {
  (void) method;
  GStrIdx elemTypeNameIdx;
  GStrIdx usedTypeNameIdx;
  bool isIndeterminate = false;
  switch (opcode) {
    case kDexOpAput: {
      // Int or float
      elemTypeNameIdx = BCUtil::GetIntIdx();
      usedTypeNameIdx = BCUtil::GetAIntIdx();
      isIndeterminate = true;
      break;
    }
    case kDexOpAputWide: {
      elemTypeNameIdx = BCUtil::GetLongIdx();
      usedTypeNameIdx = BCUtil::GetALongIdx();
      isIndeterminate = true;
      break;
    }
    case kDexOpAputObject: {
      elemTypeNameIdx = BCUtil::GetJavaObjectNameMplIdx();
      usedTypeNameIdx = BCUtil::GetAJavaObjectNameMplIdx();
      isIndeterminate = true;
      break;
    }
    case kDexOpAputBoolean: {
      elemTypeNameIdx = BCUtil::GetBooleanIdx();
      usedTypeNameIdx = BCUtil::GetABooleanIdx();
      break;
    }
    case kDexOpAputByte: {
      elemTypeNameIdx = BCUtil::GetByteIdx();
      usedTypeNameIdx = BCUtil::GetAByteIdx();
      break;
    }
    case kDexOpAputChar: {
      elemTypeNameIdx = BCUtil::GetCharIdx();
      usedTypeNameIdx = BCUtil::GetACharIdx();
      break;
    }
    case kDexOpAputShort: {
      elemTypeNameIdx = BCUtil::GetShortIdx();
      usedTypeNameIdx = BCUtil::GetAShortIdx();
      break;
    }
    default: {
      CHECK_FATAL(false, "Invalid opcode : 0x%x in DexOpAput", opcode);
      break;
    }
  }
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(elemTypeNameIdx, isIndeterminate);
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(usedTypeNameIdx, isIndeterminate);
}

void DexOpAput::SetRegTypeInTypeInferImpl() {
  if (vA.regType != nullptr && vB.regType != nullptr && vC.regType != nullptr) {
    vB.regType->AddElemType(vA.regTypeItem);
  }
}

std::list<UniqueFEIRStmt> DexOpAput::EmitToFEIRStmtsImpl() {
  DexReg vAtmp;
  vAtmp.regNum = vA.regNum;
  const std::string &arrayTypeName = GlobalTables::GetStrTable().GetStringFromStrIdx(vB.regTypeItem->typeNameIdx);
  if (arrayTypeName.size() > 1 && arrayTypeName.at(0) == 'A') {
    vAtmp.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrayTypeName.substr(1)));
  } else {
    CHECK_FATAL(false, "Invalid array type %s", arrayTypeName.c_str());
  }
  std::list<UniqueFEIRStmt> stmts = FEIRBuilder::CreateStmtArrayStore(vAtmp.GenFEIRVarReg(), vB.GenFEIRVarReg(),
                                                                      vC.GenFEIRVarReg());
  if (!((*vAtmp.regTypeItem) == (*vA.regTypeItem))) {
    UniqueFEIRStmt retypeStmt =
        FEIRBuilder::CreateStmtRetype(vAtmp.GenFEIRVarReg(), vA.GenFEIRVarReg());
    stmts.emplace_front(std::move(retypeStmt));
  }
  return stmts;
}

// ========== DexOpIget =========
DexOpIget::DexOpIget(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpIget::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpIget::SetVBImpl(uint32 num) {
  vB.regNum = num;
  usedRegs.emplace_back(&vB);
}

void DexOpIget::SetVCImpl(uint32 num) {
  index = num;
}

void DexOpIget::ParseImpl(BCClassMethod &method) {
  BCReader::ClassElem field = method.GetBCClass().GetBCParser().GetReader()->GetClassFieldFromIdx(index);
  uint64 mapIdx = (static_cast<uint64>(method.GetBCClass().GetBCParser().GetReader()->GetFileIndex()) << 32) | index;
  structElemNameIdx = FEManager::GetManager().GetFieldStructElemNameIdx(mapIdx);
  if (structElemNameIdx == nullptr) {
    structElemNameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(
        field.className, field.elemName, field.typeName);
    FEManager::GetManager().SetFieldStructElemNameIdx(mapIdx, *structElemNameIdx);
  }
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, structElemNameIdx->type);
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(structElemNameIdx->klass);
}

std::list<UniqueFEIRStmt> DexOpIget::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  FEStructFieldInfo *fieldInfo = static_cast<FEStructFieldInfo*>(
      FEManager::GetTypeManager().RegisterStructFieldInfo(*structElemNameIdx, kSrcLangJava, false));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtFieldLoad>(vB.GenFEIRVarReg(), vA.GenFEIRVarReg(), *fieldInfo, false);
  stmts.push_back(std::move(stmt));
  return stmts;
}

// ========== DexOpIput =========
DexOpIput::DexOpIput(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpIput::SetVAImpl(uint32 num) {
  vA.regNum = num;
  usedRegs.emplace_back(&vA);
}

void DexOpIput::SetVBImpl(uint32 num) {
  vB.regNum = num;
  usedRegs.emplace_back(&vB);
}

void DexOpIput::SetVCImpl(uint32 num) {
  index = num;
}

void DexOpIput::ParseImpl(BCClassMethod &method) {
  BCReader::ClassElem field = method.GetBCClass().GetBCParser().GetReader()->GetClassFieldFromIdx(index);
  uint64 mapIdx = (static_cast<uint64>(method.GetBCClass().GetBCParser().GetReader()->GetFileIndex()) << 32) | index;
  structElemNameIdx = FEManager::GetManager().GetFieldStructElemNameIdx(mapIdx);
  if (structElemNameIdx == nullptr) {
    structElemNameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(
        field.className, field.elemName, field.typeName);
    FEManager::GetManager().SetFieldStructElemNameIdx(mapIdx, *structElemNameIdx);
  }
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(structElemNameIdx->type);
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(structElemNameIdx->klass);
}

std::list<UniqueFEIRStmt> DexOpIput::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  FEStructFieldInfo *fieldInfo = static_cast<FEStructFieldInfo*>(
      FEManager::GetTypeManager().RegisterStructFieldInfo(*structElemNameIdx, kSrcLangJava, false));
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtFieldStore>(vB.GenFEIRVarReg(), vA.GenFEIRVarReg(), *fieldInfo, false);
  stmts.push_back(std::move(stmt));
  return stmts;
}

// ========== DexOpSget =========
DexOpSget::DexOpSget(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpSget::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpSget::SetVBImpl(uint32 num) {
  index = num;
}

void DexOpSget::ParseImpl(BCClassMethod &method) {
  BCReader::ClassElem field = method.GetBCClass().GetBCParser().GetReader()->GetClassFieldFromIdx(index);
  uint64 mapIdx = (static_cast<uint64>(method.GetBCClass().GetBCParser().GetReader()->GetFileIndex()) << 32) | index;
  structElemNameIdx = FEManager::GetManager().GetFieldStructElemNameIdx(mapIdx);
  if (structElemNameIdx == nullptr) {
    structElemNameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(
        field.className, field.elemName, field.typeName);
    FEManager::GetManager().SetFieldStructElemNameIdx(mapIdx, *structElemNameIdx);
  }
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, structElemNameIdx->type);
  dexFileHashCode = method.GetBCClass().GetBCParser().GetFileNameHashId();
}

std::list<UniqueFEIRStmt> DexOpSget::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> ans;
  FEStructFieldInfo *fieldInfo = static_cast<FEStructFieldInfo*>(
      FEManager::GetTypeManager().RegisterStructFieldInfo(*structElemNameIdx, kSrcLangJava, true));
  CHECK_NULL_FATAL(fieldInfo);
  fieldInfo->SetFieldID(index);
  UniqueFEIRVar var = vA.GenFEIRVarReg();
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtFieldLoad>(nullptr, std::move(var), *fieldInfo, true, dexFileHashCode);
  ans.emplace_back(std::move(stmt));
  return ans;
}

// ========== DexOpSput =========
DexOpSput::DexOpSput(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpSput::SetVAImpl(uint32 num) {
  vA.regNum = num;
  usedRegs.emplace_back(&vA);
}

void DexOpSput::SetVBImpl(uint32 num) {
  index = num;
}

void DexOpSput::ParseImpl(BCClassMethod &method) {
  BCReader::ClassElem field = method.GetBCClass().GetBCParser().GetReader()->GetClassFieldFromIdx(index);
  uint64 mapIdx = (static_cast<uint64>(method.GetBCClass().GetBCParser().GetReader()->GetFileIndex()) << 32) | index;
  structElemNameIdx = FEManager::GetManager().GetFieldStructElemNameIdx(mapIdx);
  if (structElemNameIdx == nullptr) {
    structElemNameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(
        field.className, field.elemName, field.typeName);
    FEManager::GetManager().SetFieldStructElemNameIdx(mapIdx, *structElemNameIdx);
  }
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(structElemNameIdx->type);
  dexFileHashCode = method.GetBCClass().GetBCParser().GetFileNameHashId();
}

std::list<UniqueFEIRStmt> DexOpSput::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  FEStructFieldInfo *fieldInfo = static_cast<FEStructFieldInfo*>(
      FEManager::GetTypeManager().RegisterStructFieldInfo(*structElemNameIdx, kSrcLangJava, true));
  CHECK_NULL_FATAL(fieldInfo);
  UniqueFEIRVar var = vA.GenFEIRVarReg();
  fieldInfo->SetFieldID(index);
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtFieldStore>(nullptr, std::move(var), *fieldInfo, true,
                                                             dexFileHashCode);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpInvoke =========
DexOpInvoke::DexOpInvoke(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn), argRegs(allocator.Adapter()), argVRegs(allocator.Adapter()) {}

std::string DexOpInvoke::GetReturnType() const {
  CHECK_FATAL(!retArgsTypeNames.empty(), "Should parse DexOpInvoke first for retrieving signature.");
  return retArgsTypeNames[0];
}

void DexOpInvoke::SetVAImpl(uint32 num) {
  argSize = num;
}

void DexOpInvoke::SetVBImpl(uint32 num) {
  methodIdx = num;
}

void DexOpInvoke::SetVCImpl(uint32 num) {
  arg0VRegNum = num;
}

void DexOpInvoke::SetArgsImpl(const MapleList<uint32> &args) {
  argRegs = args;
}

bool DexOpInvoke::ReplaceStringFactory(BCReader::ClassElem &methodInfo, MapleList<uint32> &argRegNums) {
  const std::string &fullName = methodInfo.className + "|" + methodInfo.elemName + "|" + methodInfo.typeName;
  if (!DexStrFactory::IsStringInit(fullName)) {
    return false;
  }
  isStringFactory = true;
  std::string strFacCalleeName = DexStrFactory::GetStringFactoryFuncname(fullName);
  CHECK_FATAL(!strFacCalleeName.empty(), "Can not find string factory call for %s", fullName.c_str());
  std::vector<std::string> names = FEUtils::Split(strFacCalleeName, '|');
  // 3 parts: ClassName|ElemName|Signature
  CHECK_FATAL(names.size() == 3, "Invalid funcname %s", strFacCalleeName.c_str());
  methodInfo.className = names.at(0);
  methodInfo.elemName = names.at(1);
  methodInfo.typeName = names.at(2);
  // push this as return
  retReg.regNum = argRegNums.front();
  retReg.isDef = true;
  argRegNums.pop_front();
  retReg.regType = allocator.GetMemPool()->New<BCRegType>(allocator, retReg, BCUtil::GetJavaStringNameMplIdx());
  defedRegs.emplace_back(&retReg);
  return true;
}

void DexOpInvoke::ParseImpl(BCClassMethod &method) {
  MapleList<uint32> &argRegNums = argRegs;
  uint64 mapIdx = (static_cast<uint64>(method.GetBCClass().GetBCParser().GetReader()->GetFileIndex()) << 32) |
      methodIdx;
  BCReader::ClassElem methodInfo = method.GetBCClass().GetBCParser().GetReader()->GetClassMethodFromIdx(methodIdx);
  // for string factory, the highest bit for string factory flag
  if (ReplaceStringFactory(methodInfo, argRegNums)) {
    mapIdx = mapIdx | 0x8000000000000000;
  }
  structElemNameIdx = FEManager::GetManager().GetMethodStructElemNameIdx(mapIdx);
  if (structElemNameIdx == nullptr) {
    structElemNameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(
        methodInfo.className, methodInfo.elemName, methodInfo.typeName);
    FEManager::GetManager().SetMethodStructElemNameIdx(mapIdx, *structElemNameIdx);
  }
  retArgsTypeNames = FEUtilJava::SolveMethodSignature(methodInfo.typeName);
  DexReg reg;
  if (!IsStatic()) {
    reg.regNum = argRegNums.front();
    argRegNums.pop_front();
    reg.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(structElemNameIdx->klass);
    argVRegs.emplace_back(reg);
  }
  for (size_t i = 1; i < retArgsTypeNames.size(); ++i) {
    reg.regNum = argRegNums.front();
    argRegNums.pop_front();
    GStrIdx usedTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        namemangler::EncodeName(retArgsTypeNames[i]));
    if (BCUtil::IsWideType(usedTypeNameIdx)) {
      argRegNums.pop_front();  // next regNum is consumed by wide type(long, double)
    }
    reg.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(usedTypeNameIdx);
    argVRegs.emplace_back(reg);
  }
  for (auto &vReg : argVRegs) {
    usedRegs.emplace_back(&vReg);
  }
}

void DexOpInvoke::PrepareInvokeParametersAndReturn(const FEStructMethodInfo &feMethodInfo,
                                                   FEIRStmtCallAssign &stmt) const {
  const MapleVector<FEIRType*> &argTypes = feMethodInfo.GetArgTypes();
  for (size_t i = argTypes.size(); i > 0; --i) {
    UniqueFEIRVar var = argVRegs[i - 1 + (IsStatic() ? 0 : 1)].GenFEIRVarReg();
    UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(var));
    stmt.AddExprArgReverse(std::move(expr));
  }
  if (!IsStatic()) {
    // push this
    UniqueFEIRVar var = argVRegs[0].GenFEIRVarReg();
    UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(var));
    stmt.AddExprArgReverse(std::move(expr));
  }
  if (HasReturn()) {
    static_cast<DexOpMoveResult*>(returnInst)->SetVATypeNameIdx(
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(retArgsTypeNames.at(0))));
    UniqueFEIRVar var = static_cast<DexOpMoveResult*>(returnInst)->GetVA().GenFEIRVarReg();
    stmt.SetVar(std::move(var));
  }
  if (isStringFactory) {
    stmt.SetVar(retReg.GenFEIRVarReg());
  }
}

std::list<UniqueFEIRStmt> DexOpInvoke::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> ans;
  UniqueFEIRStmt stmt;
  FEStructMethodInfo *info = static_cast<FEStructMethodInfo*>(
      FEManager::GetTypeManager().RegisterStructMethodInfo(*structElemNameIdx, kSrcLangJava, IsStatic()));
  auto itor = dexOpInvokeOp2MIROp.find(static_cast<DexOpCode>(opcode));
  CHECK_FATAL(itor != dexOpInvokeOp2MIROp.end(), "Unsupport opcode: 0x%x in DexOpInvoke", opcode);
  stmt = std::make_unique<FEIRStmtCallAssign>(*info, itor->second, nullptr, IsStatic());
  FEIRStmtCallAssign *ptrStmt = static_cast<FEIRStmtCallAssign*>(stmt.get());
  PrepareInvokeParametersAndReturn(*info, *ptrStmt);
  ans.emplace_back(std::move(stmt));
  retArgsTypeNames.clear();
  retArgsTypeNames.shrink_to_fit();
  return ans;
}

bool DexOpInvoke::IsStatic() const {
  return opcode == kDexOpInvokeStatic || opcode == kDexOpInvokeStaticRange || isStringFactory;
}

// ========== DexOpInvokeVirtual =========
DexOpInvokeVirtual::DexOpInvokeVirtual(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOpInvoke(allocatorIn, pcIn, opcodeIn) {}

// ========== DexOpInvokeSuper =========
DexOpInvokeSuper::DexOpInvokeSuper(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOpInvoke(allocatorIn, pcIn, opcodeIn) {}

// ========== DexOpInvokeDirect =========
DexOpInvokeDirect::DexOpInvokeDirect(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOpInvoke(allocatorIn, pcIn, opcodeIn) {}

// ========== DexOpInvokeStatic =========
DexOpInvokeStatic::DexOpInvokeStatic(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOpInvoke(allocatorIn, pcIn, opcodeIn) {}

// ========== DexOpInvokeInterface =========
DexOpInvokeInterface::DexOpInvokeInterface(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOpInvoke(allocatorIn, pcIn, opcodeIn) {}

// ========== DexOpUnaryOp =========
DexOpUnaryOp::DexOpUnaryOp(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpUnaryOp::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
  auto it = GetOpcodeMapForUnary().find(opcode);
  CHECK_FATAL(it != GetOpcodeMapForUnary().end(), "Invalid opcode: %u in DexOpUnaryOp", opcode);
  mirOp = std::get<0>(it->second);
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, std::get<1>(it->second));
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(std::get<2>(it->second));
}

void DexOpUnaryOp::SetVBImpl(uint32 num) {
  vB.regNum = num;
  // vB's type is set in SetVAImpl
  usedRegs.emplace_back(&vB);
}

std::list<UniqueFEIRStmt> DexOpUnaryOp::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr exprDread = FEIRBuilder::CreateExprDRead(vB.GenFEIRVarReg());
  UniqueFEIRExpr experUnary = nullptr;
  switch (mirOp) {
    case OP_cvt:
      experUnary = FEIRBuilder::CreateExprCvtPrim(std::move(exprDread), vA.GetPrimType());
      break;
    case OP_sext:
      experUnary = FEIRBuilder::CreateExprSExt(std::move(exprDread), vA.GetPrimType());
      break;
    case OP_zext:
      experUnary = FEIRBuilder::CreateExprZExt(std::move(exprDread), vA.GetPrimType());
      break;
    case OP_neg:
    case OP_bnot:
      experUnary = FEIRBuilder::CreateExprMathUnary(mirOp, std::move(exprDread));
      break;
    default:
      CHECK_FATAL(false, "Invalid mir opcode: %u in DexOpUnaryOp", mirOp);
      break;
  }
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vA.GenFEIRVarReg(), std::move(experUnary));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

std::map<uint8, std::tuple<Opcode, GStrIdx, GStrIdx>> DexOpUnaryOp::InitOpcodeMapForUnary() {
  std::map<uint8, std::tuple<Opcode, GStrIdx, GStrIdx>> ans;
  ans[kDexOpIntToLong] = std::make_tuple(OP_cvt, BCUtil::GetLongIdx(), BCUtil::GetIntIdx());
  ans[kDexOpIntToFloat] = std::make_tuple(OP_cvt, BCUtil::GetFloatIdx(), BCUtil::GetIntIdx());
  ans[kDexOpIntToDouble] = std::make_tuple(OP_cvt, BCUtil::GetDoubleIdx(), BCUtil::GetIntIdx());
  ans[kDexOpLongToInt] = std::make_tuple(OP_cvt, BCUtil::GetIntIdx(), BCUtil::GetLongIdx());
  ans[kDexOpLongToFloat] = std::make_tuple(OP_cvt, BCUtil::GetFloatIdx(), BCUtil::GetLongIdx());
  ans[kDexOpLongToDouble] = std::make_tuple(OP_cvt, BCUtil::GetDoubleIdx(), BCUtil::GetLongIdx());
  ans[kDexOpFloatToInt] = std::make_tuple(OP_cvt, BCUtil::GetIntIdx(), BCUtil::GetFloatIdx());
  ans[kDexOpFloatToLong] = std::make_tuple(OP_cvt, BCUtil::GetLongIdx(), BCUtil::GetFloatIdx());
  ans[kDexOpFloatToDouble] = std::make_tuple(OP_cvt, BCUtil::GetDoubleIdx(), BCUtil::GetFloatIdx());
  ans[kDexOpDoubleToInt] = std::make_tuple(OP_cvt, BCUtil::GetIntIdx(), BCUtil::GetDoubleIdx());
  ans[kDexOpDoubleToLong] = std::make_tuple(OP_cvt, BCUtil::GetLongIdx(), BCUtil::GetDoubleIdx());
  ans[kDexOpDoubleToFloat] = std::make_tuple(OP_cvt, BCUtil::GetFloatIdx(), BCUtil::GetDoubleIdx());
  ans[kDexOpIntToByte] = std::make_tuple(OP_sext, BCUtil::GetByteIdx(), BCUtil::GetIntIdx());
  ans[kDexOpIntToChar] = std::make_tuple(OP_zext, BCUtil::GetCharIdx(), BCUtil::GetIntIdx());
  ans[kDexOpIntToShort] = std::make_tuple(OP_sext, BCUtil::GetShortIdx(), BCUtil::GetIntIdx());
  ans[kDexOpNegInt] = std::make_tuple(OP_neg, BCUtil::GetIntIdx(), BCUtil::GetIntIdx());
  ans[kDexOpNotInt] = std::make_tuple(OP_bnot, BCUtil::GetIntIdx(), BCUtil::GetIntIdx());
  ans[kDexOpNegLong] = std::make_tuple(OP_neg, BCUtil::GetLongIdx(), BCUtil::GetLongIdx());
  ans[kDexOpNotLong] = std::make_tuple(OP_bnot, BCUtil::GetLongIdx(), BCUtil::GetLongIdx());
  ans[kDexOpNegFloat] = std::make_tuple(OP_neg, BCUtil::GetFloatIdx(), BCUtil::GetFloatIdx());
  ans[kDexOpNegDouble] = std::make_tuple(OP_neg, BCUtil::GetDoubleIdx(), BCUtil::GetDoubleIdx());
  return ans;
}

// ========== DexOpBinaryOp =========
DexOpBinaryOp::DexOpBinaryOp(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpBinaryOp::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
  GStrIdx typeNameIdx; // typeName of A, B, C are same
  if (kDexOpAddInt <= opcode && opcode <= kDexOpUshrInt) {
    typeNameIdx = BCUtil::GetIntIdx();
  } else if (kDexOpAddLong <= opcode && opcode <= kDexOpUshrLong) {
    typeNameIdx = BCUtil::GetLongIdx();
  } else if (kDexOpAddFloat <= opcode && opcode <= kDexOpRemFloat) {
    typeNameIdx = BCUtil::GetFloatIdx();
  } else if (kDexOpAddDouble <= opcode && opcode <= kDexOpRemDouble) {
    typeNameIdx = BCUtil::GetDoubleIdx();
  } else {
    CHECK_FATAL(false, "Invalid opcode: 0x%x in DexOpBinaryOp", opcode);
  }
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, typeNameIdx);
}

void DexOpBinaryOp::SetVBImpl(uint32 num) {
  vB.regNum = num;
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(*(vA.regTypeItem));
  usedRegs.emplace_back(&vB);
}

void DexOpBinaryOp::SetVCImpl(uint32 num) {
  vC.regNum = num;
  if (kDexOpShlLong <= opcode && opcode <= kDexOpUshrLong) {
    vC.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx());
  } else {
    vC.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(*(vA.regTypeItem));
  }
  usedRegs.emplace_back(&vC);
}

Opcode DexOpBinaryOp::GetOpcodeFromDexIns(void) const {
  auto mirOp = dexOp2MIROp.find(opcode);
  CHECK_FATAL(mirOp != dexOp2MIROp.end(), "Invalid opcode: 0x%x in DexOpBinaryOp", opcode);
  return mirOp->second;
}

std::list<UniqueFEIRStmt> DexOpBinaryOp::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> exprs;
  if (kDexOpAddInt <= opcode && opcode <= kDexOpRemDouble) {
    UniqueFEIRExpr exprA = std::make_unique<FEIRExprDRead>(vA.GenFEIRVarReg());
    UniqueFEIRExpr exprB = std::make_unique<FEIRExprDRead>(vB.GenFEIRVarReg());
    UniqueFEIRExpr exprC = std::make_unique<FEIRExprDRead>(vC.GenFEIRVarReg());
    std::unique_ptr<FEIRExprBinary> binaryOp2AddrExpr =
        std::make_unique<FEIRExprBinary>(GetOpcodeFromDexIns(), std::move(exprB), std::move(exprC));
    UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vA.GenFEIRVarReg(), std::move(binaryOp2AddrExpr));
    stmts.emplace_back(std::move(stmt));
  } else {
    CHECK_FATAL(false, "Invalid opcode: 0x%x in DexOpBinaryOp", opcode);
  }
  return stmts;
}

// ========== DexOpBinaryOp2Addr =========
DexOpBinaryOp2Addr::DexOpBinaryOp2Addr(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpBinaryOp2Addr::SetVAImpl(uint32 num) {
  vDef.regNum = num;
  vDef.isDef = true;
  vA.regNum = num;
  defedRegs.emplace_back(&vDef);
  usedRegs.emplace_back(&vA);
  // typeName of A, B are same, except `shl-long/2addr` ~ `ushr-long/2addr`
  GStrIdx typeNameAIdx;
  GStrIdx typeNameBIdx;
  if (kDexOpAddInt2Addr <= opcode && opcode <= kDexOpUshrInt2Addr) {
    typeNameAIdx = BCUtil::GetIntIdx();
    typeNameBIdx = typeNameAIdx;
  } else if (kDexOpAddLong2Addr <= opcode && opcode <= kDexOpXorLong2Addr) {
    typeNameAIdx = BCUtil::GetLongIdx();
    typeNameBIdx = typeNameAIdx;
  } else if (kDexOpShlLong2Addr <= opcode && opcode <= kDexOpUshrLong2Addr) {
    typeNameAIdx = BCUtil::GetLongIdx();
    typeNameBIdx = BCUtil::GetIntIdx();
  } else if (kDexOpAddFloat2Addr <= opcode && opcode <= kDexOpRemFloat2Addr) {
    typeNameAIdx = BCUtil::GetFloatIdx();
    typeNameBIdx = typeNameAIdx;
  } else if (kDexOpAddDouble2Addr <= opcode && opcode <= kDexOpRemDouble2Addr) {
    typeNameAIdx = BCUtil::GetDoubleIdx();
    typeNameBIdx = typeNameAIdx;
  } else {
    CHECK_FATAL(false, "Invalid opcode: 0x%x in DexOpBinaryOp2Addr", opcode);
  }
  vDef.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vDef, typeNameAIdx);
  vA.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(typeNameAIdx);
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(typeNameBIdx);
}

void DexOpBinaryOp2Addr::SetVBImpl(uint32 num) {
  vB.regNum = num;
  // type is set in SetVAImpl
  usedRegs.emplace_back(&vB);
}

Opcode DexOpBinaryOp2Addr::GetOpcodeFromDexIns(void) const {
  auto mirOp = dexOp2Addr2MIROp.find(opcode);
  CHECK_FATAL(mirOp != dexOp2Addr2MIROp.end(), "Invalid opcode: 0x%x in DexOpBinaryOp2Addr", opcode);
  return mirOp->second;
}

std::list<UniqueFEIRStmt> DexOpBinaryOp2Addr::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> exprs;
  if (kDexOpAddInt2Addr <= opcode && opcode <= kDexOpRemDouble2Addr) {
    UniqueFEIRExpr exprDst = std::make_unique<FEIRExprDRead>(vA.GenFEIRVarReg());
    UniqueFEIRExpr exprSrc = std::make_unique<FEIRExprDRead>(vB.GenFEIRVarReg());
    std::unique_ptr<FEIRExprBinary> binaryOp2AddrExpr =
        std::make_unique<FEIRExprBinary>(GetOpcodeFromDexIns(), std::move(exprDst), std::move(exprSrc));
    UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vA.GenFEIRVarReg(), std::move(binaryOp2AddrExpr));
    stmts.emplace_back(std::move(stmt));
  } else {
    CHECK_FATAL(false, "Invalid opcode: 0x%x in DexOpBinaryOp2Addr", opcode);
  }
  return stmts;
}

// ========== DexOpBinaryOpLit =========
DexOpBinaryOpLit::DexOpBinaryOpLit(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {
  isLit8 = (kDexOpAddIntLit8 <= opcode && opcode <= kDexOpUshrIntLit8);
}

void DexOpBinaryOpLit::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
  vA.regType = allocator.GetMemPool()->New<BCRegType>(allocator, vA, BCUtil::GetIntIdx());
}

void DexOpBinaryOpLit::SetVBImpl(uint32 num) {
  vB.regNum = num;
  vB.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetIntIdx());
  usedRegs.emplace_back(&vB);
}

void DexOpBinaryOpLit::SetVCImpl(uint32 num) {
  if (isLit8) {
    constValue.i8 = static_cast<int8>(num);
  } else {
    constValue.i16 = static_cast<int16>(num);
  }
}

Opcode DexOpBinaryOpLit::GetOpcodeFromDexIns() const {
  auto mirOp = dexOpLit2MIROp.find(opcode);
  CHECK_FATAL(mirOp != dexOpLit2MIROp.end(), "Invalid opcode: 0x%x in DexOpBinaryOpLit", opcode);
  return mirOp->second;
}

std::list<UniqueFEIRStmt> DexOpBinaryOpLit::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr exprConst = isLit8 ? FEIRBuilder::CreateExprConstI8(constValue.i8) :
                                      FEIRBuilder::CreateExprConstI16(constValue.i16);
  UniqueFEIRExpr exprCvt = FEIRBuilder::CreateExprCvtPrim(std::move(exprConst), PTY_i32);
  UniqueFEIRExpr exprDreadB = FEIRBuilder::CreateExprDRead(vB.GenFEIRVarReg());
  UniqueFEIRExpr exprBinary = nullptr;
  if (opcode == kDexOpRsubInt || opcode == kDexOpRsubIntLit8) {
    // reverse opnd
    exprBinary = FEIRBuilder::CreateExprMathBinary(GetOpcodeFromDexIns(), std::move(exprCvt), std::move(exprDreadB));
  } else {
    exprBinary = FEIRBuilder::CreateExprMathBinary(GetOpcodeFromDexIns(), std::move(exprDreadB), std::move(exprCvt));
  }
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(vA.GenFEIRVarReg(), std::move(exprBinary));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ========== DexOpInvokePolymorphic =========
DexOpInvokePolymorphic::DexOpInvokePolymorphic(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOpInvoke(allocatorIn, pcIn, opcodeIn) {}

void DexOpInvokePolymorphic::SetVHImpl(uint32 num) {
  protoIdx = num;
}

void DexOpInvokePolymorphic::ParseImpl(BCClassMethod &method) {
  isStatic = method.IsStatic();
  const BCReader::ClassElem &methodInfo =
      method.GetBCClass().GetBCParser().GetReader()->GetClassMethodFromIdx(methodIdx);
  const std::string &funcName = methodInfo.className + "|" + methodInfo.elemName + "|" + methodInfo.typeName;
  if (FEOptions::GetInstance().IsAOT()) {
    std::string callerClassName = method.GetBCClass().GetClassName(true);
    int32 dexFileHashCode = method.GetBCClass().GetBCParser().GetFileNameHashId();
    callerClassID = FEManager::GetTypeManager().GetTypeIDFromMplClassName(callerClassName, dexFileHashCode);
  }
  fullNameMpl = namemangler::EncodeName(funcName);
  protoName = method.GetBCClass().GetBCParser().GetReader()->GetSignature(protoIdx);
  retArgsTypeNames = FEUtilJava::SolveMethodSignature(protoName);
  DexReg reg;
  MapleList<uint32> argRegNums = argRegs;
  std::string typeName;
  // Receiver
  {
    reg.regNum = arg0VRegNum;
    reg.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(BCUtil::GetJavaMethodHandleNameMplIdx());
    argVRegs.emplace_back(reg);
  }
  if (opcode == kDexOpInvokePolymorphicRange) {
    argRegNums.pop_front(); // Skip first reg num for 'receiver'
  }
  for (size_t i = 1; i < retArgsTypeNames.size(); ++i) {
    reg.regNum = argRegNums.front();
    argRegNums.pop_front();
    typeName = retArgsTypeNames[i];
    typeName = namemangler::EncodeName(typeName);
    GStrIdx usedTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
    if (BCUtil::IsWideType(usedTypeNameIdx)) {
      argRegNums.pop_front();  // next regNum is consumed by wide type(long, double)
    }
    reg.regTypeItem = allocator.GetMemPool()->New<BCRegTypeItem>(usedTypeNameIdx);
    argVRegs.emplace_back(reg);
  }
  for (auto &vReg : argVRegs) {
    usedRegs.emplace_back(&vReg);
  }
}

std::list<UniqueFEIRStmt> DexOpInvokePolymorphic::EmitToFEIRStmtsImpl() {
  std::list<UniqueFEIRStmt> stmts;
  std::unique_ptr<std::list<UniqueFEIRVar>> args = std::make_unique<std::list<UniqueFEIRVar>>();
  for (const auto &reg : argVRegs) {
    args->emplace_back(reg.GenFEIRVarReg());
  }
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_JAVA_POLYMORPHIC_CALL, fullNameMpl, protoName, std::move(args), callerClassID, isStatic);
  if (HasReturn()) {
    static_cast<DexOpMoveResult*>(returnInst)->SetVATypeNameIdx(
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(retArgsTypeNames.at(0))));
    UniqueFEIRVar var = static_cast<DexOpMoveResult*>(returnInst)->GetVA().GenFEIRVarReg();
    stmt->SetVar(std::move(var));
  } else if (retArgsTypeNames[0] != "V") {
    GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("retvarpoly");
    std::unique_ptr<FEIRType> retTmpVarType = FEIRBuilder::CreateTypeByJavaName(retArgsTypeNames[0], false);
    UniqueFEIRVar retTmpVar = std::make_unique<FEIRVarName>(nameIdx, retTmpVarType->Clone(), false);
    stmt->SetVar(std::move(retTmpVar));
  }

  stmts.emplace_back(std::move(stmt));
  retArgsTypeNames.clear();
  retArgsTypeNames.shrink_to_fit();
  fullNameMpl.clear();
  fullNameMpl.shrink_to_fit();
  protoName.clear();
  protoName.shrink_to_fit();
  return stmts;
}

// ========== DexOpInvokeCustom =========
DexOpInvokeCustom::DexOpInvokeCustom(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn), argRegs(allocator.Adapter()), argVRegs(allocator.Adapter()) {}

void DexOpInvokeCustom::SetVBImpl(uint32 num) {
  callSiteIdx = num;
}

void DexOpInvokeCustom::SetArgsImpl(const MapleList<uint32> &args) {
  argRegs = args;
}

// ========== DexOpConstMethodHandle =========
DexOpConstMethodHandle::DexOpConstMethodHandle(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpConstMethodHandle::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpConstMethodHandle::SetVBImpl(uint32 num) {
  mhIdx = num;
}
// ========== DexOpConstMethodType =========
DexOpConstMethodType::DexOpConstMethodType(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn)
    : DexOp(allocatorIn, pcIn, opcodeIn) {}

void DexOpConstMethodType::SetVAImpl(uint32 num) {
  vA.regNum = num;
  vA.isDef = true;
  defedRegs.emplace_back(&vA);
}

void DexOpConstMethodType::SetVBImpl(uint32 num) {
  protoIdx = num;
}
}  // namespace bc
}  // namespace maple
