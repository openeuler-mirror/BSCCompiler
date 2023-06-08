/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_sra.h"
#include "me_phase_manager.h"

namespace maple {
namespace {
bool kDebug = false;
#define DEBUG_SRA() \
  if (kDebug) LogInfo::MapleLogger()

constexpr size_t kSRASizeLimit = 32;
}

struct AggUse {
  bool mayAliased = false;
  FieldID fid = -1;
  BB *bb = nullptr;
  StmtNode *parent = nullptr;
  BaseNode *access = nullptr;
};

struct AggGroup {
  bool isAddressToken = false;
  bool isRet = false;
  bool isOffsetRead = false;
  bool wholeSplit = true;
  MIRSymbol *symbol = nullptr;
  std::vector<std::unique_ptr<AggUse>> uses;
  std::set<FieldID> replaceFields;
};

/// Implementation of Scalar Replacement of Aggregates.
/// This phase analyzes the local aggregates and tries to replace them
/// with scalar ones.
///
/// step 1. It scans the whole function and collects all the uses of local aggregates,
///         including agg copies, agg inits, agg field reads/writes.
/// step 2. Analyze all the uses, determine whether the whole agg can be split or
///         just rewrite the necessary parts.
/// step 3. Leave all the dead stmts or propgatable exprs to followed phases
///         (like hdse, epre, .etc) to optimize.
class SRA {
 public:
  explicit SRA(MeFunction &f) : func(f), builder(*f.GetMIRModule().GetMIRBuilder()) {}
  ~SRA() {
    curBB = nullptr;
  }

  void Run();
 private:
  MIRSymbol *GetLocalSym(StIdx idx);
  void CollectCandidates();
  void AddUse(StIdx idx, FieldID id, BaseNode &access, StmtNode *parent);
  BaseNode *ScanNodes(BaseNode &node, StmtNode *parent);
  void ScanFunc();
  void RemoveUnsplittable();
  void DetermineSplitRange();
  template <class RhsType, class AssignType>
  void SplitAggCopy(AssignType &assignNode, MIRStructType &structureType);
  void SplitDassignAggCopy(DassignNode &dassign);
  void SplitIassignAggCopy(IassignNode &iassign);
  void DoWholeSplit(AggGroup &group);
  void DoPartialSplit(AggGroup &group);
  void DoReplace();

  MeFunction &func;
  MIRBuilder &builder;
  BB *curBB = nullptr;
  std::unordered_map<MIRSymbol*, std::unique_ptr<AggGroup>> groups;
  std::set<std::pair<StmtNode*, BB*>> removed;
};

MIRSymbol *SRA::GetLocalSym(StIdx idx) {
  if (idx.IsGlobal()) {
    return nullptr;
  }
  return func.GetMirFunc()->GetSymbolTabItem(idx.Idx());
}

static MIRStructType *GetReadedStructureType(const DreadNode &dread, const MIRFunction &func) {
  const auto &rhsStIdx = dread.GetStIdx();
  auto rhsSymbol = func.GetLocalOrGlobalSymbol(rhsStIdx);
  ASSERT_NOT_NULL(rhsSymbol);
  auto rhsAggType = rhsSymbol->GetType();
  auto rhsFieldID = dread.GetFieldID();
  if (rhsFieldID != 0) {
    CHECK_FATAL(rhsAggType->IsStructType(), "only struct has non-zero fieldID");
    rhsAggType = static_cast<MIRStructType *>(rhsAggType)->GetFieldType(rhsFieldID);
  }
  if (!rhsAggType->IsStructType()) {
    return nullptr;
  }
  return static_cast<MIRStructType *>(rhsAggType);
}

static MIRStructType *GetReadedStructureType(const IreadNode &iread, const MIRFunction &func) {
  (void)func;
  auto rhsPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx());
  CHECK_FATAL(rhsPtrType->IsMIRPtrType(), "must be pointer type");
  auto rhsAggType = static_cast<MIRPtrType*>(rhsPtrType)->GetPointedType();
  auto rhsFieldID = iread.GetFieldID();
  if (rhsFieldID != 0) {
    CHECK_FATAL(rhsAggType->IsStructType(), "only struct has non-zero fieldID");
    rhsAggType = static_cast<MIRStructType *>(rhsAggType)->GetFieldType(rhsFieldID);
  }
  if (!rhsAggType->IsStructType()) {
    return nullptr;
  }
  return static_cast<MIRStructType *>(rhsAggType);
}

template <class RhsType, class AssignType>
void SRA::SplitAggCopy(AssignType &assignNode, MIRStructType &structureType) {
  auto *readNode = static_cast<RhsType *>(assignNode.GetRHS());
  auto rhsFieldID = readNode->GetFieldID();
  auto *rhsAggType = GetReadedStructureType(*readNode, *func.GetMirFunc());
  if (&structureType != rhsAggType) {
    return;
  }

  FieldID id = 1;
  while (id <= static_cast<FieldID>(structureType.NumberOfFieldIDs())) {
    MIRType *fieldType = structureType.GetFieldType(id);
    if (fieldType->GetSize() == 0) {
      id++;
      continue; // field size is zero for empty struct/union;
    }
    if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize() == 0) {
      id++;
      continue; // bitfield size is zero
    }
    if (fieldType->IsMIRStructType()) {
      id++;
      continue;
    }
    auto *newAssign = assignNode.CloneTree(func.GetMirFunc()->GetCodeMemPoolAllocator());
    newAssign->SetFieldID(assignNode.GetFieldID() + id);
    auto *newRHS = static_cast<RhsType *>(newAssign->GetRHS());
    newRHS->SetFieldID(rhsFieldID + id);
    newRHS->SetPrimType(fieldType->GetPrimType());
    curBB->GetStmtNodes().insertAfter(&assignNode, newAssign);
    newAssign->SetExpandFromArrayOfCharFunc(assignNode.IsExpandedFromArrayOfCharFunc());
    if (fieldType->IsMIRUnionType()) {
      id += static_cast<int32>(fieldType->NumberOfFieldIDs());
    }
    id++;
  }
  (void)removed.emplace(std::make_pair(&assignNode, curBB));
}

void SRA::SplitDassignAggCopy(DassignNode &dassign) {
  auto *rhs = dassign.GetRHS();
  auto stIdx = dassign.GetStIdx();
  auto *symbol = stIdx.IsGlobal() ? GlobalTables::GetGlobalTables().GetGsymTable().GetSymbolFromStidx(stIdx.Idx()) :
      func.GetMirFunc()->GetSymbolTabItem(stIdx.Idx());
  CHECK_NULL_FATAL(symbol);
  if (dassign.GetFieldID() != 0) {
    auto *fieldType = static_cast<MIRStructType*>(symbol->GetType())->GetFieldType(dassign.GetFieldID());
    if (fieldType->IsMIRUnionType()) {
      return;
    }
  } else if (symbol->GetType()->IsMIRUnionType()) {
    return;
  }
  auto *lhsType = static_cast<MIRStructType*>(symbol->GetType())->GetFieldType(dassign.GetFieldID());
  if (!lhsType->IsMIRStructType()) {
    return;
  }
  auto *lhsAggType = static_cast<MIRStructType*>(lhsType);

  if (rhs->GetOpCode() == OP_dread) {
    return SplitAggCopy<DreadNode>(dassign, *lhsAggType);
  } else if (rhs->GetOpCode() == OP_iread) {
    return SplitAggCopy<IreadNode>(dassign, *lhsAggType);
  }
}

static MIRStructType *GetIassignedStructType(const IassignNode &iassign) {
  auto ptrTyIdx = iassign.GetTyIdx();
  auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTyIdx);
  CHECK_FATAL(ptrType->IsMIRPtrType(), "must be pointer type");
  auto aggTyIdx = static_cast<MIRPtrType *>(ptrType)->GetPointedTyIdxWithFieldID(iassign.GetFieldID());
  auto *lhsAggType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(aggTyIdx);
  if (!lhsAggType->IsStructType()) {
    return nullptr;
  }
  if (lhsAggType->GetKind() == kTypeUnion) {
    return nullptr;
  }
  return static_cast<MIRStructType *>(lhsAggType);
}

void SRA::SplitIassignAggCopy(IassignNode &iassign) {
  auto rhs = iassign.GetRHS();
  auto *lhsAggType = GetIassignedStructType(iassign);
  if (!lhsAggType) {
    return;
  }

  if (rhs->GetOpCode() == OP_dread) {
    return SplitAggCopy<DreadNode>(iassign, *lhsAggType);
  } else if (rhs->GetOpCode() == OP_iread) {
    return SplitAggCopy<IreadNode>(iassign, *lhsAggType);
  }
}

void SRA::DoWholeSplit(AggGroup &group) {
  auto *type = static_cast<MIRStructType*>(group.symbol->GetType());
  for (auto &use : group.uses) {
    if (!type->GetFieldType(use->fid)->IsMIRStructType()) {
      continue;
    }
    curBB = use->bb;
    auto *stmt = use->parent;
    if (removed.find(std::make_pair(stmt, curBB)) != removed.end()) {
      continue;
    }
    if (stmt->GetOpCode() == OP_dassign) {
      SplitDassignAggCopy(static_cast<DassignNode&>(*stmt));
    } else if (stmt->GetOpCode() == OP_iassign) {
      SplitIassignAggCopy(static_cast<IassignNode&>(*stmt));
    }
  }
}

void SRA::DoPartialSplit(AggGroup &group) {
  DEBUG_SRA() << "\nDo not split BIG aggregates";
  auto *symbol = group.symbol;
  auto *type = static_cast<MIRStructType*>(symbol->GetType());
  std::vector<StIdx> newLocal(type->NumberOfFieldIDs() + 1, StIdx(0));
  for (auto id : group.replaceFields) {
    auto name = symbol->GetName() + "@" + std::to_string(id) + "@SRA";
    auto *fieldType = type->GetFieldType(id);
    auto *fieldSym =
        builder.CreateSymbol(fieldType->GetTypeIndex(), name, kStVar, kScAuto, func.GetMirFunc(), kScopeLocal);
    DEBUG_SRA() << "Create a local symbol for %" << symbol->GetName() <<
        " field " << std::to_string(id) << ": " << fieldSym->GetName() << std::endl;
    newLocal[static_cast<uint32>(id)] = fieldSym->GetStIdx();
  }
  for (auto &use : group.uses) {
    if (!type->GetFieldType(use->fid)->IsMIRStructType()) {
      if (newLocal[static_cast<uint32>(use->fid)] == StIdx(0)) {
        continue;
      }
      if (use->access->GetOpCode() == OP_dassign) {
        static_cast<DassignNode*>(use->access)->SetStIdx(newLocal[static_cast<uint32>(use->fid)]);
        static_cast<DassignNode*>(use->access)->SetFieldID(0);
      } else if (use->access->GetOpCode() == OP_dread) {
        static_cast<DreadNode*>(use->access)->SetStIdx(newLocal[static_cast<uint32>(use->fid)]);
        static_cast<DreadNode*>(use->access)->SetFieldID(0);
      } else {
        CHECK_FATAL_FALSE("SRA: Check access op!");
      }
      continue;
    }

    curBB = use->bb;
    auto offset1 = type->GetBitOffsetFromBaseAddr(use->fid);
    auto fieldType1 = type->GetFieldType(use->fid);
    auto size1 = fieldType1->IsMIRBitFieldType() ? static_cast<MIRBitFieldType*>(fieldType1)->GetFieldSize() :
        fieldType1->GetSize() * CHAR_BIT;
    for (auto id : group.replaceFields) {
      if (use->fid >= id) {
        continue;
      }
      auto offset2 = type->GetBitOffsetFromBaseAddr(id);
      auto fieldType2 = type->GetFieldType(id);
      auto size2 = fieldType2->IsMIRBitFieldType() ? static_cast<MIRBitFieldType*>(fieldType2)->GetFieldSize() :
          fieldType2->GetSize() * CHAR_BIT;
      if (std::max(offset1, offset2) >=
          std::min(offset1 + static_cast<int64>(size1), offset2 + static_cast<int64>(size2))) {
        continue;
      }

      if (use->access->GetOpCode() == OP_dassign) {
        auto *rhs = static_cast<DassignNode*>(use->access)->GetRHS();
        auto *newRhs = rhs->CloneTree(func.GetMirFunc()->GetCodeMemPoolAllocator());
        if (rhs->GetOpCode() == OP_dread) {
          static_cast<DreadNode*>(newRhs)->SetFieldID(static_cast<DreadNode*>(newRhs)->GetFieldID() + id - use->fid);
        } else if (rhs->GetOpCode() == OP_iread) {
          static_cast<IreadNode*>(newRhs)->SetFieldID(static_cast<IreadNode*>(newRhs)->GetFieldID() + id - use->fid);
        } else {
          CHECK_FATAL_FALSE("SRA: Check access rhs op!");
        }
        newRhs->SetPrimType(fieldType2->GetPrimType());
        auto *newAssign = builder.CreateStmtDassign(newLocal[static_cast<uint32>(id)], 0, newRhs);
        curBB->GetStmtNodes().insertAfter(static_cast<DassignNode*>(use->access), newAssign);
        continue;
      }

      if (use->access->GetOpCode() == OP_dread) {
        auto *localSym = func.GetMirFunc()->GetSymbolTabItem(newLocal[static_cast<uint32>(id)].Idx());
        auto *localRead = builder.CreateDread(*localSym, fieldType2->GetPrimType());
        auto *newAssign = builder.CreateStmtDassign(symbol->GetStIdx(), id, localRead);
        curBB->InsertStmtBefore(use->parent, newAssign);
        continue;
      }
      CHECK_FATAL_FALSE("SRA: Check access op!");
    }
  }
}

void SRA::DoReplace() {
  for (auto &it : groups) {
    auto *group = it.second.get();
    if (group->wholeSplit) {
      DoWholeSplit(*group);
      continue;
    }
    DoPartialSplit(*group);
  }
  for (auto &pair : removed) {
    pair.second->RemoveStmtNode(pair.first);
  }
}

void SRA::DetermineSplitRange() {
  for (auto &it : groups) {
    auto *type = static_cast<MIRStructType*>(it.first->GetType());
    if (type->GetSize() <= kSRASizeLimit) {
      continue;
    }
    FieldID id = 1;
    while (id <= static_cast<FieldID>(type->NumberOfFieldIDs())) {
      auto *fieldType = type->GetFieldType(id);
      if (!fieldType->IsMIRUnionType()) {
        id++;
        continue;
      }
      for (auto &use : it.second->uses) {
        if (use->fid >= id && use->fid <= id + static_cast<int32>(fieldType->NumberOfFieldIDs())) {
          use->mayAliased = true;
        }
      }
      id = id + static_cast<int32>(fieldType->NumberOfFieldIDs()) + 1;
    }
    for (auto &use : it.second->uses) {
      if (!use->mayAliased && IsPrimitiveScalar(type->GetFieldType(use->fid)->GetPrimType())) {
        (void)it.second->replaceFields.emplace(use->fid);
      }
    }
    // if split part is less than 3 / 4, do not apply whole split
    if (type->GetFieldsSize() * 3 / 4 >= it.second->replaceFields.size()) {
      it.second->wholeSplit = false;
    }
  }
}

void SRA::AddUse(StIdx idx, FieldID id, BaseNode &access, StmtNode *parent) {
  auto *symbol = GetLocalSym(idx);
  auto found = groups.find(symbol);
  if (found != groups.end()) {
    auto use = std::make_unique<AggUse>();
    use->bb = curBB;
    use->parent = parent;
    use->access = &access;
    use->fid = id;
    (void)found->second->uses.emplace_back(std::move(use));
  }
}

BaseNode *SRA::ScanNodes(BaseNode &node, StmtNode *parent) {
  switch (node.GetOpCode()) {
    case OP_addrof: {
      auto &addrof = static_cast<AddrofNode&>(node);
      auto *symbol = GetLocalSym(addrof.GetStIdx());
      auto found = groups.find(symbol);
      if (found != groups.end()) {
        found->second->isAddressToken = true;
      }
      break;
    }
    case OP_dread: {
      auto &dread = static_cast<DreadNode&>(node);
      AddUse(dread.GetStIdx(), dread.GetFieldID(), node, parent);
      break;
    }
    case OP_dreadoff: {
      auto &dreadoff = static_cast<DreadoffNode&>(node);
      auto *symbol = GetLocalSym(dreadoff.stIdx);
      auto found = groups.find(symbol);
      if (found != groups.end()) {
        found->second->isOffsetRead = true;
      }
      break;
    }
    case OP_dassign: {
      auto &dassign = static_cast<DassignNode&>(node);
      AddUse(dassign.GetStIdx(), dassign.GetFieldID(), node, parent);
      break;
    }
    case OP_dassignoff: {
      auto &dassignoff = static_cast<DassignoffNode&>(node);
      auto *symbol = GetLocalSym(dassignoff.stIdx);
      auto found = groups.find(symbol);
      if (found != groups.end()) {
        found->second->isOffsetRead = true;
      }
      break;
    }
    case OP_iread: {
      auto &iread = static_cast<IreadNode&>(node);
      auto *base = iread.Opnd(0);
      if (base->GetOpCode() != OP_addrof) {
        break;
      }
      auto *addrof = static_cast<AddrofNode*>(base);
      auto *symbol = GetLocalSym(addrof->GetStIdx());
      auto found = groups.find(symbol);
      if (found == groups.end()) {
        break;
      }
      auto *ptrtype = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx()));
      auto symbolTyIdx = addrof->GetFieldID() == 0 ? symbol->GetTyIdx() :
          static_cast<MIRStructType*>(symbol->GetType())->GetFieldTyIdx(addrof->GetFieldID());
      if (symbolTyIdx != ptrtype->GetPointedTyIdx()) {
        break;
      }
      // use dread to replace this iread
      auto *dread = builder.CreateDread(*symbol, iread.GetPrimType());
      dread->SetFieldID(addrof->GetFieldID() + iread.GetFieldID());
      AddUse(dread->GetStIdx(), dread->GetFieldID(), *dread, parent);
      return dread;
    }
    case OP_iassign: {
      auto &iassign = static_cast<IassignNode&>(node);
      auto *base = iassign.Opnd(0);
      if (base->GetOpCode() != OP_addrof) {
        break;
      }
      auto *addrof = static_cast<AddrofNode*>(base);
      auto *symbol = GetLocalSym(addrof->GetStIdx());
      auto found = groups.find(symbol);
      if (found == groups.end()) {
        break;
      }
      auto *ptrtype = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign.GetTyIdx()));
      auto symbolTyIdx = addrof->GetFieldID() == 0 ? symbol->GetTyIdx() :
          static_cast<MIRStructType*>(symbol->GetType())->GetFieldTyIdx(addrof->GetFieldID());
      if (symbolTyIdx != ptrtype->GetPointedTyIdx()) {
        break;
      }
      // use dassign to replace this iassign
      auto *dassign =
          builder.CreateStmtDassign(*symbol, addrof->GetFieldID() + iassign.GetFieldID(), iassign.GetRHS());
      curBB->InsertStmtBefore(&iassign, dassign);
      curBB->RemoveStmtNode(&iassign);
      AddUse(dassign->GetStIdx(), dassign->GetFieldID(), *dassign, dassign);
      auto *replace = ScanNodes(*dassign->GetRHS(), dassign);
      if (replace) {
        dassign->SetRHS(replace);
      }
      return nullptr;
    }
    default: {
      if (!kOpcodeInfo.IsCall(node.GetOpCode())) {
        break;
      }
      auto &call = static_cast<CallNode&>(node);
      for (size_t i = 0; i < call.GetCallReturnVector()->size(); ++i) {
        auto retPair = call.GetReturnPair(i);
        if (retPair.second.IsReg()) {
          continue;
        }
        auto *symbol = GetLocalSym(retPair.first);
        auto found = groups.find(symbol);
        if (found != groups.end()) {
          found->second->isRet = true;
        }
      }
      break;
    }
  }
  for (size_t i = 0; i < node.NumOpnds(); i++) {
    auto *replace = ScanNodes(*node.Opnd(i), parent);
    if (replace) {
      node.SetOpnd(replace, i);
    }
  }
  return nullptr;
}

void SRA::ScanFunc() {
  for (BB *bb : func.GetCfg()->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    curBB = bb;
    StmtNode *stmt = to_ptr(bb->GetStmtNodes().begin());
    while (stmt) {
      auto *next = stmt->GetNext();
      (void)ScanNodes(*stmt, stmt);
      stmt = next;
    }
  }
}

void SRA::RemoveUnsplittable() {
  for (auto it = groups.begin(); it != groups.end();) {
    auto *symbol = it->first;
    auto *group = it->second.get();

    if (group->isAddressToken) {
      DEBUG_SRA() << "Symbol %" << symbol->GetName() << " rejected by SRA, because it is [addressed].\n";
      it = groups.erase(it);
      continue;
    }

    if (group->isRet) {
      DEBUG_SRA() << "Symbol %" << symbol->GetName() << " rejected by SRA, because it is a [return value].\n";
      it = groups.erase(it);
      continue;
    }

    if (group->isOffsetRead) {
      DEBUG_SRA() << "Symbol %" << symbol->GetName() << " rejected by SRA, because some parts are [read by offset].\n";
      it = groups.erase(it);
      continue;
    }

    bool notCopyUse = false;
    size_t useCount = 0;
    for (auto &use : group->uses) {
      auto *fieldTy = static_cast<MIRStructType*>(symbol->GetType())->GetFieldType(use->fid);
      if (IsPrimitiveScalar(fieldTy->GetPrimType())) {
        useCount++;
        continue;
      }
      if (use->parent->GetOpCode() != OP_dassign && use->parent->GetOpCode() != OP_iassign) {
        notCopyUse = true;
        break;
      }
    }
    if (notCopyUse) {
      DEBUG_SRA() << "Symbol %" << it->first->GetName() << " rejected by SRA, because it is [not copy use].\n";
      it = groups.erase(it);
      continue;
    }
    if (useCount == 0) {
      DEBUG_SRA() << "Symbol %" << it->first->GetName() << " rejected by SRA, because ";
      DEBUG_SRA() << "[no scalar part] need to be replaced.\n";
      it = groups.erase(it);
      continue;
    }
    ++it;
  }
}

void SRA::CollectCandidates() {
  for (size_t i = 0; i < func.GetMirFunc()->GetSymbolTabSize(); i++) {
    auto *symbol = func.GetMirFunc()->GetSymbolTabItem(static_cast<uint32>(i));
    if (!symbol || symbol->IsFormal() || symbol->IsVolatile() || symbol->IsPUStatic()) {
      continue;
    }
    auto *type = symbol->GetType();
    if (type->GetKind() != kTypeStruct) {
      continue;
    }
    auto group = std::make_unique<AggGroup>();
    group->symbol = symbol;
    (void)groups.emplace(symbol, std::move(group));
  }
}

void SRA::Run() {
  if (func.IsEmpty()) {
    return;
  }
  CollectCandidates();
  if (groups.empty()) {
    // no candidate found, exit
    return;
  }

  ScanFunc();
  RemoveUnsplittable();
  if (groups.empty()) {
    // no candidate found, exit
    return;
  }

  DetermineSplitRange();
  DoReplace();
}

void MESRA::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}

bool MESRA::PhaseRun(maple::MeFunction &f) {
  kDebug = DEBUGFUNC_NEWPM(f);
  SRA sra(f);
  sra.Run();
  return false;
}
} // namespace maple
