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
using MeStmtFactory = FunctionFactory<Opcode, MeStmt*, IRMapBuild*, StmtNode&, AccessSSANodes&>;

VarMeExpr *IRMapBuild::GetOrCreateVarFromVerSt(const VersionSt &vst) {
  size_t vindex = vst.GetIndex();
  ASSERT(vindex < irMap->vst2MeExprTable.size(), "GetOrCreateVarFromVerSt: index %d is out of range", vindex);
  MeExpr *meExpr = irMap->vst2MeExprTable.at(vindex);
  if (meExpr != nullptr) {
    return static_cast<VarMeExpr*>(meExpr);
  }
  const OriginalSt *ost = vst.GetOrigSt();
  ASSERT(ost->IsSymbolOst(), "GetOrCreateVarFromVerSt: wrong ost_type");
  auto *varx = irMap->New<VarMeExpr>(&irMap->irMapAlloc, irMap->exprID++, ost->GetIndex(), vindex,
     GlobalTables::GetTypeTable().GetTypeTable()[ost->GetTyIdx().GetIdx()]->GetPrimType());
  ASSERT(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
  varx->SetFieldID(ost->GetFieldID());
  irMap->vst2MeExprTable[vindex] = varx;
  return varx;
}

RegMeExpr *IRMapBuild::GetOrCreateRegFromVerSt(const VersionSt &vst) {
  size_t vindex = vst.GetIndex();
  ASSERT(vindex < irMap->vst2MeExprTable.size(), " GetOrCreateRegFromVerSt: index %d is out of range", vindex);
  MeExpr *meExpr = irMap->vst2MeExprTable[vindex];
  if (meExpr != nullptr) {
    return static_cast<RegMeExpr*>(meExpr);
  }
  const OriginalSt *ost = vst.GetOrigSt();
  ASSERT(ost->IsPregOst(), "GetOrCreateRegFromVerSt: PregOST expected");
  auto *regx = irMap->NewInPool<RegMeExpr>(irMap->exprID++, ost->GetPregIdx(), mirModule.CurFunction()->GetPuidx(),
                                           ost->GetIndex(), vindex, ost->GetMIRPreg()->GetPrimType());
  irMap->vst2MeExprTable[vindex] = regx;
  return regx;
}

MeExpr *IRMapBuild::BuildLHSVar(const VersionSt &vst, DassignMeStmt &defMeStmt) {
  VarMeExpr *meDef = GetOrCreateVarFromVerSt(vst);
  meDef->SetDefStmt(&defMeStmt);
  meDef->SetDefBy(kDefByStmt);
  irMap->vst2MeExprTable.at(vst.GetIndex()) = meDef;
  return meDef;
}

MeExpr *IRMapBuild::BuildLHSReg(const VersionSt &vst, RegassignMeStmt &defMeStmt, const RegassignNode &regassign) {
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
    (void)outList.insert(std::make_pair(lhs->GetOStIdx(), chiMeStmt));
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
    (void)muList.insert(std::make_pair(varMeExpr->GetOStIdx(), varMeExpr));
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

MeExpr *IRMapBuild::BuildExpr(BaseNode &mirNode) {
  Opcode op = mirNode.GetOpCode();
  if (op == OP_dread) {
    auto &addrOfNode = static_cast<AddrofSSANode&>(mirNode);
    VersionSt *vst = addrOfNode.GetSSAVar();
    VarMeExpr *varMeExpr = GetOrCreateVarFromVerSt(*vst);
    ASSERT(!vst->GetOrigSt()->IsPregOst(), "not expect preg symbol here");
    varMeExpr->SetPtyp(GlobalTables::GetTypeTable().GetTypeFromTyIdx(vst->GetOrigSt()->GetTyIdx())->GetPrimType());
    varMeExpr->SetFieldID(addrOfNode.GetFieldID());
    return varMeExpr;
  }

  if (op == OP_regread) {
    auto &regNode = static_cast<RegreadSSANode&>(mirNode);
    VersionSt *vst = regNode.GetSSAVar();
    RegMeExpr *regMeExpr = GetOrCreateRegFromVerSt(*vst);
    regMeExpr->SetPtyp(mirNode.GetPrimType());
    return regMeExpr;
  }

  MeExpr *meExpr = irMap->meBuilder.BuildMeExpr(mirNode);
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

  if (op == OP_iread) {
    ASSERT(static_cast<IvarMeExpr*>(retMeExpr)->GetMu() != nullptr, "BuildExpr: ivar node cannot have mu == nullptr");
  }

  return retMeExpr;
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
  meStmt->SetMayDassignSym(ssaPart.GetSSAVar()->GetOrigSt());
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

bool IRMapBuild::InitMeStmtFactory() {
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
  return true;
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
