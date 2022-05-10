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
#include "me_slp.h"
#include <optional>
#include "me_cfg.h"
#include "me_irmap.h"
#include "me_dominance.h"

#define SLP_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP] "; X; } } while (false)

#define SLP_OK_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP OK] "; X; } } while (false)

#define SLP_GATHER_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP GATHER] "; X; } } while (false)

#define SLP_FAILURE_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP FAILURE] "; X; } } while (false)

namespace {
constexpr maple::int32 kHugeCost = 10000;  // Used for impossible vectorized node
}  // anonymous namespace

namespace maple {
// A wrapper class of meExpr with its defStmt, this can avoid repeated searches for use-def chains
class ExprWithDef {
 public:
  ExprWithDef(MeExpr *meExpr, BB *bb = nullptr) : expr(meExpr) {
    if (expr->GetOp() == OP_regread) {
      FindDef(static_cast<ScalarMeExpr*>(expr), bb);
    }
  }

  MeExpr *GetExpr() {
    return expr;
  }

  MeStmt *GetDefStmt() {
    return defStmt;
  }

  MeExpr *GetRealExpr() {
    if (defStmt != nullptr) {
      return defStmt->GetRHS();
    }
    return expr;
  }

 private:
  void FindDef(ScalarMeExpr *scalarExpr, BB *bb) {
    if (scalarExpr->GetDefBy() != kDefByStmt) {
      return;
    }
    auto *stmt = scalarExpr->GetDefStmt();
    if (bb != nullptr && stmt->GetBB() != bb) {
      return;
    }
    defStmt = stmt;
  }

  MeExpr *expr = nullptr;
  MeStmt *defStmt = nullptr;
};

// ----------------- //
//  Memory Location  //
// ----------------- //
MeExpr *MemLoc::Emit(IRMap &irMap) const {
  return rawExpr;
}

// Dump example:
// [(mx4 mx3 -mx5), 8] : int32
void MemLoc::DumpWithoutEndl() const {
  LogInfo::MapleLogger() << "[";
  base->Dump();
  LogInfo::MapleLogger() << ", " << offset << "] : ";
  type->Dump(0);
}

void MemLoc::Dump() const {
  DumpWithoutEndl();
  LogInfo::MapleLogger() << std::endl;
}

static bool IsPointerType(PrimType type) {
  return type == PTY_ptr || type == PTY_a64;
}

// Only support add, sub, constval, iaddrof, pointer cvt for now, we can support more op if more scenes are found
void MemoryHelper::ExtractAddendOffset(const MeExpr &expr, bool isNeg, MemLoc &memLoc) {
  Opcode op = expr.GetOp();
  switch (op) {
    case OP_add: {
      ExtractAddendOffset(*expr.GetOpnd(0), isNeg, memLoc);
      ExtractAddendOffset(*expr.GetOpnd(1), isNeg, memLoc);
      break;
    }
    case OP_sub: {
      ExtractAddendOffset(*expr.GetOpnd(0), isNeg, memLoc);
      ExtractAddendOffset(*expr.GetOpnd(1), !isNeg, memLoc);
      break;
    }
    case OP_iaddrof: {
      FieldID fieldId = static_cast<const OpMeExpr&>(expr).GetFieldID();
      if (fieldId != 0) {
        auto *type = static_cast<const OpMeExpr&>(expr).GetType();
        CHECK_FATAL(type->GetKind() == kTypePointer, "must be");
        auto *pointedType = static_cast<MIRPtrType*>(type)->GetPointedType();
        CHECK_FATAL(pointedType->GetKind() == kTypeStruct || pointedType->GetKind() == kTypeUnion, "must be");
        auto bitOffset = static_cast<MIRStructType*>(pointedType)->GetBitOffsetFromBaseAddr(fieldId);
        memLoc.offset += (bitOffset / 8);
      }
      ExtractAddendOffset(*expr.GetOpnd(0), isNeg, memLoc);
      break;
    }
    case OP_constval: {
      auto val = static_cast<const ConstMeExpr&>(expr).GetIntValue();
      memLoc.offset += (isNeg ? -val : val);
      break;
    }
    case OP_cvt:
    case OP_retype: {
      const auto &opExpr = static_cast<const OpMeExpr&>(expr);
      PrimType fromType = opExpr.GetOpndType();
      PrimType toType = opExpr.GetPrimType();
      if (IsPointerType(fromType) && IsPointerType(toType)) {
        ExtractAddendOffset(*opExpr.GetOpnd(0), isNeg, memLoc);
        break;
      }
    }
    [[clang::fallthrough]];
    default: {
      if (memLoc.base == nullptr) {
        memLoc.base = alloc.GetMemPool()->New<MemBasePtr>();
      }
      memLoc.base->PushAddendExpr(expr, isNeg);
      break;
    }
  }
}

void MemoryHelper::UniqueMemLocBase(MemLoc &memLoc) {
  // must be sorted first
  memLoc.base->Sort();
  auto key = memLoc.base->GetSum();
  auto it = basePtrSet.find(key);
  if (it != basePtrSet.end()) {
    auto &vec = it->second;
    bool found = false;
    for (auto &base : vec) {
      if (*base == *memLoc.base) {
        // found it, update base
        memLoc.base = base;
        found = true;
        break;
      }
    }
    if (!found) {
      // found vec but no target element, just push it
      vec.push_back(memLoc.base);
    }
  } else {
    // Create vec and push new element
    basePtrSet[key].push_back(memLoc.base);
  }
}

MemLoc *MemoryHelper::GetMemLoc(IvarMeExpr &ivar) {
  // find memLoc from cache first
  auto it = cache.find(&ivar);
  if (it != cache.end()) {
    return it->second;
  }

  auto *memLocPtr = alloc.GetMemPool()->New<MemLoc>();
  MemLoc &memLoc = *memLocPtr;
  memLoc.rawExpr = &ivar;
  auto *ivarBase = ivar.GetBase();
  MeExpr *realBase = ivarBase;
  if (ivarBase->GetOp() == OP_regread) {
    ExprWithDef exprWithDef(ivarBase);
    realBase = exprWithDef.GetRealExpr();
  }
  ExtractAddendOffset(*realBase, false, memLoc);
  // bugfix for constval ivar.base
  if (memLoc.base == nullptr) {
    memLoc.base = alloc.GetMemPool()->New<MemBasePtr>();
    memLoc.base->PushAddendExpr(*ivar.GetBase(), false);
  }
  // unique memLoc base
  UniqueMemLocBase(memLoc);

  // Consider filedId and offset of ivar to get extraOffset
  int32 extraOffset = 0;
  if (ivar.GetFieldID() != 0) {
    auto *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetTyIdx());
    ASSERT(type->GetKind() == kTypePointer, "must be");
    auto *pointedType = static_cast<MIRPtrType*>(type)->GetPointedType();
    // pointedType is either kTypeStruct or kTypeUnion
    ASSERT(pointedType->GetKind() == kTypeStruct || pointedType->GetKind() == kTypeUnion, "must be");
    auto *structType = static_cast<MIRStructType*>(pointedType);
    auto bitOffset = structType->GetBitOffsetFromBaseAddr(ivar.GetFieldID());
    extraOffset += (bitOffset / 8);
  }
  extraOffset += ivar.GetOffset();
  memLoc.extraOffset = extraOffset;
  memLoc.offset += extraOffset;
  memLoc.type = ivar.GetType();
  // update cache
  cache.emplace(&ivar, &memLoc);
  return &memLoc;
}

bool MemoryHelper::HaveSameBase(const MemLoc &mem1, const MemLoc &mem2) {
  CHECK_NULL_FATAL(mem1.base);
  CHECK_NULL_FATAL(mem2.base);
  return mem1.base == mem2.base;
}

std::optional<int32> MemoryHelper::Distance(const MemLoc &from, const MemLoc &to) {
  if (!HaveSameBase(from, to)) {
    return {};
  }
  return to.offset - from.offset;
}

bool MemoryHelper::IsConsecutive(const MemLoc &mem1, const MemLoc &mem2, bool mustSameType) {
  if (mustSameType && mem1.type != mem2.type) {
    return false;
  }
  auto dis = Distance(mem1, mem2);
  if (!dis) {
    return false;
  }
  return dis.value() == mem1.type->GetSize();
}

bool MemoryHelper::MustHaveNoOverlap(const MemLoc &mem1, const MemLoc &mem2) {
  // If the bases are not same, we can't tell if they overlap
  if (!HaveSameBase(mem1, mem2)) {
    return false;
  }
  int32 offset1 = mem1.offset;
  int32 offset2 = mem2.offset;
  int32 size1 = mem1.type->GetSize();
  int32 size2 = mem2.type->GetSize();
  // overlapping case one:
  // mem1: |------|
  // mem2:     |------|
  if (offset1 <= offset2 && (offset2 - offset1) < size1) {
    return false;
  }
  // overlapping case two:
  // mem1:     |------|
  // mem2: |------|
  if (offset2 < offset1 && (offset1 - offset2) < size2) {
    return false;
  }
  return true;
}

// Order-insensitive for now, maybe need a new parameter named `isOrderSensitive`
bool MemoryHelper::IsAllIvarConsecutive(const std::vector<MeExpr*> &ivarVec, bool mustSameType) {
  CHECK_FATAL(ivarVec.size() >= 2, "must be");
  std::vector<MemLoc*> memLocVec;
  auto *firstMem = GetMemLoc(static_cast<IvarMeExpr&>(*ivarVec[0]));
  memLocVec.push_back(firstMem);
  for (size_t i = 1; i < ivarVec.size(); ++i) {
    auto *mem = GetMemLoc(static_cast<IvarMeExpr&>(*ivarVec[i]));
    if (mustSameType && firstMem->type != mem->type) {
      return false;
    }
    if (!HaveSameBase(*firstMem, *mem)) {
      return false;
    }
    memLocVec.push_back(mem);
  }
  std::sort(memLocVec.begin(), memLocVec.end(), [](const MemLoc *mem1, const MemLoc *mem2) {
    return mem1->offset < mem2->offset;
  });
  for (size_t i = 0; i < ivarVec.size() - 1; ++i) {
    if (!IsConsecutive(*memLocVec[i], *memLocVec[i + 1], mustSameType)) {
      return false;
    }
  }
  return true;
}

// ------------------- //
//  Assign Stmt Split  //
// ------------------- //
// Create a new regassign with specified rhs expr and insert it before anchor stmt
// return the new created regassign
AssignMeStmt *CreateAndInsertRegassign(MeExpr &rhsExpr, MeStmt &anchorStmt, MeFunction &func) {
  auto *bb = anchorStmt.GetBB();
  auto *regLhs = func.GetIRMap()->CreateRegMeExpr(rhsExpr.GetPrimType());
  auto *newAssign = func.GetIRMap()->CreateAssignMeStmt(*regLhs, rhsExpr, *bb);
  newAssign->SetSrcPos(anchorStmt.GetSrcPosition());
  bb->InsertMeStmtBefore(&anchorStmt, newAssign);
  return newAssign;
}

// Only split iassign, regassign
// Support custom exit conditions if needed
void TrySplitMeStmt(MeStmt &stmt, MeFunction &func, bool &changed) {
  Opcode op = stmt.GetOp();
  if (op == OP_iassign) {
    // DO NOT split iassign with agg type
    auto lhsType = static_cast<IassignMeStmt&>(stmt).GetLHSVal()->GetPrimType();
    if (lhsType == PTY_agg || stmt.GetRHS()->GetPrimType() == PTY_agg) {
      return;
    }
    auto *rhs = stmt.GetRHS();
    if (rhs->IsLeaf() && rhs->GetOp() != OP_addroffunc) {
      return;
    }
    auto *newAssign = CreateAndInsertRegassign(*rhs, stmt, func);
    stmt.SetOpnd(1, newAssign->GetLHS());
    changed = true;
    TrySplitMeStmt(*newAssign, func, changed);
  }

  if (op == OP_regassign) {
    auto *rhs = stmt.GetRHS();
    if (rhs->IsLeaf() || rhs->GetMeOp() == kMeOpIvar) {
      return;
    }
    auto *anchorStmt = &stmt;
    auto numOpnds = static_cast<size_t>(rhs->GetNumOpnds());
    for (size_t i = 0; i < numOpnds; ++i) {
      auto *opnd = rhs->GetOpnd(i);
      if (opnd->IsLeaf()) {
        continue;
      }
      auto *newAssign = CreateAndInsertRegassign(*opnd, *anchorStmt, func);
      // We should always use the latest rhs here
      auto *repParent = func.GetIRMap()->ReplaceMeExprExpr(*stmt.GetRHS(), *opnd, *newAssign->GetLHS());
      if (repParent != nullptr) {
        static_cast<AssignMeStmt&>(stmt).SetRHS(repParent);
      }
      changed = true;
      TrySplitMeStmt(*newAssign, func, changed);
    }
  }
}

// A wrapper class of iassign stmt with it's store memLoc, this can avoid repeated memLoc calculations
struct StoreWrapper {
  StoreWrapper(IassignMeStmt *iassign, MemLoc *memLoc)
      : stmt(iassign), storeMem(memLoc) {}

  IassignMeStmt *stmt = nullptr;
  MemLoc *storeMem = nullptr;
};

bool AreCompatibleMeExprsShallow(const MeExpr &expr1, const MeExpr &expr2) {
  // We take ivar as a special case because iread and ireadoff are compatible
  bool bothIvar = (expr1.GetMeOp() == kMeOpIvar && expr2.GetMeOp() == kMeOpIvar);
  if (!bothIvar && expr1.GetOp() != expr2.GetOp()) {
    return false;
  }
  auto type1 = expr1.GetPrimType();
  auto type2 = expr2.GetPrimType();
  // Different pointer types are always compatible
  bool bothPtrType = IsPointerType(type1) && IsPointerType(type2);
  if (!bothPtrType && expr1.GetPrimType() != expr2.GetPrimType()) {
    return false;
  }
  return true;
}

bool AreAllExprsCompatibleShallow(const std::vector<MeExpr*> &exprs) {
  CHECK_FATAL(!exprs.empty(), "container check");
  for (size_t i = 0; i < exprs.size() - 1; ++i) {
    auto *curr = exprs[i];
    auto *next = exprs[i + 1];
    if (!AreCompatibleMeExprsShallow(*curr, *next)) {
      return false;
    }
  }
  return true;
}

// Compatible stores need meet the following conditions:
// (1) same data type
// (2) 1-level compatible rhs
bool AreCompatibleStoreWrappers(const StoreWrapper &store1, const StoreWrapper &store2) {
  CHECK_NULL_FATAL(store1.stmt);
  CHECK_NULL_FATAL(store2.stmt);
  if (store1.stmt == store2.stmt) {
    return true;
  }
  // Different pointer types are always compatible
  bool bothPtrType = IsPointerType(store1.storeMem->type->GetPrimType()) &&
                     IsPointerType(store2.storeMem->type->GetPrimType());
  if (!bothPtrType && store1.storeMem->type != store2.storeMem->type) {
    return false;
  }
  return AreCompatibleMeExprsShallow(*store1.stmt->GetRHS(), *store2.stmt->GetRHS());
}

std::string GetOpName(Opcode op) {
  return kOpcodeInfo.GetTableItemAt(op).name;
}

// ------------ //
//  SSA Update  //
// ------------ //
MapleMap<OStIdx, ChiMeNode*> *MergeChiList(const std::vector<MapleMap<OStIdx, ChiMeNode*>*> &chiListVec,
    IRMap &irMap, MeStmt &newBase) {
  auto &alloc = irMap.GetIRMapAlloc();
  auto &mergedChiList = *alloc.New<MapleMap<OStIdx, ChiMeNode*>>(alloc.Adapter());
  for (auto *chiList : chiListVec) {
    for (auto &pair : *chiList) {
      auto ostIdx = pair.first;
      auto *chi = pair.second;
      auto it = mergedChiList.find(ostIdx);
      if (it == mergedChiList.end()) {
        auto *newChi = irMap.New<ChiMeNode>(&newBase);
        newChi->SetLHS(chi->GetLHS());
        chi->GetLHS()->SetDefChi(*newChi);
        newChi->SetRHS(chi->GetRHS());
        mergedChiList.emplace(ostIdx, newChi);
      } else {
        it->second->SetLHS(chi->GetLHS());
        chi->GetLHS()->SetDefChi(*it->second);
      }
    }
  }
  return &mergedChiList;
}

// ------------------------ //
//  Block Level Scheduling  //
// ------------------------ //
void SaveAndRenumberStmtIds(BB &bb, std::vector<uint32> &buffer) {
  buffer.clear();
  auto *stmt = bb.GetFirstMe();
  uint32 id = 0;
  while (stmt != nullptr) {
    buffer.push_back(stmt->GetMeStmtId());
    stmt->SetMeStmtId(id++);
    stmt = stmt->GetNext();
  }
}

void RecoverStmtIds(BB &bb, const std::vector<uint32> &buffer) {
  auto *stmt = bb.GetFirstMe();
  size_t i = 0;
  while (stmt != nullptr) {
    if (i >= buffer.size()) {
      ++i;
      stmt = stmt->GetNext();
      continue;
    }
    stmt->SetMeStmtId(buffer[i++]);
    stmt = stmt->GetNext();
  }
}

uint32 GetOrderId(MeStmt *stmt) {
  if (stmt == nullptr) {
    return UINT32_MAX;
  }
  return stmt->GetMeStmtId();
}

void SetOrderId(MeStmt *stmt, uint32 id) {
  CHECK_NULL_FATAL(stmt);
  stmt->SetMeStmtId(id);
}

// ---------- //
//  SLP Tree  //
// ---------- //
class SLPTree;  // pre-dependency

class TreeNode {
 public:
  using StmtVec = MapleVector<MeStmt*>;
  using ExprVec = MapleVector<MeExpr*>;
  using ExprWithDefVec = MapleVector<ExprWithDef*>;
  // Create TreeNode object from stmts
  TreeNode(SLPTree &inTree, MapleAllocator &alloc, const std::vector<MeStmt*> &meStmts,
           TreeNode *parentNode, uint32 inputId, MemoryHelper &memoryHelper, bool canVect)
      : tree(inTree),
        stmts(alloc.Adapter()),
        exprs(alloc.Adapter()),
        memLocs(alloc.Adapter()),
        order(alloc.Adapter()),
        outStmts(alloc.Adapter()),
        children(alloc.Adapter()),
        id(inputId),
        canVectorized(canVect) {
    ASSERT(!meStmts.empty(), "container check");
    for (auto *stmt : meStmts) {
      stmts.push_back(stmt);
    }
    auto *firstStmt = stmts[0];
    if (firstStmt->GetOp() == OP_iassign) {
      op = OP_iassign;
    } else if (firstStmt->GetOp() == OP_regassign) {
      op = firstStmt->GetRHS()->GetOp();
    } else {
      CHECK_FATAL(false, "NYI");
    }
    // Init memLocs
    if (op == OP_iassign) {
      // Init store memLoc
      for (auto *stmt : stmts) {
        auto *ivar = static_cast<IassignMeStmt*>(stmt)->GetLHSVal();
        auto *memLoc = memoryHelper.GetMemLoc(*ivar);
        memLocs.push_back(memLoc);
      }
      auto *minMemLoc = GetMinMemLoc();
      auto typeSize = minMemLoc->type->GetSize();
      for (auto *memLoc : memLocs) {
        order.push_back((memLoc->offset - minMemLoc->offset) / typeSize);
      }
    } else if (firstStmt->GetRHS() != nullptr && firstStmt->GetRHS()->GetMeOp() == kMeOpIvar) {
      // Init load memLoc
      for (auto *stmt : stmts) {
        auto *ivar = static_cast<IvarMeExpr*>(stmt->GetRHS());
        auto *memLoc = memoryHelper.GetMemLoc(*ivar);
        memLocs.push_back(memLoc);
      }
      auto *minMemLoc = GetMinMemLoc();
      auto typeSize = minMemLoc->type->GetSize();
      for (auto *memLoc : memLocs) {
        order.push_back((memLoc->offset - minMemLoc->offset) / typeSize);
      }
    }
    SetParent(parentNode);
  }

  // Create TreeNode object from exprs
  TreeNode(SLPTree &inTree, MapleAllocator &alloc, const std::vector<ExprWithDef*> &exprVec,
           TreeNode *parentNode, uint32 inputId, bool canVect)
      : tree(inTree),
        stmts(alloc.Adapter()),
        exprs(alloc.Adapter()),
        memLocs(alloc.Adapter()),
        order(alloc.Adapter()),
        outStmts(alloc.Adapter()),
        children(alloc.Adapter()),
        id(inputId),
        canVectorized(canVect) {
    ASSERT(!exprVec.empty(), "container check");
    for (auto *exprWithDef : exprVec) {
      if (canVect) {
        exprs.push_back(exprWithDef->GetRealExpr());
      } else {
        exprs.push_back(exprWithDef->GetExpr());
      }
      stmts.push_back(exprWithDef->GetDefStmt());
    }
    op = exprs[0]->GetOp();
    SetParent(parentNode);
  }

  SLPTree &GetTree() {
    return tree;
  }

  bool ConstOrNeedGather() const {
    return op == OP_constval || !CanVectorized();
  }

  Opcode GetOp() const {
    return op;
  }

  void SetId(uint32 inputId) {
    id = inputId;
  }

  uint32 GetId() const {
    return id;
  }

  const MapleVector<TreeNode*> &GetChildren() const {
    return children;
  }

  void RemoveAllChildren() {
    children.clear();
  }

  void ChangeToNeedGatherNode() {
    CHECK_FATAL(!ConstOrNeedGather(), "must be");
    exprs.resize(stmts.size());
    for (size_t i = 0; i < stmts.size(); ++i) {
      CHECK_NULL_FATAL(stmts[i]);
      CHECK_NULL_FATAL(stmts[i]->GetLHS());
      exprs[i] = stmts[i]->GetLHS();
    }
    canVectorized = false;
  }

  TreeNode *GetParent() const {
    return parent;
  }

  const TreeNode *GetTreeRoot() const {
    auto *root = GetParent();
    if (root == nullptr) {
      return this;  // current node is root node
    }
    while (root->GetParent() != nullptr) {
      root = root->GetParent();
    }
    return root;
  }

  uint32 GetBundleSize() const;

  int32 GetCost() const;

  const StmtVec &GetStmts() const {
    return stmts;
  }

  StmtVec &GetStmts() {
    return stmts;
  }

  void PushStmt(MeStmt *stmt) {
    stmts.push_back(stmt);
  }

  const ExprVec &GetExprs() const {
    return exprs;
  }

  bool SameExpr() const {
    CHECK_FATAL(!exprs.empty(), "exprs empty");
    bool allSame = true;
    auto *firstExpr = exprs[0];
    for (size_t i = 1; i < exprs.size(); ++i) {
      if (firstExpr != exprs[i]) {
        allSame = false;
        break;
      }
    }
    return allSame;
  }

  const MapleVector<MemLoc*> &GetMemLocs() const {
    return memLocs;
  }

  bool IsLoad() const {
    return op == OP_iread || op == OP_ireadoff;
  }

  bool IsStore() const {
    return op == OP_iassign;
  }

  bool IsLeaf() const {
    return children.empty();
  }

  const MapleVector<uint32> &GetOrder() const {
    return order;
  }

  void DumpOrder() const {
    if (!IsStore() && !IsLoad()) {
      LogInfo::MapleLogger() << "DumpOrder: Tree node is neither store nor load" << std::endl;
      return;
    }
    for (size_t i = 0; i < stmts.size(); ++i) {
      LogInfo::MapleLogger() << order[i] << ": ";
      memLocs[i]->Dump();
    }
  }

  // Get memLoc with min offset, only valid for store/load treeNode
  MemLoc *GetMinMemLoc() {
    CHECK_FATAL(!memLocs.empty(), "memLocs is empty, make sure this tree node is a load or store");
    MemLoc *minMem = memLocs[0];
    int32 minOffset = memLocs[0]->offset;
    for (size_t i = 1; i < memLocs.size(); ++i) {
      auto offset = memLocs[i]->offset;
      if (offset < minOffset) {
        minOffset = offset;
        minMem = memLocs[i];
      }
    }
    return minMem;
  }

  size_t GetLane() const {
    auto lane = (!exprs.empty() ? exprs.size() : stmts.size());
    return lane;
  }

  PrimType GetType() const;

  void DumpVecStmts(IRMap &irMap) const {
    LogInfo::MapleLogger() << "  Emit stmts:" << std::endl;
    for (auto *stmt : outStmts) {
      stmt->Dump(&irMap);
    }
  }

  void DumpInDot(MeFunction &func) const {
    (void)func;
    LogInfo::MapleLogger() << "[" << id << "] ";
    LogInfo::MapleLogger() << GetOpName(op);
    LogInfo::MapleLogger() << ", lane = " << GetLane();
    LogInfo::MapleLogger() << ", cost = " << GetCost();
    if (!canVectorized) {
      LogInfo::MapleLogger() << ". NOT VEC";
    }
  }

  StmtVec &GetOutStmts() {
    return outStmts;
  }

  void ClearOutStmts() {
    outStmts.clear();
  }

  void PushOutStmt(MeStmt *stmt) {
    if (stmt == nullptr) {
      return;
    }
    outStmts.push_back(stmt);
  }

  bool CanVectorized() const {
    return canVectorized;
  }

  void Dump(MeFunction &func) const {
    LogInfo::MapleLogger() << "[Tree Node " << id << "]";
    if (!canVectorized) {
      LogInfo::MapleLogger() << " (can NOT be vectorized)";
    }
    LogInfo::MapleLogger() << std::endl;
    LogInfo::MapleLogger() << "op: " << GetOpName(op) << std::endl;
    LogInfo::MapleLogger() << "lane: " << GetLane() << std::endl;
    LogInfo::MapleLogger() << "cost: " << GetCost() << std::endl;
    LogInfo::MapleLogger() << "parent: " << (parent == nullptr ? "NULL" : std::to_string(parent->id)) << std::endl;
    LogInfo::MapleLogger() << "children: ";
    for (auto *child : children) {
      LogInfo::MapleLogger() << child->id << " ";
    }
    LogInfo::MapleLogger() << std::endl;
    if (!order.empty()) {
      LogInfo::MapleLogger() << "order: ";
      for (auto ord : order) {
        LogInfo::MapleLogger() << ord << " ";
      }
      LogInfo::MapleLogger() << std::endl;
    }
    if (!exprs.empty()) {
      for (auto *expr : exprs) {
        expr->Dump(func.GetIRMap(), 0);
        LogInfo::MapleLogger() << std::endl;
      }
    } else {
      for (size_t i = 0; i < stmts.size(); ++i) {
        if (stmts[i] != nullptr) {
          stmts[i]->Dump(func.GetIRMap());
          if (!memLocs.empty()) {
            LogInfo::MapleLogger() << "---- memLoc: ";
            CHECK_FATAL(i < memLocs.size(), "must be");
            CHECK_NULL_FATAL(memLocs[i]);
            memLocs[i]->Dump();
          }
        }
      }
    }
  }

 private:
  void SetParent(TreeNode *parentNode) {
    parent = parentNode;
    if (parent != nullptr) {
      parent->children.push_back(this);
    }
  }

  SLPTree &tree;                   // SLP tree that current node belongs to
  MapleVector<MeStmt*> stmts;      // input stmts
  MapleVector<MeExpr*> exprs;      // input exprs only used when op == OP_constval
  MapleVector<MemLoc*> memLocs;    // only valid for store and load
  MapleVector<uint32> order;       // only valid for store and load
  MapleVector<MeStmt*> outStmts;   // output vectorizd stmts
  MapleVector<TreeNode*> children; // children nodes
  TreeNode *parent = nullptr;      // parent node
  Opcode op = OP_undef;            // tree node opcode
  uint32 id;                       // tree node id
  bool canVectorized = false;      // whether the tree node can be vectorized or must be gathered
};

class SLPTree {
 public:
  SLPTree(MapleAllocator &allocator, MemoryHelper &memHelper)
      : alloc(allocator), memoryHelper(memHelper), treeNodeVec(alloc.Adapter()) {}

  MapleVector<TreeNode*> &GetNodes() {
    return treeNodeVec;
  }

  TreeNode *GetRoot() {
    ASSERT(!treeNodeVec.empty(), "empty tree");
    return treeNodeVec[0];
  }

  size_t GetSize() const {
    return treeNodeVec.size();
  }

  PrimType GetType() const {
    return treeNodeVec[0]->GetType();
  }

  size_t GetLane() const {
    return treeNodeVec[0]->GetLane();
  }

  int32 GetCost() const;

  TreeNode *CreateTreeNodeByExprs(const std::vector<ExprWithDef*> &exprVec, TreeNode *parentNode, bool canVect) {
    auto *treeNode = alloc.New<TreeNode>(*this, alloc, exprVec, parentNode, treeNodeVec.size(), canVect);
    treeNodeVec.push_back(treeNode);
    return treeNode;
  }

  TreeNode *CreateTreeNodeByStmts(const std::vector<MeStmt*> &stmts, TreeNode *parentNode) {
    auto *treeNode = alloc.New<TreeNode>(*this, alloc, stmts, parentNode, treeNodeVec.size(), memoryHelper, true);
    treeNodeVec.push_back(treeNode);
    return treeNode;
  }

  void AddTreeNode(TreeNode *treeNode) {
    treeNode->SetId(treeNodeVec.size());
    treeNodeVec.push_back(treeNode);
  }

  void PopTreeNode() {
    treeNodeVec.pop_back();
  }

  // store treeNode + constval treeNode
  // can use integer register
  bool CanTreeUseScalarTypeForConstvalIassign() const {
    if (treeNodeVec.size() == 2 && treeNodeVec[1]->GetOp() == OP_constval) {
      auto *root = treeNodeVec[0];
      size_t bundleSize = GetPrimTypeBitSize(root->GetType()) * root->GetLane();
      if (bundleSize <= 64) {
        return true;
      }
    }
    return false;
  }

  bool CanTreeUseStp() const {
    if (treeNodeVec.size() != 2) {
      return false;
    }
    auto storeTypeSize = GetPrimTypeBitSize(treeNodeVec[0]->GetType());
    if (storeTypeSize != 32 && storeTypeSize != 64) {
      return false;
    }
    auto *secondTreeNode = treeNodeVec[1];
    return secondTreeNode->ConstOrNeedGather();
  }

  bool CanTreeBeVectorizedCompletely() const {
    for (auto *treeNode : treeNodeVec) {
      if (!treeNode->CanVectorized()) {
        return false;
      }
    }
    return true;
  }

 private:
  MapleAllocator &alloc;
  MemoryHelper &memoryHelper;
  MapleVector<TreeNode*> treeNodeVec;  // the first node is always root
};

PrimType TreeNode::GetType() const {
  if (op == OP_constval) {
    return exprs[0]->GetPrimType();
  }
  // sometimes memLoc type is more precise than stmt rhs.
  // example: iread u32 <*u8>
  // we should use type u8 instead of u32
  if (!memLocs.empty()) {
    return memLocs[0]->type->GetPrimType();
  }
  if (stmts[0] != nullptr) {
    return stmts[0]->GetRHS()->GetPrimType();
  }
  if (exprs[0] != nullptr) {
    return exprs[0]->GetPrimType();
  }
  return tree.GetRoot()->GetType();
}

uint32 TreeNode::GetBundleSize() const {
  return GetLane() * GetPrimTypeBitSize(tree.GetType());
}

int32 TreeNode::GetCost() const {
  if (!CanVectorized()) {
    if (IsPrimitiveFloat(GetType())) {
      // Currently CodeGen can not support set_element/get_element for float type.
      // So return a huge cost that prevent slp tree from being vectorized
      return kHugeCost;
    }
    return GetLane();
  }
  if (op == OP_constval) {
    if (tree.CanTreeUseStp()) {
      return 0;
    } else if (SameExpr()) {
      return 1;
    } else {
      return GetLane();
    }
  }
  int32 cost = -(GetLane() - 1);
  if (op == OP_ashr) {
    // aarch64 only vector shl(register), lowering vector shr will introduce an extra vector neg instruction.
    cost += 1;
  }
  return cost;
}

int32 SLPTree::GetCost() const {
  int32 cost = 0;
  for (auto *treeNode : treeNodeVec) {
    cost += treeNode->GetCost();
  }
  return cost;
}

void PrintTreeNode(TreeNode *node, MeFunction &func, bool inDot) {
  if (inDot) {
    LogInfo::MapleLogger() << "Node" << node->GetId() << " [shape=record" <<
        (node->GetCost() > 0 ? ",style=bold,color=red" : "") << ",label=\"{";
    node->DumpInDot(func);
    LogInfo::MapleLogger() << "}\"]" << std::endl;
  } else {
    node->Dump(func);
  }
}

void PrintSLPTree(TreeNode *root, MeFunction &func, bool inDot) {
  PrintTreeNode(root, func, inDot);
  for (auto *node : root->GetChildren()) {
    PrintSLPTree(node, func, inDot);
    if (inDot) {
      LogInfo::MapleLogger() << "Node" << root->GetId() << " -> Node" << node->GetId() << std::endl;
    }
  }
}

void PrintTreeInDotFormat(TreeNode *root, MeFunction &func) {
  LogInfo::MapleLogger() << "digraph {\n";
  LogInfo::MapleLogger() << "label=\"Total Cost: " << root->GetTree().GetCost() << "\"\n";
  LogInfo::MapleLogger() << "rankdir=\"BT\"\n";  // bottom to top
  PrintSLPTree(root, func, true);
  LogInfo::MapleLogger() << "}" << std::endl;
}

struct BlockScheduling {
  BlockScheduling(MeFunction &f, BB *block, bool dbg) : func(f), bb(block), os(LogInfo::MapleLogger()), debug(dbg) {}

  MeFunction &func;
  BB *bb = nullptr;
  std::vector<uint32> stmtIdBuffer;  // we should recover stmtId before any IR transformation
  std::vector<MeStmt*> stmtVec;      // map position to stmt
  std::ostream &os;
  MeExprUseInfo *exprUseInfo = nullptr;
  bool debug = false;

  std::vector<MeStmt*> &GetStmtVec() {
    return stmtVec;
  }

  void DumpStmtVec() const;

  void SwapStmts(MeStmt *stmt1, MeStmt *stmt2) {
    // swap position in stmtVec
    stmtVec[GetOrderId(stmt1)] = stmt2;
    stmtVec[GetOrderId(stmt2)] = stmt1;
    // swap orderId
    auto tmp = GetOrderId(stmt1);
    SetOrderId(stmt1, GetOrderId(stmt2));
    SetOrderId(stmt2, tmp);
  }

  bool IsRegionEmpty(MeStmt *beginStmt, MeStmt *endStmt) const {
    return beginStmt == endStmt;
  }

  BB *GetBB() {
    return bb;
  }

  void Init() {
    SaveAndRenumberStmtIds(*bb, stmtIdBuffer);
    stmtVec.clear();
    stmtVec.reserve(stmtIdBuffer.size());
    auto *stmt = bb->GetFirstMe();
    while (stmt != nullptr) {
      stmtVec.push_back(stmt);
      stmt = stmt->GetNext();
    }
  }

  bool IsStmtInRegion(MeStmt *stmt, MeStmt *beginStmt, MeStmt *endStmt) {
    if (IsRegionEmpty(beginStmt, endStmt)) {
      return false;
    }
    if (stmt->GetBB() != bb) {
      return false;
    }
    auto id = GetOrderId(stmt);
    if (id >= GetOrderId(beginStmt) && id <= GetOrderId(endStmt)) {
      return true;
    }
    return false;
  }

  MeStmt *FindAnyStmtInRegion(const std::vector<MeStmt*> &stmtVec, MeStmt *beginStmt, MeStmt *endStmt) {
    for (auto *stmt : stmtVec) {
      if (IsStmtInRegion(stmt, beginStmt, endStmt)) {
        return stmt;
      }
    }
    return nullptr;
  }

  bool IsAnyStmtInRegion(const std::vector<MeStmt*> &stmtVec, MeStmt *beginStmt, MeStmt *endStmt) {
    for (auto *stmt : stmtVec) {
      if (IsStmtInRegion(stmt, beginStmt, endStmt)) {
        return true;
      }
    }
    return false;
  }

  bool IsOstUsedByStmt(OriginalSt *ost, MeStmt *stmt);

  void ScheduleStmtBefore(MeStmt *stmt, MeStmt *anchor) {
    CHECK_FATAL(stmt->GetBB() == anchor->GetBB(), "must belong to same BB");
    int32 stmtOrderId = GetOrderId(stmt);
    int32 anchorOrderId = GetOrderId(anchor);
    CHECK_FATAL(anchorOrderId < stmtOrderId, "anchor must be before stmt");
    // should use signed id
    for (int32 id = stmtOrderId - 1; id >= anchorOrderId; --id) {
      SwapStmts(stmt, stmtVec[id]);
    }
  }

  bool CanScheduleStmtBefore(MeStmt *stmt, MeStmt *anchor);
  bool TryScheduleTwoStmtsTogether(MeStmt *first, MeStmt *second);
  // Collect use info from `begin` to `end` (not include `end`) in current BB
  // These use info are needed by dependency analysis and stmt scheduling
  void RebuildUseInfo(MeStmt *begin, MeStmt *end, MapleAllocator &alloc);
};

void BlockScheduling::DumpStmtVec() const {
  LogInfo::MapleLogger() << "--- [ DumpStmtVec ] ---" << std::endl;
  for (auto *stmt : stmtVec) {
    if (stmt == nullptr) {
      continue;
    }
    stmt->Dump(func.GetIRMap());
  }
  LogInfo::MapleLogger() << "--- [ DumpStmtVec ] ---" << std::endl;
}

// --------------------- //
//  Dependency Analysis  //
// --------------------- //
// Move these two functions into some util class when doing code refactor
// Collect defStmt of `expr`
void GetDependencyStmts(MeExpr *expr, std::vector<MeStmt*> &dependencies) {
  Opcode op = expr->GetOp();
  if (op == OP_regread) {
    // we don't need to consider defPhi, we only focus on BB level use-def
    auto *defStmt = static_cast<RegMeExpr*>(expr)->GetDefByMeStmt();
    if (defStmt != nullptr) {
      dependencies.push_back(defStmt);
    }
  } else if (expr->GetMeOp() == kMeOpIvar) {
    for (auto *mu : static_cast<IvarMeExpr*>(expr)->GetMuList()) {
      auto *defStmt = mu->GetDefByMeStmt();
      if (defStmt != nullptr) {
        dependencies.push_back(defStmt);
      }
    }
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    auto *opnd = expr->GetOpnd(i);
    GetDependencyStmts(opnd, dependencies);
  }
}

// Collect defStmt of `stmt`
void GetDependencyStmts(MeStmt *stmt, std::vector<MeStmt*> &dependencies) {
  for (size_t i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
    auto *opnd = stmt->GetOpnd(i);
    GetDependencyStmts(opnd, dependencies);
  }
  auto *chiList = stmt->GetChiList();
  if (chiList == nullptr) {
    return;
  }
  for (auto &chiNodePair : *chiList) {
    auto *chiNode = chiNodePair.second;
    auto *defStmt = chiNode->GetRHS()->GetDefByMeStmt();
    if (defStmt != nullptr) {
      dependencies.push_back(defStmt);
    }
  }
}

void GetOstsUsed(MeExpr *expr, std::unordered_set<OriginalSt*> &ostsUsed) {
  Opcode op = expr->GetOp();
  if (op == OP_regread) {
    // we don't need to consider defPhi, we only focus on BB level use-def
    auto *ost = static_cast<RegMeExpr*>(expr)->GetOst();
    ostsUsed.insert(ost);
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    auto *opnd = expr->GetOpnd(i);
    GetOstsUsed(opnd, ostsUsed);
  }
}

void GetOstsUsed(MeStmt *stmt, std::unordered_set<OriginalSt*> &ostsUsed) {
  // We only consider stmt opnd, skip chiList, because there is always no chi for preg
  for (size_t i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
    auto *opnd = stmt->GetOpnd(i);
    GetOstsUsed(opnd, ostsUsed);
  }
}

void GetUseStmtsOfChiRhs(MeExprUseInfo &useInfo, MeStmt *stmt, std::vector<MeStmt*> &useStmts) {
  auto *chiList = stmt->GetChiList();
  if (chiList == nullptr) {
    return;
  }
  for (auto &chiNodePair : *chiList) {
    auto *chiNode = chiNodePair.second;
    auto *chiRhs = chiNode->GetRHS();
    // Get use stmt of chiRhs
    auto *useList = useInfo.GetUseSitesOfExpr(chiRhs);
    if (useList == nullptr || useList->empty()) {
      continue;
    }
    for (auto &useItem : *useList) {
      // We don't care usePhi because schedule stmts in BB level
      if (useItem.IsUseByStmt()) {
        auto *useStmt = useItem.GetStmt();
        useStmts.push_back(useStmt);
      }
    }
  }
}

// `end` may be nullptr
void BlockScheduling::RebuildUseInfo(MeStmt *begin, MeStmt *end, MapleAllocator &alloc) {
  CHECK_FATAL(begin->GetBB() == bb, "begin stmt must belong to current BB");
  exprUseInfo = alloc.New<MeExprUseInfo>(alloc.GetMemPool());
  auto *useSites = alloc.New<MapleVector<ExprUseInfoPair>>(alloc.Adapter());
  exprUseInfo->SetUseSites(useSites);
  auto *stmt = begin;
  while (stmt != nullptr && stmt != end) {
    exprUseInfo->CollectUseInfoInStmt(stmt);
    stmt = stmt->GetNext();
  }
  exprUseInfo->SetState(kUseInfoOfScalar);
}

bool BlockScheduling::IsOstUsedByStmt(OriginalSt *ost, MeStmt *stmt) {
  std::unordered_set<OriginalSt*> ostsUsed;
  GetOstsUsed(stmt, ostsUsed);
  return ostsUsed.find(ost) != ostsUsed.end();
}

// `anchor` must be before `stmt`
bool BlockScheduling::CanScheduleStmtBefore(MeStmt *stmt, MeStmt *anchor) {
  CHECK_FATAL(stmt->GetBB() == anchor->GetBB(), "must belong to same BB");
  auto stmtOrderId = GetOrderId(stmt);
  auto anchorOrderId = GetOrderId(anchor);
  CHECK_FATAL(anchorOrderId < stmtOrderId, "anchor must be before stmt");
  std::vector<MeStmt*> dependencies;
  GetDependencyStmts(stmt, dependencies);
  std::vector<MeStmt*> useChiRhsStmts;
  GetUseStmtsOfChiRhs(*exprUseInfo, stmt, useChiRhsStmts);
  // range: [anchor, stmt)
  MeStmt *foundDepStmt = FindAnyStmtInRegion(dependencies, anchor, stmt);
  if (foundDepStmt != nullptr) {
    SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << " depends on stmt" <<
        GetOrderId(foundDepStmt) << std::endl);
    return false;
  }
  MeStmt *useStmtOfChiRhs = FindAnyStmtInRegion(useChiRhsStmts, anchor, stmt);
  if (useStmtOfChiRhs != nullptr) {
    SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << " chi rhs is used by stmt" <<
        GetOrderId(useStmtOfChiRhs) << std::endl);
    return false;
  }
  // Identify WAR (Write After Read) for a same preg
  if (stmt->GetOp() == OP_regassign) {
    auto *defOst = static_cast<AssignMeStmt*>(stmt)->GetLHS()->GetOst();
    for (size_t id = anchorOrderId; id < stmtOrderId; ++id) {
      if (IsOstUsedByStmt(defOst, stmtVec[id])) {
        SLP_DEBUG(os << "Found WAR: stmt" << GetOrderId(stmtVec[id]) << " uses ost ");
        if (debug) {
          defOst->Dump();
          os << " that will defined by stmt" << GetOrderId(stmt) << std::endl;
        }
        return false;
      }
    }
  }
  return true;
}

bool BlockScheduling::TryScheduleTwoStmtsTogether(MeStmt *first, MeStmt *second) {
  CHECK_FATAL(first->GetBB() == second->GetBB(), "must belong to same BB");
  auto firstOrderId = GetOrderId(first);
  auto secondOrderId = GetOrderId(second);
  CHECK_FATAL(firstOrderId < secondOrderId, "`first` must be before `second`");
  for (uint32 id = firstOrderId + 1; id < secondOrderId; ++id) {
    auto *stmt = stmtVec[id];
    if (!CanScheduleStmtBefore(stmt, first)) {
      SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << " can not be scheduled before stmt" <<
          GetOrderId(first) << std::endl);
      if (debug) {
        for (auto i = GetOrderId(first); i <= GetOrderId(stmt); ++i) {
          os << "  <" << i << "> LOC " << stmtVec[i]->GetSrcPosition().LineNum();
          stmtVec[i]->Dump(func.GetIRMap());
        }
      }
      return false;
    }
    // DO NOT cache orderIds because ScheduleStmtBefore will change them
    ScheduleStmtBefore(stmt, first);
  }
  return true;
}

// Vectorization infrastruction
MIRType *GetScalarUnsignedTypeBySize(uint32 typeSize) {
  switch (typeSize) {
    case 8: return GlobalTables::GetTypeTable().GetUInt8();
    case 16: return GlobalTables::GetTypeTable().GetUInt16();
    case 32: return GlobalTables::GetTypeTable().GetUInt32();
    case 64: return GlobalTables::GetTypeTable().GetUInt64();
    default: CHECK_FATAL(false, "error type size");
  }
  return nullptr;
}

MIRType* GenVecType(PrimType sPrimType, uint8 lanes) {
  MIRType *vecType = nullptr;
  switch (sPrimType) {
    case PTY_f32: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4Float32();
      } else if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2Float32();
      } else {
        CHECK_FATAL(false, "unsupported f32 vecotry lanes");
      }
      break;
    }
    case PTY_f64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2Float64();
      } else {
        CHECK_FATAL(false, "unsupported f64 vectory lanes");
      }
      break;
    }
    case PTY_i32: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4Int32();
      } else if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2Int32();
      } else {
        CHECK_FATAL(false, "unsupported int32 vectory lanes");
      }
      break;
    }
    case PTY_u32: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4UInt32();
      } else if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2UInt32();
      } else {
        CHECK_FATAL(false, "unsupported uint32 vectory lanes");
      }
      break;
    }
    case PTY_i16: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4Int16();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8Int16();
      } else {
        CHECK_FATAL(false, "unsupported int16 vector lanes");
      }
      break;
    }
    case PTY_u16: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4UInt16();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8UInt16();
      } else {
        CHECK_FATAL(false, "unsupported uint16 vector lanes");
      }
      break;
    }
    case PTY_i8: {
      if (lanes == 16) {
        vecType = GlobalTables::GetTypeTable().GetV16Int8();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8Int8();
      } else {
        CHECK_FATAL(false, "unsupported int8 vector lanes");
      }
      break;
    }
    case PTY_u8: {
      if (lanes == 16) {
        vecType = GlobalTables::GetTypeTable().GetV16UInt8();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8UInt8();
      } else {
        CHECK_FATAL(false, "unsupported uint8 vector lanes");
      }
      break;
    }
    case PTY_i64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2Int64();
      } else {
        CHECK_FATAL(false, "unsupported i64 vector lanes");
      }
    }
    [[clang::fallthrough]];
    case PTY_u64:
    case PTY_a64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2UInt64();
      } else {
        CHECK_FATAL(false, "unsupported a64/u64 vector lanes");
      }
    }
    [[clang::fallthrough]];
    case PTY_ptr: {
      if (GetPrimTypeSize(sPrimType) == 4) {
        if (lanes == 4)  {
          vecType = GlobalTables::GetTypeTable().GetV4UInt32();
        } else if (lanes == 2) {
          vecType = GlobalTables::GetTypeTable().GetV2UInt32();
        } else {
          CHECK_FATAL(false, "unsupported ptr vector lanes");
        }
      } else if (GetPrimTypeSize(sPrimType) == 8) {
        if (lanes == 2) {
          vecType = GlobalTables::GetTypeTable().GetV2UInt64();
        } else {
          CHECK_FATAL(false, "unsupported ptr vector lanes");
        }
      }
      break;
    }
    default:
      CHECK_FATAL(false, "NIY");
  }
  return vecType;
}

#define VECTOR_INTRN_CASE(OP, TY)                      \
  case PTY_##TY: {                                     \
    intrinsic = INTRN_##OP##_##TY;                     \
    break;                                             \
  }

#define GET_VECTOR_INTRN(OP)                           \
  VECTOR_INTRN_CASE(OP, v4i32)                         \
  VECTOR_INTRN_CASE(OP, v2i32)                         \
  VECTOR_INTRN_CASE(OP, v4u32)                         \
  VECTOR_INTRN_CASE(OP, v2u32)                         \
  VECTOR_INTRN_CASE(OP, v8i16)                         \
  VECTOR_INTRN_CASE(OP, v8u16)                         \
  VECTOR_INTRN_CASE(OP, v4i16)                         \
  VECTOR_INTRN_CASE(OP, v4u16)                         \
  VECTOR_INTRN_CASE(OP, v16i8)                         \
  VECTOR_INTRN_CASE(OP, v16u8)                         \
  VECTOR_INTRN_CASE(OP, v8i8)                          \
  VECTOR_INTRN_CASE(OP, v8u8)                          \
  VECTOR_INTRN_CASE(OP, v2i64)                         \
  VECTOR_INTRN_CASE(OP, v2u64)                         \
  VECTOR_INTRN_CASE(OP, v2f32)                         \
  VECTOR_INTRN_CASE(OP, v4f32)                         \
  VECTOR_INTRN_CASE(OP, v2f64)                         \
  default:                                             \
    CHECK_FATAL(false, "NYI");

MIRIntrinsicID GetVectorSetElementIntrnId(PrimType type) {
  MIRIntrinsicID intrinsic;
  switch (type) {
    GET_VECTOR_INTRN(vector_set_element);
  }
  return intrinsic;
}

MIRIntrinsicID GetVectorFromScalarIntrnId(PrimType type) {
  MIRIntrinsicID intrinsic;
  switch (type) {
    GET_VECTOR_INTRN(vector_from_scalar);
  }
  return intrinsic;
}

// ---------------- //
//  SLP Vectorizer  //
// ---------------- //
// Superword Level Parallelism Vectorizer for straight-line code
class SLPVectorizer {
 public:
  using StoreVec = std::vector<StoreWrapper*>;
  using StmtVec = MapleVector<MeStmt*>;
  SLPVectorizer(MemPool &mp, MeFunction &f, Dominance &dom, bool dbg)
      : alloc(&mp),
        memoryHelper(mp),
        func(f),
        irMap(*f.GetIRMap()),
        dom(dom),
        debug(dbg),
        os(LogInfo::MapleLogger()) {
  }

  void Run();
  void ProcessBB(BB &bb);
  void CollectSeedStmts();
  void FilterAndSortSeedStmts();
  bool TrySplitSeedStmts();

  void VectorizeStoreVecMap();
  void VectorizeStores(StoreVec &storeVec);
  void VectorizeCompatibleStores(StoreVec &storeVec, uint32 begin, uint32 end);
  void VectorizeConsecutiveStores(StoreVec &storeVec, uint32 begin, uint32 end);
  bool DoVectorizeSlicedStores(StoreVec &storeVec, uint32 begin, uint32 end, bool onlySchedule = false);

  bool TryScheduleTogehter(const std::vector<MeStmt*> &stmts);
  bool BuildTree(std::vector<MeStmt*> &stmts);
  TreeNode *BuildTreeRec(std::vector<ExprWithDef*> &exprVec, uint32 depth, TreeNode *parentNode);

  bool VectorizeTreeNode(TreeNode *treeNode);
  bool DoVectTreeNodeIvar(TreeNode *treeNode);
  bool DoVectTreeNodeConstval(TreeNode *treeNode);
  bool DoVectTreeNodeBinary(TreeNode *treeNode);
  bool DoVectTreeNodeIassign(TreeNode *treeNode);
  bool DoVectTreeNodeGatherNeeded(TreeNode *treeNode);
  bool VectorizeSLPTree();
  void SetStmtVectorized(MeStmt &stmt);
  bool IsStmtVectorized(MeStmt &stmt) const;

  void MarkStmtsVectorizedInTree();
  void ReplaceStmts(const std::vector<MeStmt*> &origStmtVec, std::list<MeStmt*> &newStmtList);
  void CodeMotionVectorize();
  void CodeMotionReorder();

 private:
  MapleAllocator alloc;
  MapleAllocator *tmpAlloc = nullptr;
  MemoryHelper memoryHelper;
  MeFunction &func;
  IRMap &irMap;
  Dominance &dom;
  std::unordered_map<MemBasePtr*, StoreVec> storeVecMap;  // key is base point part of store lhs ivar
  std::unordered_set<MeStmt*> vectorizedStmts;  // Vectorized stmts in current BB, to avoid vectorize a stmt twice
  BlockScheduling *blockScheduling = nullptr;
  BB *currBB = nullptr;
  bool splited = false;     // whether the stmts in currBB have been splited
  SLPTree *tree = nullptr;
  bool debug = false;
  std::ostream &os;
};

void SLPVectorizer::Run() {
  SLP_DEBUG(os << "Before processing func " << func.GetName() << std::endl);
  const auto &rpo = dom.GetReversePostOrder();
  for (BB *bb : rpo) {
    ProcessBB(*bb);
  }
  SLP_DEBUG(os << "After processing func " << func.GetName() << std::endl);
}

void SLPVectorizer::ProcessBB(BB &bb) {
  blockScheduling = nullptr;
  currBB = &bb;
  vectorizedStmts.clear();
  CollectSeedStmts();
  FilterAndSortSeedStmts();
  if (storeVecMap.empty()) {
    return;
  }
  SLP_DEBUG(os << "=========================================================" << std::endl);
  SLP_DEBUG(os << "Collected " << storeVecMap.size() << " storeVecs in BB" << bb.GetBBId() << std::endl);
  splited = false;
  TrySplitSeedStmts();
  VectorizeStoreVecMap();
}

void SLPVectorizer::VectorizeStoreVecMap() {
  if (storeVecMap.empty()) {
    return;
  }
  StackMemPool stackMemPool(memPoolCtrler, "");
  MapleAllocator stackAlloc(&stackMemPool);
  tmpAlloc = &stackAlloc;
  for (auto &entry : storeVecMap) {
    StoreVec &storeVec = entry.second;
    CHECK_FATAL(storeVec.size() >= 2, "storeVec with size less than 2 should have been removed before");
    VectorizeStores(storeVec);
  }
  tmpAlloc = nullptr;
}

// All stores in the `storeVec` have same store memory base pointer
// Exmaple1: store memory base pointer is &A
//   A[100] = ...
//   A[101] = ...
// Example2: store memory base pointer is &obj
//   obj.field1 = ...
//   obj.field2 = ...
void SLPVectorizer::VectorizeStores(StoreVec &storeVec) {
  SLP_DEBUG(os << "VectorizeStores with size " << storeVec.size() << std::endl);
  if (debug) {
    uint32 i = 0;
    for (auto *storeWrapper : storeVec) {
      os << "  " << i++ << ": ";
      storeWrapper->storeMem->DumpWithoutEndl();
      auto *rhs = storeWrapper->stmt->GetRHS();
      os << " = ";
      if (rhs->GetOp() == OP_constval) {
        rhs->Dump(&irMap);
      } else {
        os << GetOpName(rhs->GetOp());
      }
      os << " : ";
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(rhs->GetPrimType()))->Dump(0);
      os << " ...... LOC " << storeWrapper->stmt->GetSrcPosition().LineNum();
      os << std::endl;
    }
  }
  // Find compatible store range and try to vectorize them
  for (size_t i = 0; i < storeVec.size();) {
    size_t j = i + 1;
    while (j < storeVec.size() && AreCompatibleStoreWrappers(*storeVec[i], *storeVec[j])) {
      ++j;
    }
    size_t num = j - i;
    if (num <= 1) {
      // no enough compatible stores, can't do vectorization
      ++i;
      continue;
    }
    // num > 2, try to vectorize storeSlice
    VectorizeCompatibleStores(storeVec, i, j);
    i = j;
  }
}

// Try to vectorize storeVec[begin, end), in which all stores are compatible
void SLPVectorizer::VectorizeCompatibleStores(StoreVec &storeVec, uint32 begin, uint32 end) {
  SLP_DEBUG(os << "== VectorizeCompatibleStores [" << begin << ", " << end << ")" << std::endl);
  for (uint32 i = begin; i < end;) {
    uint32 j = i + 1;
    for (; j < end; ++j) {
      if (!memoryHelper.IsConsecutive(*storeVec[j - 1]->storeMem, *storeVec[j]->storeMem, false)) {
        break;
      }
    }
    uint32 num = j - i;
    if (num <= 1) {
      // no enough consecutive stores. can't do vectorization
      ++i;
      continue;
    }
    VectorizeConsecutiveStores(storeVec, i, j);
    i = j;
  }
}

// This is target-dependent, we need abstract TargetInfo in the future
uint32 GetMaxVectorFactor(PrimType scalarType) {
  CHECK_FATAL(!PrimitiveType(scalarType).IsVector(), "must be");
  auto typeSize = GetPrimTypeSize(scalarType);
  CHECK_FATAL(typeSize != 0, "unknown type size");
  return 16 / typeSize;  // max vector regsize: 16byte
}

uint32 GetMinVectorFactor(PrimType scalarType) {
  CHECK_FATAL(!PrimitiveType(scalarType).IsVector(), "must be");
  auto typeSize = GetPrimTypeSize(scalarType);
  CHECK_FATAL(typeSize != 0, "unknown type size");
  return std::max(8 / typeSize, 2u);  // min vector regsize: 8byte
}

// Try to vectorize storeVec[begin, end), in which all stores are compatible and consecutive
void SLPVectorizer::VectorizeConsecutiveStores(StoreVec &storeVec, uint32 begin, uint32 end) {
  SLP_DEBUG(os << "==== VectorizeConsecutiveStores [" << begin << ", " << end << ")" << std::endl);
  PrimType scalarType = storeVec[begin]->storeMem->type->GetPrimType();
  const uint32 maxVf = GetMaxVectorFactor(scalarType);
  const uint32 minVf = GetMinVectorFactor(scalarType);
  SLP_DEBUG(os << "minVf: " << minVf << ", maxVf: " << maxVf << std::endl);
  //uint32 startIdx = begin;
  uint32 endIdx = end;
  bool tried = false;
  std::vector<bool> vectorized(storeVec.size(), false);
  for (uint32 vf = maxVf; vf >= minVf; vf /= 2) {
    for (uint32 i = endIdx; i >= vf && i - vf >= begin;) {
      // Make sure no scalar stores have been vectorized
      // When stmts are vectorized successfully, we will set them vectorizd
      // to avoid vectorize them again.
      uint32 currBegin = i - vf;
      bool allStoresNotVectorized = true;
      for (uint32 k = currBegin; k < i; ++k) {
        if (vectorized[k]) {
          allStoresNotVectorized = false;
          break;
        }
      }
      if (!allStoresNotVectorized) {
        i -= vf;  // can use --i for more optimization at the cost of build time
        continue;
      }
      bool res = DoVectorizeSlicedStores(storeVec, currBegin, i);
      tried = true;
      if (res) {
        for (uint32 j = currBegin; j < i; ++j) {
          vectorized[j] = true;
        }
        if (i == endIdx) {
          endIdx -= vf;
        }
        i -= vf;
        continue;
      }
      i -= vf;  // can use --i for more optimization at the cost of build time
    }
  }
  if (!tried) {
    SLP_DEBUG(os << "can't find appropriate vector factor, minVf = " << minVf << ", maxVf = " << maxVf << std::endl);
    SLP_DEBUG(os << "vectorize failure, try to reorder load/store" << std::endl);
    DoVectorizeSlicedStores(storeVec, begin, end, true);
  }
}

// input: consecutive compatible stores
// Try to vectorize storeVec[begin, begin + vf)
bool SLPVectorizer::DoVectorizeSlicedStores(StoreVec &storeVec, uint32 begin, uint32 end, bool onlySchedule) {
  uint32 vf = end - begin;
  SLP_DEBUG(os << "====== DoVectorizeSlicedStores [" << begin << ", " << end << ") vf = " << vf << std::endl);
  // Init blockScheduling if needed
  auto bsPtr = std::make_unique<BlockScheduling>(func, currBB, debug);
  blockScheduling = bsPtr.get();
  SLP_DEBUG(os << "Init BlockScheduling for BB" << currBB->GetBBId() << std::endl);
  blockScheduling->Init();
  // Rebuild currBB use info before building tree
  // Rebuild if needed
  blockScheduling->RebuildUseInfo(currBB->GetFirstMe(), nullptr, *tmpAlloc);

  std::vector<MeStmt*> stmts;
  for (uint32 i = begin; i < end; ++i) {
    stmts.push_back(storeVec[i]->stmt);
  }
  bool res = BuildTree(stmts);
  if (!res) {
    CHECK_FATAL(false, "should not be here");
  }
  if (onlySchedule) {
    return true;
  }

  bool vectorized = VectorizeSLPTree();
  // If we can use stp instruction, don't need to replace scalar stmts with vector stmts
  if (vectorized && !tree->CanTreeUseStp()) {
    MarkStmtsVectorizedInTree();
    CodeMotionVectorize();
  } else {
    CodeMotionReorder();
  }
  return vectorized;
}

void SLPVectorizer::CodeMotionVectorize() {
  std::list<MeStmt*> newStmtList;
  // ReplaceStmts must be called before RecoverStmtIds
  ReplaceStmts(blockScheduling->GetStmtVec(), newStmtList);
  // We should call RecoverStmtIds before any real code motion
  RecoverStmtIds(*currBB, blockScheduling->stmtIdBuffer);
  currBB->GetMeStmts().clear();
  for (auto *stmt : newStmtList) {
    stmt->SetPrev(nullptr);
    stmt->SetNext(nullptr);
    currBB->AddMeStmtLast(stmt);
  }
}

// The output newStmtList must have no repeated elements
void SLPVectorizer::ReplaceStmts(const std::vector<MeStmt*> &origStmtVec, std::list<MeStmt*> &newStmtList) {
  bool keepStmtsExceptIassign = true;
  std::set<uint32> insertPositions;
  std::set<uint32> toBeRemoved;
  std::map<uint32, std::vector<MeStmt*>> pos2newStmts;
  for (auto *treeNode : tree->GetNodes()) {
    if (treeNode->ConstOrNeedGather()) {
      continue;
    }
    uint32 pos = 0;
    for (auto *stmt : treeNode->GetStmts()) {
      if (stmt == nullptr) {
        continue;
      }
      auto orderId = GetOrderId(stmt);
      if (keepStmtsExceptIassign && treeNode->GetOp() == OP_iassign) {
        toBeRemoved.insert(orderId);  // only remove iassgin scalar stmts
      }
      if (orderId > pos) {
        pos = orderId;
      }
    }
    std::vector<MeStmt*> newStmts;
    for (auto *child : treeNode->GetChildren()) {
      if (child->ConstOrNeedGather()) {
        newStmts.insert(newStmts.end(), child->GetOutStmts().begin(), child->GetOutStmts().end());
      }
    }
    newStmts.insert(newStmts.end(), treeNode->GetOutStmts().begin(), treeNode->GetOutStmts().end());
    if (pos != 0 && !newStmts.empty()) {
      insertPositions.insert(pos);
      pos2newStmts.emplace(pos, newStmts);
    }
  }

  for (size_t i = 0; i < origStmtVec.size(); ++i) {
    auto orderId = GetOrderId(origStmtVec[i]);
    if (orderId == *insertPositions.begin()) {
      auto it = pos2newStmts.find(orderId);
      CHECK_FATAL(it != pos2newStmts.end(), "must be");
      for (auto *stmt : it->second) {
        newStmtList.push_back(stmt);
      }
      insertPositions.erase(insertPositions.begin());
    }
    if (toBeRemoved.find(orderId) == toBeRemoved.end()) {
      newStmtList.push_back(origStmtVec[i]);
    }
  }
}

void SLPVectorizer::CodeMotionReorder() {
  if (tree->GetSize() == 0) {
    return;
  }
  auto scalarTypeSize = GetPrimTypeBitSize(tree->GetType());
  if (scalarTypeSize != 32 && scalarTypeSize != 64) {
    return;
  }
  auto lane = tree->GetLane();
  if (lane != 2) {
    return;
  }
  auto *root = tree->GetRoot();
  std::vector<MeStmt*> storeStmts;
  for (auto *stmt : root->GetStmts()) {
    storeStmts.push_back(stmt);
  }
  // sort stmts by orderId
  std::sort(storeStmts.begin(), storeStmts.end(), [](MeStmt *a, MeStmt *b) {
    return GetOrderId(a) < GetOrderId(b);
  });

  // We should call RecoverStmtIds before any real code motion
  RecoverStmtIds(*currBB, blockScheduling->stmtIdBuffer);

  // Do scheduling stores
  for (size_t i = 0; i < lane - 1; ++i) {
    auto *currStmt = storeStmts[i];
    currBB->RemoveMeStmt(currStmt);
    currBB->InsertMeStmtBefore(storeStmts.back(), currStmt);
  }
}

bool SLPVectorizer::TrySplitSeedStmts() {
  // Each bb enters this function at most once
  if (splited) {
    return false;
  }
  SLP_DEBUG(os << "Try to split seed stmts" << std::endl);
  splited = true;
  bool changed = false;
  for (auto &entry : storeVecMap) {
    StoreVec &storeVec = entry.second;
    CHECK_FATAL(storeVec.size() >= 2, "storeVec with size less than 2 should have been removed before");
    // Split the potentially vectorizable seed stmts
    for (auto *store : storeVec) {
      TrySplitMeStmt(*store->stmt, func, changed);
    }
  }
  return changed;
}

// Currently we only collect store, it may be enhanced in the future.
void SLPVectorizer::CollectSeedStmts() {
  // Clear old seed stmts first
  storeVecMap.clear();

  auto *stmt = currBB->GetFirstMe();
  while (stmt != nullptr) {
    auto *next = stmt->GetNextMeStmt();
    if (stmt->GetOp() == OP_iassign) {
      auto *lhsIvar = static_cast<IassignMeStmt*>(stmt)->GetLHSVal();
      auto *lhsIvarType = lhsIvar->GetType();
      // Skip volatile store, vector store, agg store, bitfield store
      if (lhsIvar->IsVolatile() || PrimitiveType(lhsIvarType->GetPrimType()).IsVector() ||
          lhsIvar->GetPrimType() == PTY_agg || lhsIvarType->GetKind() == kTypeBitField) {
        stmt = next;
        continue;
      }
      MemLoc *memLoc = memoryHelper.GetMemLoc(*lhsIvar);
      MemBasePtr *key = memLoc->base;
      auto *storeWrapper = alloc.GetMemPool()->New<StoreWrapper>(static_cast<IassignMeStmt*>(stmt), memLoc);
      auto it = storeVecMap.find(key);
      if (it != storeVecMap.end()) {
        it->second.push_back(storeWrapper);
      } else {
        storeVecMap[key].push_back(storeWrapper);
      }
    }
    stmt = next;
  }
}

// Remove storeVec with size less than 2 and
// sort storeVec by address offset in ascending order
void SLPVectorizer::FilterAndSortSeedStmts() {
  for (auto it = storeVecMap.begin(); it != storeVecMap.end();) {
    if (it->second.size() <= 1) {
      it = storeVecMap.erase(it);
    } else {
      auto &storeVec = it->second;
      // Sort storeVec by address offset in ascending order
      std::stable_sort(storeVec.begin(), storeVec.end(), [](const auto *a, const auto *b) {
        return a->storeMem->offset < b->storeMem->offset;
      });
      ++it;
    }
  }
}


// Schedule stmts from bottom to up
bool SLPVectorizer::TryScheduleTogehter(const std::vector<MeStmt*> &stmts) {
  // Sort stmts by source code order. The purpose of sorting is to schedule correctly.
  // We must use original stmts when we create tree node, so we need to copy stmts.
  std::vector<MeStmt*> sortedStmts = stmts;
  std::sort(sortedStmts.begin(), sortedStmts.end(), [](MeStmt *a, MeStmt *b) {
    return GetOrderId(a) < GetOrderId(b);
  });
  for (int32 i = sortedStmts.size() - 2; i >= 0; --i) {
    bool res = blockScheduling->TryScheduleTwoStmtsTogether(sortedStmts[i], sortedStmts[i + 1]);
    if (!res) {
      return false;  // Schedule fail
    }
  }
  return true;  // Schedule succeed
}

// Building SLP tree
// input: sorted stmts by memLoc offset
bool SLPVectorizer::BuildTree(std::vector<MeStmt*> &stmts) {
  tree = tmpAlloc->New<SLPTree>(*tmpAlloc, memoryHelper);
  SLP_DEBUG(os << "Build tree node for " << GetOpName(stmts[0]->GetOp()) << std::endl);
  CHECK_FATAL(stmts.size() >= 2, "must be");

  if (!TryScheduleTogehter(stmts)) {
    SLP_FAILURE_DEBUG(os << "Scheduling failure" << std::endl);
    return true;
  }
  SLP_DEBUG(os << "Scheduling OK, building tree node..." << std::endl);
  auto *rootNode = tree->CreateTreeNodeByStmts(stmts, nullptr);

  std::vector<ExprWithDef*> exprVec;
  for (auto *stmt : stmts) {
    CHECK_NULL_FATAL(stmt->GetRHS());
    auto *exprWithDef = alloc.GetMemPool()->New<ExprWithDef>(stmt->GetRHS(), currBB);
    exprVec.push_back(exprWithDef);
  }
  (void)BuildTreeRec(exprVec, 1, rootNode);

  if (debug) {
    os << "===== Print tree in dot format =====" << std::endl;
    PrintTreeInDotFormat(rootNode, func);
    os << "===== Print tree in text foramt =====" << std::endl;
    PrintSLPTree(rootNode, func, false);
  }
  return true;
}

TreeNode *SLPVectorizer::BuildTreeRec(std::vector<ExprWithDef*> &exprVec, uint32 depth, TreeNode *parentNode) {
  std::vector<MeExpr*> realExprs;
  std::vector<MeExpr*> lhsExprs;
  for (auto *exprWithDef : exprVec) {
    realExprs.push_back(exprWithDef->GetRealExpr());
    lhsExprs.push_back(exprWithDef->GetExpr());
  }
  auto *firstRealExpr = exprVec[0]->GetRealExpr();
  Opcode currOp = firstRealExpr->GetOp();
  SLP_DEBUG(os << "Build tree node for expr " << GetOpName(currOp) << std::endl);

  // (1) Compatible exprs
  bool compatible = AreAllExprsCompatibleShallow(realExprs);
  if (!compatible) {
    SLP_GATHER_DEBUG(os << "expr not compatible" << std::endl);
    if (debug) {
      for (size_t i = 0; i < realExprs.size(); ++i) {
        os << "<expr" << i << "> ";
        realExprs[i]->Dump(&irMap, 2);
        os << std::endl;
      }
    }
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }

  // Now all the exprs are compatible
  // (2) Supported ops
  static const std::vector<Opcode> supportedOps = {
    OP_iread, OP_ireadoff, OP_add, OP_sub, OP_constval, OP_bxor, OP_band, OP_mul,
  };
  if (std::find(supportedOps.begin(), supportedOps.end(), currOp) == supportedOps.end()) {
    SLP_GATHER_DEBUG(os << "unsupported expr op: " << GetOpName(currOp) << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }

  if (currOp == OP_constval) {
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, true);
  }

  // (3) [CHECK LOAD] Check load/store size and check bit field load
  if (firstRealExpr->GetMeOp() == kMeOpIvar) {
    auto *firstIvar = static_cast<IvarMeExpr*>(firstRealExpr);
    auto *loadMIRType = firstIvar->GetType();
    PrimType loadType = loadMIRType->GetPrimType();
    PrimType storeType = tree->GetType(); // The first tree node is always store
    if (GetPrimTypeSize(loadType) != GetPrimTypeSize(storeType)) {
      SLP_GATHER_DEBUG(os << "The type size of load and store can not match" << std::endl);
      return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
    }
    if (loadMIRType->GetKind() == kTypeBitField) {
      SLP_GATHER_DEBUG(os << "BitField load vectorization is not supported" << std::endl);
      return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
    }
    // All loads must be consecutive
    if (!memoryHelper.IsAllIvarConsecutive(realExprs, true)) {
      SLP_GATHER_DEBUG(os << "loads are not consecutive" << std::endl);
      return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
    }
  }

  std::vector<MeStmt*> stmts;
  bool hasNullStmt = false;
  bool hasVectorizedStmt = false;
  std::set<MeExpr*> uniqueExprs;
  std::set<MeStmt*> uniqueStmts;
  for (auto *exprWithDef : exprVec) {
    auto *currStmt = exprWithDef->GetDefStmt();
    stmts.push_back(currStmt);
    uniqueExprs.insert(exprWithDef->GetRealExpr());
    uniqueStmts.insert(currStmt);
    if (currStmt == nullptr) {
      hasNullStmt = true;
    }
    if (currStmt != nullptr && IsStmtVectorized(*currStmt)) {
      hasVectorizedStmt = true;
    }
  }

  if (hasNullStmt) {
    SLP_GATHER_DEBUG(os << "has null stmt" << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }
  if (hasVectorizedStmt) {
    SLP_GATHER_DEBUG(os << "has vectorized stmt" << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }

  bool exprNotUnique = uniqueExprs.size() < exprVec.size();
  if (exprNotUnique) {
    SLP_GATHER_DEBUG(os << "expr not unique " << func.GetName() << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }
  bool stmtNotUnique = uniqueStmts.size() < exprVec.size();
  if (stmtNotUnique) {
    SLP_GATHER_DEBUG(os << "stmt not unique" << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }

  if (!TryScheduleTogehter(stmts)) {
    SLP_GATHER_DEBUG(os << "Scheduling failure" << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }
  SLP_DEBUG(os << "Scheduling OK, building tree node..." << std::endl);
  auto *treeNode = tree->CreateTreeNodeByStmts(stmts, parentNode);

  // We no longer split ivar nodes
  if (realExprs[0]->IsLeaf() || realExprs[0]->GetMeOp() == kMeOpIvar) {
    return treeNode;
  }

  size_t numOpnd = realExprs[0]->GetNumOpnds();
  size_t numLane = exprVec.size();
  size_t numChildrenNeededGather = 0;
  for (size_t i = 0; i < numOpnd; ++i) {
    std::vector<ExprWithDef*> opndVec;
    for (size_t lane = 0; lane < numLane; ++lane) {
      auto *opnd = realExprs[lane]->GetOpnd(i);
      auto *opndWithDef = alloc.GetMemPool()->New<ExprWithDef>(opnd, currBB);
      opndVec.push_back(opndWithDef);
    }
    auto *childTreeNode = BuildTreeRec(opndVec, depth + 1, treeNode);
    if (childTreeNode->IsLeaf() && !childTreeNode->CanVectorized()) {
      ++numChildrenNeededGather;
    }
  }
  // Too many children needed gathering, we should remove them for better tree cost
  if (numChildrenNeededGather > 1) {
    treeNode->RemoveAllChildren();
    treeNode->ChangeToNeedGatherNode();
    for (size_t num = 0; num < numChildrenNeededGather; ++num) {
      tree->PopTreeNode();   // remove node from treeNodeVec
    }
  }
  return treeNode;
}

// posExprVec: pair.first means where to insert the value
//             pair.second means what value to insert
MeExpr *BuildExprAfterVectorSetElement(MeFunction &func, RegMeExpr *vecReg,
    const std::vector<std::pair<uint32, MeExpr*>> &posExprVec, PrimType laneType) {
  auto &irMap = *func.GetIRMap();
  auto vecPrimType = vecReg->GetPrimType();
  MeExpr *currVecExpr = vecReg;
  for (size_t i = 0; i < posExprVec.size(); ++i) {
    const auto &posExprPair = posExprVec[i];
    uint32 pos = posExprPair.first;
    MeExpr *valueExpr = posExprPair.second;
    if (valueExpr->GetPrimType() != laneType) {
      valueExpr = irMap.CreateMeExprTypeCvt(laneType, valueExpr->GetPrimType(), *valueExpr);
    }
    MIRIntrinsicID intrnId = GetVectorSetElementIntrnId(vecPrimType);
    NaryMeExpr naryExpr(&irMap.GetIRMapAlloc(), kInvalidExprID, OP_intrinsicop,
        vecPrimType, 3, TyIdx(0), intrnId, false);
    naryExpr.PushOpnd(valueExpr);
    naryExpr.PushOpnd(currVecExpr);
    naryExpr.PushOpnd(irMap.CreateIntConstMeExpr(pos, PTY_i32));
    currVecExpr = irMap.CreateNaryMeExpr(naryExpr);
  }
  return currVecExpr;
}

// Example:
//   constants: [ 12, 35, 78, 89 ], elemSize: 8bit
//   output: (bin) 01011001 01001110 00100011 00001100
uint64 ConstructConstants(std::vector<int64> &constants, uint32 elemSize) {
  uint64 res = 0;
  uint32 shift = 0;
  uint32 maskShift = 64 - elemSize;
  uint64 mask = static_cast<uint64>(-1) << maskShift >> maskShift;
  for (auto cst : constants) {
    res += ((cst & mask) << shift);
    shift += elemSize;
  }
  return res;
}

bool SLPVectorizer::DoVectTreeNodeConstval(TreeNode *treeNode) {
  // We will support float point constval when allowed by cg handlefunc
  if (!IsPrimitiveInteger(treeNode->GetType())) {
    SLP_FAILURE_DEBUG(os << "float point vectorization has not been supported by cg handlefunc" << std::endl);
    return false;
  }
  if (tree->CanTreeUseStp()) {
    return true;
  }
  PrimType elemType = tree->GetType();
  auto *vecType = GenVecType(tree->GetType(), treeNode->GetLane());
  CHECK_NULL_FATAL(vecType);
  bool useScalarType = tree->CanTreeUseScalarTypeForConstvalIassign();
  ScalarMeExpr *lhsReg = nullptr;
  MeExpr *rhs = nullptr;
  if (useScalarType) {
    vecType = GetScalarUnsignedTypeBySize(GetPrimTypeBitSize(vecType->GetPrimType()));
    lhsReg = irMap.CreateRegMeExpr(*vecType);
    std::vector<int64> constants(treeNode->GetExprs().size());
    std::transform(treeNode->GetExprs().begin(), treeNode->GetExprs().end(), constants.begin(), [](MeExpr *expr) {
      return static_cast<ConstMeExpr*>(expr)->GetIntValue();
    });
    uint64 mergeConstval = ConstructConstants(constants, GetPrimTypeBitSize(elemType));
    rhs = irMap.CreateIntConstMeExpr(mergeConstval, vecType->GetPrimType());
  } else if (treeNode->SameExpr()) {
    MIRIntrinsicID intrnId = GetVectorFromScalarIntrnId(vecType->GetPrimType());
    MeExpr *constExpr = treeNode->GetExprs()[0];
    if (constExpr->GetPrimType() != elemType) {
      constExpr = irMap.CreateMeExprTypeCvt(elemType, constExpr->GetPrimType(), *constExpr);
    }
    lhsReg = irMap.CreateRegMeExpr(*vecType);
    // Create vector intrinsicop
    NaryMeExpr naryExpr(&irMap.GetIRMapAlloc(), kInvalidExprID, OP_intrinsicop,
        vecType->GetPrimType(), 1, TyIdx(0), intrnId, false);
    naryExpr.PushOpnd(constExpr);
    rhs = irMap.CreateNaryMeExpr(naryExpr);
  } else {
    auto *vecReg = irMap.CreateRegMeExpr(*vecType);
    auto exprNum = treeNode->GetExprs().size();
    std::vector<std::pair<uint32, MeExpr*>> posExprVec;
    for (size_t i = 0; i < exprNum; ++i) {
      posExprVec.emplace_back(i, treeNode->GetExprs()[i]);
    }
    rhs = BuildExprAfterVectorSetElement(func, vecReg, posExprVec, elemType);
    lhsReg = irMap.CreateRegMeExpr(*vecType);
  }
  auto *constVecAssign = irMap.CreateAssignMeStmt(*lhsReg, *rhs, *currBB);
  treeNode->PushOutStmt(constVecAssign);
  if (debug) {
    treeNode->DumpVecStmts(irMap);
  }
  return true;
}

ScalarMeExpr *FindStructExprByFieldExprs(IRMap &irMap, OriginalSt &structOst,
    const std::vector<ScalarMeExpr*> &fieldExprs) {
  // Find struct vst
  bool isAllScalarIvarZeroVersion = true;
  MeStmt *latestDefStmt = nullptr;
  ScalarMeExpr *latestStructVstExpr = nullptr;
  for (ScalarMeExpr *fieldExpr  : fieldExprs) {
    auto defBy = fieldExpr->GetDefBy();
    if (defBy != kDefByNo) {
      isAllScalarIvarZeroVersion = false;
    }
    if (defBy == kDefByChi) {
      auto *defStmt = fieldExpr->GetDefByMeStmt();
      if (latestDefStmt != nullptr && GetOrderId(latestDefStmt) >= GetOrderId(defStmt)) {
        continue;
      }
      latestDefStmt = defStmt;
      auto *chiList = defStmt->GetChiList();
      CHECK_NULL_FATAL(chiList);
      auto it = chiList->find(structOst.GetIndex());
      CHECK_FATAL(it != chiList->end(), "chiList misses structOst chi");
      latestStructVstExpr = it->second->GetLHS();
    }
  }
  if (isAllScalarIvarZeroVersion) {
    latestStructVstExpr = static_cast<ScalarMeExpr*>(irMap.GetMeExprByVerID(structOst.GetZeroVersionIndex()));
  }
  return latestStructVstExpr;
}

bool SLPVectorizer::DoVectTreeNodeIvar(TreeNode *treeNode) {
  // Shuffle has not been supported for now
  CHECK_FATAL(treeNode->GetOrder().size() == treeNode->GetLane(), "must be");
  const auto &rootStoreOrder = tree->GetRoot()->GetOrder();
  if (treeNode->GetOrder() != rootStoreOrder) {
    SLP_FAILURE_DEBUG(os << "Shuffle has not been supported" << std::endl);
    if (debug) {
      os << "[Loads]\n";
      treeNode->DumpOrder();
      os << "[Stores]\n";
      tree->GetRoot()->DumpOrder();
    }
    return false;
  }
  auto *vecType = GenVecType(tree->GetType(), treeNode->GetLane());
  CHECK_NULL_FATAL(vecType);
  auto *minMem = treeNode->GetMinMemLoc();
  auto *vecPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType);
  auto *addrExpr = minMem->Emit(irMap);
  auto *ivarExpr = static_cast<IvarMeExpr*>(addrExpr);
  if (ivarExpr->GetFieldID() == 0) {
    SLP_FAILURE_DEBUG(os << "We need ivar.muList" << std::endl);
    return false;
  } else {  // filedId != 0
    auto *newBase = ivarExpr->GetBase();
    if (minMem->extraOffset != 0) {
      auto *extraOffsetExpr = irMap.CreateIntConstMeExpr(minMem->extraOffset, PTY_u64);
      newBase = irMap.CreateMeExprBinary(OP_add, PTY_u64,
          *newBase, *extraOffsetExpr);
    }
    IvarMeExpr newIvar(&irMap.GetIRMapAlloc(), -1, vecType->GetPrimType(), TyIdx(PTY_u64), 0);
    newIvar.SetTyIdx(vecPtrType->GetTypeIndex());
    newIvar.SetPtyp(vecType->GetPrimType());
    newIvar.SetBase(newBase);
    // set mu
    auto prevLevelVstIdx = ivarExpr->GetUniqueMu()->GetOst()->GetPointerVstIdx();
    auto *sameLevelOstsPtr = func.GetMeSSATab()->GetOriginalStTable().GetNextLevelOstsOfVst(prevLevelVstIdx);
    if (sameLevelOstsPtr == nullptr) {
      SLP_FAILURE_DEBUG(os << "can not find sameLevelOstsPtr, is dereferencing a null pointer?");
      return false;
    }
    auto &sameLevelOsts = *sameLevelOstsPtr;
    // Find struct ost, sometimes we can't find it
    OriginalSt *structOst = nullptr;
    for (auto *ost : sameLevelOsts) {
      if (ost->GetFieldID() == 0) {
        structOst = ost;
        break;
      }
    }
    // Use check fatal after fixing whole struct ost problem or supporting ivar.muList
    if (structOst == nullptr) {
      SLP_FAILURE_DEBUG(os << "can not find structOst when updating mu for vector ivar" << std::endl);
      return false;
    }
    std::vector<ScalarMeExpr*> fieldExprs;
    for (auto *stmt : treeNode->GetStmts()) {
      fieldExprs.push_back(static_cast<IvarMeExpr*>(stmt->GetRHS())->GetUniqueMu());
    }
    ScalarMeExpr *latestStructVstExpr = FindStructExprByFieldExprs(irMap, *structOst, fieldExprs);
    if (latestStructVstExpr != nullptr) {
      newIvar.SetMuItem(0, latestStructVstExpr);
    } else {
      SLP_FAILURE_DEBUG(os << "can not find structVst when updating mu for vector ivar" << std::endl);
      return false;
    }
    addrExpr = irMap.HashMeExpr(newIvar);
  }
  CHECK_FATAL(addrExpr->GetMeOp() == kMeOpIvar, "only iread memLoc is supported for now");
  auto *lhsReg = irMap.CreateRegMeExpr(*vecType);
  auto *vecRegassign = irMap.CreateAssignMeStmt(*lhsReg, *addrExpr, *currBB);
  vecRegassign->SetSrcPos(treeNode->GetStmts()[0]->GetSrcPosition());
  treeNode->PushOutStmt(vecRegassign);
  if (debug) {
    treeNode->DumpVecStmts(irMap);
  }
  return true;
}

bool SLPVectorizer::DoVectTreeNodeBinary(TreeNode *treeNode) {
  CHECK_FATAL(treeNode->GetChildren().size() == 2, "must be");
  auto *vecType = GenVecType(tree->GetType(), treeNode->GetLane());
  CHECK_NULL_FATAL(vecType);
  // Get the first vector operand
  auto &childOutStmts0 = treeNode->GetChildren()[0]->GetOutStmts();
  auto *defStmt0 = childOutStmts0.back();
  MeExpr *opnd0 = defStmt0->GetLHS();
  CHECK_NULL_FATAL(opnd0);
  CHECK_FATAL(opnd0->GetPrimType() == vecType->GetPrimType(), "must be");

  // Get the second vector operand
  auto &childOutStmts1 = treeNode->GetChildren()[1]->GetOutStmts();
  auto *defStmt1 = childOutStmts1.back();
  MeExpr *opnd1 = defStmt1->GetLHS();
  CHECK_NULL_FATAL(opnd1);
  CHECK_FATAL(opnd1->GetPrimType() == vecType->GetPrimType(), "must be");

  // Create binary expr and assign stmt
  auto *binaryExpr = irMap.CreateMeExprBinary(treeNode->GetOp(), vecType->GetPrimType(), *opnd0, *opnd1);
  auto *lhsReg = irMap.CreateRegMeExpr(*vecType);
  auto *assign = irMap.CreateAssignMeStmt(*lhsReg, *binaryExpr, *currBB);
  assign->SetSrcPos(treeNode->GetStmts()[0]->GetSrcPosition());
  treeNode->PushOutStmt(assign);
  if (debug) {
    treeNode->DumpVecStmts(irMap);
  }
  return true;
}

bool SLPVectorizer::DoVectTreeNodeIassign(TreeNode *treeNode) {
  CHECK_FATAL(treeNode->GetChildren().size() == 1, "must be");
  if (tree->CanTreeUseStp()) {
    for (auto *stmt : treeNode->GetStmts()) {
      CHECK_NULL_FATAL(stmt);
      treeNode->PushOutStmt(stmt);
    }
    return true;
  }
  auto *minMem = treeNode->GetMinMemLoc();
  CHECK_NULL_FATAL(minMem);
  auto *vecType = GenVecType(tree->GetType(), treeNode->GetLane());
  CHECK_NULL_FATAL(vecType);
  if (tree->CanTreeUseScalarTypeForConstvalIassign()) {
    vecType = GetScalarUnsignedTypeBySize(GetPrimTypeBitSize(vecType->GetPrimType()));
  }
  auto *vecPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType);
  MeExpr *addrExpr = minMem->Emit(irMap);
  auto *ivarExpr = static_cast<IvarMeExpr*>(addrExpr);
  if (ivarExpr->GetFieldID() == 0) {
    IvarMeExpr newIvar(*ivarExpr);
    newIvar.SetTyIdx(vecPtrType->GetTypeIndex());
    newIvar.SetPtyp(vecType->GetPrimType());
    addrExpr = irMap.HashMeExpr(newIvar);
  } else {  // fieldId != 0
    auto *newBase = ivarExpr->GetBase();
    if (minMem->extraOffset != 0) {
      auto *extraOffsetExpr = irMap.CreateIntConstMeExpr(minMem->extraOffset, PTY_u64);
      newBase = irMap.CreateMeExprBinary(OP_add, PTY_u64,
          *newBase, *extraOffsetExpr);
    }
    IvarMeExpr newIvar(&irMap.GetIRMapAlloc(), -1, vecType->GetPrimType(), TyIdx(PTY_u64), 0);
    newIvar.SetTyIdx(vecPtrType->GetTypeIndex());
    newIvar.SetPtyp(vecType->GetPrimType());
    newIvar.SetBase(newBase);
    addrExpr = irMap.HashMeExpr(newIvar);
  }
  // lhs ivar don't need mu
  auto *childStmt = treeNode->GetChildren()[0]->GetOutStmts().back();
  MeExpr *rhs = childStmt->GetLHS();
  auto *chiList = static_cast<IassignMeStmt*>(treeNode->GetStmts()[0])->GetChiList();
  auto *vecIassign = irMap.CreateIassignMeStmt(vecPtrType->GetTypeIndex(),
      static_cast<IvarMeExpr&>(*addrExpr), *rhs, *chiList);
  vecIassign->SetSrcPos(treeNode->GetStmts()[0]->GetSrcPosition());

  // Merge chiList
  std::vector<MapleMap<OStIdx, ChiMeNode*>*> chiListVec;
  std::vector<MeStmt*> sortedStmts;
  for (auto *stmt : treeNode->GetStmts()) {
    sortedStmts.push_back(stmt);
  }
  std::sort(sortedStmts.begin(), sortedStmts.end(), [](MeStmt *a, MeStmt *b) {
    return GetOrderId(a) < GetOrderId(b);
  });
  for (auto *stmt : sortedStmts) {  // must iterate stmt by orderId
    auto *scalarIassign = static_cast<IassignMeStmt*>(stmt);
    auto *chiList = scalarIassign->GetChiList();
    auto *muList = scalarIassign->GetMuList();
    (void)muList;
    chiListVec.push_back(chiList);
  }
  auto *mergedChiList = MergeChiList(chiListVec, irMap, *vecIassign);
  vecIassign->SetChiList(*mergedChiList);

  treeNode->PushOutStmt(vecIassign);
  if (debug) {
    treeNode->DumpVecStmts(irMap);
  }
 return true;
}

bool SLPVectorizer::DoVectTreeNodeGatherNeeded(TreeNode *treeNode) {
  if (tree->CanTreeUseStp()) {
    return true;
  }
  auto elemType = tree->GetType();
  auto *vecType = GenVecType(elemType, treeNode->GetLane());
  CHECK_NULL_FATAL(vecType);
  auto *vecReg = irMap.CreateRegMeExpr(*vecType);
  auto exprNum = treeNode->GetExprs().size();
  std::vector<std::pair<uint32, MeExpr*>> posExprVec;
  for (size_t i = 0; i < exprNum; ++i) {
    MeExpr *valueExpr = treeNode->GetExprs()[i];
    posExprVec.emplace_back(i, valueExpr);
  }
  MeExpr *vecRegNew = nullptr;
  if (treeNode->SameExpr()) {
    MIRIntrinsicID intrnId = GetVectorFromScalarIntrnId(vecType->GetPrimType());
    // Create vector intrinsicop
    NaryMeExpr naryExpr(&irMap.GetIRMapAlloc(), kInvalidExprID, OP_intrinsicop,
        vecType->GetPrimType(), 1, TyIdx(0), intrnId, false);
    naryExpr.PushOpnd(treeNode->GetExprs()[0]);
    vecRegNew = irMap.CreateNaryMeExpr(naryExpr);
  } else {
    vecRegNew = BuildExprAfterVectorSetElement(func, vecReg, posExprVec, elemType);
  }
  auto *lhsReg = irMap.CreateRegMeExpr(*vecType);
  auto *vecAssign = irMap.CreateAssignMeStmt(*lhsReg, *vecRegNew, *currBB);
  treeNode->PushOutStmt(vecAssign);
  CHECK_FATAL(treeNode->GetChildren().empty(), "NOT VEC treeNode should not have children");
  if (debug) {
    treeNode->DumpVecStmts(irMap);
  }
  return true;
}

bool SLPVectorizer::VectorizeTreeNode(TreeNode *treeNode) {
  for (auto *child : treeNode->GetChildren()) {
    if (!VectorizeTreeNode(child)) {
      SLP_DEBUG(os << "VectorizeTreeNode failed: tree node " << child->GetId() << ", " <<
          GetOpName(child->GetOp()) << std::endl);
      return false;
    }
  }
  SLP_DEBUG(os << "Start to vectorize tree node " << treeNode->GetId() << " (" <<
      GetOpName(treeNode->GetOp()) << ")" << std::endl);
  // Use root node type for covering the following case:
  //   [(mx943), 4] : u8 = constval : u32
  auto *vecType = GenVecType(tree->GetType(), treeNode->GetLane());
  CHECK_NULL_FATAL(vecType);

  // Tree node can not be vectorized, we use vector_set_element
  if (!treeNode->CanVectorized()) {
    return DoVectTreeNodeGatherNeeded(treeNode);
  }

  switch (treeNode->GetOp()) {
    case OP_iread:
    case OP_ireadoff: {
      return DoVectTreeNodeIvar(treeNode);
    }
    case OP_bxor:
    case OP_band:
    case OP_ashr:
    case OP_add:
    case OP_mul:
    case OP_sub: {
      return DoVectTreeNodeBinary(treeNode);
    }
    case OP_constval: {
      return DoVectTreeNodeConstval(treeNode);
    }
    case OP_iassign: {
      return DoVectTreeNodeIassign(treeNode);
    }
    default: {
      SLP_FAILURE_DEBUG(os << "unsupported op: " << GetOpName(treeNode->GetOp()) << std::endl);
      return false;
      break;
    }
  }
  return false;
}

bool SLPVectorizer::VectorizeSLPTree() {
  if (tree->GetSize() <= 1) {
    return false;
  }
  SLP_DEBUG(os << "Start to vectorize slp tree" << std::endl);
  auto cost = tree->GetCost();
  if (cost > 0) {
    SLP_FAILURE_DEBUG(os << "tree cost: " << cost << std::endl);
    return false;
  }
  bool res = VectorizeTreeNode(tree->GetRoot());
  if (!res) {
    SLP_DEBUG(os << "Vectorize tree failure" << std::endl);
    return false;
  }
  SLP_DEBUG(os << "Vectorize tree successfully" << std::endl);
  SLP_OK_DEBUG(os << "tree size: " << tree->GetSize() << ", tree cost: " << cost << ", func: " << func.GetName() <<
      ", bbId: " << currBB->GetBBId() << ", loc: 0" << std::endl);
  return true;
}

void SLPVectorizer::SetStmtVectorized(MeStmt &stmt) {
  vectorizedStmts.insert(&stmt);
}

bool SLPVectorizer::IsStmtVectorized(MeStmt &stmt) const {
  // Most of the time the container is empty, so don't worry too much about performance
  return vectorizedStmts.find(&stmt) != vectorizedStmts.end();
}

void SLPVectorizer::MarkStmtsVectorizedInTree() {
  for (auto *treeNode : tree->GetNodes()) {
    auto &stmts = treeNode->GetStmts();
    for (auto *stmt : stmts) {
      if (stmt == nullptr) {
        continue;
      }
      SetStmtVectorized(*stmt);
    }
  }
}

void MESLPVectorizer::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MESLPVectorizer::PhaseRun(MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);
  MemPool *memPool = GetPhaseMemPool();
  bool debug = DEBUGFUNC_NEWPM(f);
  SLPVectorizer slpVectorizer(*memPool, f, *dom, debug);
  slpVectorizer.Run();
  return false;
}
}  // namespace maple
