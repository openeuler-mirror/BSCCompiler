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
#include "common_utils.h"
#include "aarch64/aarch64_imm_valid.h"

#define SLP_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP] "; (X); } } while (false)

#define SLP_OK_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP OK] "; (X); } } while (false)

#define SLP_GATHER_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP GATHER] "; (X); } } while (false)

#define SLP_FAILURE_DEBUG(X) \
do { if (debug) { LogInfo::MapleLogger() << "[SLP FAILURE] "; (X); } } while (false)

namespace {
using namespace maple;
constexpr maple::int32 kHugeCost = 10000;  // Used for impossible vectorized node
constexpr size_t kMaxNumStores = 1000;

const std::vector<Opcode> supportedOps = {
    OP_iread, OP_ireadoff, OP_add, OP_sub, OP_constval, OP_bxor, OP_band, OP_mul, OP_intrinsicop
};

const std::vector<MIRIntrinsicID> supportedIntrns = {INTRN_C_rev_4, INTRN_C_rev_8, INTRN_C_rev16_2};

std::vector<int32> *localSymOffsetTab = nullptr;
}  // anonymous namespace

namespace maple {
// A wrapper class of meExpr with its defStmt, this can avoid repeated searches for use-def chains
constexpr int8 k8Bit = 8;
class ExprWithDef {
 public:
  explicit ExprWithDef(MeExpr *meExpr, BB *bb = nullptr) : expr(meExpr) {
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
  void FindDef(const ScalarMeExpr *scalarExpr, const BB *bb) {
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
MeExpr *MemLoc::Emit() const {
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
        auto bitOffset = static_cast<int32>(static_cast<MIRStructType*>(pointedType)->
                                            GetBitOffsetFromBaseAddr(fieldId));
        memLoc.offset += (bitOffset / k8Bit);
      }
      ExtractAddendOffset(*expr.GetOpnd(0), isNeg, memLoc);
      break;
    }
    case OP_constval: {
      auto val = static_cast<decltype(memLoc.offset)>(static_cast<const ConstMeExpr&>(expr).GetExtIntValue());
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
      memLoc.base->PushAddendExpr(alloc, expr, isNeg);
      break;
    }
  }
}

void MemoryHelper::UniqueMemLocBase(MemLoc &memLoc) {
  // must be sorted first
  memLoc.base->Sort();
  auto key = memLoc.base->GetHashKey();
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

MemLoc *MemoryHelper::GetMemLoc(VarMeExpr &var) {
  auto *ost = var.GetOst();
  auto *prevLevOst = ost->GetPrevLevelOst();
  CHECK_NULL_FATAL(prevLevOst);
  // find memLoc from cache first
  auto it = cache.find(&var);
  if (it != cache.end()) {
    return it->second;
  }
  auto *memLocPtr = alloc.GetMemPool()->New<MemLoc>();
  MemLoc &memLoc = *memLocPtr;
  memLoc.rawExpr = &var;
  memLoc.offset = ost->GetOffset().val / k8Bit;
  memLoc.type = ost->GetType();
  memLoc.base = alloc.GetMemPool()->New<MemBasePtr>();
  memLoc.base->SetBaseOstIdx(static_cast<int32>(prevLevOst->GetIndex().get()));

  // unique memLoc base
  UniqueMemLocBase(memLoc);

  // update cache
  cache.emplace(&var, &memLoc);
  return &memLoc;
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
    memLoc.base->PushAddendExpr(alloc, *ivar.GetBase(), false);
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
    auto bitOffset = static_cast<decltype(extraOffset)>(structType->GetBitOffsetFromBaseAddr(ivar.GetFieldID()));
    extraOffset += (bitOffset / k8Bit);
  }
  extraOffset += ivar.GetOffset();
  memLoc.extraOffset = extraOffset;
  memLoc.offset += extraOffset;
  memLoc.type = ivar.GetType();
  // update cache
  cache.emplace(&ivar, &memLoc);
  return &memLoc;
}

MemLoc *MemoryHelper::GetMemLoc(MeExpr &meExpr) {
  auto meOp = meExpr.GetMeOp();
  switch (meOp) {
    case kMeOpIvar:
      return GetMemLoc(static_cast<IvarMeExpr&>(meExpr));
    case kMeOpVar:
      return GetMemLoc(static_cast<VarMeExpr&>(meExpr));
    default:
      CHECK_FATAL(false, "MemoryHelper::GetMemLoc: not supported meOp");
      break;
  }
  return nullptr;
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

bool MemoryHelper::IsSame(const MemLoc &mem1, const MemLoc &mem2) {
  if (mem1.type != mem2.type) {
    return false;
  }
  auto dis = Distance(mem1, mem2);
  if (!dis) {
    return false;
  }
  return dis.value() == 0;
}

bool MemoryHelper::IsConsecutive(const MemLoc &mem1, const MemLoc &mem2, bool mustSameType) {
  if (mustSameType && mem1.type != mem2.type) {
    return false;
  }
  auto dis = Distance(mem1, mem2);
  if (!dis) {
    return false;
  }
  return dis.value() == static_cast<ssize_t>(mem1.type->GetSize());
}

bool MemoryHelper::MustHaveNoOverlap(const MemLoc &mem1, const MemLoc &mem2) {
  // If the bases are not same, we can't tell if they overlap
  if (!HaveSameBase(mem1, mem2)) {
    return false;
  }
  int32 offset1 = mem1.offset;
  int32 offset2 = mem2.offset;
  uint32 size1 = static_cast<uint32>(mem1.type->GetSize());
  uint32 size2 = static_cast<uint32>(mem2.type->GetSize());
  // overlapping case one:
  // mem1: |------|
  // mem2:     |------|
  if (offset1 <= offset2 && static_cast<uint32>(offset2 - offset1) < size1) {
    return false;
  }
  // overlapping case two:
  // mem1:     |------|
  // mem2: |------|
  if (offset2 < offset1 && static_cast<uint32>(offset1 - offset2) < size2) {
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

int32 GetLocalSymApproximateOffset(const MIRSymbol *sym, MeFunction &func) {
  if (localSymOffsetTab == nullptr) {
    localSymOffsetTab = new std::vector<int32>();
  }
  auto &offsetTab = *localSymOffsetTab;
  uint32 symIdx = sym->GetStIndex();
  if (symIdx < offsetTab.size()) {
    return offsetTab[symIdx];
  }
  for (uint32 i = static_cast<uint32>(offsetTab.size()); i <= symIdx; ++i) {
    if (i == 0) {
      offsetTab.push_back(0);
      continue;
    }
    auto *lastSym = func.GetMirFunc()->GetSymbolTabItem(i - 1);
    if (lastSym == nullptr || lastSym->GetStorageClass() != kScAuto || lastSym->IsDeleted()) {
      offsetTab.push_back(offsetTab[i - 1]);
      continue;
    }
    // align 8 bytes
    auto sizeAfterAlign = (lastSym->GetType()->GetSize() + 7) & -8;
    offsetTab.push_back(offsetTab[i - 1] + static_cast<int32>(sizeAfterAlign));
  }
  return offsetTab[symIdx];
}

std::optional<int32> EstimateStackOffsetOfMemLoc(MemLoc *memLoc, MeFunction &func) {
  if (memLoc->rawExpr->GetMeOp() != kMeOpIvar) {
    return {};
  }
  auto *ivarBase = static_cast<IvarMeExpr*>(memLoc->rawExpr)->GetBase();
  if (ivarBase->GetOp() != OP_regread) {
    return {};
  }
  auto *baseRegExpr = static_cast<RegMeExpr*>(ivarBase);
  if (baseRegExpr->GetDefBy() != kDefByStmt) {
    return {};
  }
  auto *defStmt = baseRegExpr->GetDefStmt();
  if (defStmt != nullptr && defStmt->GetOp() == OP_regassign) {
    auto *rhs = static_cast<AssignMeStmt*>(defStmt)->GetRHS();
    if (rhs->GetOp() == OP_addrof) {
      auto *ost = static_cast<AddrofMeExpr*>(rhs)->GetOst();
      CHECK_FATAL(ost, "ost is nullptr");
      if (ost->IsLocal()) {
        return GetLocalSymApproximateOffset(ost->GetMIRSymbol(), func);
      }
    }
  }
  return {};
}

#if defined(TARGAARCH64) && TARGAARCH64
// This is target-dependent function for armv8, maybe we need abstract it TargetInfo
bool IsIntStpLdpOffsetValid(uint32 typeBitSize, int32 offset) {
  switch (typeBitSize) {
    case 32: {
      return maplebe::StrLdr32PairImmValid(offset);
    }
    case 64: {
      return maplebe::StrLdr64PairImmValid(offset);
    }
    default: {
      CHECK_FATAL(false, "should not be here");
      break;
    }
  }
  return false;
}
#endif

#if defined(TARGAARCH64) && TARGAARCH64
// This is target-dependent function for armv8, maybe we need abstract it TargetInfo
bool IsVecStrLdrOffsetValid(uint32 typeBitSize, int32 offset) {
  switch (typeBitSize) {
    case 8: {
      return maplebe::StrLdr8ImmValid(offset);
    }
    case 16: {
      return maplebe::StrLdr16ImmValid(offset);
    }
    case 32: {
      return maplebe::StrLdr32ImmValid(offset);
    }
    case 64: {
      return maplebe::StrLdr64ImmValid(offset);
    }
    case 128: {
      return maplebe::StrLdr128ImmValid(offset);
    }
    default: {
      CHECK_FATAL(false, "should not be here");
      break;
    }
  }
  return false;
}
#endif

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
  if (op == OP_iassign || op == OP_dassign) {
    // DO NOT split iassign/dassign with agg type
    auto lhsType = op == OP_iassign ? static_cast<IassignMeStmt&>(stmt).GetLHSVal()->GetPrimType() :
        static_cast<DassignMeStmt&>(stmt).GetLHS()->GetPrimType();
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
  StoreWrapper(MeStmt *meStmt, MemLoc *memLoc)
      : stmt(meStmt), storeMem(memLoc) {}

  MeStmt *stmt = nullptr;  // IassignMeStmt or DassignMeStmt
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

// both `stmt1` and `stmt2` must have chiList
bool SwapChiForSameOstIfNeeded(MeStmt &stmt1, MeStmt &stmt2) {
  bool swapped = false;
  auto *chiList1 = stmt1.GetChiList();
  auto *chiList2 = stmt2.GetChiList();
  CHECK_NULL_FATAL(chiList1);
  CHECK_NULL_FATAL(chiList2);
  auto p = chiList1->begin();
  auto q = chiList2->begin();
  while (p != chiList1->end() && q != chiList2->end()) {
    if (p->first == q->first) {
      if (p->second->GetRHS() == q->second->GetLHS()) {
        auto *tmp = p->second;
        p->second = q->second;
        q->second = tmp;
        // Update defStmt
        p->second->SetBase(&stmt1);
        q->second->SetBase(&stmt2);
        swapped = true;
      }
      ++p;
      ++q;
    } else if (p->first < q->first) {
      ++p;
    } else {
      ++q;
    }
  }
  return swapped;
}

using VisitedPhiOpnds = std::map<size_t, ScalarMeExpr*>;

ScalarMeExpr* DoFindPreVersionByAlias(ScalarMeExpr &aliasPreVersion, ScalarMeExpr &curVersion,
    VisitedPhiOpnds &visitedPhiOpnds, bool &foundLoop);

ScalarMeExpr* DoFindPreVersionByAliasInPhi(ScalarMeExpr &aliasPreVersion, ScalarMeExpr &curVersion,
    VisitedPhiOpnds &visitedPhiOpnds, bool &foundLoop) {
  OStIdx targetOstIdx = curVersion.GetOstIdx();
  auto &aliasPhi = aliasPreVersion.GetDefPhi();
  auto &phiList = aliasPhi.GetDefBB()->GetMePhiList();
  auto it = phiList.find(targetOstIdx);
  if (it != phiList.end()) {
    return it->second->GetLHS();
  }
  auto &phiOpnds = aliasPhi.GetOpnds();
  for (ScalarMeExpr *opnd : phiOpnds) {
    if (opnd == aliasPhi.GetLHS()) {
      continue;
    }
    auto retPair = visitedPhiOpnds.try_emplace(visitedPhiOpnds.size(), opnd);
    if (!retPair.second) {
      // found loop
      foundLoop = true;
      return nullptr;
    }
    size_t oldMaxId = visitedPhiOpnds.size() - 1;
    auto *result = DoFindPreVersionByAlias(*opnd, curVersion, visitedPhiOpnds, foundLoop);
    if (!foundLoop) {
      return result;
    }
    foundLoop = false;
    for (size_t i = visitedPhiOpnds.size() - 1; i > oldMaxId; --i) {
      (void)visitedPhiOpnds.erase(i);
    }
  }
  CHECK_FATAL_FALSE("should not be here, wrong alias?");
}

ScalarMeExpr* DoFindPreVersionByAlias(ScalarMeExpr &aliasPreVersion, ScalarMeExpr &curVersion,
    VisitedPhiOpnds &visitedPhiOpnds, bool &foundLoop) {
  OStIdx targetOstIdx = curVersion.GetOstIdx();
  auto defType = aliasPreVersion.GetDefBy();
  if (defType == kDefByNo) {
    return nullptr;
  }
  if (defType == kDefByPhi) {
    return DoFindPreVersionByAliasInPhi(aliasPreVersion, curVersion, visitedPhiOpnds, foundLoop);
  }

  auto *defStmt = aliasPreVersion.GetDefByMeStmt();
  CHECK_NULL_FATAL(defStmt);  // kDefByChi, kDefByStmt or kDefByMustDef
  // Search defStmt lhs
  if (defStmt->GetOp() == OP_dassign) {
    auto *dassignLhs = static_cast<DassignMeStmt*>(defStmt)->GetLHS();
    if (dassignLhs->GetOstIdx() == targetOstIdx) {
      return dassignLhs;
    }
  }
  // Search defStmt chiList
  ScalarMeExpr *nextExpr = nullptr;
  auto *chiList = defStmt->GetChiList();
  if (chiList != nullptr) {
    const auto it = std::as_const(chiList)->find(targetOstIdx);
    if (it != chiList->end()) {
      return it->second->GetRHS();
    }
    nextExpr = aliasPreVersion.GetDefChi().GetRHS();
  }
  // Search defStmt mustDefList
  auto *mustDefList = defStmt->GetMustDefList();
  if (mustDefList != nullptr) {
    for (auto &mustDef : *mustDefList) {
      if (mustDef.GetLHS()->GetOstIdx() == targetOstIdx) {
        return mustDef.GetLHS();
      }
    }
  }
  CHECK_NULL_FATAL(nextExpr);
  return DoFindPreVersionByAlias(*nextExpr, curVersion, visitedPhiOpnds, foundLoop);
}

// Try to find previous version of `curVersion` by searching all possible defs of `aliasPreVersion`.
// Return preVersion if found, otherwise return nullptr.
ScalarMeExpr* FindPreVersionByAlias(ScalarMeExpr &aliasPreVersion, ScalarMeExpr &curVersion) {
  VisitedPhiOpnds visitedPhiOpnds;
  bool foundLoop = false;
  return DoFindPreVersionByAlias(aliasPreVersion, curVersion, visitedPhiOpnds, foundLoop);
}

// Find previous version of dassignStmt lhsVar
static ScalarMeExpr *FindPreVersionOfDassignLhs(DassignMeStmt &dassignStmt, IRMap &irMap) {
  auto *curVersion = dassignStmt.GetLHS();
  auto *dassignChiList = dassignStmt.GetChiList();
  CHECK_NULL_FATAL(dassignChiList);
  CHECK_FATAL(!dassignChiList->empty(), "chiList empty");
  ChiMeNode *aliasChi = dassignChiList->begin()->second;
  ScalarMeExpr *aliasPreVersion = aliasChi->GetRHS();
  auto *preVersion = FindPreVersionByAlias(*aliasPreVersion, *curVersion);
  if (preVersion == nullptr) {
    preVersion = irMap.GetOrCreateZeroVersionVarMeExpr(*curVersion->GetOst());
  }
  return preVersion;
}

static void MovDefsFromDassignsToVecIassign(MapleVector<MeStmt*> &dassignStmts, IassignMeStmt &vecIassign,
    IRMap &irMap) {
  for (auto *stmt : dassignStmts) {
    CHECK_FATAL(stmt->GetOp() == OP_dassign, "must be");
    auto *dassign = static_cast<DassignMeStmt*>(stmt);
    auto *curVersion = dassign->GetLHS();
    auto *preVersion = FindPreVersionOfDassignLhs(*dassign, irMap);
    auto *newChi = irMap.New<ChiMeNode>(&vecIassign);
    newChi->SetLHS(curVersion);
    newChi->SetRHS(preVersion);
    curVersion->SetDefBy(kDefByChi);
    curVersion->SetDefChi(*newChi);
    (void)vecIassign.GetChiList()->emplace(curVersion->GetOstIdx(), newChi);
  }
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

uint32 GetOrderId(const MeStmt *stmt) {
  if (stmt == nullptr) {
    return UINT32_MAX;
  }
  return stmt->GetMeStmtId();
}

void SetOrderId(MeStmt *stmt, uint32 id) {
  CHECK_NULL_FATAL(stmt);
  stmt->SetMeStmtId(id);
}

// Assume there are two stmts in a same bb, stmt1 is located before stmt2:
//   stmt1
//   ...
//   stmt2
// Now we want to schedule stmt2 before stmt1, the scenario of scheduling failure is as follows:
enum class SchedFailReason {
  kSchedSuccess,        // schedule success
  kSchedFailOpndDep,    // dependencies of stmt2's opnds is within the range [stmt1, stmt2)
  kSchedFailMuDep,      // dependencies of stmt2's mu is within the range [stmt1, stmt2)
  kSchedFailChiRhsDep,  // dependencies of stmt2's chiRhs is within the range [stmt1, stmt2)
  kSchedFailChiRhsUse,  // [cross-version ref] stmt2's chiRhs is used by stmts in the range [stmt1, stmt2)
  kSchedFailPregWAR     // preg Write After Read
};

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
    auto firstOp = firstStmt->GetOp();
    if (firstOp == OP_iassign || firstOp == OP_dassign) {
      op = firstOp;
    } else if (firstOp == OP_regassign) {
      op = firstStmt->GetRHS()->GetOp();
    } else {
      CHECK_FATAL(false, "NYI");
    }
    // Init memLocs
    if (op == OP_iassign || op == OP_dassign) {
      // Init store memLoc
      for (auto *stmt : stmts) {
        MemLoc *memLoc = nullptr;
        if (op == OP_iassign) {
          auto *ivar = static_cast<IassignMeStmt*>(stmt)->GetLHSVal();
          memLoc = memoryHelper.GetMemLoc(*ivar);
        } else {
          MeExpr *var = static_cast<DassignMeStmt*>(stmt)->GetLHS();
          CHECK_FATAL(var->GetOp() == OP_dread, "unexpected stmt");
          memLoc = memoryHelper.GetMemLoc(*static_cast<VarMeExpr*>(var));
        }
        memLocs.push_back(memLoc);
      }
      auto *minMemLoc = GetMinMemLoc();
      auto typeSize = minMemLoc->type->GetSize();
      for (auto *memLoc : memLocs) {
        order.push_back(static_cast<uint32>((memLoc->offset - minMemLoc->offset)) / typeSize);
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
        order.push_back(static_cast<uint32>((memLoc->offset - minMemLoc->offset)) / typeSize);
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

  int32 GetExternalUserCost() const;

  // Only valid for load/store treeNode for now
  int32 GetScalarCost() const;

  // Only valid for load/store treeNode for now
  int32 GetVectorCost() const;

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

  bool CanNodeUseLoadStorePair() const {
    if (!IsLoad() && !IsStore()) {
      return false;
    }
    if (IsLoad() && stmts[0] == nullptr) {
      // iread with agg primType won't be splited, it's treeNode has no valid stmts
      return false;
    }
    auto typeBitSize = GetPrimTypeBitSize(GetType());
    if (GetLane() == 2 && (typeBitSize == 32 || typeBitSize == 64)) {
      return true;
    }
    return false;
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
  MemLoc *GetMinMemLoc() const {
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

  void DumpVecStmts(const IRMap &irMap) const {
    LogInfo::MapleLogger() << "  Emit stmts:" << std::endl;
    for (auto *stmt : outStmts) {
      stmt->Dump(&irMap);
    }
  }

  void DumpInDot(const MeFunction &func) const {
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
    if (IsLoad()) {
      int32 externalUserCost = GetExternalUserCost();
      if (externalUserCost != 0) {
        LogInfo::MapleLogger() << "externalUserCost: " << externalUserCost << std::endl;
      }
    }
    if (IsLoad() || IsStore()) {
      LogInfo::MapleLogger() << "CanNodeUseLoadStorePair: " << (CanNodeUseLoadStorePair() ? "yes" : "no") << std::endl;
      LogInfo::MapleLogger() << "scalar cost: " << GetScalarCost() << std::endl;
      LogInfo::MapleLogger() << "vector cost: " << GetVectorCost() << std::endl;
      if (GetMinMemLoc()->offset == 0) {
        auto stackOffset = EstimateStackOffsetOfMemLoc(GetMinMemLoc(), func);
        if (stackOffset) {
          LogInfo::MapleLogger() << "must in stack, offset: " << stackOffset.value() << std::endl;
        }
      }
    }
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

struct BlockScheduling;
class SLPTree {
 public:
  SLPTree(MapleAllocator &allocator, MemoryHelper &memHelper, MeFunction &f, BlockScheduling &bs)
      : alloc(allocator), memoryHelper(memHelper), func(f), blockScheduling(bs), treeNodeVec(alloc.Adapter()) {}

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

  MeFunction &GetFunc() {
    return func;
  }

  MeExprUseInfo &GetUseInfo();

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
    treeNode->SetId(static_cast<uint32>(treeNodeVec.size()));
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
    if (secondTreeNode->IsLoad()) {
      if (secondTreeNode->GetStmts()[0] == nullptr) {
        return false;  // iread with agg primType won't be splited, it's treeNode has no valid stmts
      }
      if (secondTreeNode->GetLane() == 2) {
        return true;
      }
    }
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
  MeFunction &func;
  BlockScheduling &blockScheduling;
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
  return static_cast<uint32>(GetLane() * GetPrimTypeBitSize(tree.GetType()));
}

// Only valid for load/store treeNode for now
int32 TreeNode::GetScalarCost() const {
  ASSERT(IsLoad() || IsStore(), "must be");
  int32 cost = static_cast<int32>(GetLane());
  if (CanNodeUseLoadStorePair()) {
    cost /= 2;
    auto *minMemLoc = GetMinMemLoc();
    int32 offset = minMemLoc->offset;
    if (offset == 0) {
      auto stackOffset = EstimateStackOffsetOfMemLoc(minMemLoc, tree.GetFunc());
      if (stackOffset) {
        offset = stackOffset.value();
      }
    }
#if defined(TARGAARCH64) && TARGAARCH64
    // If stp/ldp for stack memory, we think that an extra insn is always needed for preparing offset
    bool offsetValid = IsIntStpLdpOffsetValid(GetPrimTypeBitSize(GetType()), offset);
#else
    bool offsetValid = false;
#endif
    if (!offsetValid) {
      cost += 1;  // invalid offset, an extra instruction is needed to prepare offset const
      if (IsStore()) {
        bool canReuseOffsetPrepareInsn = tree.GetSize() == 2 && tree.GetNodes()[1]->IsLoad() &&
            minMemLoc->base == tree.GetNodes()[1]->GetMinMemLoc()->base;
        if (canReuseOffsetPrepareInsn) {
          // If ldp and stp have same base, only 1 extra insn is needed to prepare offset const
          cost -= 1;
        }
      }
    }
  }
  return cost;
}

// Only valid for load/store treeNode for now
int32 TreeNode::GetVectorCost() const {
  // If we reorder stmts instead of vectorizing stmts, lane may be invalid, we check it here
  if (GetLane() % 2 != 0) {
    return kHugeCost;
  }
  ASSERT(IsLoad() || IsStore(), "must be");
  ASSERT(CanVectorized(), "must be");
  int32 cost = 1;
#if defined(TARGAARCH64) && TARGAARCH64
  int32 offset = GetMinMemLoc()->offset;
  if (!IsVecStrLdrOffsetValid(GetBundleSize(), offset)) {
    cost += 1;
  }
#else
  cost += 1;
#endif
  return cost;
}

int32 TreeNode::GetExternalUserCost() const {
  int32 externalUserCost = 0;
  auto &exprUseInfo = tree.GetUseInfo();
  for (auto *scalarStmt : GetStmts()) {
    auto *lhs = scalarStmt->GetLHS();
    auto *useItems = exprUseInfo.GetUseSitesOfExpr(lhs);
    if (useItems == nullptr) {
      continue;
    }
    for (auto &use : *useItems) {
      if (!use.IsUseByStmt()) {
        externalUserCost += 1;
        continue;
      }
      auto &internalUseStmts = GetParent()->GetStmts();
      if (std::find(internalUseStmts.begin(), internalUseStmts.end(), use.GetStmt()) == internalUseStmts.end()) {
        // Found external user in current BB, we can delete the scalar stmt, extra cost should be considered.
        externalUserCost += 1;
      }
    }
  }
  return externalUserCost;
}

#if defined(TARGAARCH64) && TARGAARCH64
// Wether the constval is in the range of insn's imm field
static bool IsConstvalInRangeOfInsnImm(ConstMeExpr *constExpr, Opcode op, uint32 typeBitSize) {
  if (constExpr->GetConstVal()->GetKind() != kConstInt) {
    return false;
  }
  auto val = constExpr->GetExtIntValue();
  switch (op) {
    case OP_add:
    case OP_sub:
      return maplebe::Imm12BitValid(val);
    case OP_band:
    case OP_bior:
    case OP_bxor: {
      if (typeBitSize == 32) {
        return maplebe::Imm12BitMaskValid(val);
      } else if (typeBitSize == 64) {
        return maplebe::Imm13BitMaskValid(val);
      }
      break;
    }
    default:
      break;
  }
  return false;
}
#endif

int32 TreeNode::GetCost() const {
  bool isLoadOrStore = IsLoad() || IsStore();
  if (IsPrimitiveFloat(GetType()) && !isLoadOrStore) {
    // Currently CodeGen can not support set_element/get_element for float type.
    // So return a huge cost that prevent slp tree from being vectorized.
    // It is OK for float type to load/store.
    return kHugeCost;
  }
  if (!CanVectorized()) {
    return static_cast<int32>(GetLane());
  }
  if (op == OP_constval) {
    if (SameExpr()) {
#if defined(TARGAARCH64) && TARGAARCH64
      auto typeBitSize = GetPrimTypeBitSize(GetType());
      if (IsConstvalInRangeOfInsnImm(static_cast<ConstMeExpr*>(exprs[0]), GetParent()->GetOp(), typeBitSize)) {
        return 2;  // no extra constval assign insn is needed, for example, sub x1, x1, #1
      }
#endif
      return 1;
    } else {
      return static_cast<int32>(GetLane());
    }
  }
  int32 cost = -(static_cast<int32>(GetLane() - 1));
  if (op == OP_ashr) {
    // aarch64 only vector shl(register), lowering vector shr will introduce an extra vector neg instruction.
    cost += 1;
  }
  if (isLoadOrStore) {
    int32 vectorCost = GetVectorCost();
    int32 scalarCost = GetScalarCost();
    cost = vectorCost - scalarCost;
    if (IsLoad()) {  // only consider load treeNode's external user cost for now
      int32 externalUserCost = GetExternalUserCost();
      if (externalUserCost != 0) {
        cost += externalUserCost;
      }
    }
  }
  return cost;
}

int32 SLPTree::GetCost() const {
  // Special case for store 0, stp wzr/xzr is better because no need to prepare zero vector register
  if (GetSize() == 2 && treeNodeVec[1]->GetOp() == OP_constval && treeNodeVec[1]->SameExpr()) {
    auto *constVal = static_cast<ConstMeExpr*>(treeNodeVec[1]->GetExprs()[0])->GetConstVal();
    if (constVal->GetKind() == kConstInt && static_cast<MIRIntConst*>(constVal)->GetValue() == 0) {
      // two 'str wzr' can be merged into one 'str xzr' now
      bool canMergeTwoStrWzr = GetLane() == 2 && GetPrimTypeBitSize(GetType()) == 32 &&
          treeNodeVec[0]->GetOp() == OP_iassign;
      if (!canMergeTwoStrWzr) {
        return kHugeCost;
      }
    }
  }
  int32 cost = 0;
  for (auto *treeNode : treeNodeVec) {
    cost += treeNode->GetCost();
  }
  return cost;
}

void PrintTreeNode(const TreeNode *node, MeFunction &func, bool inDot) {
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
  BlockScheduling(MeFunction &f, BB *block, MemoryHelper &memHelper, bool dbg)
      : func(f), bb(block), os(LogInfo::MapleLogger()), memoryHelper(memHelper), debug(dbg) {}

  MeFunction &func;
  BB *bb = nullptr;
  std::vector<uint32> stmtIdBuffer;  // we should recover stmtId before any IR transformation
  std::vector<MeStmt*> stmtVec;      // map position to stmt
  std::ostream &os;
  MeExprUseInfo *exprUseInfo = nullptr;
  MemoryHelper &memoryHelper;
  bool extendUseInfo = false;
  bool irModified = false;  // If IR is modified when scheduled, we must save scheduling result
  bool debug = false;
  bool rebuildUseInfo = true;

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

  bool IsRegionEmpty(const MeStmt *beginStmt, const MeStmt *endStmt) const {
    return beginStmt == endStmt;
  }

  BB *GetBB() {
    return bb;
  }

  void Init() {
    irModified = false;
    SaveAndRenumberStmtIds(*bb, stmtIdBuffer);
    stmtVec.clear();
    stmtVec.reserve(stmtIdBuffer.size());
    auto *stmt = bb->GetFirstMe();
    while (stmt != nullptr) {
      stmtVec.push_back(stmt);
      stmt = stmt->GetNext();
    }
  }

  bool IsStmtInRegion(MeStmt &stmt, MeStmt *beginStmt, MeStmt *endStmt) const {
    if (IsRegionEmpty(beginStmt, endStmt)) {
      return false;
    }
    if (stmt.GetBB() != bb) {
      return false;
    }
    auto id = GetOrderId(&stmt);
    if (id >= GetOrderId(beginStmt) && id <= GetOrderId(endStmt)) {
      return true;
    }
    return false;
  }

  MeStmt *FindAnyStmtInRegion(const std::vector<MeStmt*> &meStmtVec, MeStmt *beginStmt, MeStmt *endStmt) const {
    for (auto *stmt : meStmtVec) {
      if (IsStmtInRegion(*stmt, beginStmt, endStmt)) {
        return stmt;
      }
    }
    return nullptr;
  }

  bool IsAnyStmtInRegion(const std::vector<MeStmt*> &meStmtVec, MeStmt *beginStmt, MeStmt *endStmt) const {
    for (auto *stmt : meStmtVec) {
      if (IsStmtInRegion(*stmt, beginStmt, endStmt)) {
        return true;
      }
    }
    return false;
  }

  bool IsOstUsedByStmt(OriginalSt *ost, const MeStmt &stmt) const;

  void ScheduleStmtBefore(MeStmt *stmt, MeStmt *anchor, bool needRectifyChiList = false) {
    CHECK_FATAL(stmt->GetBB() == anchor->GetBB(), "must belong to same BB");
    int32 stmtOrderId = static_cast<int32>(GetOrderId(stmt));
    int32 anchorOrderId = static_cast<int32>(GetOrderId(anchor));
    CHECK_FATAL(anchorOrderId < stmtOrderId, "anchor must be before stmt");
    if (needRectifyChiList) {
      irModified = SwapChiForSameOstIfNeeded(*stmt, *anchor);
    }
    // should use signed id
    for (int32 id = stmtOrderId - 1; id >= anchorOrderId; --id) {
      SwapStmts(stmt, stmtVec[static_cast<uint32>(id)]);
    }
    if (debug) {
      VerifyScheduleResult(GetOrderId(stmt), GetOrderId(anchor));
    }
  }

  bool CanScheduleStmtBefore(MeStmt *stmt, MeStmt *anchor, SchedFailReason &failReason);
  bool CanScheduleAssignWithoutOverlapBefore(MeStmt *stmt, MeStmt *anchor);
  bool TryScheduleTwoStmtsTogether(MeStmt *first, MeStmt *second);
  void VerifyScheduleResult(uint32 firstOrderId, uint32 lastOrderId);
  // Collect use info from `begin` to `end` (not include `end`) in current BB
  // These use info are needed by dependency analysis and stmt scheduling
  void RebuildUseInfo(MapleAllocator &alloc);
  void ExtendUseInfo();
};

MeExprUseInfo &SLPTree::GetUseInfo() {
  blockScheduling.ExtendUseInfo();
  return *blockScheduling.exprUseInfo;
}

void BlockScheduling::DumpStmtVec() const {
  LogInfo::MapleLogger() << "--- [ DumpStmtVec ] ---" << std::endl;
  for (auto *stmt : stmtVec) {
    if (stmt == nullptr) {
      continue;
    }
    os << "  <" << GetOrderId(stmt) << "> LOC " << stmt->GetSrcPosition().LineNum();
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
  if (expr->IsScalar()) {
    // we don't need to consider defPhi, we only focus on BB level use-def
    auto *defStmt = static_cast<ScalarMeExpr*>(expr)->GetDefByMeStmt();
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
void GetDependencyStmts(MeStmt *stmt, std::vector<MeStmt*> &depsOfOpnd, std::vector<MeStmt*> &depsOfChiRhs,
    std::vector<MeStmt*> &depsOfMu) {
  for (size_t i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
    auto *opnd = stmt->GetOpnd(i);
    GetDependencyStmts(opnd, depsOfOpnd);
  }
  auto *chiList = stmt->GetChiList();
  if (chiList != nullptr) {
    for (auto &chiNodePair : std::as_const(*chiList)) {
      auto *chiNode = chiNodePair.second;
      auto *defStmt = chiNode->GetRHS()->GetDefByMeStmt();
      if (defStmt != nullptr) {
        depsOfChiRhs.push_back(defStmt);
      }
    }
  }

  auto *muList = stmt->GetMuList();
  if (muList != nullptr) {
    for (auto &muPair : std::as_const(*muList)) {
      auto *mu = muPair.second;
      auto *defStmt = mu->GetDefByMeStmt();
      if (defStmt != nullptr) {
        depsOfMu.push_back(defStmt);
      }
    }
  }
}

void GetOstsUsed(const MeExpr &expr, std::unordered_set<OriginalSt*> &ostsUsed) {
  Opcode op = expr.GetOp();
  if (op == OP_regread) {
    // we don't need to consider defPhi, we only focus on BB level use-def
    auto *ost = static_cast<const RegMeExpr&>(expr).GetOst();
    ostsUsed.insert(ost);
  }
  for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
    auto *opnd = expr.GetOpnd(i);
    GetOstsUsed(*opnd, ostsUsed);
  }
}

void GetOstsUsed(const MeStmt &stmt, std::unordered_set<OriginalSt*> &ostsUsed) {
  // We only consider stmt opnd, skip chiList, because there is always no chi for preg
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
    auto *opnd = stmt.GetOpnd(i);
    GetOstsUsed(*opnd, ostsUsed);
  }
}

void GetUseStmtsOfChiRhs(const MeExprUseInfo &useInfo, MeStmt *stmt, std::vector<MeStmt*> &useStmts) {
  auto *chiList = stmt->GetChiList();
  if (chiList == nullptr) {
    return;
  }
  for (auto &chiNodePair : std::as_const(*chiList)) {
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

void BlockScheduling::VerifyScheduleResult(uint32 firstOrderId, uint32 lastOrderId) {
  for (auto orderId = firstOrderId; orderId <= lastOrderId; ++orderId) {
    auto *stmt = stmtVec[orderId];
    // Only check iassign chiList for now
    if (stmt->GetOp() == OP_iassign) {
      continue;
    }
    auto *chiList = static_cast<IassignMeStmt*>(stmt)->GetChiList();
    if (chiList == nullptr) {
      continue;
    }
    for (auto chiPair : *chiList) {
      auto *chi = chiPair.second;
      CHECK_FATAL(chi->GetBase() == stmt, "chi base mismatch");
      auto *defStmt = chi->GetRHS()->GetDefByMeStmt();
      if (defStmt == nullptr || defStmt->GetBB() != stmt->GetBB()) {
        continue;
      }
      CHECK_FATAL(GetOrderId(defStmt) < GetOrderId(stmt), "def first, use second");
    }
  }
}

// `end` may be nullptr
void BlockScheduling::RebuildUseInfo(MapleAllocator &alloc) {
  if (!rebuildUseInfo) {
    return;
  }
  if (!exprUseInfo) {
    exprUseInfo = alloc.New<MeExprUseInfo>(alloc.GetMemPool());
    exprUseInfo->SetUseSites(alloc.New<MapleVector<ExprUseInfoPair>>(alloc.Adapter()));
  }
  exprUseInfo->GetUseSites().clear();
  exprUseInfo->CollectUseInfoInBB(bb);
  exprUseInfo->SetState(kUseInfoOfScalar);
  rebuildUseInfo = false;
}

void BlockScheduling::ExtendUseInfo() {
  if (extendUseInfo) {
    return;
  }
  ASSERT_NOT_NULL(exprUseInfo);
  for (auto *succ : bb->GetSucc()) {
    exprUseInfo->CollectUseInfoInBB(succ);
  }
  extendUseInfo = true;
}

bool BlockScheduling::IsOstUsedByStmt(OriginalSt *ost, const MeStmt &stmt) const {
  std::unordered_set<OriginalSt*> ostsUsed;
  GetOstsUsed(stmt, ostsUsed);
  return ostsUsed.find(ost) != ostsUsed.end();
}

// `anchor` must be before `stmt`
bool BlockScheduling::CanScheduleStmtBefore(MeStmt *stmt, MeStmt *anchor, SchedFailReason &failReason) {
  CHECK_FATAL(stmt->GetBB() == anchor->GetBB(), "must belong to same BB");
  auto stmtOrderId = GetOrderId(stmt);
  auto anchorOrderId = GetOrderId(anchor);
  CHECK_FATAL(anchorOrderId < stmtOrderId, "anchor must be before stmt");
  std::vector<MeStmt*> depsOfOpnd;
  std::vector<MeStmt*> depsOfChiRhs;
  std::vector<MeStmt*> depsOfMu;
  GetDependencyStmts(stmt, depsOfOpnd, depsOfChiRhs, depsOfMu);
  // check opndDep
  MeStmt *found = FindAnyStmtInRegion(depsOfOpnd, anchor, stmt);  // range: [anchor, stmt)
  if (found != nullptr) {
    failReason = SchedFailReason::kSchedFailOpndDep;
    SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << "'s opnd depends on stmt" << GetOrderId(found) << std::endl);
    return false;
  }
  // check muDep
  found = FindAnyStmtInRegion(depsOfMu, anchor, stmt);
  if (found != nullptr) {
    failReason = SchedFailReason::kSchedFailMuDep;
    SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << "'s mu depends on stmt" << GetOrderId(found) << std::endl);
    return false;
  }
  // check chiRhsDep
  found = FindAnyStmtInRegion(depsOfChiRhs, anchor, stmt);
  if (found != nullptr) {
    failReason = SchedFailReason::kSchedFailChiRhsDep;
    SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << "'s chiRhs depends on stmt" << GetOrderId(found) << std::endl);
    return false;
  }

  // check chiRhsUse
  std::vector<MeStmt*> useChiRhsStmts;
  GetUseStmtsOfChiRhs(*exprUseInfo, stmt, useChiRhsStmts);
  found = FindAnyStmtInRegion(useChiRhsStmts, anchor, stmt);
  if (found != nullptr) {
    failReason = SchedFailReason::kSchedFailChiRhsUse;
    SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << " chi rhs is used by stmt" << GetOrderId(found) << std::endl);
    return false;
  }
  // Identify WAR (Write After Read) for a same preg
  if (stmt->GetOp() == OP_regassign) {
    auto *defOst = static_cast<AssignMeStmt*>(stmt)->GetLHS()->GetOst();
    for (size_t id = anchorOrderId; id < stmtOrderId; ++id) {
      if (IsOstUsedByStmt(defOst, *stmtVec[id])) {
        failReason = SchedFailReason::kSchedFailPregWAR;
        SLP_DEBUG(os << "Found WAR: stmt" << GetOrderId(stmtVec[id]) << " uses ost ");
        if (debug) {
          defOst->Dump();
          os << " that will defined by stmt" << GetOrderId(stmt) << std::endl;
        }
        return false;
      }
    }
  }
  failReason = SchedFailReason::kSchedSuccess;
  return true;
}

// Both `anchor` and `stmt` must be iassign/dassign stmt and must be adjacent
bool BlockScheduling::CanScheduleAssignWithoutOverlapBefore(MeStmt *stmt, MeStmt *anchor) {
  bool bothDassign = (stmt->GetOp() == OP_dassign && anchor->GetOp() == OP_dassign);
  bool bothIassign = (stmt->GetOp() == OP_iassign && anchor->GetOp() == OP_iassign);
  if (!bothDassign && !bothIassign) {
    return false;
  }
  CHECK_FATAL(stmt->GetBB() == anchor->GetBB(), "must belong to same BB");
  auto stmtOrderId = GetOrderId(stmt);
  auto anchorOrderId = GetOrderId(anchor);
  // Two stmts must be adjacent:
  //   id     : anchor
  //   id + 1 : stmt
  if (stmtOrderId - 1 != anchorOrderId) {
    return false;
  }
  MeExpr *lhsAnchor = nullptr;
  MeExpr *lhsStmt = nullptr;
  if (bothDassign) {
    lhsAnchor = static_cast<DassignMeStmt*>(anchor)->GetLHS();
    lhsStmt = static_cast<DassignMeStmt*>(stmt)->GetLHS();
  } else if (bothIassign) {
    lhsAnchor = static_cast<IassignMeStmt*>(anchor)->GetLHSVal();
    lhsStmt = static_cast<IassignMeStmt*>(stmt)->GetLHSVal();
  }
  if (PrimitiveType(lhsAnchor->GetPrimType()).IsVector() ||
      PrimitiveType(lhsStmt->GetPrimType()).IsVector()) {
    return false;
  }
  auto *firstMemLoc = memoryHelper.GetMemLoc(*lhsAnchor);
  auto *secondMemLoc = memoryHelper.GetMemLoc(*lhsStmt);
  CHECK_NULL_FATAL(firstMemLoc);
  CHECK_NULL_FATAL(secondMemLoc);
  if (!MemoryHelper::HaveSameBase(*firstMemLoc, *secondMemLoc) ||
      !MemoryHelper::MustHaveNoOverlap(*firstMemLoc, *secondMemLoc)) {
    return false;
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
    SchedFailReason failReason = SchedFailReason::kSchedSuccess;
    if (!CanScheduleStmtBefore(stmt, first, failReason)) {
      SLP_DEBUG(os << "stmt" << GetOrderId(stmt) << " can not be scheduled before stmt" <<
          GetOrderId(first) << std::endl);
      if (debug) {
        for (auto i = GetOrderId(first); i <= GetOrderId(stmt); ++i) {
          os << "  <" << i << "> LOC " << stmtVec[i]->GetSrcPosition().LineNum();
          stmtVec[i]->Dump(func.GetIRMap());
        }
      }
      // Try schedule iassign/dassign stmts without overlap
      if (failReason == SchedFailReason::kSchedFailChiRhsDep && CanScheduleAssignWithoutOverlapBefore(stmt, first)) {
        ScheduleStmtBefore(stmt, first, true);
        if (debug) {
          os << "Schedule iassign/dassign stmts without overlap successfully\nAfter scheduled:" << std::endl;
          // orderIds have been swapped now
          for (auto i = GetOrderId(stmt); i <= GetOrderId(first); ++i) {
            os << "  <" << i << "> LOC " << stmtVec[i]->GetSrcPosition().LineNum();
            stmtVec[i]->Dump(func.GetIRMap());
          }
        }
        continue;
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
    case PTY_u32:
    case PTY_a32: {
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

#define GET_VECTOR_INTRN_8BIT(OP)                      \
  VECTOR_INTRN_CASE(OP, v16i8)                         \
  VECTOR_INTRN_CASE(OP, v16u8)                         \
  VECTOR_INTRN_CASE(OP, v8i8)                          \
  VECTOR_INTRN_CASE(OP, v8u8)

#define GET_VECTOR_INTRN_16BIT(OP)                     \
  VECTOR_INTRN_CASE(OP, v8i16)                         \
  VECTOR_INTRN_CASE(OP, v8u16)                         \
  VECTOR_INTRN_CASE(OP, v4i16)                         \
  VECTOR_INTRN_CASE(OP, v4u16)

#define GET_VECTOR_INTRN_32BIT(OP)                     \
  VECTOR_INTRN_CASE(OP, v4i32)                         \
  VECTOR_INTRN_CASE(OP, v2i32)                         \
  VECTOR_INTRN_CASE(OP, v4u32)                         \
  VECTOR_INTRN_CASE(OP, v2u32)

#define GET_VECTOR_INTRN_NYI                           \
  default:                                             \
    CHECK_FATAL(false, "NYI");

#define GET_VECTOR_INTRN(OP)                           \
  GET_VECTOR_INTRN_32BIT(OP)                           \
  GET_VECTOR_INTRN_16BIT(OP)                           \
  GET_VECTOR_INTRN_8BIT(OP)                            \
  VECTOR_INTRN_CASE(OP, v2i64)                         \
  VECTOR_INTRN_CASE(OP, v2u64)                         \
  VECTOR_INTRN_CASE(OP, v2f32)                         \
  VECTOR_INTRN_CASE(OP, v4f32)                         \
  VECTOR_INTRN_CASE(OP, v2f64)                         \
  GET_VECTOR_INTRN_NYI

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

MIRIntrinsicID GetVectorReverse32IntrnId(PrimType type) {
  MIRIntrinsicID intrinsic;
  switch (type) {
    GET_VECTOR_INTRN(vector_reverse);
  }
  return intrinsic;
}

MIRIntrinsicID GetVectorReverse16IntrnId(PrimType type) {
  MIRIntrinsicID intrinsic;
  switch (type) {
    GET_VECTOR_INTRN_8BIT(vector_reverse16)
    GET_VECTOR_INTRN_NYI
  }
  return intrinsic;
}

MIRIntrinsicID GetVectorReverse64IntrnId(PrimType type) {
  MIRIntrinsicID intrinsic;
  switch (type) {
    GET_VECTOR_INTRN_32BIT(vector_reverse64)
    GET_VECTOR_INTRN_16BIT(vector_reverse64)
    GET_VECTOR_INTRN_8BIT(vector_reverse64)
    GET_VECTOR_INTRN_NYI
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
  void AddSeedStore(MeStmt *stmt, MemLoc *memLoc);
  void FilterAndSortSeedStmts();
  bool TrySplitSeedStmts();

  void VectorizeStoreVecMap();
  void VectorizeStores(StoreVec &storeVec);
  void VectorizeCompatibleStores(StoreVec &storeVec, uint32 begin, uint32 end);
  void VectorizeConsecutiveStores(StoreVec &storeVec, uint32 begin, uint32 end);
  void DoCodeMotion(bool vectorized);
  bool DoVectorizeSlicedStores(StoreVec &storeVec, uint32 begin, uint32 end, bool onlySchedule = false);

  bool TryScheduleTogehter(const std::vector<MeStmt*> &stmts);
  void BuildTree(std::vector<MeStmt*> &stmts);
  TreeNode *BuildTreeRec(std::vector<ExprWithDef*> &exprVec, uint32 depth, TreeNode *parentNode);

  bool VectorizeTreeNode(TreeNode *treeNode);
  bool DoVectTreeNodeIvar(TreeNode *treeNode);
  bool DoVectTreeNodeConstval(TreeNode *treeNode);
  bool DoVectTreeNodeBinary(TreeNode *treeNode);
  bool DoVectTreeNodeNary(TreeNode *treeNode);
  bool DoVectTreeNodeNaryReverse(TreeNode *treeNode, MIRIntrinsicID intrnId);
  bool DoVectTreeNodeIassign(TreeNode *treeNode);
  bool DoVectTreeNodeGatherNeeded(TreeNode &treeNode);
  bool VectorizeSLPTree();
  void SetStmtVectorized(MeStmt &stmt);
  bool IsStmtVectorized(MeStmt &stmt) const;
  MIRType* GenMergedType(PrimType primType, uint8 lanes) const;

  void MarkStmtsVectorizedInTree();
  void ReplaceStmts(const std::vector<MeStmt*> &origStmtVec, std::list<MeStmt*> &newStmtList);
  void CodeMotionVectorize();
  void CodeMotionReorderStoresAndLoads();
  void CodeMotionSaveSchedulingResult();

 private:
  MapleAllocator alloc;
  MapleAllocator *tmpAlloc = nullptr;
  MemoryHelper memoryHelper;
  MeFunction &func;
  IRMap &irMap;
  Dominance &dom;
  std::map<MemBasePtr*, StoreVec, MemBasePtrCmp> storeVecMap;  // key is base point part of store lhs ivar
  std::unordered_set<MeStmt*> vectorizedStmts;  // Vectorized stmts in current BB, to avoid vectorize a stmt twice
  BlockScheduling *blockScheduling = nullptr;
  BB *currBB = nullptr;
  bool splited = false;     // whether the stmts in currBB have been splited
  bool mergeToVecType = true;
  SLPTree *tree = nullptr;
  bool debug = false;
  std::ostream &os;
};

void SLPVectorizer::Run() {
  SLP_DEBUG(os << "Before processing func " << func.GetName() << std::endl);
  const auto &rpo = dom.GetReversePostOrder();
  for (auto *bb : rpo) {
    ProcessBB(*func.GetCfg()->GetBBFromID(BBId(bb->GetID())));
  }
  if (localSymOffsetTab != nullptr) {
    delete localSymOffsetTab;
    localSymOffsetTab = nullptr;
  }
  SLP_DEBUG(os << "After processing func " << func.GetName() << std::endl);
}

void SLPVectorizer::ProcessBB(BB &bb) {
  currBB = &bb;
  auto bs = BlockScheduling(func, currBB, memoryHelper, debug);
  blockScheduling = &bs;
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
    // If the number of stores exceeds the threshold, slp will not be performed for build time considerations
    if (storeVec.size() > kMaxNumStores) {
      continue;
    }
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
    VectorizeCompatibleStores(storeVec, static_cast<uint32_t>(i), static_cast<uint32_t>(j));
    i = j;
  }
}

// Try to vectorize storeVec[begin, end), in which all stores are compatible
void SLPVectorizer::VectorizeCompatibleStores(StoreVec &storeVec, uint32 begin, uint32 end) {
  SLP_DEBUG(os << "== VectorizeCompatibleStores [" << begin << ", " << end << ")" << std::endl);
  for (uint32 i = begin; i < end;) {
    uint32 j = i + 1;
    for (; j < end; ++j) {
      if (!MemoryHelper::IsConsecutive(*storeVec[j - 1]->storeMem, *storeVec[j]->storeMem, false)) {
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
// The first is VectorFactor, the second is ScalarFactor
std::pair<uint32, uint32> GetMaxFactors(PrimType scalarType) {
  CHECK_FATAL(!PrimitiveType(scalarType).IsVector(), "must be");
  auto typeSize = GetPrimTypeSize(scalarType);
  CHECK_FATAL(typeSize != 0, "unknown type size");
  return { 16 / typeSize, 8 / typeSize };  // max vector regsize: 16byte, max scalar regsize: 8byte
}

std::pair<uint32, uint32> GetMinFactors(PrimType scalarType) {
  CHECK_FATAL(!PrimitiveType(scalarType).IsVector(), "must be");
  auto typeSize = GetPrimTypeSize(scalarType);
  CHECK_FATAL(typeSize != 0, "unknown type size");
  return { std::max(8 / typeSize, 2u), 2u };  // min vector regsize: 8byte, minSf is always 2
}

bool IsTwoStoreWrapperSame(StoreWrapper *s1, StoreWrapper *s2) {
  return MemoryHelper::IsSame(*s1->storeMem, *s2->storeMem) && s1->stmt->GetRHS() == s2->stmt->GetRHS();
}

bool IsSurroundedByRepeatedStore(SLPVectorizer::StoreVec &storeVec, uint32 first, uint32 last) {
  auto size = storeVec.size();
  if (first > 0 && IsTwoStoreWrapperSame(storeVec[first - 1], storeVec[first])) {
    return true;
  }
  if (last < size - 1 && IsTwoStoreWrapperSame(storeVec[last], storeVec[last + 1])) {
    return true;
  }
  return false;
}

// Try to vectorize storeVec[begin, end), in which all stores are compatible and consecutive
void SLPVectorizer::VectorizeConsecutiveStores(StoreVec &storeVec, uint32 begin, uint32 end) {
  SLP_DEBUG(os << "==== VectorizeConsecutiveStores [" << begin << ", " << end << ")" << std::endl);
  PrimType scalarType = storeVec[begin]->storeMem->type->GetPrimType();
  const auto &maxFactors = GetMaxFactors(scalarType);
  const auto &minFactors = GetMinFactors(scalarType);
  const uint32 maxVf = maxFactors.first;
  const uint32 maxSf = maxFactors.second;
  const uint32 minVf = minFactors.first;
  const uint32 minSf = minFactors.second;
  SLP_DEBUG(os << "minVf: " << minVf << ", maxVf: " << maxVf <<
      "; minSf: " << minSf << ", maxSf: " << maxSf << std::endl);
  uint32 endIdx = end;
  bool tried = false;
  std::vector<bool> vectorized(storeVec.size(), false);
  for (uint32 factor = maxVf; factor >= minSf; factor /= k2BitSize) {
    for (uint32 i = endIdx; i >= factor && i - factor >= begin;) {
      // Make sure no scalar stores have been vectorized
      // When stmts are vectorized successfully, we will set them vectorizd
      // to avoid vectorize them again.
      mergeToVecType = factor >= minVf;
      uint32 currBegin = i - factor;
      bool allStoresNotVectorized = true;
      for (uint32 k = currBegin; k < i; ++k) {
        if (vectorized[k]) {
          allStoresNotVectorized = false;
          break;
        }
      }
      if (!allStoresNotVectorized) {
        i -= factor;  // can use --i for more optimization at the cost of build time
        continue;
      }
      if (IsSurroundedByRepeatedStore(storeVec, currBegin, i - 1)) {  // we trust hdse
        SLP_FAILURE_DEBUG(os << "surrounded by repeated stores" << std::endl);
        i -= factor;
        continue;
      }
      bool res = DoVectorizeSlicedStores(storeVec, currBegin, i);
      tried = true;
      if (res) {
        for (uint32 j = currBegin; j < i; ++j) {
          vectorized[j] = true;
        }
        if (i == endIdx) {
          endIdx -= factor;
        }
        i -= factor;
        continue;
      }
      i -= factor;  // can use --i for more optimization at the cost of build time
    }
  }
  if (!tried) {
    SLP_DEBUG(os << "can't find appropriate vector factor, minVf = " << minVf << ", maxVf = " << maxVf << std::endl);
    SLP_DEBUG(os << "vectorize failure, try to reorder load/store" << std::endl);
    DoVectorizeSlicedStores(storeVec, begin, end, true);
  }
}

void SLPVectorizer::DoCodeMotion(bool vectorized) {
  if (vectorized) {
    SLP_DEBUG(os << "CodeMotionVectorize" << std::endl);
    MarkStmtsVectorizedInTree();
    CodeMotionVectorize();
    blockScheduling->rebuildUseInfo = true;
    return;
  }

  if (blockScheduling->irModified) {
    // If IR have been modified after scheduling (such as chiList), scheduling result must be saved
    SLP_DEBUG(os << "CodeMotionSaveSchedulingResult" << std::endl);
    CodeMotionSaveSchedulingResult();
    blockScheduling->rebuildUseInfo = true;
    return;
  }

  if (tree->CanTreeUseStp()) {
    SLP_DEBUG(os << "CodeMotionReorderStoresAndLoads" << std::endl);
    CodeMotionReorderStoresAndLoads();
    blockScheduling->rebuildUseInfo = true;
    return;
  }

  if (tree->GetSize() > k2BitSize) {
    bool hasLoadStorePair = false;
    for (auto *treeNode : tree->GetNodes()) {
      if (treeNode->CanNodeUseLoadStorePair()) {
        hasLoadStorePair = true;
        break;
      }
    }
    if (hasLoadStorePair) {
      SLP_DEBUG(os << "CodeMotionSaveSchedulingResult" << std::endl);
      CodeMotionSaveSchedulingResult();  // Save scheduling result to put load/store pair together
      blockScheduling->rebuildUseInfo = true;
    }
  }
}

// input: consecutive compatible stores
// Try to vectorize storeVec[begin, begin + vf)
bool SLPVectorizer::DoVectorizeSlicedStores(StoreVec &storeVec, uint32 begin, uint32 end, bool onlySchedule) {
  uint32 vf = end - begin;
  SLP_DEBUG(os << "====== DoVectorizeSlicedStores [" << begin << ", " << end << ") vf = " << vf << std::endl);
  // Init blockScheduling if needed
  SLP_DEBUG(os << "Init BlockScheduling for BB" << currBB->GetBBId() << std::endl);
  blockScheduling->Init();
  // Rebuild currBB use info before building tree
  // Rebuild if needed
  blockScheduling->RebuildUseInfo(*tmpAlloc);
  std::vector<MeStmt*> stmts;
  for (uint32 i = begin; i < end; ++i) {
    stmts.push_back(storeVec[i]->stmt);
  }
  BuildTree(stmts);
  if (onlySchedule) {
    SLP_DEBUG(os << "onlySchedule save result" << std::endl);
    CodeMotionSaveSchedulingResult();
    return true;
  }
  auto vectorized = VectorizeSLPTree();
  DoCodeMotion(vectorized);
  // If we can use stp instruction, don't need to replace scalar stmts with vector stmts
  return vectorized;
}

void SLPVectorizer::CodeMotionSaveSchedulingResult() {
  // We should call RecoverStmtIds before any real code motion
  RecoverStmtIds(*currBB, blockScheduling->stmtIdBuffer);
  currBB->GetMeStmts().clear();
  for (auto *stmt : blockScheduling->GetStmtVec()) {
    stmt->SetPrev(nullptr);
    stmt->SetNext(nullptr);
    currBB->AddMeStmtLast(stmt);
  }
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

static MeStmt *TryReuseRepeatedOutStmts(const std::vector<MeStmt*> &existingNewStmts,
    const std::vector<MeStmt*> &newStmts, IRMap &irMap) {
  if (existingNewStmts.size() != 1 || newStmts.size() != 1) {
    return nullptr;
  }
  auto *existingStmt = existingNewStmts[0];
  auto *repeatedStmt = newStmts[0];
  if (existingStmt->GetOp() != OP_regassign || repeatedStmt->GetOp() != OP_regassign) {
    return nullptr;
  }
  CHECK_FATAL(existingStmt->GetRHS() == repeatedStmt->GetRHS(), "expect same rhs");
  auto *reuseStmt =
      irMap.CreateAssignMeStmt(*repeatedStmt->GetLHS(), *existingStmt->GetLHS(), *existingStmt->GetBB());
  return reuseStmt;
}

static void HandleRepeatedOutStmts(std::vector<MeStmt*> &existingNewStmts,
    std::vector<MeStmt*> &newStmts, IRMap &irMap) {
  auto *reuseStmt = TryReuseRepeatedOutStmts(existingNewStmts, newStmts, irMap);
  if (reuseStmt != nullptr) {
    existingNewStmts.push_back(reuseStmt);
  } else {
    existingNewStmts.insert(existingNewStmts.end(), newStmts.begin(), newStmts.end());
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
      if (keepStmtsExceptIassign && (treeNode->GetOp() == OP_iassign || treeNode->GetOp() == OP_dassign)) {
        toBeRemoved.insert(orderId);  // only remove store scalar stmts
      }
      if (orderId > pos) {
        pos = orderId;
      }
    }
    std::vector<MeStmt*> newStmts;
    for (auto *child : treeNode->GetChildren()) {
      if (child->ConstOrNeedGather()) {
        (void)newStmts.insert(newStmts.end(), child->GetOutStmts().begin(), child->GetOutStmts().end());
      }
    }
    newStmts.insert(newStmts.end(), treeNode->GetOutStmts().begin(), treeNode->GetOutStmts().end());
    if (pos != 0 && !newStmts.empty()) {
      insertPositions.insert(pos);
      auto ret = pos2newStmts.try_emplace(pos, newStmts);
      if (!ret.second) {
        // insert failure, indicating there are same treeNodes
        auto &existingNewStmts = ret.first->second;
        HandleRepeatedOutStmts(existingNewStmts, newStmts, irMap);
      }
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
    } else {
      auto *removeStmt = origStmtVec[i];
      if (removeStmt->GetOp() == OP_iassign) {
        static_cast<IassignMeStmt*>(removeStmt)->GetLHSVal()->SetDefStmt(nullptr);
      }
    }
  }
}

void SLPVectorizer::CodeMotionReorderStoresAndLoads() {
  if (tree->GetSize() == 0) {
    return;
  }
  auto scalarTypeSize = GetPrimTypeBitSize(tree->GetType());
  if (scalarTypeSize != k32BitSize && scalarTypeSize != k64BitSize) {
    return;
  }
  auto lane = tree->GetLane();
  if (lane != k2BitSize) {
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

  bool isRhsLoad = root->GetChildren()[0]->IsLoad();
  std::vector<MeStmt*> loadStmts;
  if (isRhsLoad) {
    for (auto *stmt : root->GetChildren()[0]->GetStmts()) {
      loadStmts.push_back(stmt);
    }
    std::sort(loadStmts.begin(), loadStmts.end(), [](MeStmt *a, MeStmt *b) {
      return GetOrderId(a) < GetOrderId(b);
    });
  }

  // We should call RecoverStmtIds before any real code motion
  RecoverStmtIds(*currBB, blockScheduling->stmtIdBuffer);

  // Do scheduling stores
  for (size_t i = 0; i < lane - 1; ++i) {
    auto *currStmt = storeStmts[i];
    currBB->RemoveMeStmt(currStmt);
    currBB->InsertMeStmtBefore(storeStmts.back(), currStmt);
  }

  if (isRhsLoad) {
    // Do scheduling loads
    // To improve: If we can identify that all loads have been scheduled together before,
    // we can skip the following code to save build time.
    MeStmt *anchor = loadStmts[0];
    for (size_t i = 1; i < lane; ++i) {
      auto *currStmt = loadStmts[i];
      currBB->RemoveMeStmt(currStmt);
      currBB->InsertMeStmtAfter(anchor, currStmt);
      anchor = currStmt;
    }
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
    CHECK_FATAL(storeVec.size() >= k2BitSize, "storeVec with size less than 2 should have been removed before");
    // Split the potentially vectorizable seed stmts
    for (auto *store : storeVec) {
      TrySplitMeStmt(*store->stmt, func, changed);
    }
  }
  return changed;
}

void SLPVectorizer::AddSeedStore(MeStmt *stmt, MemLoc *memLoc) {
  MemBasePtr *key = memLoc->base;
  auto *storeWrapper = alloc.GetMemPool()->New<StoreWrapper>(stmt, memLoc);
  auto it = storeVecMap.find(key);
  if (it != storeVecMap.end()) {
    it->second.push_back(storeWrapper);
  } else {
    storeVecMap[key].push_back(storeWrapper);
  }
}

static bool CanSlpForStoreType(PrimType type) {
  if (PrimitiveType(type).IsVector()) {
    return false;
  }
  if (type == PTY_agg || type == PTY_u1) {
    return false;
  }
  return true;
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
      // Skip volatile store, vector store, agg store, u1 store, bitfield store
      if (lhsIvar->IsVolatile() || !CanSlpForStoreType(lhsIvar->GetPrimType()) ||
          lhsIvarType->GetKind() == kTypeBitField) {
        stmt = next;
        continue;
      }
      MemLoc *memLoc = memoryHelper.GetMemLoc(*lhsIvar);
      AddSeedStore(stmt, memLoc);
    } else if (stmt->GetOp() == OP_dassign) {
      auto *lhs = static_cast<DassignMeStmt*>(stmt)->GetLHS();
      if (lhs->GetOp() != OP_dread || lhs->IsVolatile() || !CanSlpForStoreType(lhs->GetPrimType())) {
        stmt = next;
        continue;
      }
      auto *lhsVar = static_cast<VarMeExpr*>(lhs);
      auto *lhsVarType = lhsVar->GetType();
      if (lhsVarType->GetKind() == kTypeBitField || lhsVar->GetFieldID() == 0) {  // only consider agg field dassign
        stmt = next;
        continue;
      }
      MemLoc *memLoc = memoryHelper.GetMemLoc(*lhsVar);
      AddSeedStore(stmt, memLoc);
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
  for (int32 i = static_cast<int32>(sortedStmts.size()) - 2; i >= 0; --i) { // i should be int
    bool res = blockScheduling->TryScheduleTwoStmtsTogether(sortedStmts[static_cast<uint32>(i)],
                                                            sortedStmts[static_cast<uint32>(i + 1)]);
    if (!res) {
      return false;  // Schedule fail
    }
  }
  return true;  // Schedule succeed
}

// Building SLP tree
// input: sorted stmts by memLoc offset
void SLPVectorizer::BuildTree(std::vector<MeStmt*> &stmts) {
  tree = tmpAlloc->New<SLPTree>(*tmpAlloc, memoryHelper, func, *blockScheduling);
  SLP_DEBUG(os << "Build tree node for " << GetOpName(stmts[0]->GetOp()) << std::endl);
  CHECK_FATAL(stmts.size() >= k2BitSize, "must be");

  if (!TryScheduleTogehter(stmts)) {
    SLP_FAILURE_DEBUG(os << "Scheduling failure" << std::endl);
    return;
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
  // (2) Supported ops/intrinsicops
  if (std::find(supportedOps.begin(), supportedOps.end(), currOp) == supportedOps.end()) {
    SLP_GATHER_DEBUG(os << "unsupported expr op: " << GetOpName(currOp) << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
  }
  if (currOp == OP_intrinsicop) {
    auto intrnId = static_cast<NaryMeExpr*>(firstRealExpr)->GetIntrinsic();
    if (std::find(supportedIntrns.begin(), supportedIntrns.end(), intrnId) == supportedIntrns.end()) {
      SLP_GATHER_DEBUG(os << "unsupported intrinsicop: " << static_cast<int>(intrnId) << std::endl);
      return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
    }
  }

  if (currOp == OP_constval) {
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, true);
  }

  // mul v2u64 and mul v2i64 are not supported by armv8
  // This is target-dependent configuration, maybe we need abstract it TargetInfo
  if (currOp == OP_mul && exprVec.size() == 2 && GetPrimTypeBitSize(firstRealExpr->GetPrimType()) == 64) {
    SLP_GATHER_DEBUG(os << "Unsupported vector mul pattern" << std::endl);
    return tree->CreateTreeNodeByExprs(exprVec, parentNode, false);
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
uint64 ConstructConstants(std::vector<uint64> &constants, uint32 elemSize) {
  uint64 res = 0;
  uint32 shift = 0;
  uint32 maskShift = 64 - elemSize;
  uint64 mask = (static_cast<uint64>(-1) << maskShift) >> maskShift;
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
  PrimType elemType = tree->GetType();
  auto *vecType = GenMergedType(tree->GetType(), static_cast<uint8>(treeNode->GetLane()));
  CHECK_NULL_FATAL(vecType);
  bool useScalarType = tree->CanTreeUseScalarTypeForConstvalIassign();
  ScalarMeExpr *lhsReg = nullptr;
  MeExpr *rhs = nullptr;
  if (useScalarType) {
    if (!mergeToVecType) {
      auto firstConstVal = static_cast<ConstMeExpr*>(treeNode->GetExprs()[0])->GetIntValue();
      if (!treeNode->SameExpr() || firstConstVal != 0) {
        SLP_FAILURE_DEBUG(os << "all constVals must be 0 if mergeToScalarType considering big const performance" <<
            std::endl);
        return false;
      }
    }
    vecType = GetScalarUnsignedTypeBySize(GetPrimTypeBitSize(vecType->GetPrimType()));
    lhsReg = irMap.CreateRegMeExpr(*vecType);
    std::vector<uint64> constants(treeNode->GetExprs().size());
    std::transform(treeNode->GetExprs().cbegin(), treeNode->GetExprs().cend(), constants.begin(),
        [](const MeExpr *expr) {
          return static_cast<uint64>(static_cast<const ConstMeExpr*>(expr)->GetExtIntValue());
    });
    uint64 mergeConstval = ConstructConstants(constants, GetPrimTypeBitSize(elemType));
    rhs = irMap.CreateIntConstMeExpr(static_cast<int64>(mergeConstval), vecType->GetPrimType());
  } else if (treeNode->SameExpr()) {
    if (!mergeToVecType) {
      SLP_DEBUG(os << "vector_from_scalar is not supproted for scalar mergedType" << std::endl);
      return false;
    }
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
    if (!mergeToVecType) {
      SLP_DEBUG(os << "vector_set_element is not supproted for scalar mergedType" << std::endl);
      return false;
    }
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

void SetMuListForVectorIvar(IvarMeExpr &ivar, const TreeNode &treeNode) {
  auto &order = treeNode.GetOrder();
  ivar.GetMuList().resize(order.size(), nullptr);
  for (size_t i = 0; i < order.size(); ++i) {
    uint32 orderIdx = order[i];
    auto *mu = static_cast<IvarMeExpr*>(treeNode.GetMemLocs()[i]->Emit())->GetUniqueMu();
    ivar.SetMuItem(orderIdx, mu);
  }
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
  auto *vecType = GenMergedType(tree->GetType(), static_cast<uint8>(treeNode->GetLane()));
  CHECK_NULL_FATAL(vecType);
  auto *minMem = treeNode->GetMinMemLoc();
  auto *vecPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType);
  auto *addrExpr = minMem->Emit();
  auto *ivarExpr = static_cast<IvarMeExpr*>(addrExpr);
  auto *newBase = ivarExpr->GetBase();
  if (minMem->extraOffset != 0) {
    auto *extraOffsetExpr = irMap.CreateIntConstMeExpr(minMem->extraOffset, PTY_u64);
    newBase = irMap.CreateMeExprBinary(OP_add, PTY_u64, *newBase, *extraOffsetExpr);
  }
  IvarMeExpr newIvar(&irMap.GetIRMapAlloc(), -1, vecType->GetPrimType(), TyIdx(PTY_u64), 0);
  newIvar.SetTyIdx(vecPtrType->GetTypeIndex());
  newIvar.SetPtyp(vecType->GetPrimType());
  newIvar.SetBase(newBase);
  SetMuListForVectorIvar(newIvar, *treeNode);
  addrExpr = irMap.HashMeExpr(newIvar);

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
  auto *vecType = GenMergedType(tree->GetType(), static_cast<uint8>(treeNode->GetLane()));
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

void GetRevInfoFromScalarRevIntrnId(MIRIntrinsicID intrnId, uint32 &rangeBitSize, uint32 &elementBitSize) {
  switch (intrnId) {
    case INTRN_C_rev_4:
      rangeBitSize = 32;
      elementBitSize = 8;
      break;
    case INTRN_C_rev_8:
      rangeBitSize = 64;
      elementBitSize = 8;
      break;
    case INTRN_C_rev16_2:
      rangeBitSize = 16;
      elementBitSize = 8;
      break;
    default:
      CHECK_FATAL(false, "NYI");
      break;
  }
}

bool SLPVectorizer::DoVectTreeNodeNary(TreeNode *treeNode) {
  CHECK_FATAL(treeNode->GetOp() == OP_intrinsicop, "only support intrinsicop for now");
  CHECK_FATAL(treeNode->GetChildren().size() == 1, "must be");
  CHECK_FATAL(!treeNode->GetStmts().empty(), "treeNode need gather?");
  auto *firstRealExpr = treeNode->GetStmts()[0]->GetRHS();
  CHECK_FATAL(firstRealExpr->GetOp() == OP_intrinsicop, "must be");
  auto intrnId = static_cast<NaryMeExpr*>(firstRealExpr)->GetIntrinsic();
  if (std::find(supportedIntrns.begin(), supportedIntrns.end(), intrnId) == supportedIntrns.end()) {
    SLP_FAILURE_DEBUG(os << "unsupported intrinsic" << std::endl);
    return false;
  }
  switch (intrnId) {
    case INTRN_C_rev_4:
    case INTRN_C_rev_8:
    case INTRN_C_rev16_2:
      return DoVectTreeNodeNaryReverse(treeNode, intrnId);
    default:
      CHECK_FATAL(false, "NYI");
  }
  return false;
}

bool SLPVectorizer::DoVectTreeNodeNaryReverse(TreeNode *treeNode, MIRIntrinsicID intrnId) {
  uint32 rangeBitSize = 0;
  uint32 elementBitSize = 0;
  GetRevInfoFromScalarRevIntrnId(intrnId, rangeBitSize, elementBitSize);
  uint32 opndBitSize = GetPrimTypeBitSize(treeNode->GetType());
  uint32 vecOpndBitSize = static_cast<uint32>(opndBitSize * treeNode->GetLane());
  if (vecOpndBitSize != 64 && vecOpndBitSize != 128) {
    SLP_FAILURE_DEBUG(
        os << "only v64 and v128 are allowed as vector reverse operand, but get " << vecOpndBitSize << std::endl);
    return false;
  }
  auto *vecType = GenMergedType(tree->GetType(), static_cast<uint8>(treeNode->GetLane()));
  CHECK_NULL_FATAL(vecType);
  uint32 newNumLane = vecOpndBitSize / elementBitSize;
  auto isSign = !PrimitiveType(treeNode->GetType()).IsUnsigned();
  auto newElementType = GetIntegerPrimTypeBySizeAndSign(elementBitSize, isSign);
  auto revVecType = GenMergedType(newElementType, static_cast<uint8>(newNumLane));
  MIRIntrinsicID vecIntrnId;
  switch (rangeBitSize) {
    case 16:
      vecIntrnId = GetVectorReverse16IntrnId(revVecType->GetPrimType());
      break;
    case 32:
      vecIntrnId = GetVectorReverse32IntrnId(revVecType->GetPrimType());
      break;
    case 64:
      vecIntrnId = GetVectorReverse64IntrnId(revVecType->GetPrimType());
      break;
    default:
      CHECK_FATAL(false, "should not be here");
      break;
  }
  NaryMeExpr naryExpr(&irMap.GetIRMapAlloc(), kInvalidExprID, treeNode->GetOp(),
      vecType->GetPrimType(), 1, TyIdx(0), vecIntrnId, false);
  for (auto *child : treeNode->GetChildren()) {
    auto *defStmt = child->GetOutStmts().back();
    auto *opnd = defStmt->GetLHS();
    CHECK_NULL_FATAL(opnd);
    naryExpr.PushOpnd(opnd);
  }
  auto *vecExpr = irMap.HashMeExpr(naryExpr);
  auto *lhsReg = irMap.CreateRegMeExpr(*vecType);
  auto *assign = irMap.CreateAssignMeStmt(*lhsReg, *vecExpr, *currBB);
  assign->SetSrcPos(treeNode->GetStmts()[0]->GetSrcPosition());
  treeNode->PushOutStmt(assign);
  if (debug) {
    treeNode->DumpVecStmts(irMap);
  }
  return true;
}

bool SLPVectorizer::DoVectTreeNodeIassign(TreeNode *treeNode) {
  CHECK_FATAL(treeNode->GetChildren().size() == 1, "must be");
  auto *minMem = treeNode->GetMinMemLoc();
  CHECK_NULL_FATAL(minMem);
  // Use root node type for covering the following case:
  //   [(mx943), 4] : u8 = constval : u32
  auto *vecType = GenMergedType(tree->GetType(), static_cast<uint8>(treeNode->GetLane()));
  CHECK_NULL_FATAL(vecType);
  if (tree->CanTreeUseScalarTypeForConstvalIassign()) {
    vecType = GetScalarUnsignedTypeBySize(GetPrimTypeBitSize(vecType->GetPrimType()));
  }
  auto *vecPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType);
  MeExpr *addrExpr = minMem->Emit();
  if (treeNode->GetOp() == OP_dassign) {
    auto &varMeExpr = static_cast<VarMeExpr&>(*addrExpr);
    AddrofMeExpr addrofExpr(-1, PTY_ptr, varMeExpr.GetOst());
    auto *newBase = irMap.HashMeExpr(addrofExpr);
    IvarMeExpr newIvar(&irMap.GetIRMapAlloc(), -1, vecType->GetPrimType(), TyIdx(PTY_u64), 0);
    newIvar.SetTyIdx(vecPtrType->GetTypeIndex());
    newIvar.SetPtyp(vecType->GetPrimType());
    newIvar.SetBase(newBase);
    addrExpr = irMap.HashMeExpr(newIvar);
  } else {
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
    auto *cl = stmt->GetChiList();
    CHECK_NULL_FATAL(cl);
    chiListVec.push_back(cl);
  }
  auto *mergedChiList = MergeChiList(chiListVec, irMap, *vecIassign);
  vecIassign->SetChiList(*mergedChiList);

  treeNode->PushOutStmt(vecIassign);
  if (debug) {
    treeNode->DumpVecStmts(irMap);
  }
  return true;
}

bool SLPVectorizer::DoVectTreeNodeGatherNeeded(TreeNode &treeNode) {
  if (!mergeToVecType) {
    SLP_DEBUG(os << "vector_set_element/vector_from_scalar is not supproted for scalar mergedType" << std::endl);
    return false;
  }
  auto elemType = tree->GetType();
  auto *vecType = GenMergedType(elemType, static_cast<uint8>(treeNode.GetLane()));
  CHECK_NULL_FATAL(vecType);
  auto *vecReg = irMap.CreateRegMeExpr(*vecType);
  auto exprNum = treeNode.GetExprs().size();
  std::vector<std::pair<uint32, MeExpr*>> posExprVec;
  for (size_t i = 0; i < exprNum; ++i) {
    MeExpr *valueExpr = treeNode.GetExprs()[i];
    posExprVec.emplace_back(i, valueExpr);
  }
  MeExpr *vecRegNew = nullptr;
  if (treeNode.SameExpr()) {
    MIRIntrinsicID intrnId = GetVectorFromScalarIntrnId(vecType->GetPrimType());
    // Create vector intrinsicop
    NaryMeExpr naryExpr(&irMap.GetIRMapAlloc(), kInvalidExprID, OP_intrinsicop,
        vecType->GetPrimType(), 1, TyIdx(0), intrnId, false);
    MeExpr *expr0 = treeNode.GetExprs()[0];
    // When tree node type is different from tree type, add cvt if needed
    if (!IsNoCvtNeeded(elemType, expr0->GetPrimType())) {
      expr0 = irMap.CreateMeExprTypeCvt(elemType, expr0->GetPrimType(), *expr0);
    }
    naryExpr.PushOpnd(expr0);
    vecRegNew = irMap.CreateNaryMeExpr(naryExpr);
  } else {
    vecRegNew = BuildExprAfterVectorSetElement(func, vecReg, posExprVec, elemType);
  }
  auto *lhsReg = irMap.CreateRegMeExpr(*vecType);
  auto *vecAssign = irMap.CreateAssignMeStmt(*lhsReg, *vecRegNew, *currBB);
  treeNode.PushOutStmt(vecAssign);
  CHECK_FATAL(treeNode.GetChildren().empty(), "NOT VEC treeNode should not have children");
  if (debug) {
    treeNode.DumpVecStmts(irMap);
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

  // Tree node can not be vectorized, we use vector_set_element
  if (!treeNode->CanVectorized()) {
    return DoVectTreeNodeGatherNeeded(*treeNode);
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
    case OP_intrinsicop: {
      return DoVectTreeNodeNary(treeNode);
    }
    case OP_constval: {
      return DoVectTreeNodeConstval(treeNode);
    }
    case OP_dassign:
    case OP_iassign: {
      bool ret = DoVectTreeNodeIassign(treeNode);
      if (ret && treeNode->GetOp() == OP_dassign) {
        auto &dassignStmts = treeNode->GetStmts();
        auto *vecIassign = treeNode->GetOutStmts().back();
        CHECK_FATAL(vecIassign->GetOp() == OP_iassign, "must be");
        MovDefsFromDassignsToVecIassign(dassignStmts, static_cast<IassignMeStmt&>(*vecIassign), irMap);
      }
      return ret;
    }
    default: {
      SLP_FAILURE_DEBUG(os << "unsupported op: " << GetOpName(treeNode->GetOp()) << std::endl);
      return false;
    }
  }
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

MIRType *SLPVectorizer::GenMergedType(PrimType primType, uint8 lanes) const {
  if (mergeToVecType) {
    return GenVecType(primType, lanes);
  }
  uint32 totalByteBitSize = GetPrimTypeBitSize(primType) * lanes;
  CHECK_FATAL(totalByteBitSize >= 16, "must be");
  return GetScalarUnsignedTypeBySize(totalByteBitSize);
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
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  CHECK_NULL_FATAL(dom);
  MemPool *memPool = GetPhaseMemPool();
  bool debug = DEBUGFUNC_NEWPM(f);
  SLPVectorizer slpVectorizer(*memPool, f, *dom, debug);
  slpVectorizer.Run();
  return false;
}
}  // namespace maple

