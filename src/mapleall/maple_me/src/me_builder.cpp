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
#include "me_builder.h"
#include "ssa_mir_nodes.h"
#include "factory.h"

namespace maple {
using MeExprBuildFactory = FunctionFactory<Opcode, MeExpr*, const MeBuilder*, BaseNode&>;

MeExpr &MeBuilder::CreateMeExpr(int32 exprId, MeExpr &meExpr) const {
  MeExpr *resultExpr = nullptr;
  switch (meExpr.GetMeOp()) {
    case kMeOpIvar:
      resultExpr = New<IvarMeExpr>(exprId, static_cast<IvarMeExpr&>(meExpr));
      break;
    case kMeOpOp:
      resultExpr = New<OpMeExpr>(static_cast<OpMeExpr&>(meExpr), exprId);
      break;
    case kMeOpConst:
      resultExpr = New<ConstMeExpr>(exprId, static_cast<ConstMeExpr&>(meExpr).GetConstVal(), meExpr.GetPrimType());
      break;
    case kMeOpConststr:
      resultExpr = New<ConststrMeExpr>(exprId, static_cast<ConststrMeExpr&>(meExpr).GetStrIdx(), meExpr.GetPrimType());
      break;
    case kMeOpConststr16:
      resultExpr = New<Conststr16MeExpr>(exprId, static_cast<Conststr16MeExpr&>(meExpr).GetStrIdx(), meExpr.GetPrimType());
      break;
    case kMeOpSizeoftype:
      resultExpr = New<SizeoftypeMeExpr>(exprId, meExpr.GetPrimType(), static_cast<SizeoftypeMeExpr&>(meExpr).GetTyIdx());
      break;
    case kMeOpFieldsDist: {
      auto &expr = static_cast<FieldsDistMeExpr&>(meExpr);
      resultExpr = New<FieldsDistMeExpr>(exprId, meExpr.GetPrimType(), expr.GetTyIdx(), expr.GetFieldID1(), expr.GetFieldID2());
      break;
    }
    case kMeOpAddrof:
      resultExpr = New<AddrofMeExpr>(exprId, meExpr.GetPrimType(), static_cast<AddrofMeExpr&>(meExpr).GetOstIdx());
      static_cast<AddrofMeExpr*>(resultExpr)->SetFieldID(static_cast<AddrofMeExpr&>(meExpr).GetFieldID());
      break;
    case kMeOpNary:
      resultExpr = NewInPool<NaryMeExpr>(exprId, static_cast<NaryMeExpr&>(meExpr));
      break;
    case kMeOpAddroffunc:
      resultExpr = New<AddroffuncMeExpr>(exprId, static_cast<AddroffuncMeExpr&>(meExpr).GetPuIdx());
      break;
    case kMeOpGcmalloc:
      resultExpr = New<GcmallocMeExpr>(exprId, meExpr.GetOp(), meExpr.GetPrimType(), static_cast<GcmallocMeExpr&>(meExpr).GetTyIdx());
      break;
    default:
      CHECK_FATAL(false, "not yet implement");
  }
  if (meExpr.GetMeOp() == kMeOpOp || meExpr.GetMeOp() == kMeOpNary) {
    resultExpr->UpdateDepth();
  }
  return *resultExpr;
}

VarMeExpr *MeBuilder::BuildVarMeExpr(int32 exprID, OStIdx oStIdx, size_t vStIdx,
                                     PrimType pType, FieldID fieldID) const {
  VarMeExpr *varMeExpr = New<VarMeExpr>(&allocator, exprID, oStIdx, vStIdx, pType);
  varMeExpr->SetFieldID(fieldID);
  return varMeExpr;
}

MeExpr *MeBuilder::BuildMeExpr(BaseNode &mirNode) const {
  auto func = CreateProductFunction<MeExprBuildFactory>(mirNode.GetOpCode());
  ASSERT(func != nullptr, "NIY BuildExpe");
  return func(this, mirNode);
}

MeExpr *MeBuilder::BuildAddrofMeExpr(BaseNode &mirNode) const {
  auto &addrofNode = static_cast<AddrofSSANode&>(mirNode);
  AddrofMeExpr &meExpr = *New<AddrofMeExpr>(kInvalidExprID, addrofNode.GetPrimType(), addrofNode.GetSSAVar()->GetOrigSt()->GetIndex());
  meExpr.SetFieldID(addrofNode.GetFieldID());
  return &meExpr;
}

MeExpr *MeBuilder::BuildAddroffuncMeExpr(BaseNode &mirNode) const {
  AddroffuncMeExpr &meExpr = *New<AddroffuncMeExpr>(kInvalidExprID, static_cast<AddroffuncNode&>(mirNode).GetPUIdx());
  return &meExpr;
}

MeExpr *MeBuilder::BuildGCMallocMeExpr(BaseNode &mirNode) const {
  GcmallocMeExpr &meExpr = *New<GcmallocMeExpr>(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), static_cast<GCMallocNode&>(mirNode).GetTyIdx());
  return &meExpr;
}

MeExpr *MeBuilder::BuildSizeoftypeMeExpr(BaseNode &mirNode) const {
  SizeoftypeMeExpr &meExpr = *New<SizeoftypeMeExpr>(kInvalidExprID, mirNode.GetPrimType(), static_cast<SizeoftypeNode&>(mirNode).GetTyIdx());
  return &meExpr;
}

MeExpr *MeBuilder::BuildFieldsDistMeExpr(BaseNode &mirNode) const {
  auto &fieldsDistNode = static_cast<FieldsDistNode&>(mirNode);
  FieldsDistMeExpr &meExpr = *New<FieldsDistMeExpr>(kInvalidExprID, mirNode.GetPrimType(), fieldsDistNode.GetTyIdx(),
                                                    fieldsDistNode.GetFiledID1(), fieldsDistNode.GetFiledID2());
  return &meExpr;
}

MeExpr *MeBuilder::BuildIvarMeExpr(BaseNode &mirNode) const {
  auto &ireadSSANode = static_cast<IreadSSANode&>(mirNode);
  IvarMeExpr &meExpr = *New<IvarMeExpr>(kInvalidExprID, mirNode.GetPrimType(), ireadSSANode.GetTyIdx(), ireadSSANode.GetFieldID());
  return &meExpr;
}

MeExpr *MeBuilder::BuildConstMeExpr(BaseNode &mirNode) const {
  auto &constvalNode = static_cast<ConstvalNode &>(mirNode);
  ConstMeExpr &meExpr = *New<ConstMeExpr>(kInvalidExprID, constvalNode.GetConstVal(), mirNode.GetPrimType());
  meExpr.SetOp(OP_constval);
  return &meExpr;
}

MeExpr *MeBuilder::BuildConststrMeExpr(BaseNode &mirNode) const {
  ConststrMeExpr &meExpr = *New<ConststrMeExpr>(kInvalidExprID, static_cast<ConststrNode&>(mirNode).GetStrIdx(), mirNode.GetPrimType());
  return &meExpr;
}

MeExpr *MeBuilder::BuildConststr16MeExpr(BaseNode &mirNode) const {
  Conststr16MeExpr &meExpr = *New<Conststr16MeExpr>(kInvalidExprID, static_cast<Conststr16Node&>(mirNode).GetStrIdx(), mirNode.GetPrimType());
  return &meExpr;
}

MeExpr *MeBuilder::BuildOpMeExprForCompare(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetOpndType(static_cast<CompareNode&>(mirNode).GetOpndType());
  return meExpr;
}

MeExpr *MeBuilder::BuildOpMeExprForTypeCvt(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetOpndType(static_cast<TypeCvtNode&>(mirNode).FromType());
  return meExpr;
}

MeExpr *MeBuilder::BuildOpMeExprForRetype(BaseNode &mirNode) const {
  auto &retypeNode = static_cast<RetypeNode&>(mirNode);
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetOpndType(retypeNode.FromType());
  meExpr->SetTyIdx(retypeNode.GetTyIdx());
  return meExpr;
}

MeExpr *MeBuilder::BuildOpMeExprForIread(BaseNode &mirNode) const {
  auto &ireadNode = static_cast<IreadNode&>(mirNode);
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetTyIdx(ireadNode.GetTyIdx());
  meExpr->SetFieldID(ireadNode.GetFieldID());
  return meExpr;
}

MeExpr *MeBuilder::BuildOpMeExprForExtractbits(BaseNode &mirNode) const {
  auto &extractbitsNode = static_cast<ExtractbitsNode&>(mirNode);
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetBitsOffSet(extractbitsNode.GetBitsOffset());
  meExpr->SetBitsSize(extractbitsNode.GetBitsSize());
  return meExpr;
}

MeExpr *MeBuilder::BuildOpMeExprForJarrayMalloc(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetTyIdx(static_cast<JarrayMallocNode&>(mirNode).GetTyIdx());
  return meExpr;
}

MeExpr *MeBuilder::BuildOpMeExprForResolveFunc(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetFieldID(static_cast<ResolveFuncNode&>(mirNode).GetPuIdx());
  return meExpr;
}

MeExpr *MeBuilder::BuildNaryMeExprForArray(BaseNode &mirNode) const {
  auto &arrayNode = static_cast<ArrayNode&>(mirNode);
  NaryMeExpr &meExpr =
      *NewInPool<NaryMeExpr>(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds(), arrayNode.GetTyIdx(), INTRN_UNDEFINED, arrayNode.GetBoundsCheck());
  return &meExpr;
}

MeExpr *MeBuilder::BuildNaryMeExprForIntrinsicop(BaseNode &mirNode) const {
  NaryMeExpr &meExpr =
      *NewInPool<NaryMeExpr>(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds(), TyIdx(0), static_cast<IntrinsicopNode&>(mirNode).GetIntrinsic(), false);
  return &meExpr;
}

MeExpr *MeBuilder::BuildNaryMeExprForIntrinsicWithType(BaseNode &mirNode) const {
  auto &intrinNode = static_cast<IntrinsicopNode&>(mirNode);
  NaryMeExpr &meExpr = *NewInPool<NaryMeExpr>(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds(), intrinNode.GetTyIdx(), intrinNode.GetIntrinsic(), false);
  return &meExpr;
}

UnaryMeStmt &MeBuilder::BuildUnaryMeStmt(Opcode op, MeExpr &opnd, BB &bb, const SrcPosition &src) const {
  UnaryMeStmt &unaryMeStmt = BuildUnaryMeStmt(op, opnd, bb);
  unaryMeStmt.SetSrcPos(src);
  return unaryMeStmt;
}

UnaryMeStmt &MeBuilder::BuildUnaryMeStmt(Opcode op, MeExpr &opnd, BB &bb) const {
  UnaryMeStmt &unaryMeStmt = BuildUnaryMeStmt(op, opnd);
  unaryMeStmt.SetBB(&bb);
  return unaryMeStmt;
}

UnaryMeStmt &MeBuilder::BuildUnaryMeStmt(Opcode op, MeExpr &opnd) const {
  UnaryMeStmt &unaryMeStmt = *New<UnaryMeStmt>(op);
  unaryMeStmt.SetMeStmtOpndValue(&opnd);
  return unaryMeStmt;
}

bool MeBuilder::InitMeExprBuildFactory() {
  RegisterFactoryFunction<MeExprBuildFactory>(OP_addrof, &MeBuilder::BuildAddrofMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_addroffunc, &MeBuilder::BuildAddroffuncMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcmalloc, &MeBuilder::BuildGCMallocMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcpermalloc, &MeBuilder::BuildGCMallocMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sizeoftype, &MeBuilder::BuildSizeoftypeMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_fieldsdist, &MeBuilder::BuildFieldsDistMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_iread, &MeBuilder::BuildIvarMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_constval, &MeBuilder::BuildConstMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_conststr, &MeBuilder::BuildConststrMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_conststr16, &MeBuilder::BuildConststr16MeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_eq, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ne, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lt, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gt, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_le, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ge, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cmpg, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cmpl, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cmp, &MeBuilder::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ceil, &MeBuilder::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cvt, &MeBuilder::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_floor, &MeBuilder::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_round, &MeBuilder::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_trunc, &MeBuilder::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_retype, &MeBuilder::BuildOpMeExprForRetype);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_abs, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bnot, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lnot, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_neg, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_recip, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sqrt, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_alloca, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_malloc, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_iaddrof, &MeBuilder::BuildOpMeExprForIread);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sext, &MeBuilder::BuildOpMeExprForExtractbits);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_zext, &MeBuilder::BuildOpMeExprForExtractbits);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_extractbits, &MeBuilder::BuildOpMeExprForExtractbits);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcmallocjarray, &MeBuilder::BuildOpMeExprForJarrayMalloc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcpermallocjarray, &MeBuilder::BuildOpMeExprForJarrayMalloc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_resolveinterfacefunc, &MeBuilder::BuildOpMeExprForResolveFunc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_resolvevirtualfunc, &MeBuilder::BuildOpMeExprForResolveFunc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sub, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_mul, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_div, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_rem, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ashr, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lshr, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_shl, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_max, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_min, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_band, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bior, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bxor, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_land, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lior, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_add, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_select, &MeBuilder::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_array, &MeBuilder::BuildNaryMeExprForArray);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_intrinsicop, &MeBuilder::BuildNaryMeExprForIntrinsicop);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_intrinsicopwithtype, &MeBuilder::BuildNaryMeExprForIntrinsicWithType);
  return true;
}
}  // namespace maple
