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
#include "me_ssa_prop.h"
#include "me_phase_manager.h"
#include "constantfold.h"
namespace maple {
SSAProp::SSAProp(MeFunction *f, Dominance *d) : func(f), dom(d), ssaTab(func->GetMeSSATab()) {
  auto &origTab = func->GetMeSSATab()->GetOriginalStTable().GetOriginalStVector();
  vstLiveStack.resize(origTab.size());
  for (size_t i = 1; i < origTab.size(); ++i) {
    OriginalSt *ost = origTab[i];
    if (ost->GetVersionsIndices().size() != 0) {
      VersionSt *zeroVst = ssaTab->GetVersionStTable().GetZeroVersionSt(origTab[i]);
      vstLiveStack[i].push(zeroVst);
    }
  }
}

// iread fld1 (addrof fld2 a) => dread fld1+fld2 a
BaseNode *SSAProp::SimplifyIreadAddrof(IreadSSANode *ireadSSA) const {
  if (ireadSSA->Opnd(0)->GetOpCode() != OP_addrof) {
    return ireadSSA;
  }
  BaseNode *base = ireadSSA->Opnd(0);
  ASSERT(base->IsSSANode(), "ssa-tab should run before!");
  OriginalSt *baseOst = static_cast<AddrofSSANode *>(base)->GetSSAVar()->GetOst();
  TyIdx baseTyIdx = baseOst->GetTyIdx(); // note: baseTyIdx is memory type, not pointer to memory type
  IreadNode *iread = static_cast<IreadNode *>(ireadSSA->GetNoSSANode());
  MIRType *ireadBaseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
  ASSERT(ireadBaseType->IsMIRPtrType(), "Type of IreadNode's base must be pointer!");
  // type consistent check : if access type of iread is different from that of addrofNode, we cannot simplify it.
  // for example : `a : <[2] i32>; iread i32 <* i32> (addrof 0 a)` can not be simplified to `dread i32 a`;
  if (static_cast<MIRPtrType *>(ireadBaseType)->GetPointedTyIdx() != baseTyIdx) {
    return ireadSSA;
  }
  ASSERT(baseOst->IsSymbolOst(), "Ost of Addrof must be symbol ost!");
  MIRSymbol *sym = baseOst->GetMIRSymbol();
  FieldID newFld = baseOst->GetFieldID() + iread->GetFieldID();
  VersionSt *mu = ireadSSA->GetSSAVar();
  if (mu != nullptr) { // mu will be set after alias analysis.
    OriginalSt *muOst = mu->GetOst();
    if (!muOst->IsSymbolOst() || muOst->GetMIRSymbol() != sym || muOst->GetFieldID() != newFld) {
      return ireadSSA;
    }
  }
  DreadNode *dread = func->GetMIRModule().GetMIRBuilder()->CreateExprDread(iread->GetPrimType(), newFld, *sym);
  auto *dreadSSANode = static_cast<SSANode *>(ssaTab->CreateSSAExpr(dread));
  if (mu != nullptr) {
    dreadSSANode->SetSSAVar(*mu);
  }
  return dreadSSANode;
}

BaseNode *SSAProp::SimplifyExpr(BaseNode *expr) const {
  for (size_t i = 0; i < expr->NumOpnds(); ++i) {
    expr->SetOpnd(SimplifyExpr(expr->Opnd(i)), i);
  }
  // iread fld1 (addrof fld2 a)
  BaseNode *result = expr;
  if (expr->GetOpCode() == OP_iread) {
    ASSERT(expr->IsSSANode(), "ssa-tab should run before!");
    result = SimplifyIreadAddrof(static_cast<IreadSSANode *>(expr));
    if (result != expr) {
      return result;
    }
  }
  // process other simplify like constant fold
  if (expr->IsBinaryNode() && expr->Opnd(0)->IsConstval() && expr->Opnd(1)->IsConstval()) {
    ConstantFold folder(func->GetMIRModule());
    result = folder.Fold(expr);
    if (result != nullptr) {
      return result;
    }
  }
  return expr;
}

// iassign fld1 (addrof fld2 sym) => dassign fld1+fld2 sym
StmtNode * SSAProp::SimplifyIassignAddrof(IassignNode *iassign, BB *bb) const {
  BaseNode *base = iassign->Opnd(0);
  if (base->GetOpCode() != OP_addrof) {
    return iassign;
  }
  AddrofNode *addrofNode =
      static_cast<AddrofNode *>((base->IsSSANode()) ? static_cast<AddrofSSANode *>(base)->GetNoSSANode() : base);
  // 1. try to simplify
  ConstantFold folder(func->GetMIRModule());
  StmtNode *dassign = folder.SimplifyIassignWithAddrofBaseNode(*iassign, *addrofNode);
  if (dassign == iassign) { // cannot be simplified
    return iassign;
  }
  // 2. set ssa part for new stmt.
  ssaTab->CreateSSAStmt(*dassign, bb);
  if (func->IsMemSSAValid()) { // after memory ssa, we should maintain maydef list.
    StmtsSSAPart &ssaPart = ssaTab->GetStmtsSSAPart();
    // check if new assigned ost is in maydef list.
    TypeOfMayDefList &oldMayDefList = ssaPart.GetMayDefNodesOf(*iassign);
    VersionSt *newDef = ssaPart.GetAssignedVarOf(*dassign);
    OriginalSt *newOst = newDef->GetOst();
    auto it = oldMayDefList.find(newOst->GetIndex());
    if (it == oldMayDefList.end()) {
      return iassign; // no must def version find, not simplify.
    }
    // set ssa var as new version found in may list.
    VersionSt *vst = it->second.GetResult();
    ssaPart.SSAPartOf(*dassign)->SetSSAVar(*vst);
    // note: should update vst's def info, otherwise it will be defined by mayDef list of old stmt.
    vst->SetAssignNode(dassign);
    vst->SetDefType(VersionSt::kAssign);
    // copy other item of original may def list.
    TypeOfMayDefList &newMayDefList = ssaPart.GetMayDefNodesOf(*dassign);
    ASSERT(newMayDefList.empty(), "A new iassign should not be set maydef list before");
    newMayDefList.insert(oldMayDefList.begin(), oldMayDefList.end());
    newMayDefList.erase(newOst->GetIndex());
    // note: should update maydefNode's def info, otherwise it will be associated with old stmt.
    std::for_each(newMayDefList.begin(), newMayDefList.end(), [dassign](std::pair<const OStIdx, MayDefNode> &item) {
      item.second.SetStmt(dassign);
    });
  }
  return dassign;
}

StmtNode *SSAProp::SimplifyStmt(StmtNode *stmt, BB *bb) const {
  for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
    BaseNode *oldOpnd = stmt->Opnd(i);
    BaseNode *newOpnd = SimplifyExpr(oldOpnd);
    if (oldOpnd != newOpnd) {
      stmt->SetOpnd(newOpnd, i);
    }
  }
  if (stmt->GetOpCode() == OP_iassign && stmt->Opnd(0)->GetOpCode() == OP_addrof) {
    return SimplifyIassignAddrof(static_cast<IassignNode *>(stmt), bb);
  }
  return stmt;
}

bool SSAProp::CanOstProped(OriginalSt *ost) const {
  if (ost->IsVolatile()) {
    return false;
  }
  if (ost->IsPregOst() && ost->GetPregIdx() < 0) {
    return false; // no prop for special reg.
  }
  if (!ost->IsLocal() && !ost->IsFinal() && !ost->IsIgnoreRC()) {
    return false;
  }
  if (func->IsMemSSAValid()) {
    return true;
  }
  if (func->IsTopLevelSSAValid()) {
    return IsLocalTopLevelOst(*ost);
  }
  return false;
}
// check if :
//   1) rhs is leaf node and,
//   2) prop rhs must not cross a new version
bool SSAProp::Propagatable(BaseNode *rhs) const {
  if (!rhs->IsLeaf()) { // propagate non-leaf expr will generate compound expr
    return false;
  }
  switch (rhs->GetOpCode()) {
    case OP_regread:
    case OP_dread: {
      VersionSt *vst = static_cast<SSANode *>(rhs)->GetSSAVar();
      if (!CanOstProped(vst->GetOst())) {
        return false;
      }
      // version consistent : make sure no vst cross a new version
      if (vstLiveStack[vst->GetOst()->GetIndex()].top() != vst) {
        return false;
      }
      return true;
    }
    case OP_addrof:
    case OP_constval: {
      return true;
    }
    default:
      return false;
  }
}

void SSAProp::UpdateDef(VersionSt *vst) {
  if (vst != nullptr && !vst->IsInitVersion()) {
    vstLiveStack[vst->GetOst()->GetIndex()].push(vst);
  }
}

void SSAProp::UpdateDefOfMayDef(const StmtNode &stmt) {
  if (!kOpcodeInfo.HasSSADef(stmt.GetOpCode()) || stmt.GetOpCode() == maple::OP_regassign) {
    return;
  }
  StmtsSSAPart &ssaPart = ssaTab->GetStmtsSSAPart();
  TypeOfMayDefList &mayDefList = ssaPart.GetMayDefNodesOf(stmt);
  for (auto it = mayDefList.begin(); it != mayDefList.end(); ++it) {
    MayDefNode &mayDef = it->second;
    VersionSt *vst = mayDef.GetResult();
    UpdateDef(vst);
  }
}

// if lhs is smaller than rhs, insert operation to simulate the truncation
// effect of rhs being stored into lhs; otherwise, just return rhs
BaseNode * SSAProp::CheckTruncation(BaseNode *lhs, BaseNode *rhs) const {
  PrimType lhsPtyp = lhs->GetPrimType();
  PrimType rhsPtyp = rhs->GetPrimType();
  if (func->GetMIRModule().IsJavaModule() || !IsPrimitiveInteger(rhsPtyp)) {
    return rhs;
  }
  ASSERT(lhs->IsSSANode(), "ssatab must run before!");
  OriginalSt *ost = static_cast<SSANode *>(lhs)->GetSSAVar()->GetOst();
  MIRType *lhsType = ost->GetType();
  if (lhsType->GetKind() == kTypeBitField) {
    auto *bfType = static_cast<MIRBitFieldType *>(lhsType);
    if (GetPrimTypeBitSize(rhsPtyp) <= bfType->GetFieldSize()) { // no need cvt for : large type <- small type
      return rhs;
    }
    Opcode extOp = IsSignedInteger(lhsPtyp) ? OP_sext : OP_zext;
    PrimType extPtyp = (bfType->GetFieldSize() <= 32) ? (IsSignedInteger(lhsPtyp) ? PTY_i32 : PTY_u32)
                                                      : (IsSignedInteger(lhsPtyp) ? PTY_i64 : PTY_i32);
    return func->GetMIRModule().GetMIRBuilder()->CreateExprExtractbits(extOp, extPtyp, 0, bfType->GetFieldSize(), rhs);
  }
  if (IsPrimitiveInteger(lhsPtyp) != IsPrimitiveInteger(rhsPtyp) ||
      GetPrimTypeSize(lhsPtyp) < GetPrimTypeSize(rhsPtyp) ||
      (GetPrimTypeSize(lhsPtyp) == GetPrimTypeSize(rhsPtyp) && IsSignedInteger(lhsPtyp) != IsSignedInteger(rhsPtyp))) {
    return func->GetMIRModule().GetMIRBuilder()->CreateExprTypeCvt(OP_cvt, lhsPtyp, rhsPtyp, *rhs);
  }

  return rhs;
}

// if all phiOpnds are the same(after proped), we can prop it, otherwise return nullptr.
BaseNode * SSAProp::PropPhi(PhiNode *phi) {
  if (!MeOption::propAtPhi) {
    return nullptr;
  }
  MapleVector<VersionSt *> &phiOpnds = phi->GetPhiOpnds();
  VersionSt *phiOpndLast = phiOpnds.back();
  // To avoid endless loop : if phiOpnd is def by a phi, stop propagating.
  BaseNode *opndLastProp = PropVst(phiOpndLast, false);
  if (opndLastProp == nullptr) {
    return nullptr;
  }
  for (auto it = phiOpnds.rbegin() + 1; it != phiOpnds.rend(); ++it) {
    BaseNode *opnd = PropVst(*it, false);
    if (!IsSameContent(opnd, opndLastProp, true)) {
      return nullptr;
    }
  }
  return opndLastProp;
}

// if vst can be propagated, return expr, otherwise return nullptr;
BaseNode *SSAProp::PropVst(VersionSt *vst, bool checkPhi) {
  if (vst == nullptr || vst->GetOst()->IsVolatile()) {
    return nullptr;
  }
  if (vst->GetDefType() == VersionSt::kPhi && checkPhi && MeOption::propAtPhi) {
    // try to prop phi if all phiOpnds are the same.
    return PropPhi(vst->GetPhi());
  }
  if (vst->GetDefType() != VersionSt::kAssign) {
    return nullptr;
  }
  StmtNode *defStmt = vst->GetAssignNode();
  // check if defRHS can be prop
  BaseNode *defRHS = defStmt->GetRHS();
  if (!Propagatable(defRHS)) {
    return nullptr;
  }
  // replace expr with defRHS
  MapleAllocator &allocator = func->GetMIRModule().GetMPAllocator();
  BaseNode *newRHS = defRHS->CloneTree(allocator);
  return newRHS;
}

BaseNode *SSAProp::PropExpr(BaseNode *expr, bool &subProp) {
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    BaseNode *oldOpnd = expr->Opnd(i);
    BaseNode *newOpnd = PropExpr(oldOpnd, subProp);
    if (newOpnd != oldOpnd) { // prop happen
      expr->SetOpnd(newOpnd, i);
      subProp = true;
    }
  }
  if (!expr->IsSSANode() || expr->GetOpCode() == OP_addrof) {
    return expr;
  }
  VersionSt *vst = static_cast<SSANode *>(expr)->GetSSAVar();
  BaseNode *newRHS = PropVst(vst, true);
  if (newRHS == nullptr) {
    return expr; // no prop
  }
  newRHS = CheckTruncation(expr, newRHS);
  subProp = true; // update state.
  return newRHS;
}



void SSAProp::TraverseStmt(StmtNode *stmt, BB *currBB) {
  Opcode op = stmt->GetOpCode();
  if (op == OP_asm) { // do not prop use in inline asm
    UpdateDefOfMayDef(*stmt);
    return;
  }
  // 1.prop use in stmt's opnds
  bool proped = false;
  for (size_t i = 0; i < stmt->GetNumOpnds(); ++i) {
    BaseNode *oldOpnd = stmt->Opnd(i);
    BaseNode *newOpnd = PropExpr(oldOpnd, proped);
    if (newOpnd != oldOpnd) {
      stmt->SetOpnd(newOpnd, i);
      proped = true;
    }
  }
  if (proped) {
    // try to simplify
    StmtNode *newStmt = SimplifyStmt(stmt, currBB);
    if (newStmt != stmt) {
      currBB->ReplaceStmt(stmt, newStmt);
      stmt = newStmt;
    }
  }
  // 2.Push def version to vstLiveStack according to stmt kind.
  //   1) def of reg-/d-assign
  //   2) must def list
  //   3) may def list
  StmtsSSAPart &ssaPart = ssaTab->GetStmtsSSAPart();
  // 1) def by regassign/dassign
  if (kOpcodeInfo.AssignActualVar(stmt->GetOpCode())) {
    VersionSt *vst = ssaPart.GetAssignedVarOf(*stmt);
    UpdateDef(vst);
  } else if (kOpcodeInfo.IsCallAssigned(stmt->GetOpCode())) {
    // 2) def by must def list
    MapleVector<MustDefNode> &mustDefs = ssaPart.GetMustDefNodesOf(*stmt);
    for (MustDefNode &mustDefNode : mustDefs) {
      VersionSt *vst = mustDefNode.GetResult();
      UpdateDef(vst);
    }
  }
  // 3) def by may def list
  if (kOpcodeInfo.HasSSADef(stmt->GetOpCode())) {
    UpdateDefOfMayDef(*stmt);
  }
}

void SSAProp::TraversalBB(BB &bb) {
  // record stack size for recovering vstLiveStack after traversing dominance children BB
  std::vector<size_t> curStackSize(vstLiveStack.size(), 0);
  for (size_t i = 1; i < vstLiveStack.size(); ++i) {
    curStackSize[i] = vstLiveStack[i].size();
  }
  // update def from phi list
  for (auto &phi : bb.GetPhiList()) {
    UpdateDef(phi.second.GetResult());
  }
  // traverse stmt
  for (auto &stmt : bb.GetStmtNodes()) {
    TraverseStmt(&stmt, &bb);
  }
  // traverse dominator children
  auto &domChildren = dom->GetDomChildren(bb.GetBBId());
  for (auto it = domChildren.begin(); it != domChildren.end(); ++it) {
    TraversalBB(*func->GetCfg()->GetBBFromID(*it));
  }

  // recover vstLive
  for (size_t i = 1; i < vstLiveStack.size(); ++i) {
    std::stack<VersionSt *> &liveStack = vstLiveStack[i];
    if (i < curStackSize.size()) {
      while (curStackSize[i] < liveStack.size()) {
        liveStack.pop();
      }
    } else {
      // clear tmp vst
      while (!liveStack.empty()) {
        liveStack.pop();
      }
    }
  }
}

void SSAProp::PropInFunc() {
  TraversalBB(*func->GetCfg()->GetCommonEntryBB());
}

void MESSAProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MESSAProp::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);
  if (!f.IsMplIRAvailable()) {
    return false;
  }
  SSAProp ssaProp(&f, dom);
  ssaProp.PropInFunc();
  return false;
}
} // namespace maple
