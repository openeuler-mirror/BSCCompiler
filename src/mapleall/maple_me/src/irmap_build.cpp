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
#include "prop.h"

// Methods to convert Maple IR to MeIR

namespace maple {
#ifdef USE_ARM32_MACRO
static constexpr uint32 kMaxRegParamNum = 4;
#else
static constexpr uint32 kMaxRegParamNum = 8;
#endif

using MeExprBuildFactory = FunctionFactory<Opcode, MeExpr*, const IRMapBuild*, BaseNode&>;
using MeStmtFactory = FunctionFactory<Opcode, MeStmt*, IRMapBuild*, StmtNode&, AccessSSANodes&>;

VarMeExpr *IRMapBuild::GetOrCreateVarFromVerSt(const VersionSt &vst) {
  size_t vindex = vst.GetIndex();
  ASSERT(vindex < irMap->verst2MeExprTable.size(), "GetOrCreateVarFromVerSt: index %d is out of range", vindex);
  MeExpr *meExpr = irMap->verst2MeExprTable.at(vindex);
  if (meExpr != nullptr) {
    return static_cast<VarMeExpr*>(meExpr);
  }
  OriginalSt *ost = vst.GetOst();
  ASSERT(ost->IsSymbolOst() || ost->GetIndirectLev() > 0, "GetOrCreateVarFromVerSt: wrong ost_type");
  auto *varx = irMap->New<VarMeExpr>(irMap->exprID++, ost, vindex,
      GlobalTables::GetTypeTable().GetTypeTable()[ost->GetTyIdx().GetIdx()]->GetPrimType());
  ASSERT(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
  irMap->verst2MeExprTable[vindex] = varx;
  return varx;
}

RegMeExpr *IRMapBuild::GetOrCreateRegFromVerSt(const VersionSt &vst) {
  size_t vindex = vst.GetIndex();
  ASSERT(vindex < irMap->verst2MeExprTable.size(), " GetOrCreateRegFromVerSt: index %d is out of range", vindex);
  MeExpr *meExpr = irMap->verst2MeExprTable[vindex];
  if (meExpr != nullptr) {
    return static_cast<RegMeExpr*>(meExpr);
  }
  OriginalSt *ost = vst.GetOst();
  ASSERT(ost->IsPregOst(), "GetOrCreateRegFromVerSt: PregOST expected");
  auto *regx = irMap->New<RegMeExpr>(irMap->exprID++,
                                     ost, vindex, kMeOpReg, OP_regread, ost->GetMIRPreg()->GetPrimType());
  irMap->verst2MeExprTable[vindex] = regx;
  return regx;
}

MeExpr *IRMapBuild::BuildLHSVar(const VersionSt &vst, DassignMeStmt &defMeStmt) {
  VarMeExpr *meDef = GetOrCreateVarFromVerSt(vst);
  meDef->SetDefStmt(&defMeStmt);
  meDef->SetDefBy(kDefByStmt);
  irMap->verst2MeExprTable.at(vst.GetIndex()) = meDef;
  return meDef;
}

MeExpr *IRMapBuild::BuildLHSReg(const VersionSt &vst, AssignMeStmt &defMeStmt, const RegassignNode &regassign) {
  RegMeExpr *meDef = GetOrCreateRegFromVerSt(vst);
  meDef->SetPtyp(regassign.GetPrimType());
  meDef->SetDefStmt(&defMeStmt);
  meDef->SetDefBy(kDefByStmt);
  irMap->verst2MeExprTable.at(vst.GetIndex()) = meDef;
  return meDef;
}

// build Me chilist from MayDefNode list
void IRMapBuild::BuildChiList(MeStmt &meStmt, TypeOfMayDefList &mayDefNodes,
                              MapleMap<OStIdx, ChiMeNode*> &outList) {
  for (auto &mayDefNodeItem : mayDefNodes) {
    VersionSt *opndSt = mayDefNodeItem.second.GetOpnd();
    VersionSt *resSt = mayDefNodeItem.second.GetResult();

    auto *chiMeStmt = irMap->New<ChiMeNode>(&meStmt);
    if (opndSt->GetOst()->IsSymbolOst()) {
      chiMeStmt->SetRHS(GetOrCreateVarFromVerSt(*opndSt));
    } else {
      chiMeStmt->SetRHS(GetOrCreateRegFromVerSt(*opndSt));
    }

    ScalarMeExpr *lhs = nullptr;
    if (resSt->GetOst()->IsSymbolOst()) {
      lhs = GetOrCreateVarFromVerSt(*resSt);
    } else {
      lhs = GetOrCreateRegFromVerSt(*resSt);
    }
    lhs->SetDefBy(kDefByChi);
    lhs->SetDefChi(*chiMeStmt);
    chiMeStmt->SetLHS(lhs);
    (void)outList.insert(std::make_pair(lhs->GetOstIdx(), chiMeStmt));
  }
}

void IRMapBuild::BuildMustDefList(MeStmt &meStmt, TypeOfMustDefList &mustDefList,
                                  MapleVector<MustDefMeNode> &mustDefMeList) {
  for (auto &mustDefNode : mustDefList) {
    VersionSt *vst = mustDefNode.GetResult();
    ScalarMeExpr *lhs = nullptr;
    if (vst->GetOst()->IsSymbolOst()) {
      lhs = GetOrCreateVarFromVerSt(*vst);
    } else {
      lhs = GetOrCreateRegFromVerSt(*vst);
    }
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

void IRMapBuild::BuildMuList(TypeOfMayUseList &mayUseList, MapleMap<OStIdx, ScalarMeExpr*> &muList) {
  for (auto &mayUseNodeItem : mayUseList) {
    VersionSt *vst = mayUseNodeItem.second.GetOpnd();
    ScalarMeExpr *muExpr = nullptr;
    if (vst->GetOst()->IsPregOst()) {
      muExpr = GetOrCreateRegFromVerSt(*vst);
    } else {
      muExpr = GetOrCreateVarFromVerSt(*vst);
    }
    (void)muList.insert(std::make_pair(muExpr->GetOstIdx(), muExpr));
  }
}

void IRMapBuild::SetMeExprOpnds(MeExpr &meExpr, BaseNode &mirNode, bool atParm, bool noProp) {
  auto &opMeExpr = static_cast<OpMeExpr&>(meExpr);
  if (mirNode.IsUnaryNode()) {
    if (mirNode.GetOpCode() != OP_iread) {
      opMeExpr.SetOpnd(0, BuildExpr(*static_cast<UnaryNode&>(mirNode).Opnd(0), atParm, noProp));
    }
  } else if (mirNode.IsBinaryNode()) {
    auto &binaryNode = static_cast<BinaryNode&>(mirNode);
    opMeExpr.SetOpnd(0, BuildExpr(*binaryNode.Opnd(0), atParm, noProp));
    opMeExpr.SetOpnd(1, BuildExpr(*binaryNode.Opnd(1), atParm, noProp));
    opMeExpr.SetHasAddressValue();
  } else if (mirNode.IsTernaryNode()) {
    auto &ternaryNode = static_cast<TernaryNode&>(mirNode);
    opMeExpr.SetOpnd(0, BuildExpr(*ternaryNode.Opnd(0), atParm, noProp));
    opMeExpr.SetOpnd(1, BuildExpr(*ternaryNode.Opnd(1), atParm, noProp));
    opMeExpr.SetOpnd(2, BuildExpr(*ternaryNode.Opnd(2), atParm, noProp));
    opMeExpr.SetHasAddressValue();
  } else if (mirNode.IsNaryNode()) {
    auto &naryMeExpr = static_cast<NaryMeExpr&>(meExpr);
    auto &naryNode = static_cast<NaryNode&>(mirNode);
    for (size_t i = 0; i < naryNode.NumOpnds(); ++i) {
      naryMeExpr.GetOpnds().push_back(BuildExpr(*naryNode.Opnd(i), atParm, noProp));
    }
  } else {
    // No need to do anything
  }
}

MeExpr *IRMapBuild::BuildAddrofMeExpr(const BaseNode &mirNode) const {
  auto &addrofNode = static_cast<const AddrofSSANode&>(mirNode);
  auto meExpr = new AddrofMeExpr(kInvalidExprID, addrofNode.GetPrimType(),
                                 addrofNode.GetSSAVar()->GetOst()->GetIndex());
  meExpr->SetFieldID(addrofNode.GetFieldID());
  return meExpr;
}

MeExpr *IRMapBuild::BuildAddroffuncMeExpr(const BaseNode &mirNode) const {
  auto meExpr = new AddroffuncMeExpr(kInvalidExprID, static_cast<const AddroffuncNode&>(mirNode).GetPUIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildAddroflabelMeExpr(const BaseNode &mirNode) const {
  auto meExpr = new AddroflabelMeExpr(kInvalidExprID, static_cast<const AddroflabelNode&>(mirNode).GetOffset());
  return meExpr;
}

MeExpr *IRMapBuild::BuildGCMallocMeExpr(const BaseNode &mirNode) const {
  auto meExpr = new GcmallocMeExpr(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(),
                                   static_cast<const GCMallocNode&>(mirNode).GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildSizeoftypeMeExpr(const BaseNode &mirNode) const {
  auto meExpr = new SizeoftypeMeExpr(kInvalidExprID, mirNode.GetPrimType(),
                                     static_cast<const SizeoftypeNode&>(mirNode).GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildFieldsDistMeExpr(const BaseNode &mirNode) const {
  auto &fieldsDistNode = static_cast<const FieldsDistNode&>(mirNode);
  auto meExpr = new FieldsDistMeExpr(kInvalidExprID, mirNode.GetPrimType(), fieldsDistNode.GetTyIdx(),
                                     fieldsDistNode.GetFiledID1(), fieldsDistNode.GetFiledID2());
  return meExpr;
}

MeExpr *IRMapBuild::BuildIvarMeExpr(const BaseNode &mirNode) const {
  auto &ireadSSANode = static_cast<const IreadSSANode&>(mirNode);
  auto meExpr = new IvarMeExpr(kInvalidExprID, mirNode.GetPrimType(), ireadSSANode.GetTyIdx(),
                               ireadSSANode.GetFieldID());
  return meExpr;
}

MeExpr *IRMapBuild::BuildConstMeExpr(BaseNode &mirNode) const {
  auto &constvalNode = static_cast<ConstvalNode&>(mirNode);
  auto meExpr = new ConstMeExpr(kInvalidExprID, constvalNode.GetConstVal(), mirNode.GetPrimType());
  meExpr->SetOp(OP_constval);
  return meExpr;
}

MeExpr *IRMapBuild::BuildConststrMeExpr(const BaseNode &mirNode) const {
  auto meExpr = new ConststrMeExpr(kInvalidExprID, static_cast<const ConststrNode&>(mirNode).GetStrIdx(),
                                   mirNode.GetPrimType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildConststr16MeExpr(const BaseNode &mirNode) const {
  auto meExpr = new Conststr16MeExpr(kInvalidExprID, static_cast<const Conststr16Node&>(mirNode).GetStrIdx(),
                                     mirNode.GetPrimType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForCompare(const BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetOpndType(static_cast<const CompareNode&>(mirNode).GetOpndType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForTypeCvt(const BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetOpndType(static_cast<const TypeCvtNode&>(mirNode).FromType());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForRetype(const BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  auto &retypeNode = static_cast<const RetypeNode&>(mirNode);
  meExpr->SetOpndType(retypeNode.FromType());
  meExpr->SetTyIdx(retypeNode.GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForIread(const BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  auto &ireadNode = static_cast<const IreadNode&>(mirNode);
  meExpr->SetTyIdx(ireadNode.GetTyIdx());
  meExpr->SetFieldID(ireadNode.GetFieldID());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForExtractbits(const BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  auto &extractbitsNode = static_cast<const ExtractbitsNode&>(mirNode);
  meExpr->SetBitsOffSet(extractbitsNode.GetBitsOffset());
  meExpr->SetBitsSize(extractbitsNode.GetBitsSize());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForJarrayMalloc(const BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetTyIdx(static_cast<const JarrayMallocNode&>(mirNode).GetTyIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildOpMeExprForResolveFunc(const BaseNode &mirNode) const {
  OpMeExpr *meExpr = BuildOpMeExpr(mirNode);
  meExpr->SetFieldID(static_cast<const ResolveFuncNode&>(mirNode).GetPuIdx());
  return meExpr;
}

MeExpr *IRMapBuild::BuildNaryMeExprForArray(const BaseNode &mirNode) const {
  auto &arrayNode = static_cast<const ArrayNode&>(mirNode);
  auto meExpr = new NaryMeExpr(&irMap->irMapAlloc, kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(),
                               mirNode.GetNumOpnds(), arrayNode.GetTyIdx(), INTRN_UNDEFINED,
                               arrayNode.GetBoundsCheck());
  return meExpr;
}

MeExpr *IRMapBuild::BuildNaryMeExprForIntrinsicop(const BaseNode &mirNode) const {
  auto meExpr = new NaryMeExpr(&irMap->irMapAlloc, kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(),
                               mirNode.GetNumOpnds(), TyIdx(0),
                               static_cast<const IntrinsicopNode&>(mirNode).GetIntrinsic(), false);
  return meExpr;
}

MeExpr *IRMapBuild::BuildNaryMeExprForIntrinsicWithType(const BaseNode &mirNode) const {
  auto &intrinNode = static_cast<const IntrinsicopNode&>(mirNode);
  NaryMeExpr *meExpr = new NaryMeExpr(&irMap->irMapAlloc, kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(),
                                      mirNode.GetNumOpnds(), intrinNode.GetTyIdx(), intrinNode.GetIntrinsic(), false);
  return meExpr;
}

MeExpr *IRMapBuild::BuildExpr(BaseNode &mirNode, bool atParm, bool noProp) {
  Opcode op = mirNode.GetOpCode();
  if (op == OP_dread) {
    auto &addrOfNode = static_cast<AddrofSSANode&>(mirNode);
    VersionSt *vst = addrOfNode.GetSSAVar();
    VarMeExpr *varMeExpr = GetOrCreateVarFromVerSt(*vst);
    ASSERT(!vst->GetOst()->IsPregOst(), "not expect preg symbol here");
    varMeExpr->SetPtyp(GlobalTables::GetTypeTable().GetTypeFromTyIdx(vst->GetOst()->GetTyIdx())->GetPrimType());
    varMeExpr->GetOst()->SetFieldID(addrOfNode.GetFieldID());
    MeExpr *retmeexpr;
    if (propagater && !noProp) {
      MeExpr *propedMeExpr = &propagater->PropVar(*varMeExpr, atParm, true);
      MeExpr *simplifiedMeexpr = nullptr;
      if (propedMeExpr->GetMeOp() == kMeOpOp) {
        simplifiedMeexpr = irMap->SimplifyOpMeExpr(static_cast<OpMeExpr *>(propedMeExpr));
      }
      retmeexpr = simplifiedMeexpr ? simplifiedMeexpr : propedMeExpr;
    } else {
      retmeexpr = varMeExpr;
    }
    uint32 typesize = GetPrimTypeSize(retmeexpr->GetPrimType());
    if (typesize < GetPrimTypeSize(addrOfNode.GetPrimType()) && typesize != 0) {
      // need to insert a convert
      if (typesize < 4) {
        OpMeExpr opmeexpr(kInvalidExprID, IsSignedInteger(retmeexpr->GetPrimType()) ? OP_sext : OP_zext,
                          addrOfNode.GetPrimType(), 1);
        opmeexpr.SetBitsSize(static_cast<uint8>(typesize * 8));
        opmeexpr.SetOpnd(0, retmeexpr);
        retmeexpr = irMap->HashMeExpr(opmeexpr);
      } else {
        bool isSigned = IsSignedInteger(addrOfNode.GetPrimType());
        retmeexpr = irMap->CreateMeExprTypeCvt(addrOfNode.GetPrimType(), isSigned ? PTY_i32 : PTY_u32, *retmeexpr);
      }
      // Try to simplify new added cast expr
      MeExpr *simplified = irMap->SimplifyMeExpr(retmeexpr);
      retmeexpr = (simplified != nullptr ? simplified : retmeexpr);
    }
    return retmeexpr;
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
  SetMeExprOpnds(*meExpr, mirNode, atParm, noProp);

  if (op == OP_iread) {
    IvarMeExpr *ivarMeExpr = static_cast<IvarMeExpr*>(meExpr);
    IreadSSANode &iReadSSANode = static_cast<IreadSSANode&>(mirNode);
    ivarMeExpr->SetBase(BuildExpr(*iReadSSANode.Opnd(0), atParm, false));
    VersionSt *verSt = iReadSSANode.GetSSAVar();
    if (verSt != nullptr) {
      VarMeExpr *varMeExpr = GetOrCreateVarFromVerSt(*verSt);
      ivarMeExpr->SetMuVal(varMeExpr);
      if (verSt->GetOst()->IsVolatile()) {
        ivarMeExpr->SetVolatileFromBaseSymbol(true);
      }
    }

    IvarMeExpr *canIvar = static_cast<IvarMeExpr *>(irMap->HashMeExpr(*ivarMeExpr));
    delete ivarMeExpr;
    ASSERT(static_cast<IvarMeExpr*>(canIvar)->GetMu() != nullptr, "BuildExpr: ivar node cannot have mu == nullptr");
    MeExpr *retmeexpr;
    if (propagater && !noProp) {
      MeExpr *propedMeExpr = &propagater->PropIvar(*canIvar);
      MeExpr *simplifiedMeexpr = nullptr;
      if (propedMeExpr->GetMeOp() == kMeOpOp) {
        simplifiedMeexpr = irMap->SimplifyOpMeExpr(static_cast<OpMeExpr *>(propedMeExpr));
      } else if (propedMeExpr->GetMeOp() == kMeOpIvar) {
        simplifiedMeexpr = irMap->SimplifyIvar(static_cast<IvarMeExpr *>(propedMeExpr), false);
      }
      retmeexpr = simplifiedMeexpr ? simplifiedMeexpr : propedMeExpr;
    } else {
      retmeexpr = canIvar;
    }

    uint32 typesize = GetPrimTypeSize(retmeexpr->GetPrimType());
    if (typesize < GetPrimTypeSize(iReadSSANode.GetPrimType()) && typesize != 0) {
      // need to insert a convert
      if (typesize < 4) {
        OpMeExpr opmeexpr(kInvalidExprID, IsSignedInteger(retmeexpr->GetPrimType()) ? OP_sext : OP_zext,
                          iReadSSANode.GetPrimType(), 1);
        opmeexpr.SetBitsSize(static_cast<uint8>(typesize * 8));
        opmeexpr.SetOpnd(0, retmeexpr);
        retmeexpr = irMap->HashMeExpr(opmeexpr);
      } else {
        retmeexpr = irMap->CreateMeExprTypeCvt(iReadSSANode.GetPrimType(), retmeexpr->GetPrimType(), *retmeexpr);
      }
    }
    return retmeexpr;
  }

  auto *simplifiedMeExpr = irMap->SimplifyOpMeExpr(static_cast<OpMeExpr*>(meExpr));
  if (simplifiedMeExpr != nullptr) {
    return simplifiedMeExpr;
  }

  if (op == OP_mul) {
    OpMeExpr *opMeExpr = static_cast<OpMeExpr *>(meExpr);
    if (opMeExpr->GetOpnd(0)->GetMeOp() == kMeOpConst) {
      // canonicalize constant operand to be operand 1
      MeExpr *savedOpnd = opMeExpr->GetOpnd(0);
      opMeExpr->SetOpnd(0, opMeExpr->GetOpnd(1));
      opMeExpr->SetOpnd(1, savedOpnd);
    }
  }

  MeExpr *retMeExpr = irMap->HashMeExpr(*meExpr);
  delete meExpr;

  return retMeExpr;
}

void IRMapBuild::InitMeExprBuildFactory() {
  RegisterFactoryFunction<MeExprBuildFactory>(OP_addrof, &IRMapBuild::BuildAddrofMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_addroffunc, &IRMapBuild::BuildAddroffuncMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_addroflabel, &IRMapBuild::BuildAddroflabelMeExpr);
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
  RegisterFactoryFunction<MeExprBuildFactory>(OP_ror, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_max, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_min, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_band, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bior, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_bxor, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_land, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_lior, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_add, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_select, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cand, &IRMapBuild::BuildOpMeExpr);
  RegisterFactoryFunction<MeExprBuildFactory>(OP_cior, &IRMapBuild::BuildOpMeExpr);
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
      condGotoMeStmt->SetOpnd(0, BuildExpr(*condGotoNode.Opnd(0), false, false));
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
    case OP_assignassertnonnull:
    case OP_returnassertnonnull:
    case OP_eval:
    case OP_igoto:
    case OP_free:
    case OP_switch: {
      auto &unaryStmt = static_cast<UnaryStmtNode&>(stmt);
      auto *unMeStmt = static_cast<UnaryMeStmt*>((op == OP_switch) ? irMap->NewInPool<SwitchMeStmt>(&stmt)
                                                                   : irMap->New<UnaryMeStmt>(&stmt));
      unMeStmt->SetOpnd(0, BuildExpr(*unaryStmt.Opnd(0), false, false));
      return unMeStmt;
    }
    case OP_assertge:
    case OP_assertlt:
    case OP_returnassertle:
    case OP_assignassertle: {
      auto &naryStmt = static_cast<NaryStmtNode&>(stmt);
      auto *naryMeStmt = static_cast<NaryMeStmt*>(irMap->NewInPool<NaryMeStmt>(&stmt));
      naryMeStmt->PushBackOpnd(BuildExpr(*naryStmt.Opnd(0), false, false));
      naryMeStmt->PushBackOpnd(BuildExpr(*naryStmt.Opnd(1), false, false));
      return naryMeStmt;
    }
    case OP_callassertle: {
      auto &checkStmt = static_cast<CallAssertBoundaryStmtNode&>(stmt);
      auto *naryMeStmt = static_cast<NaryMeStmt*>(irMap->NewInPool<CallAssertBoundaryMeStmt>(&checkStmt));
      naryMeStmt->PushBackOpnd(BuildExpr(*checkStmt.Opnd(0), false, false));
      naryMeStmt->PushBackOpnd(BuildExpr(*checkStmt.Opnd(1), false, false));
      return naryMeStmt;
    }
    case OP_callassertnonnull: {
      auto &checkStmt = static_cast<CallAssertNonnullStmtNode&>(stmt);
      auto *checkMeStmt = irMap->New<CallAssertNonnullMeStmt>(&checkStmt);
      checkMeStmt->SetOpnd(0, BuildExpr(*checkStmt.Opnd(0), false, false));
      return checkMeStmt;
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
}

static bool IncDecAmountIsSmallInteger(MeExpr *x) {
  if (x->GetMeOp() != kMeOpConst) {
    return false;
  }
  ConstMeExpr *cMeExpr = static_cast<ConstMeExpr *>(x);
  MIRIntConst *cnode = dynamic_cast<MIRIntConst *>(cMeExpr->GetConstVal());
  return cnode != nullptr && cnode->GetValue() < 0x1000;
}

MeStmt *IRMapBuild::BuildDassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  DassignMeStmt *meStmt = irMap->NewInPool<DassignMeStmt>(&stmt);
  DassignNode &dassiNode = static_cast<DassignNode&>(stmt);
  VarMeExpr *varLHS = static_cast<VarMeExpr*>(BuildLHSVar(*ssaPart.GetSSAVar(), *meStmt));
  meStmt->SetLHS(varLHS);
  MeExpr *propedRHS = BuildExpr(*dassiNode.GetRHS(), false, false);
  bool noprop = (propagater && propagater->NoPropUnionAggField(meStmt, &stmt, propedRHS));
  if (!noprop) {
    meStmt->SetRHS(propedRHS);
  } else {
    meStmt->SetRHS(BuildExpr(*dassiNode.GetRHS(), false, true));
  }
  irMap->SimplifyCastForAssign(meStmt);
  BuildChiList(*meStmt, ssaPart.GetMayDefNodes(), *meStmt->GetChiList());
  // determine isIncDecStmt
  if (meStmt->GetChiList()->empty()) {
    MeExpr *rhs = meStmt->GetRHS();
    OriginalSt *ost = varLHS->GetOst();
    if (ost->GetType()->GetSize() >= 4 &&
        (rhs->GetOp() == OP_add || rhs->GetOp() == OP_sub) &&
        IsPrimitivePureScalar(rhs->GetPrimType())) {
      OpMeExpr *oprhs = static_cast<OpMeExpr *>(rhs);
      if (oprhs->GetOpnd(0)->GetMeOp() == kMeOpVar && IncDecAmountIsSmallInteger(oprhs->GetOpnd(1))) {
        meStmt->isIncDecStmt = ost == static_cast<VarMeExpr *>(oprhs->GetOpnd(0))->GetOst();
      }
    }
  }
  if (propagater) {
    propagater->PropUpdateDef(*meStmt->GetLHS());
    propagater->PropUpdateChiListDef(*meStmt->GetChiList());
  }
  return meStmt;
}

MeStmt *IRMapBuild::BuildAssignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *meStmt = irMap->New<AssignMeStmt>(&stmt);
  auto &regNode = static_cast<RegassignNode&>(stmt);
  meStmt->SetRHS(BuildExpr(*regNode.Opnd(0), false, false));
  auto *regLHS = static_cast<RegMeExpr*>(BuildLHSReg(*ssaPart.GetSSAVar(), *meStmt, regNode));
  meStmt->UpdateLhs(regLHS);
  irMap->SimplifyCastForAssign(meStmt);
  // determine isIncDecStmt
  MeExpr *rhs = meStmt->GetRHS();
  if (rhs->GetOp() == OP_add || rhs->GetOp() == OP_sub) {
    OpMeExpr *oprhs = static_cast<OpMeExpr *>(rhs);
    if (oprhs->GetOpnd(0)->GetMeOp() == kMeOpReg && oprhs->GetOpnd(1)->GetMeOp() == kMeOpConst) {
      meStmt->isIncDecStmt = regLHS->GetOst() == static_cast<RegMeExpr *>(oprhs->GetOpnd(0))->GetOst();
    }
  }
  if (propagater) {
    propagater->PropUpdateDef(*meStmt->GetLHS());
  }
  return meStmt;
}

MeStmt *IRMapBuild::BuildIassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto &iasNode = static_cast<IassignNode&>(stmt);

  auto *baseAddr = BuildExpr(*iasNode.Opnd(0), false, false);
  if (baseAddr->GetOp() == OP_addrof) {
    auto *addrofExpr = static_cast<AddrofMeExpr*>(baseAddr);
    auto *ost = ssaTab.GetOriginalStFromID(addrofExpr->GetOstIdx());
    auto *iassType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iasNode.GetTyIdx());
    if (static_cast<MIRPtrType*>(iassType)->GetPointedTyIdx() == ost->GetTyIdx()) {
      auto fldId = iasNode.GetFieldID() + addrofExpr->GetFieldID();
      OStIdx ostIdx(0);
      if (fldId != 0) {
        auto preLevType = ost->GetPrevLevelOst()->GetType();
        auto type = static_cast<MIRPtrType*>(preLevType)->GetPointedType();
        CHECK_FATAL(fldId <= type->NumberOfFieldIDs(), "field id out of range");
        auto fieldType = static_cast<MIRStructType*>(type)->GetFieldType(fldId);
        OffsetType offset(static_cast<MIRStructType*>(type)->GetBitOffsetFromBaseAddr(fldId));
        auto fieldOst = ssaTab.GetOriginalStTable().FindExtraLevOriginalSt(
            ost->GetPrevLevelOst()->GetNextLevelOsts(), fieldType, fldId, offset);
        if (fieldOst != nullptr) {
          ostIdx = fieldOst->GetIndex();
        }
      } else {
        ostIdx = addrofExpr->GetOstIdx();
      }
      if (ostIdx != 0) {
        auto *vst = ssaPart.GetMayDefNodes().at(ostIdx).GetResult();
        VarMeExpr *lhs = GetOrCreateVarFromVerSt(*vst);
        CHECK_FATAL(fldId == lhs->GetFieldID(), "field id must be equal");
        auto rhs = BuildExpr(*iasNode.GetRHS(), false, false);
        auto *meStmt = irMap->New<DassignMeStmt>(&irMap->GetIRMapAlloc(), lhs, rhs);

        BuildChiList(*meStmt, ssaPart.GetMayDefNodes(), *(meStmt->GetChiList()));
        meStmt->GetChiList()->erase(lhs->GetOstIdx());
        lhs->SetDefByStmt(*meStmt);
        lhs->SetDefBy(kDefByStmt);
        if (propagater) {
          propagater->PropUpdateDef(*meStmt->GetLHS());
          propagater->PropUpdateChiListDef(*meStmt->GetChiList());
        }
        return meStmt;
      }
    }
  }

  IassignMeStmt *meStmt = irMap->NewInPool<IassignMeStmt>(&stmt);
  meStmt->SetTyIdx(iasNode.GetTyIdx());
  meStmt->SetRHS(BuildExpr(*iasNode.GetRHS(), false, false));
  auto *lhsIvar = irMap->BuildLHSIvar(*baseAddr, *meStmt, iasNode.GetFieldID());
  if (mirModule.IsCModule()) {
    bool isVolt = false;
    for (auto &maydefItem : ssaPart.GetMayDefNodes()) {
      const OriginalSt *ost = maydefItem.second.GetResult()->GetOst();
      if (ost->IsVolatile()) {
        isVolt = true;
        break;
      }
    }
    if (isVolt) {
      lhsIvar->SetVolatileFromBaseSymbol(true);
    }
  }
  auto *simplifiedIvar = irMap->SimplifyIvar(lhsIvar, true);
  if (simplifiedIvar != nullptr && simplifiedIvar->GetMeOp() == kMeOpIvar) {
    lhsIvar = static_cast<IvarMeExpr *>(simplifiedIvar);
  }
  meStmt->SetLHSVal(lhsIvar);

  irMap->SimplifyCastForAssign(meStmt);
  BuildChiList(*meStmt, ssaPart.GetMayDefNodes(), *(meStmt->GetChiList()));
  if (propagater) {
    propagater->PropUpdateChiListDef(*meStmt->GetChiList());
  }
  return meStmt;
}

MeStmt *IRMapBuild::BuildMaydassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *meStmt = irMap->NewInPool<MaydassignMeStmt>(&stmt);
  auto &dassiNode = static_cast<DassignNode&>(stmt);
  meStmt->SetRHS(BuildExpr(*dassiNode.GetRHS(), false, false));
  meStmt->SetMayDassignSym(ssaPart.GetSSAVar()->GetOst());
  meStmt->SetFieldID(dassiNode.GetFieldID());
  BuildChiList(*meStmt, ssaPart.GetMayDefNodes(), *(meStmt->GetChiList()));
  if (propagater) {
    propagater->PropUpdateChiListDef(*meStmt->GetChiList());
  }
  return meStmt;
}

MeStmt *IRMapBuild::BuildCallMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto *callMeStmt = irMap->NewInPool<CallMeStmt>(&stmt);
  auto &intrinNode = static_cast<CallNode&>(stmt);
  callMeStmt->SetPUIdx(intrinNode.GetPUIdx());
  for (size_t i = 0; i < intrinNode.NumOpnds(); ++i) {
    MeExpr *meExpr = BuildExpr(*intrinNode.Opnd(i), true, false);
    if (i >= kMaxRegParamNum) {
      PrimType origType = intrinNode.Opnd(i)->GetPrimType();
      PrimType curType = meExpr->GetPrimType();
      if (GetPrimTypeSize(origType) != GetPrimTypeSize(curType)) {
        // When we pass parameters by stack, cvt is needed for call opnds if type size changes
        meExpr = irMap->CreateMeExprTypeCvt(origType, curType, *meExpr);
      }
    }
    callMeStmt->PushBackOpnd(meExpr);
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(callMeStmt->GetMuList()));
  if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
    BuildMustDefList(*callMeStmt, ssaPart.GetMustDefNodes(), *(callMeStmt->GetMustDefList()));
  }
  BuildChiList(*callMeStmt, ssaPart.GetMayDefNodes(), *(callMeStmt->GetChiList()));
  if (propagater) {
    propagater->PropUpdateChiListDef(*callMeStmt->GetChiList());
    if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
      propagater->PropUpdateMustDefList(callMeStmt);
    }
  }
  return callMeStmt;
}


MeStmt *IRMapBuild::BuildNaryMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  Opcode op = stmt.GetOpCode();
  NaryMeStmt *naryMeStmt = (op == OP_icall || op == OP_icallassigned)
                           ? static_cast<NaryMeStmt*>(irMap->NewInPool<IcallMeStmt>(&stmt))
                           : static_cast<NaryMeStmt*>(irMap->NewInPool<IntrinsiccallMeStmt>(&stmt));
  auto &naryStmtNode = static_cast<NaryStmtNode&>(stmt);
  for (size_t i = 0; i < naryStmtNode.NumOpnds(); ++i) {
    naryMeStmt->PushBackOpnd(BuildExpr(*naryStmtNode.Opnd(i), false, false));
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(naryMeStmt->GetMuList()));
  if (kOpcodeInfo.IsCallAssigned(op)) {
    BuildMustDefList(*naryMeStmt, ssaPart.GetMustDefNodes(), *(naryMeStmt->GetMustDefList()));
  }
  BuildChiList(*naryMeStmt, ssaPart.GetMayDefNodes(), *(naryMeStmt->GetChiList()));
  if (propagater) {
    propagater->PropUpdateChiListDef(*naryMeStmt->GetChiList());
  }
  return naryMeStmt;
}

MeStmt *IRMapBuild::BuildRetMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto &retStmt = static_cast<NaryStmtNode&>(stmt);
  auto *meStmt = irMap->NewInPool<RetMeStmt>(&stmt);
  for (size_t i = 0; i < retStmt.NumOpnds(); ++i) {
    meStmt->PushBackOpnd(BuildExpr(*retStmt.Opnd(i), false, false));
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
  tmeStmt->SetMeStmtOpndValue(BuildExpr(*unaryNode.Opnd(0), false, false));
  BuildMuList(ssaPart.GetMayUseNodes(), *(tmeStmt->GetMuList()));
  return tmeStmt;
}

MeStmt *IRMapBuild::BuildSyncMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  auto &naryNode = static_cast<NaryStmtNode&>(stmt);
  auto *naryStmt = irMap->NewInPool<SyncMeStmt>(&stmt);
  for (size_t i = 0; i < naryNode.NumOpnds(); ++i) {
    naryStmt->PushBackOpnd(BuildExpr(*naryNode.Opnd(i), false, false));
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(naryStmt->GetMuList()));
  BuildChiList(*naryStmt, ssaPart.GetMayDefNodes(), *(naryStmt->GetChiList()));
  return naryStmt;
}

MeStmt *IRMapBuild::BuildAsmMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart) {
  AsmNode *asmNode = &static_cast<AsmNode&>(stmt);
  AsmMeStmt *asmMeStmt = irMap->NewInPool<AsmMeStmt>(asmNode);
  for (size_t i = 0; i < asmNode->NumOpnds(); ++i) {
    asmMeStmt->PushBackOpnd(BuildExpr(*asmNode->Opnd(i), true, true));
  }
  BuildMuList(ssaPart.GetMayUseNodes(), *(asmMeStmt->GetMuList()));
  BuildMustDefList(*asmMeStmt, ssaPart.GetMustDefNodes(), *(asmMeStmt->GetMustDefList()));
  BuildChiList(*asmMeStmt, ssaPart.GetMayDefNodes(), *(asmMeStmt->GetChiList()));
  asmMeStmt->asmString = asmNode->asmString;
  asmMeStmt->inputConstraints = asmNode->inputConstraints;
  asmMeStmt->outputConstraints = asmNode->outputConstraints;
  asmMeStmt->clobberList = asmNode->clobberList;
  asmMeStmt->gotoLabels = asmNode->gotoLabels;
  asmMeStmt->qualifiers = asmNode->qualifiers;
  if (propagater) {
    propagater->PropUpdateChiListDef(*asmMeStmt->GetChiList());
    propagater->PropUpdateMustDefList(asmMeStmt);
  }
  return asmMeStmt;
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
  RegisterFactoryFunction<MeStmtFactory>(OP_regassign, &IRMapBuild::BuildAssignMeStmt);
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
  RegisterFactoryFunction<MeStmtFactory>(OP_asm, &IRMapBuild::BuildAsmMeStmt);
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
  std::vector<uint32> curStackSizeVec;
  if (propagater) { // proper is not null that means we will do propragation during build meir.
    propagater->SetCurBB(&bb);
    uint32 vstVecsize = propagater->GetVstLiveStackVecSize();
    curStackSizeVec.resize(vstVecsize);
    for (uint32 i = 1; i < vstVecsize; ++i) {
      curStackSizeVec[i] = 0;
      curStackSizeVec[i] = static_cast<uint32>(propagater->GetVstLiveStackVec(i)->size());
    }
    // traversal phi nodes
    MapleMap<OStIdx, MePhiNode *> &mePhiList = bb.GetMePhiList();
    for (auto it = mePhiList.begin(); it != mePhiList.end(); ++it) {
      MePhiNode *phimenode = it->second;
      propagater->PropUpdateDef(*phimenode->GetLHS());
    }
  }

  if (!bb.IsEmpty()) {
    for (auto &stmt : bb.GetStmtNodes()) {
      MeStmt *meStmt = BuildMeStmt(stmt);
      bb.AddMeStmtLast(meStmt);
    }
  }
  // travesal bb's dominated tree
  ASSERT(bbID < dominance.GetDomChildrenSize(), " index out of range in IRMapBuild::BuildBB");
  const auto &domChildren = dominance.GetDomChildren(bbID);
  for (auto bbIt = domChildren.begin(); bbIt != domChildren.end(); ++bbIt) {
    BBId childBBId = *bbIt;
    BuildBB(*irMap->GetBB(childBBId), bbIRMapProcessed);
  }
  if (propagater) {
    for (uint32 i = 1; i < curStackSizeVec.size(); i++) {
      MapleStack<MeExpr *> *liveStack = propagater->GetVstLiveStackVec(i);
      uint32 curSize = curStackSizeVec[i];
      while (liveStack->size() > curSize) {
        liveStack->pop();
      }
    }
  }
}
}  // namespace maple
