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

#include "factory.h"
#include "irmap_build.h"

// Methods to convert Maple IR to MeIR

namespace maple {
using MeExprBuildFactory = FunctionFactory<Opcode, MeExpr*, const IRMapBuild*, BaseNode&>;
using MeStmtFactory = FunctionFactory<Opcode, MeStmt*, IRMapBuild*, StmtNode&, AccessSSANodes&>;

VarMeExpr *IRMapBuild::GetOrCreateVarFromVerSt(VersionSt &vst) {
  size_t vindex = vst.GetIndex();
  ASSERT(vindex < irMap->vst2MeExprTable.size(), "GetOrCreateVarFromVerSt: index %d is out of range", vindex);
  MeExpr *meExpr = irMap->vst2MeExprTable.at(vindex);
  if (meExpr != nullptr) {
    return static_cast<VarMeExpr*>(meExpr);
  }
  OriginalSt *ost = vst.GetOst();
  ASSERT(ost->IsSymbolOst(), "GetOrCreateVarFromVerSt: wrong ost_type");
  auto *varx = irMap->NewInPool<VarMeExpr>(irMap->exprID++, ost, vindex,
     GlobalTables::GetTypeTable().GetTypeTable()[ost->GetTyIdx().GetIdx()]->GetPrimType());
  ASSERT(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
  irMap->vst2MeExprTable[vindex] = varx;
  return varx;
}

RegMeExpr *IRMapBuild::GetOrCreateRegFromVerSt(VersionSt &vst) {
  size_t vindex = vst.GetIndex();
  ASSERT(vindex < irMap->vst2MeExprTable.size(), " GetOrCreateRegFromVerSt: index %d is out of range", vindex);
  MeExpr *meExpr = irMap->vst2MeExprTable[vindex];
  if (meExpr != nullptr) {
    return static_cast<RegMeExpr*>(meExpr);
  }
  OriginalSt *ost = vst.GetOst();
  ASSERT(ost->IsPregOst(), "GetOrCreateRegFromVerSt: PregOST expected");
  auto *regx = irMap->New<RegMeExpr>(irMap->exprID++,
                                           ost, vindex, ost->GetMIRPreg()->GetPrimType());
  irMap->vst2MeExprTable[vindex] = regx;
  return regx;
}

MeExpr *IRMapBuild::BuildLHSVar(VersionSt &vst, DassignMeStmt &defMeStmt) {
  VarMeExpr *meDef = GetOrCreateVarFromVerSt(vst);
  meDef->SetDefStmt(&defMeStmt);
  meDef->SetDefBy(kDefByStmt);
  irMap->vst2MeExprTable.at(vst.GetIndex()) = meDef;
  return meDef;
}

MeExpr *IRMapBuild::BuildLHSReg(VersionSt &vst, RegassignMeStmt &defMeStmt, const RegassignNode &regassign) {
  RegMeExpr *meDef = GetOrCreateRegFromVerSt(vst);
  meDef->SetPtyp(regassign.GetPrimType());
  meDef->SetDefStmt(&defMeStmt);
  meDef->SetDefBy(kDefByStmt);
  irMap->vst2MeExprTable.at(vst.GetIndex()) = meDef;
  return meDef;
}

// build Me chilist from MayDefNode list
void IRMapBuild::BuildChiList(MeStmt &meStmt, TypeOfMayDefList &mayDefNodes,
                              MapleMap<OStIdx, ChiMeNode*> &outList) {
  for (auto &mayDefNode : mayDefNodes) {
    VersionSt *opndSt = mayDefNode.GetOpnd();
    VersionSt *resSt = mayDefNode.GetResult();
    auto *chiMeStmt = irMap->New<ChiMeNode>(&meStmt);
    chiMeStmt->SetRHS(GetOrCreateVarFromVerSt(*opndSt));
    VarMeExpr *lhs = GetOrCreateVarFromVerSt(*resSt);
    lhs->SetDefBy(kDefByChi);
    lhs->SetDefChi(*chiMeStmt);
    chiMeStmt->SetLHS(lhs);
    (void)outList.insert(std::make_pair(lhs->GetOst()->GetIndex(), chiMeStmt));
  }
}

void IRMapBuild::BuildMustDefList(MeStmt &meStmt, TypeOfMustDefList &mustDefList,
                                  MapleVector<MustDefMeNode> &mustDefMeList) {
  for (auto &mustDefNode : mustDefList) {
    VersionSt *vst = mustDefNode.GetResult();
    VarMeExpr *lhs = GetOrCreateVarFromVerSt(*vst);
    ASSERT(lhs->GetMeOp() == kMeOpReg || lhs->GetMeOp() == kMeOpVar, "unexpected opcode");
    mustDefMeList.emplace_back(MustDefMeNode(lhs, &meStmt));
  }
}

void IRMapBuild::BuildPhiMeNode(BB &bb) {
  for (auto &phi : bb.GetPhiList()) {
    const OriginalSt *oSt = ssaTab.GetOriginalStFromID(phi.first);
    VersionSt *vSt = phi.second.GetResult();

    auto *phiMeNode = irMap->NewInPool<MePhiNode>();
    phiMeNode->SetDefBB(&bb);
    (void)bb.GetMePhiList().insert(std::make_pair(oSt->GetIndex(), phiMeNode));
    if (oSt->IsPregOst()) {
      RegMeExpr *meDef = GetOrCreateRegFromVerSt(*vSt);
      phiMeNode->UpdateLHS(*meDef);
      // build phi operands
      for (VersionSt *opnd : phi.second.GetPhiOpnds()) {
        phiMeNode->GetOpnds().push_back(GetOrCreateRegFromVerSt(*opnd));
      }
    } else {
      VarMeExpr *meDef = GetOrCreateVarFromVerSt(*vSt);
      phiMeNode->UpdateLHS(*meDef);
      // build phi operands
      for (VersionSt *opnd : phi.second.GetPhiOpnds()) {
        phiMeNode->GetOpnds().push_back(GetOrCreateVarFromVerSt(*opnd));
      }
    }
  }
}

void IRMapBuild::BuildMuList(TypeOfMayUseList &mayUseList, MapleMap<OStIdx, VarMeExpr*> &muList) {
  for (auto &mayUseNode : mayUseList) {
    VersionSt *vst = mayUseNode.GetOpnd();
    VarMeExpr *varMeExpr = GetOrCreateVarFromVerSt(*vst);
    (void)muList.insert(std::make_pair(varMeExpr->GetOst()->GetIndex(), varMeExpr));
  }
}

void IRMapBuild::SetMeExprOpnds(MeExpr &meExpr, BaseNode &mirNode) {
  auto &opMeExpr = static_cast<OpMeExpr&>(meExpr);
  if (mirNode.IsUnaryNode()) {
    if (mirNode.GetOpCode() != OP_iread) {
      opMeExpr.SetOpnd(0, BuildExpr(*static_cast<UnaryNode&>(mirNode).Opnd(0)));
    }
  } else if (mirNode.IsBinaryNode()) {
    auto &binaryNode = static_cast<BinaryNode&>(mirNode);
    opMeExpr.SetOpnd(0, BuildExpr(*binaryNode.Opnd(0)));
    opMeExpr.SetOpnd(1, BuildExpr(*binaryNode.Opnd(1)));
  } else if (mirNode.IsTernaryNode()) {
    auto &ternaryNode = static_cast<TernaryNode&>(mirNode);
    opMeExpr.SetOpnd(0, BuildExpr(*ternaryNode.Opnd(0)));
    opMeExpr.SetOpnd(1, BuildExpr(*ternaryNode.Opnd(1)));
    opMeExpr.SetOpnd(2, BuildExpr(*ternaryNode.Opnd(2)));
  } else if (mirNode.IsNaryNode()) {
    auto &naryMeExpr = static_cast<NaryMeExpr&>(meExpr);
    auto &naryNode = static_cast<NaryNode&>(mirNode);
    for (size_t i = 0; i < naryNode.NumOpnds(); ++i) {
      naryMeExpr.GetOpnds().push_back(BuildExpr(*naryNode.Opnd(i)));
    }
  } else {
    // No need to do anything
  }
}

MeExpr *IRMapBuild::BuildAddrofMeExpr(BaseNode &mirNode) const {
  auto &addrofNode = static_cast<AddrofSSANode&>(mirNode);
  AddrofMeExpr *meExpr = new AddrofMeExpr(kInvalidExprID, addrofNode.GetPrimType(), addrofNode.GetSSAVar()->GetOrigSt()->GetIndex());
  meExpr->SetFieldID(addrofNode.GetFieldID());
  return meExpr;
}

MeExpr *IRMapBuild::BuildAddroffuncMeExpr(BaseNode &mirNode) const {
  AddroffuncMeExpr *meExpr = new AddroffuncMeExpr(kInvalidExprID, static_cast<AddroffuncNode&>(mirNode).GetPUIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildGCMallocMeExpr(BaseNode &mirNode) const {
  GcmallocMeExpr *meExpr = new GcmallocMeExpr(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), static_cast<GCMallocNode&>(mirNode).GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildSizeoftypeMeExpr(BaseNode &mirNode) const {
  SizeoftypeMeExpr *meExpr = new SizeoftypeMeExpr(kInvalidExprID, mirNode.GetPrimType(), static_cast<SizeoftypeNode&>(mirNode).GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildFieldsDistMeExpr(BaseNode &mirNode) const {
  auto &fieldsDistNode = static_cast<FieldsDistNode&>(mirNode);
  FieldsDistMeExpr *meExpr = new FieldsDistMeExpr(kInvalidExprID, mirNode.GetPrimType(), fieldsDistNode.GetTyIdx(),
                                                    fieldsDistNode.GetFiledID1(), fieldsDistNode.GetFiledID2());
  return meExpr;
}

MeExpr *IRMapBuild::BuildIvarMeExpr(BaseNode &mirNode) const {
  auto &ireadSSANode = static_cast<IreadSSANode&>(mirNode);
  IvarMeExpr *meExpr = new IvarMeExpr(kInvalidExprID, mirNode.GetPrimType(), ireadSSANode.GetTyIdx(), ireadSSANode.GetFieldID());
  return meExpr;
}

MeExpr *IRMapBuild::BuildConstMeExpr(BaseNode &mirNode) const {
  auto &constvalNode = static_cast<ConstvalNode &>(mirNode);
  ConstMeExpr *meExpr = new ConstMeExpr(kInvalidExprID, constvalNode.GetConstVal(), mirNode.GetPrimType());
  meExpr->SetOp(OP_constval);
  return meExpr;
}

MeExpr *IRMapBuild::BuildConststrMeExpr(BaseNode &mirNode) const {
  ConststrMeExpr *meExpr = new ConststrMeExpr(kInvalidExprID, static_cast<ConststrNode&>(mirNode).GetStrIdx(), mirNode.GetPrimType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildConststr16MeExpr(BaseNode &mirNode) const {
  Conststr16MeExpr *meExpr = new Conststr16MeExpr(kInvalidExprID, static_cast<Conststr16Node&>(mirNode).GetStrIdx(), mirNode.GetPrimType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForCompare(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetOpndType(static_cast<CompareNode&>(mirNode).GetOpndType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForTypeCvt(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetOpndType(static_cast<TypeCvtNode&>(mirNode).FromType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForRetype(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  auto &retypeNode = static_cast<RetypeNode&>(mirNode);
  meExpr->SetOpndType(retypeNode.FromType());
  meExpr->SetTyIdx(retypeNode.GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForIread(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  auto &ireadNode = static_cast<IreadNode&>(mirNode);
  meExpr->SetTyIdx(ireadNode.GetTyIdx());
  meExpr->SetFieldID(ireadNode.GetFieldID());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForExtractbits(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  auto &extractbitsNode = static_cast<ExtractbitsNode&>(mirNode);
  meExpr->SetBitsOffSet(extractbitsNode.GetBitsOffset());
  meExpr->SetBitsSize(extractbitsNode.GetBitsSize());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForJarrayMalloc(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetTyIdx(static_cast<JarrayMallocNode&>(mirNode).GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForResolveFunc(BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetFieldID(static_cast<ResolveFuncNode&>(mirNode).GetPuIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildNaryMeExprForArray(BaseNode &mirNode) const {
  auto &arrayNode = static_cast<ArrayNode&>(mirNode);
  NaryMeExpr *meExpr =
      new NaryMeExpr(&irMap->irMapAlloc, kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds(), arrayNode.GetTyIdx(), INTRN_UNDEFINED, arrayNode.GetBoundsCheck());
  return meExpr;
}

MeExpr *IRMapBuild::BuildNaryMeExprForIntrinsicop(BaseNode &mirNode) const {
  NaryMeExpr *meExpr =
      new NaryMeExpr(&irMap->irMapAlloc, kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds(), TyIdx(0), static_cast<IntrinsicopNode&>(mirNode).GetIntrinsic(), false);
  return meExpr;
}

MeExpr *IRMapBuild::BuildNaryMeExprForIntrinsicWithType(BaseNode &mirNode) const {
  auto &intrinNode = static_cast<IntrinsicopNode&>(mirNode);
  NaryMeExpr *meExpr = new NaryMeExpr(&irMap->irMapAlloc, kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds(), intrinNode.GetTyIdx(), intrinNode.GetIntrinsic(), false);
  return meExpr;
}

MeExpr *IRMapBuild::BuildExpr(BaseNode &mirNode) {
  Opcode op = mirNode.GetOpCode();
  if (op == OP_dread) {
    auto &addrOfNode = static_cast<AddrofSSANode&>(mirNode);
    VersionSt *vst = addrOfNode.GetSSAVar();
    VarMeExpr *varMeExpr = GetOrCreateVarFromVerSt(*vst);
    ASSERT(!vst->GetOrigSt()->IsPregOst(), "not expect preg symbol here");
    varMeExpr->SetPtyp(GlobalTables::GetTypeTable().GetTypeFromTyIdx(vst->GetOrigSt()->GetTyIdx())->GetPrimType());
    return varMeExpr;
  }

  if (op == OP_regread) {
    auto &regNode = static_cast<RegreadSSANode&>(mirNode);
    VersionSt *vst = regNode.GetSSAVar();
    RegMeExpr *regMeExpr = GetOrCreateRegFromVerSt(*vst);
    regMeExpr->SetPtyp(mirNode.GetPrimType());
    return regMeExpr;
  }

  auto func = CreateProductFunction<MeExprBuildFactory>(mirNode.GetOpCode());
  ASSERT(func != nullptr, "NIY BuildExpe");
  MeExpr *meExpr = func(this, mirNode);
  SetMeExprOpnds(*meExpr, mirNode);

  if (op == OP_iread) {
    auto *ivarMeExpr = static_cast<IvarMeExpr*>(meExpr);
    auto &iReadSSANode = static_cast<IreadSSANode&>(mirNode);
    ivarMeExpr->SetBase(BuildExpr(*iReadSSANode.Opnd(0)));
    VersionSt *verSt = iReadSSANode.GetSSAVar();
    if (verSt != nullptr) {
      VarMeExpr *varMeExpr = GetOrCreateVarFromVerSt(*verSt);
      ivarMeExpr->SetMuVal(varMeExpr);
    }
  }

  MeExpr *retMeExpr = irMap->HashMeExpr(*meExpr);
  delete meExpr;

  if (op == OP_iread) {
    ASSERT(static_cast<IvarMeExpr*>(retMeExpr)->GetMu() != nullptr, "BuildExpr: ivar node cannot have mu == nullptr");
  }

  return retMeExpr;
}

void IRMapBuild::InitMeExprBuildFactory() {
  RegisterFactoryFunction<MeExprBuildFactory>(OP_addrof, &IRMapBuild::BuildAddrofMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_addroffunc, &IRMapBuild::BuildAddroffuncMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcmalloc, &IRMapBuild::BuildGCMallocMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcpermalloc, &IRMapBuild::BuildGCMallocMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sizeoftype, &IRMapBuild::BuildSizeoftypeMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_fieldsdist, &IRMapBuild::BuildFieldsDistMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_iread, &IRMapBuild::BuildIvarMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_constval, &IRMapBuild::BuildConstMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_conststr, &IRMapBuild::BuildConststrMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_conststr16, &IRMapBuild::BuildConststr16MeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_eq, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ne, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lt, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gt, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_le, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ge, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cmpg, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cmpl, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cmp, &IRMapBuild::BuildOpMeExprForCompare);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ceil, &IRMapBuild::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cvt, &IRMapBuild::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_floor, &IRMapBuild::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_round, &IRMapBuild::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_trunc, &IRMapBuild::BuildOpMeExprForTypeCvt);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_retype, &IRMapBuild::BuildOpMeExprForRetype);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_abs, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bnot, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lnot, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_neg, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_recip, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sqrt, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_alloca, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_malloc, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_iaddrof, &IRMapBuild::BuildOpMeExprForIread);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sext, &IRMapBuild::BuildOpMeExprForExtractbits);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_zext, &IRMapBuild::BuildOpMeExprForExtractbits);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_extractbits, &IRMapBuild::BuildOpMeExprForExtractbits);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcmallocjarray, &IRMapBuild::BuildOpMeExprForJarrayMalloc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_gcpermallocjarray, &IRMapBuild::BuildOpMeExprForJarrayMalloc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_resolveinterfacefunc, &IRMapBuild::BuildOpMeExprForResolveFunc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_resolvevirtualfunc, &IRMapBuild::BuildOpMeExprForResolveFunc);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_sub, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_mul, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_div, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_rem, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ashr, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lshr, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_shl, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_max, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_min, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_band, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bior, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bxor, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_land, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lior, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_add, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_select, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_array, &IRMapBuild::BuildNaryMeExprForArray);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_intrinsicop, &IRMapBuild::BuildNaryMeExprForIntrinsicop);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_intrinsicopwithtype, &IRMapBuild::BuildNaryMeExprForIntrinsicWithType);
}

MeStmt *IRMapBuild::BuildMeStmtWithNoSSAPart(StmtNode &stmt) {
  Opcode op = stmt.GetOpCode();
  switch (op) {
    case OP_jscatch:
    case OP_finally:
    case OP_endtry:
    case OP_cleanuptry:
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstorestore:
    case OP_membarstoreload:
      return irMap->New<MeStmt>(&stmt);
    case OP_goto:
      return irMap->New<GotoMeStmt>(&stmt);
    case OP_comment:
      return irMap->NewInPool<CommentMeStmt>(&stmt);
    case OP_jstry:
      return irMap->New<JsTryMeStmt>(&stmt);
    case OP_catch:
      return irMap->NewInPool<CatchMeStmt>(&stmt);
    case OP_brfalse:
    case OP_brtrue: {
      auto &condGotoNode = static_cast<CondGotoNode&>(stmt);
      auto *condGotoMeStmt = irMap->New<CondGotoMeStmt>(&stmt);
      condGotoMeStmt->SetOpnd(0, BuildExpr(*condGotoNode.Opnd(0)));
      return condGotoMeStmt;
    }
    case OP_try: {
      auto &tryNode = static_cast<TryNode&>(stmt);
      auto *tryMeStmt = irMap->NewInPool<TryMeStmt>(&stmt);
      for (size_t i = 0; i < tryNode.GetOffsetsCount(); ++i) {
        tryMeStmt->OffsetsPushBack(tryNode.GetOffset(i));
      }
      return tryMeStmt;
    }
    case OP_assertnonnull:
    case OP_eval:
    case OP_free:
    case OP_switch: {
      auto &unaryStmt = static_cast<UnaryStmtNode&>(stmt);
      auto *unMeStmt = static_cast<UnaryMeStmt*>((op == OP_switch) ? irMap->NewInPool<SwitchMeStmt>(&stmt)
                                                                   : irMap->New<UnaryMeStmt>(&stmt));
      unMeStmt->SetOpnd(0, BuildExpr(*unaryStmt.Opnd(0)));
      return unMeStmt;
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
}

MeStmt *IRMapBuild::BuildDassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *meStmt = irMap->NewInPool<DassignMeStmt>(&stmt);
  auto &dassiNode = static_cast<DassignNode&>(stmt);
  meStmt->SetRHS(BuildExpr(*dassiNode.GetRHS()));
  auto *varLHS = static_cast<VarMeExpr*>(BuildLHSVar(*ssaPart.GetSSAVar(), *meStmt));
  meStmt->SetLHS(varLHS);
  BuildChiList(*meStmt, ssaPart.GetMayDefNodes(), *meStmt->GetChiList());
  return meStmt;
}

MeStmt *IRMapBuild::BuildRegassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *meStmt = irMap->New<RegassignMeStmt>(&stmt);
  auto &regNode = static_cast<RegassignNode&>(stmt);
  meStmt->SetRHS(BuildExpr(*regNode.Opnd(0)));
  auto *regLHS = static_cast<RegMeExpr*>(BuildLHSReg(*ssaPart.GetSSAVar(), *meStmt, regNode));
  meStmt->SetLHS(regLHS);
  return meStmt;
}

MeStmt *IRMapBuild::BuildIassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto &iasNode = static_cast<IassignNode&>(stmt);
  auto *meStmt = irMap->NewInPool<IassignMeStmt>(&stmt);
  meStmt->SetTyIdx(iasNode.GetTyIdx());
  meStmt->SetRHS(BuildExpr(*iasNode.GetRHS()));
  meStmt->SetLHSVal(irMap->BuildLHSIvar(*BuildExpr(*iasNode.Opnd(0)), *meStmt, iasNode.GetFieldID()));
  BuildChiList(*meStmt, ssaPart.GetMayDefNodes(), *(meStmt->GetChiList()));
  return meStmt;
}

MeStmt *IRMapBuild::BuildMaydassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *meStmt = irMap->NewInPool<MaydassignMeStmt>(&stmt);
  auto &dassiNode = static_cast<DassignNode&>(stmt);
  meStmt->SetRHS(BuildExpr(*dassiNode.GetRHS()));
  meStmt->SetMayDassignSym(ssaPart.GetSSAVar()->GetOst());
  meStmt->SetFieldID(dassiNode.GetFieldID());
  BuildChiList(*meStmt, ssaPart.GetMayDefNodes(), *(meStmt->GetChiList()));
  return meStmt;
}

MeStmt *IRMapBuild::BuildCallMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *callMeStmt = irMap->NewInPool<CallMeStmt>(&stmt);
  auto &intrinNode = static_cast<CallNode&>(stmt);
  callMeStmt->SetPUIdx(intrinNode.GetPUIdx());
  for (size_t i = 0; i < intrinNode.NumOpnds(); ++i) {
    callMeStmt->PushBackOpnd(BuildExpr(*intrinNode.Opnd(i)));
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(callMeStmt->GetMuList()));
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    BuildMustDefList(*callMeStmt, ssaPart.GetMustDefNodes(), *(callMeStmt->GetMustDefList()));
  }
  BuildChiList(*callMeStmt, ssaPart.GetMayDefNodes(), *(callMeStmt->GetChiList()));
  return callMeStmt;
}


MeStmt *IRMapBuild::BuildNaryMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  Opcode op = stmt.GetOpCode();
  NaryMeStmt *naryMeStmt = (op == OP_icall || op == OP_icallassigned)
                           ? static_cast<NaryMeStmt*>(irMap->NewInPool<IcallMeStmt>(&stmt))
                           : static_cast<NaryMeStmt*>(irMap->NewInPool<IntrinsiccallMeStmt>(&stmt));
  auto &naryStmtNode = static_cast<NaryStmtNode&>(stmt);
  for (size_t i = 0; i < naryStmtNode.NumOpnds(); ++i) {
    naryMeStmt->PushBackOpnd(BuildExpr(*naryStmtNode.Opnd(i)));
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(naryMeStmt->GetMuList()));
  if (kOpcodeInfo.IsCallAssigned(op)) {
    BuildMustDefList(*naryMeStmt, ssaPart.GetMustDefNodes(), *(naryMeStmt->GetMustDefList()));
  }
  BuildChiList(*naryMeStmt, ssaPart.GetMayDefNodes(), *(naryMeStmt->GetChiList()));
  return naryMeStmt;
}

MeStmt *IRMapBuild::BuildRetMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto &retStmt = static_cast<NaryStmtNode&>(stmt);
  auto *meStmt = irMap->NewInPool<RetMeStmt>(&stmt);
  for (size_t i = 0; i < retStmt.NumOpnds(); ++i) {
    meStmt->PushBackOpnd(BuildExpr(*retStmt.Opnd(i)));
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(meStmt->GetMuList()));
  return meStmt;
}

MeStmt *IRMapBuild::BuildWithMuMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *retSub = irMap->NewInPool<WithMuMeStmt>(&stmt);
  BuildMuList(ssaPart.GetMayUseNodes(), *(retSub->GetMuList()));
  return retSub;
}

MeStmt *IRMapBuild::BuildGosubMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *goSub = irMap->NewInPool<GosubMeStmt>(&stmt);
  BuildMuList(ssaPart.GetMayUseNodes(), *(goSub->GetMuList()));
  return goSub;
}

MeStmt *IRMapBuild::BuildThrowMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto &unaryNode = static_cast<UnaryStmtNode&>(stmt);
  auto *tmeStmt = irMap->NewInPool<ThrowMeStmt>(&stmt);
  tmeStmt->SetMeStmtOpndValue(BuildExpr(*unaryNode.Opnd(0)));
  BuildMuList(ssaPart.GetMayUseNodes(), *(tmeStmt->GetMuList()));
  return tmeStmt;
}

MeStmt *IRMapBuild::BuildSyncMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto &naryNode = static_cast<NaryStmtNode&>(stmt);
  auto *naryStmt = irMap->NewInPool<SyncMeStmt>(&stmt);
  for (size_t i = 0; i < naryNode.NumOpnds(); ++i) {
    naryStmt->PushBackOpnd(BuildExpr(*naryNode.Opnd(i)));
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(naryStmt->GetMuList()));
  BuildChiList(*naryStmt, ssaPart.GetMayDefNodes(), *(naryStmt->GetChiList()));
  return naryStmt;
}

MeStmt *IRMapBuild::BuildMeStmt(StmtNode &stmt) {
  AccessSSANodes *ssaPart = ssaTab.GetStmtsSSAPart().SSAPartOf(stmt);
  if (ssaPart == nullptr) {
    return BuildMeStmtWithNoSSAPart(stmt);
  }

  auto func = CreateProductFunction<MeStmtFactory>(stmt.GetOpCode());
  CHECK_FATAL(func != nullptr, "func nullptr check");
  return func(this, stmt, *ssaPart);
}

void IRMapBuild::InitMeStmtFactory() {
  RegisterFactoryFunction<MeStmtFactory>(OP_dassign, &IRMapBuild::BuildDassignMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_regassign, &IRMapBuild::BuildRegassignMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_iassign, &IRMapBuild::BuildIassignMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_maydassign, &IRMapBuild::BuildMaydassignMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_call, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_virtualcall, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_virtualicall, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_superclasscall, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_interfacecall, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_interfaceicall, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_customcall, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_polymorphiccall, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_callassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_virtualcallassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_virtualicallassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_superclasscallassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_interfacecallassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_interfaceicallassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_customcallassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_polymorphiccallassigned, &IRMapBuild::BuildCallMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_icall, &IRMapBuild::BuildNaryMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_icallassigned, &IRMapBuild::BuildNaryMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_intrinsiccall, &IRMapBuild::BuildNaryMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_xintrinsiccall, &IRMapBuild::BuildNaryMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_intrinsiccallwithtype, &IRMapBuild::BuildNaryMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_intrinsiccallassigned, &IRMapBuild::BuildNaryMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_intrinsiccallwithtypeassigned, &IRMapBuild::BuildNaryMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_return, &IRMapBuild::BuildRetMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_retsub, &IRMapBuild::BuildWithMuMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_gosub, &IRMapBuild::BuildGosubMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_throw, &IRMapBuild::BuildThrowMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_syncenter, &IRMapBuild::BuildSyncMeStmt);
  RegisterFactoryFunction<MeStmtFactory>(OP_syncexit, &IRMapBuild::BuildSyncMeStmt);
}

// recursively invoke itself in a pre-order traversal of the dominator tree of
// the CFG to build the HSSA representation for the code in each BB
void IRMapBuild::BuildBB(BB &bb, std::vector<bool> &bbIRMapProcessed) {
  BBId bbID = bb.GetBBId();
  if (bbIRMapProcessed[bbID]) {
    return;
  }
  bbIRMapProcessed[bbID] = true;
  curBB = &bb;
  irMap->SetCurFunction(bb);
  // iterate phi list to update the definition by phi
  BuildPhiMeNode(bb);
  if (!bb.IsEmpty()) {
    for (auto &stmt : bb.GetStmtNodes()) {
      MeStmt *meStmt = BuildMeStmt(stmt);
      bb.AddMeStmtLast(meStmt);
    }
  }
  // travesal bb's dominated tree
  ASSERT(bbID < dominance.GetDomChildrenSize(), " index out of range in IRMapBuild::BuildBB");
  const MapleSet<BBId> &domChildren = dominance.GetDomChildren(bbID);
  for (auto bbIt = domChildren.begin(); bbIt != domChildren.end(); ++bbIt) {
    BBId childBBId = *bbIt;
    BuildBB(*irMap->GetBB(childBBId), bbIRMapProcessed);
  }
}
}  // namespace maple
