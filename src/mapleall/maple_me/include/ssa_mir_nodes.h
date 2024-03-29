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
#ifndef MAPLE_ME_INCLUDE_SSA_MIR_NODES_H
#define MAPLE_ME_INCLUDE_SSA_MIR_NODES_H
#include <iostream>
#include "bb.h"
#include "ver_symbol.h"

// This file define data structures to store SSA information in the IR instructions
namespace maple {
class MayDefNode {
 public:
  MayDefNode(VersionSt *sym, StmtNode *st) : opnd(sym), result(sym), stmt(st) {}

  ~MayDefNode() = default;

  VersionSt *GetOpnd() {
    return opnd;
  }
  const VersionSt *GetOpnd() const {
    return opnd;
  }

  VersionSt *GetResult() {
    return result;
  }
  const VersionSt *GetResult() const {
    return result;
  }

  StmtNode *GetStmt() {
    return stmt;
  }
  const StmtNode *GetStmt() const {
    return stmt;
  }

  void SetStmt(StmtNode *s) {
    stmt = s;
  }

  void SetOpnd(VersionSt *sym) {
    opnd = sym;
  }

  void SetResult(VersionSt *sym) {
    result = sym;
  }

  void Dump() const {
    result->Dump();
    LogInfo::MapleLogger() << " = MAYD(";
    opnd->Dump();
    LogInfo::MapleLogger() << ")\n";
  }

  bool operator==(const MayDefNode &other) const {
    return opnd == other.opnd && result == other.result && stmt == other.stmt;
  }

  VersionSt *base = nullptr; // only provided if indirectLev is 1 and attached to iassign
 private:
  VersionSt *opnd;
  VersionSt *result;
  StmtNode *stmt;
};

class MayUseNode {
 public:
  explicit MayUseNode(VersionSt *sym) : opnd(sym) {}

  ~MayUseNode() = default;

  VersionSt *GetOpnd() const {
    return opnd;
  }

  void SetOpnd(VersionSt *sym) {
    opnd = sym;
  }

  void Dump() const {
    LogInfo::MapleLogger() << " MAYU(";
    opnd->Dump();
    LogInfo::MapleLogger() << ")\n";
  }

  bool operator==(const MayUseNode &other) const {
    return opnd == other.opnd;
  }

 private:
  VersionSt *opnd;
};

// this is only used in the callassigned type of call statements
class MustDefNode {
 public:
  MustDefNode() = default;

  MustDefNode(VersionSt *sym, StmtNode *st) : result(sym), stmt(st) {}

  ~MustDefNode() = default;

  VersionSt *GetResult() {
    return result;
  }
  const VersionSt *GetResult() const {
    return result;
  }

  void SetResult(VersionSt *sym) {
    result = sym;
  }

  StmtNode *GetStmt() {
    return stmt;
  }
  const StmtNode *GetStmt() const {
    return stmt;
  }

  void Dump() const {
    result->Dump();
    LogInfo::MapleLogger() << " = MUSTDEF\n";
  }

 private:
  VersionSt *result = nullptr;
  StmtNode *stmt = nullptr;
};

using TypeOfMayUseList = MapleMap<OStIdx, MayUseNode>;
using TypeOfMayDefList = MapleMap<OStIdx, MayDefNode>;
using TypeOfMustDefList = MapleVector<MustDefNode>;
class AccessSSANodes {
 public:
  AccessSSANodes() = default;
  virtual ~AccessSSANodes() = default;

  virtual const TypeOfMayDefList &GetMayDefNodes() const {
    CHECK_FATAL(false, "No mayDefNodes");
  }

  virtual TypeOfMayDefList &GetMayDefNodes() {
    CHECK_FATAL(false, "No mayDefNodes");
  }

  virtual const TypeOfMayUseList &GetMayUseNodes() const {
    CHECK_FATAL(false, "No mayUseNodes");
  }

  virtual TypeOfMayUseList &GetMayUseNodes() {
    CHECK_FATAL(false, "No mayUseNodes");
  }

  virtual const TypeOfMustDefList &GetMustDefNodes() const {
    CHECK_FATAL(false, "No mustDefNodes");
  }
  virtual TypeOfMustDefList &GetMustDefNodes() {
    CHECK_FATAL(false, "No mustDefNodes");
  }

  virtual const VersionSt *GetSSAVar() const {
    CHECK_FATAL(false, "No ssaVar");
  }

  virtual VersionSt *GetSSAVar() {
    CHECK_FATAL(false, "No ssaVar");
  }

  virtual void SetSSAVar(VersionSt&) {
    CHECK_FATAL(false, "No ssaVar");
  }

  virtual void DumpMayDefNodes(const MIRModule&) const {
    for (const auto &mayDefNode : GetMayDefNodes()) {
      mayDefNode.second.Dump();
    }
  }

  virtual void DumpMayUseNodes(const MIRModule&) const {
    for (const auto &mapItem : GetMayUseNodes()) {
      mapItem.second.Dump();
    }
  }

  virtual void DumpMustDefNodes(const MIRModule&) const {
    for (const auto &mustDefNode : GetMustDefNodes()) {
      mustDefNode.Dump();
    }
  }

  inline void InsertMayDefNode(MayDefNode mayDefNode) {
    auto &mayDefNodes = GetMayDefNodes();
    OStIdx ostIdx = mayDefNode.GetOpnd()->GetOrigIdx();
    (void)mayDefNodes.insert({ ostIdx, mayDefNode });
  }

  inline void InsertMayUseNode(MayUseNode mayUseNode) {
    auto &mayUseNodes = GetMayUseNodes();
    OStIdx ostIdx = mayUseNode.GetOpnd()->GetOrigIdx();
    (void)mayUseNodes.insert({ ostIdx, mayUseNode });
  }

  virtual void InsertMustDefNode(VersionSt *sym, StmtNode *s) {
    GetMustDefNodes().emplace_back(MustDefNode(sym, s));
  }
};

class MayDefPart : public AccessSSANodes {
 public:
  explicit MayDefPart(MapleAllocator *alloc) : mayDefNodes(alloc->Adapter()) {}

  ~MayDefPart() override = default;

  const TypeOfMayDefList &GetMayDefNodes() const override {
    return mayDefNodes;
  }

  TypeOfMayDefList &GetMayDefNodes() override {
    return mayDefNodes;
  }

 private:
  TypeOfMayDefList mayDefNodes;
};

class MayUsePart : public AccessSSANodes {
 public:
  explicit MayUsePart(MapleAllocator *alloc) : mayUseNodes(alloc->Adapter()) {}

  ~MayUsePart() override = default;

  const TypeOfMayUseList &GetMayUseNodes() const override {
    return mayUseNodes;
  }

  TypeOfMayUseList &GetMayUseNodes() override {
    return mayUseNodes;
  }

 private:
  TypeOfMayUseList mayUseNodes;
};

class MustDefPart : public AccessSSANodes {
 public:
  explicit MustDefPart(MapleAllocator *alloc) : mustDefNodes(alloc->Adapter()) {}

  ~MustDefPart() override = default;

  const TypeOfMustDefList &GetMustDefNodes() const override {
    return mustDefNodes;
  }

  TypeOfMustDefList &GetMustDefNodes() override {
    return mustDefNodes;
  }

 private:
  TypeOfMustDefList mustDefNodes;
};

class MayDefPartWithVersionSt : public AccessSSANodes {
 public:
  explicit MayDefPartWithVersionSt(MapleAllocator *alloc) : mayDefNodes(alloc->Adapter()) {}

  ~MayDefPartWithVersionSt() override = default;

  const TypeOfMayDefList &GetMayDefNodes() const override {
    return mayDefNodes;
  }

  TypeOfMayDefList &GetMayDefNodes() override {
    return mayDefNodes;
  }

  const VersionSt *GetSSAVar() const override {
    return ssaVar;
  }

  VersionSt *GetSSAVar() override {
    return ssaVar;
  }

  void SetSSAVar(VersionSt &ssaVarPara) override {
    ssaVar = &ssaVarPara;
  }

 private:
  VersionSt *ssaVar = nullptr;
  TypeOfMayDefList mayDefNodes;
};

class VersionStPart : public AccessSSANodes {
 public:
  VersionStPart() = default;
  ~VersionStPart() override = default;

  const VersionSt *GetSSAVar() const override {
    return ssaVar;
  }

  VersionSt *GetSSAVar() override {
    return ssaVar;
  }

  void SetSSAVar(VersionSt &ssaVarPara) override {
    ssaVar = &ssaVarPara;
  }

 private:
  VersionSt *ssaVar = nullptr;
};

class MayDefMayUsePart : public AccessSSANodes {
 public:
  explicit MayDefMayUsePart(MapleAllocator *alloc)
      : mayDefNodes(alloc->Adapter()), mayUseNodes(alloc->Adapter()) {}

  ~MayDefMayUsePart() override = default;

  const TypeOfMayDefList &GetMayDefNodes() const override {
    return mayDefNodes;
  }

  TypeOfMayDefList &GetMayDefNodes() override {
    return mayDefNodes;
  }

  const TypeOfMayUseList &GetMayUseNodes() const override {
    return mayUseNodes;
  }

  TypeOfMayUseList &GetMayUseNodes() override {
    return mayUseNodes;
  }

 private:
  TypeOfMayDefList mayDefNodes;
  TypeOfMayUseList mayUseNodes;
};

class MayDefMayUseMustDefPart : public AccessSSANodes {
 public:
  explicit MayDefMayUseMustDefPart(MapleAllocator *alloc)
      : mayDefNodes(alloc->Adapter()),
        mayUseNodes(alloc->Adapter()),
        mustDefNodes(alloc->Adapter()) {}

  ~MayDefMayUseMustDefPart() override = default;

  const TypeOfMayDefList &GetMayDefNodes() const override {
    return mayDefNodes;
  }

  TypeOfMayDefList &GetMayDefNodes() override {
    return mayDefNodes;
  }

  const TypeOfMayUseList &GetMayUseNodes() const override {
    return mayUseNodes;
  }

  TypeOfMayUseList &GetMayUseNodes() override {
    return mayUseNodes;
  }

  const TypeOfMustDefList &GetMustDefNodes() const override {
    return mustDefNodes;
  }
  TypeOfMustDefList &GetMustDefNodes() override {
    return mustDefNodes;
  }

 private:
  TypeOfMayDefList mayDefNodes;
  TypeOfMayUseList mayUseNodes;
  TypeOfMustDefList mustDefNodes;
};

// statement nodes are covered by StmtsSSAPart
class StmtsSSAPart {
 public:
  // Key of ssaPart is stmtID
  // Each element of ssaPart, depending on the stmt, can be pointer to one of:
  // (1) MayDefPart
  // (2) MayUsePart
  // (3) MayDefMayUsePart
  // (4) MayDefPartWithVersionSt
  // (5) MayDefMayUseMustDefPart
  // (6) VersionSt
  explicit StmtsSSAPart(MemPool *memPool)
      : ssaPartMp(memPool), ssaPartAlloc(memPool), ssaPart(ssaPartAlloc.Adapter()) {}

  ~StmtsSSAPart() = default;

  AccessSSANodes *SSAPartOf(const StmtNode &stmt) {
    return ssaPart[stmt.GetStmtID()];
  }
  const AccessSSANodes *SSAPartOf(const StmtNode &stmt) const {
    return ssaPart.at(stmt.GetStmtID());
  }

  bool HasMayDefPart(const StmtNode &stmt) {
    switch (stmt.GetOpCode()) {
      case OP_maydassign:
      case OP_dassign:
      case OP_syncenter:
      case OP_syncexit:
      case OP_iassign:
        return !GetMayDefNodesOf(stmt).empty();
      default:
        return kOpcodeInfo.IsCall(stmt.GetOpCode()) && !GetMayDefNodesOf(stmt).empty();
    }
  }

  bool HasMayDef(const StmtNode &stmt) {
    return kOpcodeInfo.HasSSADef(stmt.GetOpCode()) && !GetMayDefNodesOf(stmt).empty();
  }

  TypeOfMayDefList &GetMayDefNodesOf(const StmtNode &stmt) {
    return ssaPart[stmt.GetStmtID()]->GetMayDefNodes();
  }

  TypeOfMayUseList &GetMayUseNodesOf(const StmtNode &stmt) {
    return ssaPart[stmt.GetStmtID()]->GetMayUseNodes();
  }

  MapleVector<MustDefNode> &GetMustDefNodesOf(const StmtNode &stmt) {
    return ssaPart.at(stmt.GetStmtID())->GetMustDefNodes();
  }
  const MapleVector<MustDefNode> &GetMustDefNodesOf(const StmtNode &stmt) const {
    return ssaPart.at(stmt.GetStmtID())->GetMustDefNodes();
  }

  VersionSt *GetAssignedVarOf(const StmtNode &stmt) {
    return ssaPart.at(stmt.GetStmtID())->GetSSAVar();
  }
  const VersionSt *GetAssignedVarOf(const StmtNode &stmt) const {
    return ssaPart.at(stmt.GetStmtID())->GetSSAVar();
  }

  template <class T>
  void SetSSAPartOf(const StmtNode &s, T *p) {
    ssaPart[s.GetStmtID()] = static_cast<AccessSSANodes *>(p);
  }

  void SetSSAPartOf(const StmtNode &s, VersionSt *vst) {
    auto *vstSSAPart = GetSSAPartMp()->New<VersionStPart>();
    vstSSAPart->SetSSAVar(*vst);
    ssaPart[s.GetStmtID()] = vstSSAPart;
  }

  MemPool *GetSSAPartMp() {
    return ssaPartMp;
  }

  MapleAllocator &GetSSAPartAlloc() {
    return ssaPartAlloc;
  }

 private:
  MemPool *ssaPartMp;
  MapleAllocator ssaPartAlloc;
  MapleMap<uint32, AccessSSANodes*> ssaPart;  // key is stmtID
};

class SSANode : public BaseNode {
 public:
  SSANode(Opcode op, PrimType typ, uint8 numOpr) : BaseNode(op, typ, numOpr) {}

  ~SSANode() override = default;

  VersionSt *GetSSAVar() {
    return ssaVar;
  }
  const VersionSt *GetSSAVar() const {
    return ssaVar;
  }

  void SetSSAVar(VersionSt &ssaVarPara) {
    ssaVar = &ssaVarPara;
  }

  bool IsSSANode() const override {
    return true;
  }

  // MIRSymbol query
  const MIRSymbol &GetMIRSymbol() const {
    return *(GetSSAVar()->GetOst()->GetMIRSymbol());
  }

  virtual BaseNode *GetNoSSANode()= 0;

 protected:
  virtual void SetNoSSANode(BaseNode *node) = 0;
  VersionSt *ssaVar = nullptr;
};

// The following expression nodes need extra fields to represent SSA
class AddrofSSANode : public SSANode {
 public:
  explicit AddrofSSANode(AddrofNode &addrNode)
      : SSANode(addrNode.GetOpCode(), addrNode.GetPrimType(), addrNode.NumOpnds()), addrofNode(&addrNode) {}

  ~AddrofSSANode() override = default;

  void Dump(int32 indent) const override {
    addrofNode->Dump(indent);
    if (GetSSAVar() != nullptr) {
      GetSSAVar()->Dump(true);
    }
  }

  StIdx GetStIdx() const {
    return addrofNode->GetStIdx();
  }

  FieldID GetFieldID() const {
    return addrofNode->GetFieldID();
  }

  BaseNode *GetNoSSANode() override {
    return addrofNode;
  }

  AddrofSSANode *CloneTree(MapleAllocator &allocator) const override {
    auto *node = allocator.New<AddrofSSANode>(*this);
    BaseNode *newAddrof = node->GetNoSSANode()->CloneTree(allocator);
    node->SetNoSSANode(newAddrof);
    return node;
  }

 protected:
  void SetNoSSANode(BaseNode *node) override {
    ASSERT(node->GetOpCode() == OP_addrof || node->GetOpCode() == OP_dread, "must be!");
    addrofNode = static_cast<AddrofNode *>(node);
  }
  AddrofNode *addrofNode;
};

class IreadSSANode : public SSANode {
 public:
  explicit IreadSSANode(IreadNode &iread)
      : SSANode(iread.GetOpCode(), iread.GetPrimType(), iread.NumOpnds()), ireadNode(&iread) {}

  ~IreadSSANode() override = default;

  void Dump(int32 indent) const override {
    if (GetSSAVar() != nullptr) {
      GetSSAVar()->Dump(true);
    }
    ireadNode->Dump(indent);
  }

  FieldID GetFieldID() const {
    return ireadNode->GetFieldID();
  }

  TyIdx GetTyIdx() const {
    return ireadNode->GetTyIdx();
  }

  BaseNode *Opnd(size_t idx) const override {
    return ireadNode->Opnd(idx);
  }

  void SetOpnd(BaseNode *node, size_t i = 0) override {
    ireadNode->SetOpnd(node, i);
  }

  BaseNode *GetNoSSANode() override {
    return ireadNode;
  }

  IreadSSANode *CloneTree(MapleAllocator &allocator) const override {
    auto *node = allocator.New<IreadSSANode>(*this);
    BaseNode *newIread = node->GetNoSSANode()->CloneTree(allocator);
    node->SetNoSSANode(newIread);
    return node;
  }

 protected:
  void SetNoSSANode(BaseNode *node) override {
    ASSERT(node->GetOpCode() == OP_iread, "must be!");
    ireadNode = static_cast<IreadNode *>(node);
  }
  IreadNode *ireadNode;
};

class RegreadSSANode : public SSANode {
 public:
  explicit RegreadSSANode(RegreadNode &regRead)
      : SSANode(regRead.GetOpCode(), regRead.GetPrimType(), regRead.GetNumOpnds()), regreadNode(&regRead) {}

  ~RegreadSSANode() override = default;

  void Dump(int32 indent) const override {
    regreadNode->Dump(indent);
    if (GetSSAVar() != nullptr) {
      GetSSAVar()->Dump(true);
    }
  }

  PregIdx GetRegIdx() const {
    return regreadNode->GetRegIdx();
  }

  BaseNode *GetNoSSANode() override {
    return regreadNode;
  }

  RegreadSSANode *CloneTree(MapleAllocator &allocator) const override {
    auto *node = allocator.New<RegreadSSANode>(*this);
    BaseNode *newRegread = node->GetNoSSANode()->CloneTree(allocator);
    node->SetNoSSANode(newRegread);
    return node;
  }

 protected:
  void SetNoSSANode(BaseNode *node) override {
    ASSERT(node->GetOpCode() == OP_regread, "must be!");
    regreadNode = static_cast<RegreadNode *>(node);
  }
  RegreadNode *regreadNode;
};

void GenericSSAPrint(const MIRModule &mod, const StmtNode &stmtNode, int32 indent, StmtsSSAPart &stmtsSSAPart);
TypeOfMayDefList *SSAGenericGetMayDefsFromVersionSt(const VersionSt &sym, StmtsSSAPart &stmtsSSAPart);
bool HasMayUseOpnd(const BaseNode &baseNode, SSATab &func);

inline bool HasMallocOpnd(const BaseNode *x) {
  return x->op == OP_malloc || x->op == OP_gcmalloc || x->op == OP_gcmallocjarray || x->op == OP_alloca ||
         x->op == OP_stackmalloc || x->op == OP_stackmallocjarray;
}

bool IsSameContent(const BaseNode *exprA, const BaseNode *exprB, bool isZeroVstEqual = true);
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SSA_MIR_NODES_H
