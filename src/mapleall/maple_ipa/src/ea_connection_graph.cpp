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
#include "ea_connection_graph.h"

namespace maple {
constexpr maple::uint32 kInvalid = 0xffffffff;
void EACGBaseNode::CheckAllConnectionInNodes() {
#if DEBUG
  for (EACGBaseNode *inNode : in) {
    ASSERT_NOT_NULL(eaCG->nodes[inNode->id - 1]);
    ASSERT(eaCG->nodes[inNode->id - 1] == inNode, "must be inNode");
  }
  for (EACGBaseNode *outNode : out) {
    ASSERT_NOT_NULL(eaCG->nodes[outNode->id - 1]);
    ASSERT(eaCG->nodes[outNode->id - 1] == outNode, "must be outNode");
  }
  for (EACGObjectNode *obj : pointsTo) {
    ASSERT_NOT_NULL(eaCG->nodes[obj->id - 1]);
    ASSERT(eaCG->nodes[obj->id - 1] == obj, "must be obj");
  }
  if (IsFieldNode()) {
    for (EACGObjectNode *obj : static_cast<EACGFieldNode*>(this)->GetBelongsToObj()) {
      ASSERT_NOT_NULL(eaCG->nodes[obj->id - 1]);
      ASSERT(eaCG->nodes[obj->id - 1] == obj, "must be obj");
    }
  }
#endif
}

bool EACGBaseNode::AddOutNode(EACGBaseNode &newOut) {
  if (out.find(&newOut) != out.end()) {
    return false;
  }
  bool newIsLocal = newOut.UpdateEAStatus(eaStatus);
  if (eaStatus == kGlobalEscape && pointsTo.size() > 0) {
    if (newIsLocal) {
      eaCG->SetCGUpdateFlag();
    }
    return newIsLocal;
  }
  (void)out.insert(&newOut);
  (void)newOut.in.insert(this);
  ASSERT(newOut.pointsTo.size() != 0, "must be greater than zero");
  bool hasChanged = UpdatePointsTo(newOut.pointsTo);
  eaCG->SetCGUpdateFlag();
  return hasChanged;
}

void EACGBaseNode::PropagateEAStatusForNode(const EACGBaseNode*) const {
  for (EACGBaseNode *outNode : out) {
    outNode->UpdateEAStatus(eaStatus);
  }
}

std::string EACGBaseNode::GetName(const IRMap *irMap) const {
  std::string name;
  if (irMap == nullptr || meExpr == nullptr) {
    name += std::to_string(id);
  } else {
    name += std::to_string(id);
    name += "\\n";
    if (meExpr->GetMeOp() == kMeOpVar) {
      VarMeExpr *varMeExpr = static_cast<VarMeExpr*>(meExpr);
      const MIRSymbol *sym = varMeExpr->GetOst()->GetMIRSymbol();
      name += ((sym->GetStIdx().IsGlobal() ? "$" : "%") + sym->GetName() + "\\nmx" +
              std::to_string(meExpr->GetExprID()) + " (field)" + std::to_string(varMeExpr->GetFieldID()));
    } else if (meExpr->GetMeOp() == kMeOpIvar) {
      IvarMeExpr *ivarMeExpr = static_cast<IvarMeExpr*>(meExpr);
      MeExpr *base = ivarMeExpr->GetBase();
      VarMeExpr *varMeExpr = nullptr;
      if (base->GetMeOp() == kMeOpVar) {
        varMeExpr = static_cast<VarMeExpr*>(base);
      } else {
        name += std::to_string(id);
        return name;
      }
      const MIRSymbol *sym = varMeExpr->GetOst()->GetMIRSymbol();
      name += (std::string("base :") + (sym->GetStIdx().IsGlobal() ? "$" : "%") + sym->GetName() + "\\nmx" +
              std::to_string(meExpr->GetExprID()) + " (field)" + std::to_string(ivarMeExpr->GetFieldID()));
    } else if (meExpr->GetOp() == OP_gcmalloc || meExpr->GetOp() == OP_gcmallocjarray) {
      name += "mx" + std::to_string(meExpr->GetExprID());
    }
  }
  return name;
}

bool EACGBaseNode::UpdatePointsTo(const std::set<EACGObjectNode*> &cPointsTo) {
  uint32 oldPtSize = pointsTo.size();
  pointsTo.insert(cPointsTo.begin(), cPointsTo.end());
  if (oldPtSize == pointsTo.size()) {
    return false;
  }
  for (EACGObjectNode *pt : pointsTo) {
    pt->Insert2PointsBy(this);
  }
  for (EACGBaseNode *pred : in) {
    pred->UpdatePointsTo(pointsTo);
  }
  return true;
}

void EACGBaseNode::GetNodeFormatInDot(std::string &label, std::string &color) const {
  switch (GetEAStatus()) {
    case kNoEscape:
      label += "NoEscape";
      color = "darkgreen";
      break;
    case kArgumentEscape:
      label += "ArgEscape";
      color = "brown";
      break;
    case kReturnEscape:
      label += "RetEscape";
      color = "orange";
      break;
    case kGlobalEscape:
      label += "GlobalEscape";
      color = "red";
      break;
  }
}

bool EACGBaseNode::CanIgnoreRC() const {
  for (auto obj : pointsTo) {
    if (!obj->GetIgnorRC()) {
      return false;
    }
  }
  return true;
}

void EACGObjectNode::CheckAllConnectionInNodes() {
#if DEBUG
  for (EACGBaseNode *inNode : in) {
    ASSERT_NOT_NULL(eaCG->nodes[inNode->id - 1]);
    ASSERT(eaCG->nodes[inNode->id - 1] == inNode, "must be inNode");
  }
  for (EACGBaseNode *outNode : out) {
    ASSERT_NOT_NULL(eaCG->nodes[outNode->id - 1]);
    ASSERT(eaCG->nodes[outNode->id - 1] == outNode, "must be outNode");
  }
  for (EACGBaseNode *pBy : pointsBy) {
    ASSERT_NOT_NULL(eaCG->nodes[pBy->id - 1]);
    ASSERT(eaCG->nodes[pBy->id - 1] == pBy, "must be pBy");
  }
  for (auto fieldPair : fieldNodes) {
    EACGFieldNode *field = fieldPair.second;
    ASSERT(field->fieldID == fieldPair.first, "must be fieldPair.first");
    ASSERT_NOT_NULL(eaCG->nodes[field->id - 1]);
    ASSERT(eaCG->nodes[field->id - 1] == field, "must be filed");
  }
#endif
}

bool EACGObjectNode::IsPointedByFieldNode() const {
  for (EACGBaseNode *pBy : pointsBy) {
    if (pBy->IsFieldNode()) {
      return true;
    }
  }
  return false;
}

bool EACGObjectNode::AddOutNode(EACGBaseNode &newOut) {
  ASSERT(newOut.IsFieldNode(), "must be fieldNode");
  EACGFieldNode *field = static_cast<EACGFieldNode*>(&newOut);
  fieldNodes[field->GetFieldID()] = field;
  newOut.UpdateEAStatus(eaStatus);
  field->AddBelongTo(this);
  return true;
}

bool EACGObjectNode::ReplaceByGlobalNode() {
  ASSERT(out.size() == 0, "must be zero");
  for (EACGBaseNode *node : pointsBy) {
    node->pointsTo.erase(this);
    (void)node->pointsTo.insert(eaCG->GetGlobalObject());
  }
  pointsBy.clear();
  for (EACGBaseNode *inNode : in) {
    (void)inNode->out.erase(this);
    (void)inNode->out.insert(eaCG->GetGlobalObject());
  }
  in.clear();
  for (auto fieldPair : fieldNodes) {
    EACGFieldNode *field = fieldPair.second;
    field->belongsTo.erase(this);
  }
  fieldNodes.clear();
  if (meExpr != nullptr) {
    eaCG->expr2Nodes[meExpr]->clear();
    eaCG->expr2Nodes[meExpr]->insert(eaCG->GetGlobalObject());
  }
  ASSERT(eaCG->nodes[id - 1] == this, "must be");
  eaCG->nodes[id - 1] = nullptr;
  return true;
}

void EACGObjectNode::PropagateEAStatusForNode(const EACGBaseNode*) const {
  for (auto fieldNodePair : fieldNodes) {
    EACGFieldNode *field = fieldNodePair.second;
    field->UpdateEAStatus(eaStatus);
  }
}

void EACGObjectNode::DumpDotFile(std::ostream &fout, std::map<EACGBaseNode*, bool> &dumped, bool dumpPt,
                                 const IRMap *irMap) {
  if (dumped[this]) {
    return;
  }
  dumped[this] = true;

  std::string name = GetName(nullptr);
  std::string label;
  label = GetName(irMap) + " Object\\n";
  std::string color;
  GetNodeFormatInDot(label, color);
  std::string style;
  if (IsPhantom()) {
    style = "dotted";
  } else {
    style = "bold";
  }
  fout << name << " [shape=box, label=\"" << label << "\", fontcolor=" << color << ", style=" << style << "];\n";
  for (auto fieldPair : fieldNodes) {
    EACGBaseNode *field = fieldPair.second;
    fout << name << "->" << field->GetName(nullptr) << ";" << "\n";
  }
  for (auto fieldPair : fieldNodes) {
    EACGBaseNode *field = fieldPair.second;
    field->DumpDotFile(fout, dumped, dumpPt, irMap);
  }
}

void EACGRefNode::DumpDotFile(std::ostream &fout, std::map<EACGBaseNode*, bool> &dumped, bool dumpPt,
                              const IRMap *irMap) {
  if (dumped[this]) {
    return;
  }
  dumped[this] = true;

  std::string name = GetName(nullptr);
  std::string label;
  label = GetName(irMap) + " Reference\\n";
  if (IsStaticRef()) {
    label += "Static\\n";
  }
  std::string color;
  GetNodeFormatInDot(label, color);
  fout << name << " [shape=ellipse, label=\"" << label << "\", fontcolor=" << color << "];" << "\n";
  if (dumpPt) {
    for (auto obj : pointsTo) {
      fout << name << "->" << obj->GetName(nullptr) << ";" << "\n";
    }
    for (auto obj : pointsTo) {
      obj->DumpDotFile(fout, dumped, dumpPt, irMap);
    }
  } else {
    for (auto outNode : out) {
      std::string edgeStyle;
      if (!outNode->IsObjectNode()) {
        edgeStyle = " [style =\"dotted\"]";
      }
      fout << name << "->" << outNode->GetName(nullptr) << edgeStyle << ";" << "\n";
    }
    for (auto outNode : out) {
      outNode->DumpDotFile(fout, dumped, dumpPt, irMap);
    }
  }
}

bool EACGRefNode::ReplaceByGlobalNode() {
  for (EACGBaseNode *inNode : in) {
    ASSERT(inNode->id > 3, "must be greater than three"); // the least valid idx is 3
    (void)inNode->out.erase(this);
    (void)inNode->out.insert(eaCG->GetGlobalReference());
  }
  in.clear();
  for (EACGBaseNode *outNode : out) {
    (void)outNode->in.erase(this);
  }
  out.clear();
  for (EACGObjectNode *base : pointsTo) {
    base->EraseNodeFromPointsBy(this);
  }
  pointsTo.clear();
  if (meExpr != nullptr) {
    eaCG->expr2Nodes[meExpr]->clear();
    eaCG->expr2Nodes[meExpr]->insert(eaCG->GetGlobalReference());
  }
  ASSERT(eaCG->nodes[id - 1] == this, "must be this");
  eaCG->nodes[id - 1] = nullptr;
  return true;
}

void EACGPointerNode::DumpDotFile(std::ostream &fout, std::map<EACGBaseNode*, bool> &dumped, bool dumpPt,
                                  const IRMap *irMap) {
  if (dumped[this]) {
    return;
  }
  dumped[this] = true;
  std::string name = GetName(nullptr);
  std::string label;
  label = GetName(irMap) + "\\nPointer Indirect Level : " + std::to_string(indirectLevel) + "\\n";
  std::string color;
  GetNodeFormatInDot(label, color);
  fout << name << " [shape=ellipse, label=\"" << label << "\", fontcolor=" << color << "];" << "\n";
  for (EACGBaseNode *outNode : out) {
    fout << name << "->" << outNode->GetName(nullptr) << " [style =\"dotted\", color = \"blue\"];" << "\n";
  }
  for (auto outNode : out) {
    outNode->DumpDotFile(fout, dumped, dumpPt, irMap);
  }
}

void EACGActualNode::DumpDotFile(std::ostream &fout, std::map<EACGBaseNode*, bool> &dumped, bool dumpPt,
                                 const IRMap *irMap) {
  if (dumped[this]) {
    return;
  }
  dumped[this] = true;

  std::string name = GetName(nullptr);
  std::string label;
  if (IsReturn()) {
    label = GetName(irMap) + "\\nRet Idx : " + std::to_string(GetArgIndex()) + "\\n";
  } else {
    label = GetName(irMap) + "\\nArg Idx : " + std::to_string(GetArgIndex()) +
            " Call Site : " + std::to_string(GetCallSite()) + "\\n";
  }
  std::string style;
  if (IsPhantom()) {
    style = "dotted";
  } else {
    style = "bold";
  }
  std::string color;
  GetNodeFormatInDot(label, color);
  fout << name << " [shape=ellipse, label=\"" << label << "\", fontcolor=" << color << ", style=" << style << "];\n";
  if (dumpPt) {
    for (auto obj : pointsTo) {
      fout << name << "->" << obj->GetName(nullptr) << ";\n";
    }
    for (auto obj : pointsTo) {
      obj->DumpDotFile(fout, dumped, dumpPt, irMap);
    }
  } else {
    for (auto outNode : out) {
      std::string edgeStyle;
      if (!outNode->IsObjectNode()) {
        edgeStyle = " [style =\"dotted\"]";
      }
      fout << name << "->" << outNode->GetName(nullptr) << edgeStyle << ";\n";
    }
    for (auto outNode : out) {
      outNode->DumpDotFile(fout, dumped, dumpPt, irMap);
    }
  }
}

bool EACGActualNode::ReplaceByGlobalNode() {
  ASSERT(callSiteInfo == kInvalid, "must be invalid");
  ASSERT(out.size() == 1, "the size of out must be one");
  ASSERT(pointsTo.size() == 1, "the size of pointsTo must be one");
  for (EACGBaseNode *inNode : in) {
    inNode->out.erase(this);
  }
  in.clear();
  return false;
}

void EACGFieldNode::DumpDotFile(std::ostream &fout, std::map<EACGBaseNode*, bool> &dumped, bool dumpPt,
                                const IRMap *irMap) {
  if (dumped[this]) {
    return;
  }
  dumped[this] = true;
  std::string name = GetName(nullptr);
  std::string label;
  label = GetName(irMap) + "\\nFIdx : " + std::to_string(GetFieldID()) + "\\n";
  std::string color;
  GetNodeFormatInDot(label, color);
  std::string style;
  if (IsPhantom()) {
    style = "dotted";
  } else {
    style = "bold";
  }
  fout << name << " [shape=circle, label=\"" << label << "\", fontcolor=" << color << ", style=" << style <<
      ", margin=0];\n";
  if (dumpPt) {
    for (auto obj : pointsTo) {
      fout << name << "->" << obj->GetName(nullptr) << ";\n";
    }
    for (auto obj : pointsTo) {
      obj->DumpDotFile(fout, dumped, dumpPt, irMap);
    }
  } else {
    for (auto outNode : out) {
      std::string edgeStyle;
      if (!outNode->IsObjectNode()) {
        edgeStyle = " [style =\"dotted\"]";
      }
      fout << name << "->" << outNode->GetName(nullptr) << edgeStyle << ";\n";
    }
    for (auto outNode : out) {
      outNode->DumpDotFile(fout, dumped, dumpPt, irMap);
    }
  }
}

bool EACGFieldNode::ReplaceByGlobalNode() {
  for (EACGObjectNode *obj : pointsTo) {
    obj->pointsBy.erase(this);
  }
  pointsTo.clear();
  (void)pointsTo.insert(eaCG->GetGlobalObject());
  for (EACGBaseNode *outNode : out) {
    outNode->in.erase(this);
  }
  out.clear();
  (void)out.insert(eaCG->GetGlobalObject());
  bool canDelete = true;
  std::set<EACGObjectNode*> tmp = belongsTo;
  for (EACGObjectNode *obj : tmp) {
    if (obj->GetEAStatus() != kGlobalEscape) {
      canDelete = false;
    } else {
      belongsTo.erase(obj);
    }
  }
  if (canDelete) {
    ASSERT(eaCG->nodes[id - 1] == this, "must be this");
    eaCG->nodes[id - 1] = nullptr;
    for (EACGBaseNode *inNode : in) {
      ASSERT(!inNode->IsObjectNode(), "must be ObjectNode");
      inNode->out.erase(this);
      (void)inNode->out.insert(eaCG->globalField);
    }
    for (auto exprPair : eaCG->expr2Nodes) {
      size_t eraseSize = exprPair.second->erase(this);
      if (eraseSize != 0 && exprPair.first->GetMeOp() != kMeOpIvar && exprPair.first->GetMeOp() != kMeOpOp) {
        ASSERT(false, "must be kMeOpIvar or kMeOpOp");
      }
      if (exprPair.second->size() == 0) {
        exprPair.second->insert(eaCG->globalField);
      }
    }
    in.clear();
    return true;
  }
  return false;
}

void EAConnectionGraph::DeleteEACG() const {
  for (EACGBaseNode *node : nodes) {
    if (node == nullptr) {
      continue;
    }
    delete node;
    node = nullptr;
  }
}

void EAConnectionGraph::TrimGlobalNode() const {
  for (EACGBaseNode *node : nodes) {
    if (node == nullptr) {
      continue;
    }
    constexpr int leastIdx = 3;
    if (node->id <= leastIdx) {
      continue;
    }
    bool canDelete = false;
    if (node->GetEAStatus() == kGlobalEscape) {
      canDelete = node->ReplaceByGlobalNode();
    }
#ifdef DEBUG
    node->CheckAllConnectionInNodes();
#endif
    if (canDelete) {
      delete node;
      node = nullptr;
    }
  }
}

void EAConnectionGraph::InitGlobalNode() {
  globalObj = CreateObjectNode(nullptr, kNoEscape, true, TyIdx(0));
  globalRef = CreateReferenceNode(nullptr, kNoEscape, true);
  globalRef->AddOutNode(*globalObj);
  globalRef->AddOutNode(*globalRef);
  globalField = CreateFieldNode(nullptr, kNoEscape, -1, globalObj, true);  // -1 expresses global
  globalField->AddOutNode(*globalObj);
  globalField->AddOutNode(*globalRef);
  globalField->AddOutNode(*globalField);
  globalRef->AddOutNode(*globalField);
  globalObj->eaStatus = kGlobalEscape;
  globalField->eaStatus = kGlobalEscape;
  globalRef->eaStatus = kGlobalEscape;
}

EACGObjectNode *EAConnectionGraph::CreateObjectNode(MeExpr *expr, EAStatus initialEas, bool isPh, TyIdx) {
  EACGObjectNode *newObjNode =
      new (std::nothrow) EACGObjectNode(mirModule, alloc, *this, expr, initialEas, nodes.size() + 1, isPh);
  ASSERT_NOT_NULL(newObjNode);
  nodes.push_back(newObjNode);
  if (expr != nullptr) {
    if (expr2Nodes.find(expr) == expr2Nodes.end()) {
      expr2Nodes[expr] = alloc->GetMemPool()->New<MapleSet<EACGBaseNode*>>(alloc->Adapter());
      expr2Nodes[expr]->insert(newObjNode);
    } else {
      ASSERT(false, "must find expr");
    }
  }
  return newObjNode;
}

EACGPointerNode *EAConnectionGraph::CreatePointerNode(MeExpr *expr, EAStatus initialEas, int inderictL) {
  EACGPointerNode *newPointerNode =
      new (std::nothrow) EACGPointerNode(mirModule, alloc, *this, expr, initialEas, nodes.size() + 1, inderictL);
  ASSERT_NOT_NULL(newPointerNode);
  nodes.push_back(newPointerNode);
  if (expr != nullptr) {
    if (expr2Nodes.find(expr) == expr2Nodes.end()) {
      expr2Nodes[expr] = alloc->GetMemPool()->New<MapleSet<EACGBaseNode*>>(alloc->Adapter());
      expr2Nodes[expr]->insert(newPointerNode);
    } else {
      ASSERT(false, "must find expr");
    }
  }
  return newPointerNode;
}

EACGRefNode *EAConnectionGraph::CreateReferenceNode(MeExpr *expr, EAStatus initialEas, bool isStatic) {
  EACGRefNode *newRefNode =
      new (std::nothrow) EACGRefNode(mirModule, alloc, *this, expr, initialEas, nodes.size() + 1, isStatic);
  ASSERT_NOT_NULL(newRefNode);
  nodes.push_back(newRefNode);
  if (expr != nullptr) {
    if (expr2Nodes.find(expr) == expr2Nodes.end()) {
      expr2Nodes[expr] = alloc->GetMemPool()->New<MapleSet<EACGBaseNode*>>(alloc->Adapter());
      expr2Nodes[expr]->insert(newRefNode);
    } else {
      ASSERT(false, "must find expr");
    }
    if (expr->GetMeOp() != kMeOpVar && expr->GetMeOp() != kMeOpAddrof &&
        expr->GetMeOp() != kMeOpReg && expr->GetMeOp() != kMeOpOp) {
      ASSERT(false, "must be kMeOpVar, kMeOpAddrof, kMeOpReg or kMeOpOp");
    }
  }
  return newRefNode;
}

void EAConnectionGraph::TouchCallSite(uint32 callSiteInfo) {
  CHECK_FATAL(callSite2Nodes.find(callSiteInfo) != callSite2Nodes.end(), "find failed");
  if (callSite2Nodes[callSiteInfo] == nullptr) {
    MapleVector<EACGBaseNode*> *tmp = alloc->GetMemPool()->New<MapleVector<EACGBaseNode*>>(alloc->Adapter());
    callSite2Nodes[callSiteInfo] = tmp;
  }
}

EACGActualNode *EAConnectionGraph::CreateActualNode(EAStatus initialEas, bool isReurtn, bool isPh,
                                                    uint8 argIdx, uint32 callSiteInfo) {
  MeExpr *expr = nullptr;
  ASSERT(isPh, "must be ph");
  ASSERT(callSiteInfo != 0, "must not be zero");
  EACGActualNode *newActNode = new (std::nothrow) EACGActualNode(
      mirModule, alloc, *this, expr, initialEas, nodes.size() + 1, isReurtn, isPh, argIdx, callSiteInfo);
  ASSERT_NOT_NULL(newActNode);
  nodes.push_back(newActNode);
  if (expr != nullptr) {
    if (expr2Nodes.find(expr) == expr2Nodes.end()) {
      expr2Nodes[expr] = alloc->GetMemPool()->New<MapleSet<EACGBaseNode*>>(alloc->Adapter());
      expr2Nodes[expr]->insert(newActNode);
    } else {
      ASSERT(false, "must find expr");
    }
  }
  if (callSiteInfo != kInvalid) {
    ASSERT(callSite2Nodes[callSiteInfo] != nullptr, "must touched before");
    callSite2Nodes[callSiteInfo]->push_back(newActNode);
#ifdef DEBUG
    CheckArgNodeOrder(*callSite2Nodes[callSiteInfo]);
#endif
  } else {
    funcArgNodes.push_back(newActNode);
  }
  return newActNode;
}

EACGFieldNode *EAConnectionGraph::CreateFieldNode(MeExpr *expr, EAStatus initialEas, FieldID fId,
                                                  EACGObjectNode *belongTo, bool isPh) {
  EACGFieldNode *newFieldNode = new (std::nothrow) EACGFieldNode(
      mirModule, alloc, *this, expr, initialEas, nodes.size() + 1, fId, belongTo, isPh);
  ASSERT_NOT_NULL(newFieldNode);
  nodes.push_back(newFieldNode);
  if (expr != nullptr) {
    if (expr2Nodes.find(expr) == expr2Nodes.end()) {
      expr2Nodes[expr] = alloc->GetMemPool()->New<MapleSet<EACGBaseNode*>>(alloc->Adapter());
      expr2Nodes[expr]->insert(newFieldNode);
    } else {
      expr2Nodes[expr]->insert(newFieldNode);
    }
    if (expr->GetMeOp() != kMeOpIvar && expr->GetMeOp() != kMeOpOp) {
      ASSERT(false, "must be kMeOpIvar or kMeOpOp");
    }
  }
  return newFieldNode;
}

EACGBaseNode *EAConnectionGraph::GetCGNodeFromExpr(MeExpr *me) {
  if (expr2Nodes.find(me) == expr2Nodes.end()) {
    return nullptr;
  }
  return *(expr2Nodes[me]->begin());
}

void EAConnectionGraph::UpdateExprOfNode(EACGBaseNode &node, MeExpr *me) {
  if (expr2Nodes.find(me) == expr2Nodes.end()) {
    expr2Nodes[me] = alloc->GetMemPool()->New<MapleSet<EACGBaseNode*>>(alloc->Adapter());
    expr2Nodes[me]->insert(&node);
  } else {
    if (node.IsFieldNode()) {
      expr2Nodes[me]->insert(&node);
    } else {
      if (expr2Nodes[me]->find(&node) == expr2Nodes[me]->end()) {
        CHECK_FATAL(false, "must be filed node");
      }
    }
  }
  node.SetMeExpr(*me);
}

void EAConnectionGraph::UpdateExprOfGlobalRef(MeExpr *me) {
  UpdateExprOfNode(*globalRef, me);
}

EACGActualNode *EAConnectionGraph::GetReturnNode() const {
  if (funcArgNodes.size() == 0) {
    return nullptr;
  }
  EACGActualNode *ret = static_cast<EACGActualNode*>(funcArgNodes[funcArgNodes.size() - 1]);
  if (ret->IsReturn()) {
    return ret;
  }
  return nullptr;
}
#ifdef DEBUG
void EAConnectionGraph::CheckArgNodeOrder(MapleVector<EACGBaseNode*> &funcArgV) {
  uint8 preIndex = 0;
  for (size_t i = 0; i < funcArgV.size(); ++i) {
    ASSERT(funcArgV[i]->IsActualNode(), "must be ActualNode");
    EACGActualNode *actNode = static_cast<EACGActualNode*>(funcArgV[i]);
    if (i == funcArgV.size() - 1) {
      if (actNode->IsReturn()) {
        continue;
      } else {
        ASSERT(actNode->GetArgIndex() >= preIndex, "must be greater than preIndex");
      }
    } else {
      ASSERT(!actNode->IsReturn(), "must be return");
      ASSERT(actNode->GetArgIndex() >= preIndex, "must be greater than preIndex");
    }
    preIndex = actNode->GetArgIndex();
  }
}
#endif
bool EAConnectionGraph::ExprCanBeOptimized(MeExpr &expr) {
  if (expr2Nodes.find(&expr) == expr2Nodes.end()) {
    MeExpr *rhs = nullptr;
    if (expr.GetMeOp() == kMeOpVar) {
      ASSERT(static_cast<VarMeExpr*>(&expr)->GetDefBy() == kDefByStmt, "must be kDefByStmt");
      ASSERT(static_cast<VarMeExpr*>(&expr)->GetDefStmt()->GetOp() == OP_dassign, "must be OP_dassign");
      MeStmt *defStmt = static_cast<VarMeExpr*>(&expr)->GetDefStmt();
      DassignMeStmt *dassignStmt = static_cast<DassignMeStmt*>(defStmt);
      rhs = dassignStmt->GetRHS();
    } else if (expr.GetMeOp() == kMeOpReg) {
      ASSERT(static_cast<RegMeExpr*>(&expr)->GetDefBy() == kDefByStmt, "must be kDefByStmt");
      ASSERT(static_cast<RegMeExpr*>(&expr)->GetDefStmt()->GetOp() == OP_regassign, "must be OP_regassign");
      MeStmt *defStmt = static_cast<RegMeExpr*>(&expr)->GetDefStmt();
      AssignMeStmt *regassignStmt = static_cast<AssignMeStmt*>(defStmt);
      rhs = regassignStmt->GetRHS();
    } else {
      CHECK_FATAL(false, "impossible");
    }
    ASSERT(expr2Nodes.find(rhs) != expr2Nodes.end(), "impossible");
    expr = *rhs;
  }
  MapleSet<EACGBaseNode*> &nodesTmp = *expr2Nodes[&expr];

  for (EACGBaseNode *node : nodesTmp) {
    for (EACGObjectNode *obj : node->GetPointsToSet()) {
      if (obj->GetEAStatus() != kNoEscape && obj->GetEAStatus() != kReturnEscape) {
        return false;
      }
    }
  }
  return true;
}

MapleVector<EACGBaseNode*> *EAConnectionGraph::GetCallSiteArgNodeVector(uint32 callSite) {
  CHECK_FATAL(callSite2Nodes.find(callSite) != callSite2Nodes.end(), "find failed");
  ASSERT_NOT_NULL(callSite2Nodes[callSite]);
  return callSite2Nodes[callSite];
}

// if we have scc of connection graph, it will be more efficient.
void EAConnectionGraph::PropogateEAStatus() {
  bool oldStatus = CGHasUpdated();
  do {
    UnSetCGUpdateFlag();
    for (EACGBaseNode *node : nodes) {
      if (node == nullptr) {
        continue;
      }
      if (node->IsObjectNode()) {
        EACGObjectNode *obj = static_cast<EACGObjectNode*>(node);
        for (auto fieldPair : obj->GetFieldNodeMap()) {
          EACGBaseNode *field = fieldPair.second;
          field->UpdateEAStatus(obj->GetEAStatus());
        }
      } else {
        for (EACGBaseNode *pointsToNode : node->GetPointsToSet()) {
          pointsToNode->UpdateEAStatus(node->GetEAStatus());
        }
      }
    }
    ASSERT(!CGHasUpdated(), "must be Updated");
  } while (CGHasUpdated());
  RestoreStatus(oldStatus);
}

const MapleVector<EACGBaseNode*> *EAConnectionGraph::GetFuncArgNodeVector() const {
  return &funcArgNodes;
}

// this func is called from callee context
void EAConnectionGraph::UpdateEACGFromCaller(const MapleVector<EACGBaseNode*> &callerCallSiteArg,
                                             const MapleVector<EACGBaseNode*> &calleeFuncArg) {
  ASSERT(abs(static_cast<int>(callerCallSiteArg.size()) - static_cast<int>(calleeFuncArg.size())) <= 1, "greater than");

  UnSetCGUpdateFlag();
  for (uint32 i = 0; i < callerCallSiteArg.size(); ++i) {
    EACGBaseNode *callerNode = callerCallSiteArg[i];
    ASSERT_NOT_NULL(callerNode);
    ASSERT(callerNode->IsActualNode(), "must be ActualNode");
    if ((i == callerCallSiteArg.size() - 1) && static_cast<EACGActualNode*>(callerNode)->IsReturn()) {
      continue;
    }
    bool hasGlobalEA = false;
    for (EACGObjectNode *obj : callerNode->GetPointsToSet()) {
      if (obj->GetEAStatus() == kGlobalEscape) {
        hasGlobalEA = true;
        break;
      }
    }
    if (hasGlobalEA) {
      EACGBaseNode *calleeNode = (calleeFuncArg)[i];
      for (EACGObjectNode *obj : calleeNode->GetPointsToSet()) {
        obj->UpdateEAStatus(kGlobalEscape);
      }
    }
  }
  if (CGHasUpdated()) {
    PropogateEAStatus();
  }
  TrimGlobalNode();
}

void EAConnectionGraph::DumpDotFile(const IRMap *irMap, bool dumpPt, MapleVector<EACGBaseNode*> *dumpVec) {
  if (dumpVec == nullptr) {
    dumpVec = &nodes;
  }
  std::filebuf fb;
  std::string outFile = GlobalTables::GetStrTable().GetStringFromStrIdx(funcStIdx) + "-connectiongraph.dot";
  fb.open(outFile, std::ios::trunc | std::ios::out);
  CHECK_FATAL(fb.is_open(), "open file failed");
  std::ostream cgDotFile(&fb);
  cgDotFile << "digraph connectiongraph{\n";
  std::map<EACGBaseNode*, bool> dumped;
  for (auto node : nodes) {
    dumped[node] = false;
  }
  for (EACGBaseNode *node : *dumpVec) {
    if (node == nullptr) {
      continue;
    }
    if (dumped[node]) {
      continue;
    }
    node->DumpDotFile(cgDotFile, dumped, dumpPt, irMap);
    dumped[node] = true;
  }
  cgDotFile << "}\n";
  fb.close();
}

void EAConnectionGraph::CountObjEAStatus() const {
  int sum = 0;
  int eaCount[4]; // There are four EAStatus.
  for (size_t i = 0; i < 4; ++i) {
    eaCount[i] = 0;
  }
  for (EACGBaseNode *node : nodes) {
    if (node == nullptr) {
      continue;
    }

    if (node->IsObjectNode()) {
      EACGObjectNode *objNode = static_cast<EACGObjectNode*>(node);
      if (!objNode->IsPhantom()) {
        CHECK_FATAL(objNode->locInfo != nullptr, "Impossible");
        MIRType *type = nullptr;
        const MeExpr *expr = objNode->GetMeExpr();
        CHECK_FATAL(expr != nullptr, "Impossible");
        if (expr->GetOp() == OP_gcmalloc || expr->GetOp() == OP_gcpermalloc) {
          TyIdx tyIdx = static_cast<const GcmallocMeExpr*>(expr)->GetTyIdx();
          type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        } else {
          TyIdx tyIdx = static_cast<const OpMeExpr*>(expr)->GetTyIdx();
          type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        }
        LogInfo::MapleLogger() << "[LOCATION] [" << objNode->locInfo->GetModName() << " " <<
            objNode->locInfo->GetFileId() << " " << objNode->locInfo->GetLineId() << " " <<
            EscapeName(objNode->GetEAStatus()) << " " << expr->GetExprID() << " ";
        type->Dump(0, false);
        LogInfo::MapleLogger() << "]\n";
        ++sum;
        ++eaCount[node->GetEAStatus()];
      }
    }
  }
  LogInfo::MapleLogger() << "[gcmalloc object statistics]  " <<
      GlobalTables::GetStrTable().GetStringFromStrIdx(funcStIdx) << " " <<
      "Gcmallocs: " << sum << " " << "NoEscape: " << eaCount[kNoEscape] << " " <<
      "RetEscape: " << eaCount[kReturnEscape] << " " << "ArgEscape: " << eaCount[kArgumentEscape] << " " <<
      "GlobalEscape: " << eaCount[kGlobalEscape] << "\n";
}

void EAConnectionGraph::RestoreStatus(bool old) {
  if (old) {
    SetCGHasUpdated();
  } else {
    UnSetCGUpdateFlag();
  }
}

// Update caller's ConnectionGraph using callee's summary information.
// If the callee's summary is not found, we just mark all the pointsTo nodes of caller's actual node to GlobalEscape.
// Otherwise, we do these steps:
//
// 1, update caller nodes using callee's summary, new node might be added into caller's CG in this step.
//
// 2, update caller edges using callee's summary, new points-to edge might be added into caller's CG in this step.
bool EAConnectionGraph::MergeCG(MapleVector<EACGBaseNode*> &caller, const MapleVector<EACGBaseNode*> *callee) {
  TrimGlobalNode();
  bool cgChanged = false;
  bool oldStatus = CGHasUpdated();
  UnSetCGUpdateFlag();
  if (callee == nullptr) {
    for (EACGBaseNode *actualInCaller : caller) {
      for (EACGObjectNode *p : actualInCaller->GetPointsToSet()) {
        p->UpdateEAStatus(EAStatus::kGlobalEscape);
      }
    }
    cgChanged = CGHasUpdated();
    if (!cgChanged) {
      RestoreStatus(oldStatus);
    }
    TrimGlobalNode();
    return cgChanged;
  }
  size_t callerSize = caller.size();
  size_t calleeSize = callee->size();
  if (callerSize > calleeSize) {
    ASSERT((callerSize - calleeSize) <= 1, "must be one in EAConnectionGraph::MergeCG()");
  } else {
    ASSERT((calleeSize - callerSize) <= 1, "must be one in EAConnectionGraph::MergeCG()");
  }
  if (callerSize == 0 || calleeSize == 0) {
    cgChanged = CGHasUpdated();
    if (!cgChanged) {
      RestoreStatus(oldStatus);
    }
    return cgChanged;
  }
  if ((callerSize != calleeSize) &&
      (callerSize != calleeSize + 1 || static_cast<EACGActualNode*>(callee->back())->IsReturn()) &&
      (callerSize != calleeSize - 1 || !static_cast<EACGActualNode*>(callee->back())->IsReturn())) {
    ASSERT(false, "Impossible");
  }

  callee2Caller.clear();
  UpdateCallerNodes(caller, *callee);
  UpdateCallerEdges();
  UpdateCallerRetNode(caller, *callee);
  callee2Caller.clear();

  cgChanged = CGHasUpdated();
  if (!cgChanged) {
    RestoreStatus(oldStatus);
  }
  TrimGlobalNode();
  return cgChanged;
}

void EAConnectionGraph::AddMaps2Object(EACGObjectNode *caller, EACGObjectNode *callee) {
  if (callee2Caller.find(callee) == callee2Caller.end()) {
    std::set<EACGObjectNode*> callerSet;
    callee2Caller[callee] = callerSet;
  }
  (void)callee2Caller[callee].insert(caller);
}

void EAConnectionGraph::UpdateCallerRetNode(MapleVector<EACGBaseNode*> &caller,
                                            const MapleVector<EACGBaseNode*> &callee) {
  EACGActualNode *lastInCaller = static_cast<EACGActualNode*>(caller.back());
  EACGActualNode *lastInCallee = static_cast<EACGActualNode*>(callee.back());
  if (!lastInCaller->IsReturn()) {
    return;
  }
  CHECK_FATAL(lastInCaller->GetOutSet().size() == 1, "Impossible");
  for (EACGBaseNode *callerRetNode : lastInCaller->GetOutSet()) {
    for (EACGObjectNode *calleeRetNode : lastInCallee->GetPointsToSet()) {
      for (EACGObjectNode *objInCaller : callee2Caller[calleeRetNode]) {
        auto pointsToSet = callerRetNode->GetPointsToSet();
        if (pointsToSet.find(objInCaller) == pointsToSet.end()) {
          callerRetNode->AddOutNode(*objInCaller);
        }
      }
    }
  }
}

// Update caller node by adding some nodes which are mapped from callee.
void EAConnectionGraph::UpdateCallerNodes(const MapleVector<EACGBaseNode*> &caller,
                                          const MapleVector<EACGBaseNode*> &callee) {
  const size_t callerSize = caller.size();
  const size_t calleeSize = callee.size();
  const size_t actualCount = ((callerSize < calleeSize) ? callerSize : calleeSize);
  bool firstTime = true;

  for (size_t i = 0; i < actualCount; ++i) {
    EACGBaseNode *actualInCaller = caller.at(i);
    EACGBaseNode *actualInCallee = callee.at(i);
    UpdateNodes(*actualInCallee, *actualInCaller, firstTime);
  }
}

// Update caller edges using information from callee.
void EAConnectionGraph::UpdateCallerEdges() {
  std::set<EACGObjectNode*> set;
  for (auto pair : callee2Caller) {
    (void)set.insert(pair.first);
  }
  for (EACGObjectNode *p : set) {
    for (auto tempPair : p->GetFieldNodeMap()) {
      uint32 fieldID = tempPair.first;
      EACGBaseNode *fieldNode = tempPair.second;
      for (EACGObjectNode *q : fieldNode->GetPointsToSet()) {
        UpdateCallerEdgesInternal(p, fieldID, q);
      }
    }
  }
}

// Update caller edges using information of given ObjectNode from callee.
void EAConnectionGraph::UpdateCallerEdgesInternal(EACGObjectNode *node1, uint32 fieldID, EACGObjectNode *node2) {
  CHECK_FATAL(callee2Caller.find(node1) != callee2Caller.end(), "find failed");
  CHECK_FATAL(callee2Caller.find(node2) != callee2Caller.end(), "find failed");
  for (EACGObjectNode *p1 : callee2Caller[node1]) {
    for (EACGObjectNode *q1 : callee2Caller[node2]) {
      EACGFieldNode *fieldNode = p1->GetFieldNodeFromIdx(fieldID);
      if (fieldNode == nullptr) {
        CHECK_NULL_FATAL(node1);
        fieldNode = node1->GetFieldNodeFromIdx(fieldID);
        CHECK_FATAL(fieldNode != nullptr, "fieldNode must not be nullptr because we have handled it before!");
        CHECK_FATAL(fieldNode->IsBelongTo(this), "must be belong to this");
        p1->AddOutNode(*fieldNode);
      }
      fieldNode->AddOutNode(*q1);
    }
  }
}

void EAConnectionGraph::UpdateNodes(const EACGBaseNode &actualInCallee, EACGBaseNode &actualInCaller, bool firstTime) {
  ASSERT(actualInCallee.GetPointsToSet().size() > 0, "actualInCallee->GetPointsToSet().size() must gt 0!");
  for (EACGObjectNode *objInCallee : actualInCallee.GetPointsToSet()) {
    if (actualInCaller.GetPointsToSet().size() == 0) {
      std::set<EACGObjectNode*> &mapsTo = callee2Caller[objInCallee];
      if (mapsTo.size() > 0) {
        for (EACGObjectNode *temp : mapsTo) {
          actualInCaller.AddOutNode(*temp);
        }
      } else if (objInCallee->IsBelongTo(this)) {
        ASSERT(false, "must be belong to this");
      } else {
        EACGObjectNode *phantom = CreateObjectNode(nullptr, actualInCaller.GetEAStatus(), true, TyIdx(0));
        actualInCaller.AddOutNode(*phantom);
        AddMaps2Object(phantom, objInCallee);
        UpdateCallerWithCallee(*phantom, *objInCallee, firstTime);
      }
    } else {
      for (EACGObjectNode *objInCaller : actualInCaller.GetPointsToSet()) {
        std::set<EACGObjectNode*> &mapsTo = callee2Caller[objInCallee];
        if (mapsTo.find(objInCaller) == mapsTo.end()) {
          AddMaps2Object(objInCaller, objInCallee);
          UpdateCallerWithCallee(*objInCaller, *objInCallee, firstTime);
        }
      }
    }
  }
}

// The escape state of the nodes in MapsTo(which is the object node in caller) is marked
// GlobalEscape if the escape state of object node in callee is GlobalEscape.
// Otherwise, the escape state of the caller nodes is not affected.
void EAConnectionGraph::UpdateCallerWithCallee(EACGObjectNode &objInCaller, const EACGObjectNode &objInCallee,
                                               bool firstTime) {
  if (objInCallee.GetEAStatus() == EAStatus::kGlobalEscape) {
    objInCaller.UpdateEAStatus(EAStatus::kGlobalEscape);
  }

  // At this moment, a node in caller is mapped to the corresponding node in callee,
  // we need make sure that all the field nodes also exist in caller. If not,
  // we create both the field node and the phantom object node it should point to for the caller.
  for (auto tempPair : objInCallee.GetFieldNodeMap()) {
    EACGFieldNode *fieldInCaller = objInCaller.GetFieldNodeFromIdx(tempPair.first);
    EACGFieldNode *fieldInCallee = tempPair.second;
    if (fieldInCaller == nullptr && fieldInCallee->IsBelongTo(this)) {
      objInCaller.AddOutNode(*fieldInCallee);
    }
    fieldInCaller = GetOrCreateFieldNodeFromIdx(objInCaller, tempPair.first);
    UpdateNodes(*fieldInCallee, *fieldInCaller, firstTime);
  }
}

EACGFieldNode *EAConnectionGraph::GetOrCreateFieldNodeFromIdx(EACGObjectNode &obj, uint32 fieldID) {
  EACGFieldNode *ret = obj.GetFieldNodeFromIdx(fieldID);
  if (ret == nullptr) {
    // this node is always phantom
    ret = CreateFieldNode(nullptr, obj.GetEAStatus(), fieldID, &obj, true);
  }
  return ret;
}
}  // namespace maple
