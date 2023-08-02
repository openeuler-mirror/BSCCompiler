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
#include "inline_summary.h"
#include <utility>
#include "constantfold.h"
#include "dominance.h"

namespace {
using maple::int32;
constexpr int32 kRemoveStmtNone = 0;
constexpr int32 kRemoveStmtHalf = 1;
constexpr int32 kRemoveStmtAll = 2;
}

namespace maple {
// This is a convenient function for debugging
InlineSummary *GetSummaryByPuIdx(uint32 puIdx) {
  auto *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  return func->GetInlineSummary();
}

int32 CostSize2NumInsn(int32 costSize) {
  return costSize * static_cast<int32>(kInsn2Threshold) / static_cast<int32>(kSizeScale);
}

ExprBoolResult GetReverseExprBoolResult(ExprBoolResult res) {
  if (res == kExprResTrue) {
    return kExprResFalse;
  }
  if (res == kExprResFalse) {
    return kExprResTrue;
  }
  return res;
}

void DumpStmtSafe(const StmtNode &stmtNode, MIRFunction &func) {
  auto *oldFunc = theMIRModule->CurFunction();
  theMIRModule->SetCurFunction(&func);
  stmtNode.Dump();
  theMIRModule->SetCurFunction(oldFunc);
}

// return <type, constVal> pair if `expr` can be folded into a constantant.
// return { PTY_unknown, 0 } otherwise.
std::pair<PrimType, int64> TryFoldConst(BaseNode &expr) {
  MIRConst *resConst = nullptr;
  if (expr.GetOpCode() == OP_constval) {
    resConst = static_cast<ConstvalNode&>(expr).GetConstVal();
  } else {
    static ConstantFold cf(*theMIRModule);
    auto *foldExpr = cf.Fold(&expr);
    if (foldExpr != nullptr && foldExpr->GetOpCode() == OP_constval) {
      resConst = static_cast<ConstvalNode*>(foldExpr)->GetConstVal();
    }
  }
  if (resConst == nullptr) {
    return { PTY_unknown, 0 };
  }
  int64 val = 0;
  auto type = resConst->GetType().GetPrimType();
  if (IsPrimitiveInteger(type)) {
    val = static_cast<MIRIntConst*>(resConst)->GetExtValue();
  } else if (type == PTY_f32) {
    val = static_cast<MIRFloatConst*>(resConst)->GetIntValue();
  } else if (type == PTY_f64) {
    val = static_cast<MIRDoubleConst*>(resConst)->GetIntValue();
  } else {
    return { PTY_unknown, 0 };
  }
  return { type, val };
}

// If `expr` is an unmodified formal parameter (zero version), return formal index, otherwise return none.
std::optional<uint32> GetZeroVersionParamIndex(const MeExpr &expr, MeFunction &func) {
  if (expr.GetMeOp() != kMeOpVar) {
    return {};
  }
  auto &varExpr = static_cast<const VarMeExpr&>(expr);
  if (varExpr.IsVolatile() || !varExpr.IsZeroVersion()) {
    return {};
  }
  auto *ost = varExpr.GetOst();
  if (ost->GetFieldID() != 0 || !ost->IsFormal() || ost->GetIndirectLev() != 0) {
    return {};
  }
  auto *symbol = ost->GetMIRSymbol();
  for (uint32 i = 0; i < func.GetMirFunc()->GetFormalCount(); ++i) {
    auto *formalSt = func.GetMirFunc()->GetFormal(i);
    if (formalSt != nullptr && symbol->GetNameStrIdx() == formalSt->GetNameStrIdx()) {
      return i;
    }
  }
  return {};
}

// Return true if the `expr` is the leaf expr with one of the specified `kinds`
bool IsExprSpecifiedKinds(const MeExpr &expr, uint32 kinds, MeFunction &func) {
  if (((kinds & kExprKindConstNumber) != 0) && expr.GetOp() == OP_constval) {
    auto constKind = static_cast<const ConstMeExpr&>(expr).GetConstVal()->GetKind();
    if (constKind == kConstInt || constKind == kConstFloatConst || constKind == kConstDoubleConst) {
      return true;
    }
  }
  if (((kinds & kExprKindConst) != 0) && expr.GetOp() == OP_constval) {
    return true;
  }
  if (((kinds & kExprKindParam) != 0) && GetZeroVersionParamIndex(expr, func).has_value()) {
    return true;
  }
  return false;
}

// Return true if the `expr` is only composed of specified `kinds`
bool IsExprOnlyComposedOf(const MeExpr &expr, uint32 kinds, MeFunction &func) {
  if (expr.IsLeaf()) {
    return IsExprSpecifiedKinds(expr, kinds, func);
  }
  static const std::vector<Opcode> unsupportedOps = { OP_iread, OP_ireadoff, OP_array };
  if (std::find(unsupportedOps.begin(), unsupportedOps.end(), expr.GetOp()) != unsupportedOps.end()) {
    return false;
  }
  for (uint32 i = 0; i < expr.GetNumOpnds(); ++i) {
    if (!IsExprOnlyComposedOf(*expr.GetOpnd(i), kinds, func)) {
      return false;
    }
  }
  return true;
}

// Return a new generated liteExpr allocated in `alloc` for `expr` if `expr` is only composed of
// specified `kinds` (see enum ExprKind). Return nullptr otherwise.
// `paramsUsed` is an out parameter that is a param index bitMap indicating which parameters are used by `expr`
LiteExpr *LiteExpr::TryBuildLiteExpr(MeExpr &expr, uint32 kinds, MeFunction &func, MapleAllocator &alloc,
    uint32 *paramsUsed) {
  if (!IsExprOnlyComposedOf(expr, kinds, func)) {
    return nullptr;
  }
  return &DoBuildLiteExpr(expr, func, alloc, paramsUsed);
}

LiteExpr &LiteExpr::DoBuildLiteExpr(MeExpr &expr, MeFunction &func, MapleAllocator &alloc, uint32 *paramsUsed) {
  auto opcode = expr.GetOp();
  // Build liteExpr for constant number
  if (opcode == OP_constval) {
    auto *mirConst = static_cast<ConstMeExpr&>(expr).GetConstVal();
    auto kind = mirConst->GetKind();
    CHECK_FATAL(kind == kConstInt || kind == kConstFloatConst || kind == kConstDoubleConst, "must be const number");
    auto *liteExpr = alloc.New<LiteExpr>(alloc, mirConst->GetType().GetPrimType(), mirConst);
    return *liteExpr;
  }
  // Build liteExpr for parameter
  if (auto ret = GetZeroVersionParamIndex(expr, func)) {
    auto paramIndex = ret.value();
    if (paramsUsed != nullptr) {
      (*paramsUsed) |= (1u << paramIndex);  // record parameters used by current expr
    }
    auto *liteExpr = alloc.New<LiteExpr>(alloc, expr.GetPrimType(), paramIndex);
    return *liteExpr;
  }
  auto *resultExpr = alloc.New<LiteExpr>(alloc, opcode, expr.GetPrimType());
  // Append extra info for special opcodes
  if (resultExpr->IsCvt() || resultExpr->IsCmp()) {
    resultExpr->SetOpndType(static_cast<OpMeExpr&>(expr).GetOpndType());
  } else if (resultExpr->IsExtractbits()) {
    auto &opMeExpr = static_cast<OpMeExpr&>(expr);
    resultExpr->SetBitsSize(opMeExpr.GetBitsSize());
    resultExpr->SetBitsOffset(opMeExpr.GetBitsOffSet());
  }
  // Build liteExpr for opnds recursively
  auto numOpnds = expr.GetNumOpnds();
  resultExpr->GetOpnds().resize(numOpnds, nullptr);
  for (uint32 i = 0; i < numOpnds; ++i) {
    auto &liteExpr = DoBuildLiteExpr(*expr.GetOpnd(i), func, alloc, paramsUsed);
    resultExpr->GetOpnds()[i] = &liteExpr;
  }
  return *resultExpr;
}

// Clone liteExpr deeply into `newAlloc`
LiteExpr *LiteExpr::Clone(MapleAllocator &newAlloc) const {
  auto *copy = newAlloc.New<LiteExpr>(newAlloc, *this);
  for (uint32 i = 0; i < opnds.size(); ++i) {
    auto *opndCopy = opnds[i]->Clone(newAlloc);
    copy->opnds[i] = opndCopy;
  }
  return copy;
}

void LiteExpr::DumpWithoutEndl(uint32 indent) const {
  auto &os = LogInfo::MapleLogger();
  os << kOpcodeInfo.GetTableItemAt(op).name << " " << GetPrimTypeName(type) << " ";
  if (IsCvt() || IsCmp()) {
    os << GetPrimTypeName(GetOpndType()) << " ";
  } else if (IsExtractbits()) {
    os << static_cast<uint32>(GetBitsSize()) << "[offset:" <<
        static_cast<uint32>(GetBitsOffset()) << "] ";
  } else if (IsConst()) {
    if (IsPrimitiveInteger(type)) {
      os << GetInt();
    } else if (type == PTY_f32) {
      os << GetFloat();
    } else if (type == PTY_f64) {
      os << GetDouble();
    } else {
      CHECK_FATAL(false, "should not be here");
    }
  } else if (IsParam()) {
    os << "param" << GetParamIndex();
  }

  if (opnds.empty()) {
    return;
  }
  constexpr uint32 indentStep = 2;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    os << std::endl;
    os << std::string(indent + indentStep, ' ') << "opnd[" << i << "] = ";
    opnds[i]->DumpWithoutEndl(indent + indentStep);
  }
}

void LiteExpr::Dump(uint32 indent) const {
  DumpWithoutEndl(indent);
  LogInfo::MapleLogger() << std::endl;
}

// If any param can not be evaluated, return nullptr
BaseNode *LiteExpr::ConvertToMapleIR(MapleAllocator &alloc, const ArgInfoVec *argInfoVec) const {
  if (IsConst()) {
    return alloc.New<ConstvalNode>(type, data.cst);
  }
  // Substitute constant real arguments into parameter expressions
  if (IsParam()) {
    uint32 paramIndex = GetParamIndex();
    if (argInfoVec == nullptr || paramIndex >= argInfoVec->size() || (*argInfoVec)[paramIndex] == nullptr) {
      // no valid real arg value
      return nullptr;
    }
    auto *argInfo = (*argInfoVec)[paramIndex];
    if (!argInfo->IsSingleValue()) {
      // value range has not been supported for now
      return nullptr;
    }
    MIRConst *mirConst = argInfo->GetLower();
    auto fromType = mirConst->GetType().GetPrimType();
    auto toType = GetType();
    BaseNode *result = alloc.New<ConstvalNode>(fromType, mirConst);
    if (!IsNoCvtNeeded(toType, fromType)) {
      result = alloc.New<TypeCvtNode>(OP_cvt, toType, fromType, result);
    }
    return result;
  }
  CHECK_FATAL(!opnds.empty(), "there is other leaf node other than const and param?");
  std::vector<BaseNode*> opndNodes(opnds.size(), nullptr);
  BaseNode *node0 = opnds[0]->ConvertToMapleIR(alloc, argInfoVec);
  if (node0 == nullptr) {
    return nullptr;
  }
  if (opndNodes.size() == kOperandNumUnary) {
    if (IsCvt()) {
      return alloc.New<TypeCvtNode>(op, type, GetOpndType(), node0);
    }
    if (IsExtractbits()) {
      return alloc.New<ExtractbitsNode>(op, type, GetBitsOffset(), GetBitsSize(), node0);
    }
  } else if (opndNodes.size() == kOperandNumBinary) {
    auto *node1 = opnds[1]->ConvertToMapleIR(alloc, argInfoVec);
    if (node1 == nullptr) {
      return nullptr;
    }
    if (IsCmp()) {
      return alloc.New<CompareNode>(op, type, GetOpndType(), node0, node1);
    }
    return alloc.New<BinaryNode>(op, type, node0, node1);
  } else if (opndNodes.size() == kOperandNumTernary) {
    if (op == OP_select) {
      auto *node1 = opnds[1]->ConvertToMapleIR(alloc, argInfoVec);
      auto *node2 = opnds[2]->ConvertToMapleIR(alloc, argInfoVec);
      return (!node1 || !node2) ? nullptr : alloc.New<TernaryNode>(op, type, node0, node1, node2);
    }
  } else {
    CHECK_FATAL(false, "NYI");
  }
  return nullptr;
}

// Evaluate `liteExpr` if possible.
// `liteExpr` should not be changed because it is shared by all callsites.
// Implementation: liteExpr is converted to maple IR first (so we need specify an allocator),
// then maple IR is folded into const if possible
MIRConst *LiteExpr::Evaluate(MapleAllocator &alloc, const ArgInfoVec *argInfoVec) const {
  auto *baseNode = ConvertToMapleIR(alloc, argInfoVec);
  if (baseNode == nullptr) {
    return nullptr;
  }
  MIRConst *resConst = nullptr;
  if (baseNode->GetOpCode() == OP_constval) {
    resConst = static_cast<ConstvalNode&>(*baseNode).GetConstVal();
  } else {
    static ConstantFold cf(*theMIRModule);
    auto *foldExpr = cf.Fold(baseNode);
    if (foldExpr != nullptr && foldExpr->GetOpCode() == OP_constval) {
      resConst = static_cast<ConstvalNode*>(foldExpr)->GetConstVal();
    }
  }
  return resConst;
}

ExprBoolResult LiteExpr::EvaluateToBool(MapleAllocator &alloc, const ArgInfoVec *argInfoVec) const {
  auto *mirConst = Evaluate(alloc, argInfoVec);
  if (mirConst == nullptr) {
    return kExprResUnknown;
  }
  CHECK_FATAL(mirConst->GetKind() == kConstInt, "condition type is not integer?");
  return mirConst->IsZero() ? kExprResFalse : kExprResTrue;
}

// convert meExpr to liteExpr, then evaluate liteExpr
MIRConst *EvaluateMeExpr(MeExpr &meExpr, MeFunction &func, MapleAllocator &alloc) {
  // Only care meExpr that is composed of const expr
  auto *liteExpr = LiteExpr::TryBuildLiteExpr(meExpr, kExprKindConstNumber, func, alloc);
  if (liteExpr != nullptr) {
    return liteExpr->Evaluate(alloc, nullptr);
  }
  return nullptr;
}

void Condition::Dump() const {
  if (reverse) {
    LogInfo::MapleLogger() << "!";
  }
  expr->Dump();
}

void Predicate::AddAssert(Assert newAssert, const MapleVector<Condition*> &conditions) {
  if (newAssert == 0) {  // (true)
    return;
  }
  if (newAssert == 1) {  // (false)
    // False assert makes the whole predicate false.
    asserts = { 1 };
    return;
  }
  if (IsFalsePredicate()) {
    return;
  }

  // Try to insert newAssert
  std::vector<std::pair<Assert, Assert>> replaceList;
  for (auto assert : asserts) {
    // `assert` implies `newAssert`, add nothing
    // For example:
    //   `assert`    : (cond1 || cond2)
    //   `newAssert` : (cond1 || cond2 || cond3)
    if ((assert & newAssert) == assert) {
      return;
    }
    // `newAssert` implies `assert`, replace `assert` with `newAssert`
    if ((assert & newAssert) == newAssert) {
      (void)replaceList.emplace_back(assert, newAssert);
    }
  }

  // Try to find out asserts that are obviously true such ass (x == 42 || x != 42)
  for (uint32 i = 0; i < kMaxNumCondition; ++i) {
    if ((newAssert & (kAssertOne << i)) == 0) {
      continue;
    }
    auto *cond1 = conditions[i];
    for (uint32 j = i + 1; j < kMaxNumCondition; ++j) {
      if ((newAssert & (kAssertOne << j)) == 0) {
        continue;
      }
      auto *cond2 = conditions[j];
      if (cond1->IsReverseWithCondition(*cond2)) {
        return;
      }
    }
  }

  if (!replaceList.empty()) {
    for (const auto &r : replaceList) {
      (void)asserts.erase(r.first);
      (void)asserts.insert(r.second);
    }
  } else {
    (void)asserts.insert(newAssert);
  }
}

const Predicate *Predicate::And(const Predicate &a, const Predicate &b, MapleAllocator &alloc) {
  if (b.IsFalsePredicate() || a.IsTruePredicate()) {
    return &b;
  }
  if (a.IsFalsePredicate() || b.IsTruePredicate()) {
    return &a;
  }
  std::vector<Assert> mergedAsserts;
  mergedAsserts.reserve(a.asserts.size() + b.asserts.size());
  for (auto assert : a.asserts) {
    mergedAsserts.push_back(assert);
  }
  for (auto assert : b.asserts) {
    mergedAsserts.push_back(assert);
  }
  return alloc.New<Predicate>(mergedAsserts, alloc);
}

const Predicate *Predicate::Or(const Predicate &a, const Predicate &b, const MapleVector<Condition*> &conditions,
    MapleAllocator &alloc) {
  if (b.IsFalsePredicate() || a.IsTruePredicate()) {
    return &a;
  }
  if (a.IsFalsePredicate() || b.IsTruePredicate()) {
    return &b;
  }
  if (a == b) {
    return &a;
  }

  auto *result = alloc.New<Predicate>(alloc);
  for (auto it = a.asserts.begin(); it != a.asserts.end(); ++it) {
    for (auto jt = b.asserts.begin(); jt != b.asserts.end(); ++jt) {
      auto newAssert = *it | *jt;
      result->AddAssert(newAssert, conditions);
    }
  }
  if (result->asserts.empty()) {
    return TruePredicate();  // There is no any assert, return true predicate
  }
  return result;
}

const Predicate *Predicate::Clone(MapleAllocator &newAlloc) const {
  // truePredicate and falsePredicate are globally unique, no need to clone
  if (IsTruePredicate()) {
    return Predicate::TruePredicate();
  }
  if (IsFalsePredicate()) {
    return Predicate::FalsePredicate();
  }
  if (IsNotInlinePredicate()) {
    return Predicate::NotInlinePredicate();
  }
  auto *copy = newAlloc.New<Predicate>(asserts, newAlloc);
  return copy;
}

void Predicate::Dump(const MapleVector<Condition*> &conditions) const {
  auto &os(LogInfo::MapleLogger());
  if (IsTruePredicate()) {
    os << "(true)\n";
    return;
  }
  if (IsFalsePredicate()) {
    os << "(false)\n";
    return;
  }
  uint32 idx = 0;
  for (auto assert : asserts) {
    bool firstItem = true;
    if (idx != 0) {
      os << " && ";
    }
    os << "(";
    for (uint32 i = 0; i < kMaxNumCondition; ++i) {
      if ((assert & (kAssertOne << i)) != 0) {
        if (!firstItem) {
          os << " || ";
        }
        if (i == kCondIdxNotInline) {
          os << "not inline";
          continue;
        }
        firstItem = false;
        conditions[i]->Dump();
      }
    }
    os << ")";
    ++idx;
  }
  LogInfo::MapleLogger() << std::endl;
}

// Return true if all parameters used by current predicate are in `paramMapping`
bool Predicate::AreAllUsedParamsInParamMapping(const std::vector<int32> &paramMapping,
    const MapleVector<Condition*> &conditions) const {
  if (paramMapping.empty()) {
    return false;
  }
  uint32 paramsUsed = GetParamsUsed(conditions);
  if (paramsUsed == 0) {
    return false;
  }
  bool allMapping = true;
  uint32 paramsNum = std::min(sizeof(paramsUsed) * kNumBitsPerByte, paramMapping.size());
  for (uint32 i = 0; i < paramsNum; ++i) {
    if (((paramsUsed & (1u << i)) != 0) && paramMapping[i] == -1) {
      allMapping = false;  // found parameter that has no mapping item
      break;
    }
  }
  return allMapping;
}

void MergeInlineSummary(MIRFunction &caller, MIRFunction &callee, const StmtNode &callStmt,
    const std::map<uint32, uint32> &callMeStmtIdMap) {
  // Merge inline summary
  auto *callerSummary = caller.GetInlineSummary();
  CHECK_NULL_FATAL(callerSummary);
  auto *calleeSummary = callee.GetInlineSummary();
  CHECK_NULL_FATAL(calleeSummary);
  auto callStmtId = callStmt.GetStmtID();
  auto &argInfosMap = callerSummary->GetArgInfosMap();
  ArgInfoVec *argInfoVec = nullptr;
  const auto &it = std::as_const(argInfosMap).find(callStmtId);
  if (it != argInfosMap.end()) {
    argInfoVec = it->second;
  }
  std::vector<ExprBoolResult> condResultVec;
  // argInfoVec may be nullptr
  auto tmpMp = std::make_unique<MemPool>(memPoolCtrler, "");
  MapleAllocator tmpAlloc(tmpMp.get());
  calleeSummary->EvaluateConditions(tmpAlloc, argInfoVec, condResultVec, true);
  callerSummary->MergeSummary(*calleeSummary, callStmt.GetStmtID(), tmpAlloc, condResultVec, callMeStmtIdMap);
}

void InlineSummary::MergeSummary(const InlineSummary &fromSummary, uint32 callStmtId, MapleAllocator &tmpAlloc,
    const std::vector<ExprBoolResult> &condResultVec, const std::map<uint32, uint32> &callMeStmtIdMap) {
  if (func == fromSummary.func) {
    // recursive inlining MergeSummary is not supported for now, we mark inline summary untrustworthy
    trustworthy = false;
    return;
  }
  // (1) find paramMapping for current callsite
  std::vector<int32> paramMapping;
  GetParamMappingForCallsite(callStmtId, paramMapping);

  InlineEdgeSummary *callEdgeSummary = nullptr;
  int32 callFrequency = -1;
  auto *callBBPredicate = Predicate::TruePredicate();
  const auto &eit = std::as_const(edgeSummaryMap).find(callStmtId);
  if (eit != edgeSummaryMap.end()) {
    callEdgeSummary = eit->second;
    callFrequency = callEdgeSummary->frequency;
    callBBPredicate = callEdgeSummary->predicate;
  }

  // (2) specialization for costTable in the fromSummary
  InlineCost unnecessaryCost{0, 0};
  std::vector<std::pair<const Predicate*, InlineCost>> spCostTable;  // specialized costTable
  auto condsNeedCopy
      = fromSummary.SpecializeCostTable(condResultVec, paramMapping, callFrequency, spCostTable, unnecessaryCost);
  // update static cost
  auto sizeGrowth = fromSummary.GetStaticInsns() - unnecessaryCost.GetInsns();
  AddStaticInsns(sizeGrowth);
  auto timeGrowth = fromSummary.GetStaticCycles() - unnecessaryCost.GetCycles();
  if (callFrequency > 0) {
    timeGrowth *= CallFreqPercent(callFrequency);
  }
  AddStaticCycles(timeGrowth);

  // (3) merge conditions, remap condition index, replace param index
  std::array<int8, kMaxNumCondition> oldCondIdx2New;
  MergeAndRemapConditions(fromSummary, condsNeedCopy, paramMapping, oldCondIdx2New);

  // Remap condition index
  for (uint32 i = 1; i < spCostTable.size(); ++i) {  // Skip the first truePredicate
    const Predicate *pred = spCostTable[i].first;
    const Predicate *newPred = nullptr;
    if (callBBPredicate->IsTruePredicate()) {
      newPred = pred->RemapCondIdx(oldCondIdx2New, summaryAlloc);
    } else {
      newPred = pred->RemapCondIdx(oldCondIdx2New, tmpAlloc);  // avoid meory leak
      newPred = Predicate::And(*newPred, *callBBPredicate, summaryAlloc);
    }
    spCostTable[i].first = newPred;
  }

  // Merge spCostTable into toSummary costTable
  costTable[0].second.Add(spCostTable[0].second);
  for (uint32 i = 1; i < spCostTable.size(); ++i) {
    costTable.push_back(spCostTable[i]);
  }
  MergeEdgeSummary(fromSummary, { callStmtId, callEdgeSummary }, tmpAlloc, oldCondIdx2New, callMeStmtIdMap);
  MergeArgInfosMap(fromSummary, callStmtId, callMeStmtIdMap);
}

// Return condIdx bitMap that need be copied to toSummary
Assert InlineSummary::SpecializeCostTable(const std::vector<ExprBoolResult> &condResultVec,
    const std::vector<int32> &paramMapping, int32 callFrequency,
    std::vector<std::pair<const Predicate*, InlineCost>> &spCostTable,
    InlineCost &unnecessaryCost) const {
  // The first costTable item is always truePredicate
  (void)spCostTable.emplace_back(Predicate::TruePredicate(), InlineCost{0, 0});
  Assert condsNeedCopy = 0;
  for (const auto &costPair : costTable) {
    auto *predicate = costPair.first;
    const auto &cost = costPair.second;
    auto res = predicate->Evaluate(condResultVec);
    if (res == kExprResFalse) {
      unnecessaryCost.Add(cost);  // unnecessaryCost will be scaled outside the function
      continue;  // There is no need to consider cost if predicate evaluates to false
    }
    if (res == kExprResTrue) {
      spCostTable[0].second.Add(cost, callFrequency);  // Accumulate cost for predicate with true result
      continue;
    }
    // Now we need handle predicates with "unknown" value
    if (!predicate->AreAllUsedParamsInParamMapping(paramMapping, conditions)) {
      // These predicates are "really unknown". To be conservative, we treat them as true predicates
      spCostTable[0].second.Add(cost, callFrequency);
      continue;
    }
    // These predicates are "maybe known", we will replace param index of these predicates later
    (void)spCostTable.emplace_back(predicate, cost.Scale(callFrequency));
    condsNeedCopy |= predicate->GetCondsUsed();
  }
  return condsNeedCopy;
}

void InlineSummary::MergeAndRemapConditions(const InlineSummary &fromSummary, Assert condsNeedCopy,
    const std::vector<int32> &paramMapping, std::array<int8, kMaxNumCondition> &oldCondIdx2New) {
  const auto newCondBeginIdx = conditions.size();
  auto currCondIdx = newCondBeginIdx;
  // Map old conditon index (in the fromSummary) to new condition index (in the toSummary)
  oldCondIdx2New.fill(-1);  // Init all elements with invalid value
  for (uint32 i = kCondIdxRealBegin; i < kMaxNumCondition; ++i) {
    if ((condsNeedCopy & (kAssertOne << i)) == 0) {
      continue;
    }
    if (currCondIdx >= kMaxNumCondition) {
      oldCondIdx2New[i] = kCondIdxOverflow;
      break;
    }
    oldCondIdx2New[i] = static_cast<int8>(currCondIdx);
    ++currCondIdx;
    Condition *fromCond = fromSummary.conditions[i];
    if (fromCond->GetParamsUsed() != 0) {
      // Clone a new cond and replace param index for new cond
      fromCond = fromCond->Clone(summaryAlloc);
      fromCond->GetExpr()->RemapParamIdx(paramMapping);
    }
    conditions.push_back(fromCond);
  }
}

void InlineSummary::MergeEdgeSummary(const InlineSummary &fromSummary,
    std::pair<uint32, InlineEdgeSummary*> callEdgeSummaryPair,
    MapleAllocator &tmpAlloc, const std::array<int8, kMaxNumCondition> &oldCondIdx2New,
    const std::map<uint32, uint32> &callMeStmtIdMap) {
  // Merge callStmtPredicates
  uint32 callStmtId = callEdgeSummaryPair.first;
  InlineEdgeSummary *callEdgeSummary = callEdgeSummaryPair.second;
  auto *callBBPredicate = callEdgeSummary ? callEdgeSummary->predicate : Predicate::TruePredicate();
  FreqType callBBFreq = callEdgeSummary ? callEdgeSummary->frequency : -1;
  for (auto &pair : fromSummary.edgeSummaryMap) {
    auto fromCallId = pair.first;
    auto jt = callMeStmtIdMap.find(fromCallId);
    if (jt == callMeStmtIdMap.end()) {
      // callStmt with fromCallId may be removed from callee by other phases (such as outline)
      continue;
    }
    fromCallId = jt->second;
    InlineEdgeSummary *newEdgeSummary = pair.second;
    CHECK_NULL_FATAL(newEdgeSummary);
    auto *newPred = newEdgeSummary->predicate;
    if (callBBPredicate->IsTruePredicate()) {
      newPred = newPred->RemapCondIdx(oldCondIdx2New, summaryAlloc);
    } else {
      newPred = newPred->RemapCondIdx(oldCondIdx2New, tmpAlloc);
      newPred = Predicate::And(*newPred, *callBBPredicate, summaryAlloc);
    }
    int32 newFrequency = newEdgeSummary->frequency;
    if (newFrequency == -1 || callBBFreq == -1) {
      newFrequency = -1;
    } else {
      newFrequency = static_cast<int32>(callBBFreq * newFrequency / static_cast<int64>(kCallFreqBase));
      if (newFrequency > kCallFreqMax) {
        newFrequency = kCallFreqMax;
      }
    }
    AddEdgeSummary(fromCallId, newPred, newFrequency, newEdgeSummary->callCost.GetInsns(),
        static_cast<int32>(newEdgeSummary->callCost.GetCycles()));
  }
  RemoveEdgeSummary(callStmtId);
}

void InlineSummary::MergeArgInfosMap(const InlineSummary &fromSummary, uint32 callStmtId,
    const std::map<uint32, uint32> &callMeStmtIdMap) {
  auto it = argInfosMap.find(callStmtId);
  if (it == argInfosMap.end()) {
    // The callsite has no real arguments, no need to remap param index.
    // Just simplely merge arg info and return
    for (const auto &pair : fromSummary.argInfosMap) {
      auto fromCallId = pair.first;
      auto jt = callMeStmtIdMap.find(fromCallId);
      if (jt != callMeStmtIdMap.end()) {
        fromCallId = jt->second;
      }
      (void)argInfosMap.emplace(fromCallId, pair.second);
    }
    return;
  }
  ArgInfoVec &currArgInfos = *it->second;
  for (const auto &pair : fromSummary.argInfosMap) {
    auto fromCallId = pair.first;
    auto cit = callMeStmtIdMap.find(fromCallId);
    if (cit == callMeStmtIdMap.end()) {
      // callStmt with fromCallId may be removed from callee by other phases (such as outline)
      continue;
    }
    fromCallId = cit->second;
    auto *argInfoVec = pair.second;
    CHECK_FATAL(argInfoVec != nullptr, "there should not be null argInfos");
    // key: callsite arg index, value: caller formal index
    auto *newArgInfoVec = summaryAlloc.New<ArgInfoVec>(summaryAlloc.Adapter());
    for (uint32 innerIdx = 0; innerIdx < argInfoVec->size(); ++innerIdx) {
      auto *argInfo = (*argInfoVec)[innerIdx];
      if (argInfo == nullptr) {
        newArgInfoVec->push_back(nullptr);
        continue;
      }
      if (!argInfo->IsParamPassThrough()) {
        newArgInfoVec->push_back(argInfo);  // keep argInfo with known value range
        continue;
      }
      // Param index remap example:
      // foo(int x, int y) {                   ... outerIdx = 1
      //   bar(41, 42, y); }                   ... midIdx   = 2
      // bar(int a, int b, int c) { hi(c); }   ... innerIdx = 0
      // when bar is inlined into foo, the formalIdx of hi(c) should be change from innerIdx to outerIdx.
      // If outerIdx is -1, this means hi(c) has no matters with new caller's formals, its argInfo has no use now.
      auto midIdx = static_cast<size_t>(static_cast<uint32>(argInfo->GetCallerFormalIdx()));
      if (midIdx >= currArgInfos.size() || currArgInfos[midIdx] == nullptr) {
        newArgInfoVec->push_back(nullptr);
        continue;
      }
      int32 outerIdx = currArgInfos[midIdx]->GetCallerFormalIdx();
      if (outerIdx == -1) {
        newArgInfoVec->push_back(nullptr);
        continue;
      }
      newArgInfoVec->push_back(summaryAlloc.New<ArgInfo>(outerIdx));
    }
    CHECK_FATAL(newArgInfoVec->size() == argInfoVec->size(), "must be");
    auto result = argInfosMap.emplace(fromCallId, newArgInfoVec);
    // Inline a same func twice
    CHECK_FATAL(result.second, "callStmtId conflicts");
  }
  (void)argInfosMap.erase(callStmtId);
}

void InlineSummary::DumpConditions() const {
  LogInfo::MapleLogger() << "===== DumpConditions =====\n";
  for (auto *cond : conditions) {
    if (cond == nullptr) {
      continue;
    }
    cond->Dump();
    LogInfo::MapleLogger() << "expr address: " << cond->GetExpr() << std::endl;
    LogInfo::MapleLogger() << std::endl;
  }
}

void InlineSummary::DumpCostTable() const {
  LogInfo::MapleLogger() << "===== DumpCostTable =====\n";
  uint32 i = 0;
  for (const auto &pair : costTable) {
    LogInfo::MapleLogger() << "[" << i++ << "] " << pair.first << ", insns: " << pair.second.GetInsns() <<
        ", cycles: " << pair.second.GetCycles() << std::endl;
    pair.first->Dump(conditions);
  }
}

void InlineSummary::DumpArgInfosMap() const {
  LogInfo::MapleLogger() << "===== DumpArgInfosMap =====\n";
  for (const auto &pair : argInfosMap) {
    auto callStmtId = pair.first;
    LogInfo::MapleLogger() << "callStmtId: " << callStmtId << std::endl;
    const auto &argValTable = *pair.second;
    for (uint32 i = 0; i < argValTable.size(); ++i) {
      const auto *argRange = argValTable[i];
      if (argRange == nullptr) {
        continue;
      }
      LogInfo::MapleLogger() << "  arg" << i << ": ";
      if (argRange->IsSingleValue()) {
        argRange->GetLower()->Dump(nullptr);
      } else {
        LogInfo::MapleLogger() << "[" << argRange->GetLower() << ", " << argRange->GetUpper() << "]";
      }
      LogInfo::MapleLogger() << std::endl;
    }
  }
}

void InlineSummary::DumpEdgeSummaryMap() const {
  LogInfo::MapleLogger() << "===== DumpEdgeSummaryMap =====\n";
  for (auto &pair : edgeSummaryMap) {
    auto *edgeSummary = pair.second;
    LogInfo::MapleLogger() << " callId: " << pair.first << ", freq: " << edgeSummary->frequency << std::endl;
  }
}

void InlineSummary::Dump() const {
  LogInfo::MapleLogger() << "Inline summary for func: " << func->GetName() << std::endl;
  LogInfo::MapleLogger() << "static insns: " << staticCost.GetInsns() <<
      ", static cycles: " << staticCost.GetCycles() << std::endl;
  DumpArgInfosMap();
  DumpCostTable();
  DumpConditions();
  DumpEdgeSummaryMap();
}

// All collected real arg values will be saved into inline summary.
void InlineSummaryCollector::CollectArgInfo(MeFunction &meFunc) {
  InlineSummary *summary = meFunc.GetMirFunc()->GetOrCreateInlineSummary();
  if (summary->IsArgInfoCollected()) {
    return;
  }
  summary->SetArgInfoCollected(true);
  // tmpAllocator is only used by EvaluateMeExpr.
  LocalMapleAllocator tmpAllocator(meFunc.GetStackMemPool());
  auto &sumAlloc = summary->GetSummaryAlloc();
  for (auto *bb : meFunc.GetCfg()->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    for (auto &stmt : bb->GetMeStmts()) {
      if (stmt.GetOp() != OP_call && stmt.GetOp() != OP_callassigned) {
        continue;
      }
      auto &callStmt = static_cast<CallMeStmt&>(stmt);
      auto &callee = callStmt.GetTargetFunction();
      bool recursiveCall = meFunc.GetMirFunc()->GetPuidx() == callee.GetPuidx();
      (void)callee;  // filter by callee if necessary
      for (uint32 i = 0; i < callStmt.GetOpnds().size(); ++i) {
        auto *argOpnd = callStmt.GetOpnd(i);
        // (1) Record mapping from argument index to caller parameter index
        // To improv: simple operation can be allowed to perform on caller's formals. such as (formal + constant)
        if (!recursiveCall) {  // param mapping not work for recursive call currently
          auto paramIndex = GetZeroVersionParamIndex(*argOpnd, meFunc);
          if (paramIndex.has_value()) {
            auto *argInfo = sumAlloc.New<ArgInfo>(paramIndex.value());
            summary->AddArgInfo(callStmt.GetMeStmtId(), i, argInfo);
          }
        }
        // (2) Evaluate arguments value
        auto *mirConst = EvaluateMeExpr(*argOpnd, meFunc, tmpAllocator);
        if (mirConst == nullptr) {
          continue;  // argOpnd is not a constant number
        }
        auto *argInfo = sumAlloc.New<ArgInfo>(mirConst, mirConst);
        summary->AddArgInfo(callStmt.GetMeStmtId(), i, argInfo);
      }
    }
  }
}

static void GetAllDomChildren(Dominance &dom, uint32 bbId, std::vector<uint32> &allChildren) {
  const auto &children = dom.GetDomChildren(bbId);
  for (auto id : children) {
    allChildren.push_back(id);
    GetAllDomChildren(dom, id, allChildren);
  }
}

void InlineSummaryCollector::InitUnlikelyBBs() {
  const auto &rpoBBs = dom.GetReversePostOrder();
  for (auto *node : rpoBBs) {
    if (node == nullptr || node->GetID() <= 1) {
      continue;
    }
    auto *bb = func->GetCfg()->GetBBFromID(BBId(node->GetID()));
    if (!bb->IsImmediateUnlikelyBB()) {
      continue;
    }
    (void)unlikelyBBs.insert(bb);
    std::vector<uint32> allChildren;
    GetAllDomChildren(dom, bb->GetID(), allChildren);
    for (auto id : allChildren) {
      auto *child = func->GetCfg()->GetBBFromID(BBId(id));
      (void)unlikelyBBs.insert(child);
    }
  }
}

// First iteration: compute succ edge predicate for condgoto bb and switch bb
void InlineSummaryCollector::ComputeEdgePredicate() {
  uint32 numBBs = func->GetCfg()->NumBBs();
  allBBPred.resize(numBBs, nullptr);
  for (uint32 i = 0; i < numBBs; ++i) {
    auto *bb = func->GetCfg()->GetAllBBs()[i];
    // We need also consider Switch BB in the future
    if (bb == nullptr || bb->GetKind() != kBBCondGoto) {
      continue;
    }
    // for CondGoto bb
    auto *lastStmt = bb->GetLastMe();
    ASSERT_NOT_NULL(lastStmt);
    CHECK_FATAL(lastStmt->GetOp() == OP_brtrue || lastStmt->GetOp() == OP_brfalse, "must be");
    auto *condStmt = static_cast<CondGotoMeStmt*>(lastStmt);
    CHECK_NULL_FATAL(condStmt);
    auto *condExpr = condStmt->GetOpnd(0);
    uint32 paramsUsed = 0;
    auto *condLiteExpr = GetOrCreateLiteExpr(*condExpr, kExprKindConstNumber | kExprKindParam, paramsUsed);
    if (condLiteExpr == nullptr) {
      continue;
    }
    // set succ edge predicates for current bb
    PreparePredicateForBB(*bb);
    allBBPred[i]->succEdgePredicates.resize(bb->GetSucc().size(), nullptr);
    for (uint32 j = 0; j < bb->GetSucc().size(); ++j) {
      auto *succ = bb->GetSucc(j);
      bool isBrtrue = condStmt->GetOp() == OP_brtrue;
      bool isJump = condStmt->GetOffset() == succ->GetBBLabel();
      bool isTrueBranch = (isBrtrue == isJump);
      allBBPred[i]->succEdgePredicates[j] =
          inlineSummary->AddCondition(condLiteExpr, !isTrueBranch, paramsUsed, tmpAlloc);
    }
  }
}

// Second iteration: propagate bb predicate in the CFG
// All predicates generated by this function are allocated in tmpAlloc
// All liteExprs and conditions are allocated in summaryAlloc
void InlineSummaryCollector::PropBBPredicate() {
  auto *commonEntry = func->GetCfg()->GetCommonEntryBB();
  CHECK_FATAL(commonEntry->GetSucc().size() == 1, "must be");
  auto *startBB = commonEntry->GetSucc(0);
  PreparePredicateForBB(*startBB);
  allBBPred[startBB->GetBBId()]->predicate = Predicate::TruePredicate();
  for (auto *node : dom.GetReversePostOrder()) {  // What about loops?
    auto bb = func->GetCfg()->GetBBFromID(BBId(node->GetID()));
    if (bb == nullptr || bb->GetBBId() <= 1) {
      continue;
    }
    uint32 bbId = bb->GetBBId().get();
    auto *p = Predicate::FalsePredicate();
    for (uint32 i = 0; i < bb->GetPred().size(); ++i) {
      auto *pred = bb->GetPred(i);
      auto *srcBBAux = allBBPred[pred->GetBBId()];
      if (srcBBAux != nullptr && srcBBAux->predicate != nullptr) {
        const Predicate *destBBPredicate = srcBBAux->predicate;
        int retIndex = pred->GetSuccIndex(*bb);
        CHECK_FATAL(retIndex >= 0, "must be");
        auto succIndex = static_cast<size_t>(static_cast<uint32>(retIndex));
        if (succIndex < srcBBAux->succEdgePredicates.size()) {
          auto *succEdgePredicate = srcBBAux->succEdgePredicates[succIndex];
          CHECK_NULL_FATAL(succEdgePredicate);
          destBBPredicate = Predicate::And(*destBBPredicate, *succEdgePredicate, tmpAlloc);
        }
        p = Predicate::Or(*destBBPredicate, *p, inlineSummary->GetConditions(), tmpAlloc);
        if (p->IsTruePredicate()) {
          break;
        }
      }
    }
    PreparePredicateForBB(*bb);
    if (allBBPred[bbId]->predicate == nullptr) {
      allBBPred[bbId]->predicate = p;
    }
  }
}

// Returned callFrequency value range: [0, kCallFreqMax]
static int32 ComputeCallFrequency(const BB &bb) {
  auto bbFreq = bb.GetFrequency();
  if (bbFreq == 0) {  // bb freq is invalid
    return kCallFreqBase;
  }
  auto freq = static_cast<int32>(bbFreq * 1.0 * kCallFreqBase / kFreqBase);
  if (freq > kCallFreqMax) {
    freq = kCallFreqMax;
  }
  return freq;
}

// Get approximated probability of `stmt` might be remove after inlining
int32 GetRemoveStmtProbAfterInlining(const MeStmt &stmt) {
  auto op = stmt.GetOp();
  if (op == OP_return) {
    return kRemoveStmtAll;
  }
  (void)kRemoveStmtHalf;
  return kRemoveStmtNone;
}

static void PrintStmtFreqAndCost(MeFunction &func, const MeStmt &stmt,
    int32 callFrequency, int32 stmtInsns, int32 stmtCycles) {
  auto &module = func.GetMIRModule();
  auto *oldFunc = module.CurFunction();
  module.SetCurFunction(func.GetMirFunc());
  stmt.GetSrcPosition().Dump();
  stmt.Dump(func.GetIRMap());
  module.SetCurFunction(oldFunc);
  LogInfo::MapleLogger() << "  freq: " << CallFreqPercent(callFrequency) << ", insns: " << stmtInsns <<
      ", cycles: " << stmtCycles << std::endl;
}

static void InlineAnalysisForCallStmt(const CallMeStmt &callStmt, const MIRFunction &caller,
    InlineSummary &callerSummary) {
  auto &targetFunc = callStmt.GetTargetFunction();
  if (targetFunc.GetName().find("setjmp") != std::string::npos) {
    callerSummary.SetInlineFailedCode(kIfcSetjmpInCallee);
  }
  // Recursive call is not supported by ginline for now.
  // Small recursive callee are supposed to be inlined by einline.
  if (targetFunc.GetPuidx() == caller.GetPuidx()) {
    callerSummary.SetInlineFailedCode(kIfcGinlineRecursiveCall);
  }
}

std::pair<int32, double> InlineSummaryCollector::AnalyzeCondCostForStmt(MeStmt &stmt, int32 callFrequency,
    const Predicate *bbPredClone, const BBPredicate &bbPredicate, bool isUnlikelyBB) {
  Opcode op = stmt.GetOp();
  const auto stmtInsns = stmtCostAnalyzer.EstimateNumInsns(&stmt);  // Estimated number of instructions
  const auto stmtCycles = stmtCostAnalyzer.EstimateNumCycles(&stmt);
  const auto stmtCyclesWithFreq = stmtCycles * CallFreqPercent(callFrequency);
  CHECK_FATAL(stmtInsns >= 0, "must be");
  CHECK_FATAL(stmtCyclesWithFreq >= 0, "must be");
  if (debug) {
    PrintStmtFreqAndCost(*func, stmt, callFrequency, stmtInsns, stmtCycles);
  }
  // Record inline edge summary
  bool isCall = op == OP_call || op == OP_callassigned;
  if (isCall) {
    if (bbPredClone == nullptr) {
      // We need clone call bb predicate from tmpAlloc to summaryAlloc
      bbPredClone = bbPredicate.predicate->Clone(summaryAlloc);
    }
    // Init inline edge summary
    // Does edge summary need loop depth info of callBB?
    auto newEdgeSummary =
        inlineSummary->AddEdgeSummary(stmt.GetMeStmtId(), bbPredClone, callFrequency, stmtInsns, stmtCycles);
    if (newEdgeSummary != nullptr && isUnlikelyBB) {
      newEdgeSummary->SetAttr(CallsiteAttrKind::kUnlikely);
    }
    InlineAnalysisForCallStmt(static_cast<CallMeStmt&>(stmt), *func->GetMirFunc(), *inlineSummary);
  } else if (op == OP_switch) {
    auto &switchStmt = static_cast<SwitchMeStmt&>(stmt);
    constexpr size_t numSwitchCases = 8;
    if (switchStmt.GetSwitchTable().size() >= numSwitchCases) {
      inlineSummary->SetHasBigSwitch(true);
    }
  }
  if (stmtInsns != 0 || stmtCyclesWithFreq > 0) {
    int32 removeProb = GetRemoveStmtProbAfterInlining(stmt);
    if (removeProb == kRemoveStmtAll) {
      auto *notInlinePredicate = Predicate::NotInlinePredicate();
      auto *predicate = Predicate::And(*bbPredicate.predicate, *notInlinePredicate, tmpAlloc);
      inlineSummary->AddCondCostItem(*predicate, stmtInsns, stmtCyclesWithFreq, debug);
    } else {
      inlineSummary->AddCondCostItem(*bbPredicate.predicate, stmtInsns, stmtCyclesWithFreq, debug);
    }
  }
  return { stmtInsns, stmtCyclesWithFreq };
}

void InlineSummaryCollector::ComputeConditionalCost() {
  ASSERT(inlineSummary != nullptr, "should be created before");
  NumInsns staticSize = 0;
  NumCycles staticTime = 0;
  // Init notInline size cost
  constexpr NumInsns notInlineSizeCost = 2;
  inlineSummary->AddCondCostItem(*Predicate::NotInlinePredicate(), notInlineSizeCost, 0, debug);
  const auto &rpoBBs = dom.GetReversePostOrder();
  for (auto *node : rpoBBs) {
    if (node == nullptr || node->GetID() <= 1) {
      continue;
    }
    auto bb = func->GetCfg()->GetBBFromID(BBId(node->GetID()));
    bool isUnlikelyBB = (unlikelyBBs.find(bb) != unlikelyBBs.end());
    BBPredicate *bbPredicate = allBBPred[bb->GetBBId()];
    const Predicate *bbPredClone = nullptr;
    if (debug) {
      LogInfo::MapleLogger() << "\nBB " << bb->GetBBId() << " predicate: ";
      bbPredicate->predicate->Dump(inlineSummary->GetConditions());
    }
    int32 callFrequency = ComputeCallFrequency(*bb);
    CHECK_FATAL(callFrequency >= 0, "must be");
    MeStmt *stmt = bb->GetFirstMe();
    while (stmt != nullptr) {
      auto *next = stmt->GetNextMeStmt();
      auto op = stmt->GetOp();
      if (op == OP_comment) {
        stmt = next;
        continue;
      }
      auto cost = AnalyzeCondCostForStmt(*stmt, callFrequency, bbPredClone, *bbPredicate, isUnlikelyBB);
      // Record static cost
      const auto stmtInsns = cost.first;
      const auto stmtCyclesWithFreq = cost.second;
      staticSize += stmtInsns;
      staticTime += stmtCyclesWithFreq;
      // CondgotoStmt can be ignored if the condition expr is constant
      // we can use 'true' succ edge predicate of current bb
      stmt = next;
    }
  }
  inlineSummary->SetStaticCost(staticSize, staticTime);
  // Clone necessary predicates from tmpAlloc to summaryAlloc
  inlineSummary->CloneAllPredicatesToSummaryAlloc();
}

InlineCost GetCondInlineCost(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt) {
  auto *memPool = memPoolCtrler.NewMemPool("", true);
  MapleAllocator tmpAlloc(memPool);
  auto *callerSummary = caller.GetInlineSummary();
  CHECK_NULL_FATAL(callerSummary);
  auto *calleeSummary = callee.GetInlineSummary();
  CHECK_NULL_FATAL(calleeSummary);
  auto callStmtId = callStmt.GetStmtID();
  const auto &argInfosMap = callerSummary->GetArgInfosMap();
  auto &conditionalCostTable = calleeSummary->GetCostTable();
  ArgInfoVec *argInfoVec = nullptr;
  auto it = argInfosMap.find(callStmtId);
  if (it != argInfosMap.end()) {
    argInfoVec = it->second;
  }
  std::vector<ExprBoolResult> condResultVec;
  calleeSummary->EvaluateConditions(tmpAlloc, argInfoVec, condResultVec, true);
  int32 condSize = 0;
  double condTime = 0;
  for (const auto &pair : conditionalCostTable) {
    const auto *predicate = pair.first;
    // If predicate is TRUE or UNKNOWN, cost should be appended
    if (predicate->Evaluate(condResultVec) != kExprResFalse) {
      const auto &inlineCost = pair.second;
      condSize += inlineCost.GetInsns();
      condTime += inlineCost.GetCycles();
    }
  }
  memPoolCtrler.DeleteMemPool(memPool);
  return { condSize, condTime };
}
}  // namespace maple

