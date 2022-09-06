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
#include "outline.h"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "cfg_primitive_types.h"
#include "global_tables.h"
#include "ipa_collect.h"
#include "ipa_phase_manager.h"
#include "mir_builder.h"
#include "mir_function.h"
#include "mir_nodes.h"
#include "mir_symbol.h"
#include "mir_symbol_builder.h"
#include "mir_type.h"
#include "mpl_logging.h"
#include "opcodes.h"
#include "region_identify.h"
#include "inline_analyzer.h"
#include "stmt_identify.h"
#include "types_def.h"

namespace maple {
static constexpr size_t kParameterNumberLimit = 8;
static constexpr size_t kOneInsn = 100;
static constexpr char kOutlinePhase[] = "outline";

static bool ShareSameParameter(const BaseNode &lhs, const BaseNode &rhs) {
  if (lhs.GetOpCode() == OP_addrof) {
    return static_cast<const AddrofNode &>(lhs).AccessSameMemory(&rhs);
  }
  if (rhs.GetOpCode() == OP_addrof) {
    return static_cast<const AddrofNode &>(rhs).AccessSameMemory(&lhs);
  }
  return lhs.IsSameContent(&rhs);
}

template<typename Functor>
static void ForEachOpnd(BaseNode &opnd, BaseNode *parent, Functor &opndProcessor) {
  if (opnd.GetOpCode() == OP_block) {
    auto &stmtList = static_cast<BlockNode &>(opnd).GetStmtNodes();
    for (auto it = stmtList.begin(); it != stmtList.end(); ++it) {
      ForEachOpnd(*to_ptr(it), parent, opndProcessor);
    }
    return;
  }
  opndProcessor(opnd, parent);
  for (size_t i = 0; i < opnd.GetNumOpnds(); ++i) {
    ForEachOpnd(*opnd.Opnd(i), &opnd, opndProcessor);
  }
}

void OutlineCandidate::InsertIntoParameterList(BaseNode &expr) {
  for (size_t i = 0; i < parameters.size(); ++i) {
    if (!ShareSameParameter(expr, *parameters[i])) {
      continue;
    }
    exprToParameterIndex[&expr] = i;
    if (expr.GetOpCode() == OP_addrof) {
      parameters[i]->SetOpCode(OP_addrof);
    }
    return;
  }
  exprToParameterIndex[&expr] = parameters.size();
  (void)parameters.emplace_back(expr.CloneTree(regionCandidate->GetFunction()->GetCodeMempoolAllocator()));
}

class OutlineInfoCollector {
 public:
  explicit OutlineInfoCollector(OutlineCandidate *candidate) : candidate(candidate) {}
  ~OutlineInfoCollector() {
    candidate = nullptr;
  }
  void operator() (BaseNode &node, BaseNode *parent) {
    (void)parent;
    switch (node.GetOpCode()) {
      case OP_callassigned:
      case OP_icallassigned:
      case OP_icallprotoassigned:
      case OP_intrinsiccallassigned:
      case OP_callinstantassigned:
      case OP_regassign:
      case OP_dassign: {
        auto &regionOutPuts = candidate->GetRegionCandidate()->GetRegionOutPuts();
        auto symbolRegPair = RegionCandidate::GetSymbolRegPair(node);
        if (regionOutPuts.find(symbolRegPair) != regionOutPuts.end()) {
          candidate->CreateReturnExpr(symbolRegPair);
          CollectParameter(*candidate->GetReturnExpr());
        }
        break;
      }
      case OP_dread:
      case OP_addrof:
      case OP_regread: {
        CollectParameter(node);
        break;
      }
      case OP_constval:
      case OP_conststr:
      case OP_conststr16: {
        auto &valueNumberToExprMap = candidate->GetRegionValueNumberToSrcExprMap();
        (void)valueNumberToExprMap.emplace_back(&node);
        break;
      }
      default:
        break;
    }
  }
 private:
  void CollectParameter(BaseNode &node) {
    auto &regionInputs = candidate->GetRegionCandidate()->GetRegionInPuts();
    auto symbolRegPair = RegionCandidate::GetSymbolRegPair(node);
    if (regionInputs.find(symbolRegPair) != regionInputs.end()) {
      candidate->InsertIntoParameterList(node);
    }
  }
  OutlineCandidate *candidate;
};

class OutlineRegionExtractor {
 public:
  OutlineRegionExtractor(OutlineCandidate *candidate, MIRFunction *func)
      : candidate(candidate), newFunc(func),
        oldFunc(candidate->GetRegionCandidate()->GetFunction()),
        builder(oldFunc->GetModule()->GetMIRBuilder()) {
    oldFunc->SetCurrentFunctionToThis();
  }
  ~OutlineRegionExtractor() {
    candidate = nullptr;
    newFunc = nullptr;
    oldFunc = nullptr;
    builder = nullptr;
  };

  void operator() (BaseNode &node, BaseNode *parent) {
    auto &exprToParameterMap = candidate->GetExprToParameterIndexMap();
    switch (node.GetOpCode()) {
      case OP_addrof:
      case OP_dread: {
        if (exprToParameterMap.find(&node) != exprToParameterMap.end() &&
            candidate->GetParameters()[exprToParameterMap[&node]]->GetOpCode() == OP_addrof) {
          ReplaceExpr(node, parent);
          break;
        }
        ReplaceStIdx<DreadNode>(node);
        break;
      }
      case OP_dassign: {
        ReplaceStIdx<DassignNode>(node);
        break;
      }
      case OP_regread: {
        ReplacePregIdx<RegreadNode>(node);
        break;
      }
      case OP_regassign: {
        ReplacePregIdx<RegassignNode>(node);
        break;
      }
      case OP_asm:
      case OP_callassigned:
      case OP_icallassigned:
      case OP_icallprotoassigned:
      case OP_intrinsiccallassigned: {
        ReplaceCallReturnVector(node);
        break;
      }
      case OP_brtrue:
      case OP_brfalse: {
        ReplaceLableIdx<CondGotoNode>(node);
        break;
      }
      case OP_goto: {
        ReplaceLableIdx<GotoNode>(node);
        break;
      }
      case OP_label: {
        auto &labelNode = static_cast<LabelNode&>(node);
        labelNode.SetLabelIdx(GetOrCreateNewLabelIdx(labelNode.GetLabelIdx()));
        break;
      }
      default: {
        if (exprToParameterMap.find(&node) != exprToParameterMap.end()) {
          ReplaceExpr(node, parent);
        }
        break;
      }
    }
  }

  void GenerateInputAndOutput() {
    ReplaceOutput();
    GenerateInput();
    GenerateOutput();
  }

 private:
  StIdx CreateNewStIdx(const StIdx oldStIdx) {
    auto *oldSym = oldFunc->GetSymbolTabItem(oldStIdx.Idx());
    auto *newSym = newFunc->GetSymTab()->CloneLocalSymbol(*oldSym);
    newSym->SetIsTmp(true);
    newSym->ResetIsDeleted();
    if (newSym->IsFormal()) {
      newSym->SetStorageClass(kScAuto);
    }
    auto flag = newFunc->GetSymTab()->AddStOutside(newSym);
    CHECK_FATAL(flag, "symbol clone failed");
    return newSym->GetStIdx();
  }

  StIdx GetOrCreateNewStIdx(const StIdx stIdx) {
    if (stIdx.IsGlobal()) {
      return stIdx;
    }
    if (symbolMap.find(stIdx) == symbolMap.end()) {
      auto newStIdx = CreateNewStIdx(stIdx);
      symbolMap[stIdx] = newStIdx;
    }
    return symbolMap[stIdx];
  }

  PregIdx GetOrCreateNewPregIdx(PregIdx pregIdx) {
    if (regMap.find(pregIdx) == regMap.end()) {
      auto *preg = oldFunc->GetPregItem(pregIdx);
      auto newPregIdx = newFunc->GetPregTab()->ClonePreg(*preg);
      regMap[pregIdx] = newPregIdx;
    }
    return regMap[pregIdx];
  }

  LabelIdx GetOrCreateEndLabelIdx() {
    if (endLabel == kInvalidLabelIdx) {
      endLabel = newFunc->GetLabelTab()->CreateLabel();
    }
    return endLabel;
  }

  LabelIdx GetOrCreateNewLabelIdx(LabelIdx labelIdx) {
    if (labelMap.find(labelIdx) == labelMap.end()) {
      auto newLabelIdx = newFunc->GetLabelTab()->CreateLabel();
      labelMap[labelIdx] = newLabelIdx;
    }
    return labelMap[labelIdx];
  }

  MIRSymbol *CreateNewSymbolForParameter(PrimType primType) {
    auto symbolNameString = "parameter_" + std::to_string(++newParameterSymbolCount);
    bool created = false;
    auto *symbol = builder->GetOrCreateLocalDecl(symbolNameString, TyIdx(primType), *newFunc->GetSymTab(), created);
    CHECK_FATAL(created, "parameter created before when outlining");
    symbol->SetSKind(kStVar);
    return symbol;
  }

  BaseNode *GetOrCreateNewExpr(BaseNode &node) {
    auto *srcNode = &node;
    auto &exprToParameterMap = candidate->GetExprToParameterIndexMap();
    if (exprToParameterMap.find(&node) == exprToParameterMap.end()) {
      return srcNode;
    }
    srcNode = candidate->GetParameters()[exprToParameterMap[srcNode]];
    BaseNode *returnNode = nullptr;
    if (exprMap.find(srcNode) != exprMap.end()) {
      returnNode = exprMap[srcNode]->CloneTree(oldFunc->GetCodeMemPoolAllocator());
    } else {
      auto primType = node.IsConstExpr() ? srcNode->GetPrimType() : GetExactPtrPrimType();
      auto *newSymbol = CreateNewSymbolForParameter(primType);
      returnNode = builder->CreateExprDread(*newSymbol);
      exprMap[srcNode] = returnNode;
    }
    if (node.GetOpCode() == OP_dread) {
      auto &dread = static_cast<DreadNode&>(node);
      auto *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(TyIdx(dread.GetPrimType()));
      return builder->CreateExprIread(dread.GetPrimType(), ptrType->GetTypeIndex(), 0, returnNode);
    }
    return returnNode;
  }

  template<typename T>
  void ReplaceStIdx(BaseNode &node) {
    auto &srcNode = static_cast<T&>(node);
    auto stIdx = srcNode.GetStIdx();
    if (stIdx.IsGlobal()) {
      return;
    }
    srcNode.SetStIdx(GetOrCreateNewStIdx(stIdx));
  }

  template<typename T>
  void ReplacePregIdx(BaseNode &node) {
    auto &srcNode = static_cast<T&>(node);
    auto pregIdx = srcNode.GetRegIdx();
    srcNode.SetRegIdx(GetOrCreateNewPregIdx(pregIdx));
  }

  void ReplaceCallReturnVector(BaseNode &node) {
    auto *callReturnVector = node.GetCallReturnVector();
    if (callReturnVector->empty()) {
      return;
    }
    for (auto &returnPair : *callReturnVector) {
      if (returnPair.second.IsReg()) {
        auto pregIdx = returnPair.second.GetPregIdx();
        returnPair.second.SetPregIdx(GetOrCreateNewPregIdx(pregIdx));
      } else {
        auto stIdx = returnPair.first;
        returnPair.first = GetOrCreateNewStIdx(stIdx);
      }
    }
  }

  template<typename T>
  void ReplaceLableIdx(BaseNode &node) {
    auto &srcNode = static_cast<T&>(node);
    auto labelIdx = srcNode.GetOffset();
    auto &stmtJumpToEnd = candidate->GetRegionCandidate()->GetStmtJumpToEnd();
    if (stmtJumpToEnd.find(&node) != stmtJumpToEnd.end()) {
      srcNode.SetOffset(GetOrCreateEndLabelIdx());
    } else {
      srcNode.SetOffset(GetOrCreateNewLabelIdx(labelIdx));
    }
  }

  void ReplaceExpr(BaseNode &node, BaseNode* parent) {
    if (!parent) {
      return;
    }
    for (size_t i = 0; i < parent->NumOpnds(); ++i) {
      if (parent->Opnd(i) == &node) {
        parent->SetOpnd(GetOrCreateNewExpr(node), i);
      }
    }
  }

  void ReplaceOutput() {
    auto *node = candidate->GetReturnExpr();
    if (node == nullptr) {
      return;
    }
    switch (node->GetOpCode()) {
      case OP_dread: {
        ReplaceStIdx<DreadNode>(*node);
        break;
      }
      case OP_regread: {
        ReplacePregIdx<RegreadNode>(*node);
        break;
      }
      default: {
        CHECK_FATAL(false, "unexpected op for return value");
        break;
      }
    }
  }

  void GenerateInput() {
    for (auto *parameter : candidate->GetParameters()) {
      MIRSymbol *symbol = nullptr;
      switch (parameter->GetOpCode()) {
        case OP_dread: {
          ReplaceStIdx<DreadNode>(*parameter);
          auto *dread = static_cast<DreadNode*>(parameter);
          auto stIdx = dread->GetStIdx();
          symbol = newFunc->GetSymbolTabItem(stIdx.Idx());
          symbol->SetStorageClass(kScFormal);
          break;
        }
        case OP_regread: {
          ReplacePregIdx<RegreadNode>(*parameter);
          auto *regread = static_cast<RegreadNode*>(parameter);
          auto pregIdx = regread->GetRegIdx();
          symbol = builder->CreatePregFormalSymbol(TyIdx(regread->GetPrimType()), pregIdx, *newFunc);
          break;
        }
        case OP_addrof:
        case OP_constval:
        case OP_conststr:
        case OP_conststr16: {
          auto *dread = static_cast<DreadNode*>(exprMap[parameter]);
          auto stIdx = dread->GetStIdx();
          symbol = newFunc->GetSymbolTabItem(stIdx.Idx());
          symbol->SetStorageClass(kScFormal);
          break;
        }
        default: {
          CHECK_FATAL(false, "unexpected op code for formal symbol creation");
          break;
        }
      }
      (void)newFunc->GetFormalDefVec().emplace_back(FormalDef(symbol, symbol->GetTyIdx(), symbol->GetAttrs()));
      (void)newFunc->GetMIRFuncType()->GetParamTypeList().emplace_back(symbol->GetTyIdx());
      (void)newFunc->GetMIRFuncType()->GetParamAttrsList().emplace_back(symbol->GetAttrs());
    }
  }

  void GenerateOutput() {
    newFunc->GetModule()->SetCurFunction(newFunc);
    if (endLabel != kInvalidLabelIdx) {
      newFunc->GetBody()->AddStatement(builder->CreateStmtLabel(endLabel));
    }
    auto *returnNode = newFunc->GetCodeMemPool()->New<NaryStmtNode>(newFunc->GetCodeMempoolAllocator(), OP_return);
    newFunc->GetBody()->AddStatement(returnNode);
    if (candidate->GetReturnExpr() != nullptr) {
      returnNode->PushOpnd(candidate->GetReturnExpr()->CloneTree(newFunc->GetCodeMempoolAllocator()));
    }
  }

  OutlineCandidate *candidate;
  MIRFunction *newFunc;
  MIRFunction *oldFunc;
  MIRBuilder *builder;
  LabelIdx endLabel = kInvalidLabelIdx;
  uint32 newParameterSymbolCount = 0;
  std::unordered_map<StIdx, StIdx> symbolMap;
  std::unordered_map<PregIdx, PregIdx> regMap;
  std::unordered_map<LabelIdx, LabelIdx> labelMap;
  std::unordered_map<BaseNode*, BaseNode*> exprMap;
};

void OutlineGroup::CollectOutlineInfo() {
  for (auto &outlineCandidate : regionGroup) {
    auto *region = outlineCandidate.GetRegionCandidate();
    auto collector = OutlineInfoCollector(&outlineCandidate);
    region->TraverseRegion([&collector] (const StmtIterator &it) {
      ForEachOpnd(*to_ptr(it), nullptr, collector);
    });
  }
}

void OutlineGroup::PrepareExtraParameterLists() {
  auto candidateStart = regionGroup.begin();
  for (size_t i = 0; i < candidateStart->GetRegionValueNumberToSrcExprMap().size(); ++i) {
    for (auto candidateIter = std::next(candidateStart); candidateIter != regionGroup.end(); ++candidateIter) {
      auto *leftConst = candidateStart->GetRegionValueNumberToSrcExprMap()[i];
      auto *rightConst = candidateIter->GetRegionValueNumberToSrcExprMap()[i];
      if (!leftConst->IsSameContent(rightConst)) {
        extraParameterValueNumber.push_back(i);
        break;
      }
    }
  }
  for (auto &candidate : regionGroup) {
    for (auto valueNumber : extraParameterValueNumber) {
      auto *expr = candidate.GetRegionValueNumberToSrcExprMap()[valueNumber];
      candidate.InsertIntoParameterList(*expr);
    }
  }
}

void OutlineGroup::ReplaceOutlineCandidateWithCall() {
  for (auto &candidate : regionGroup) {
    auto *region = candidate.GetRegionCandidate();
    auto *block = region->GetStart()->GetCurrBlock();
    auto hasReturnvalue = !candidate.GetRegionCandidate()->GetRegionOutPuts().empty();
    auto op = hasReturnvalue ? OP_callassigned : OP_call;
    auto &alloc = outlineFunc->GetCodeMempoolAllocator();
    auto *stmt = outlineFunc->GetCodeMemPool()->New<CallNode>(alloc, op, outlineFunc->GetPuidx());
    stmt->SetNumOpnds(static_cast<uint8>(candidate.GetParameters().size()));
    if (hasReturnvalue) {
      auto symbolRegPair = candidate.GetRegionCandidate()->GetRegionOutPuts().begin();
      auto stIdx = symbolRegPair->first;
      auto pregIdx = symbolRegPair->second;
      (void)stmt->GetCallReturnVector()->emplace_back(CallReturnPair(stIdx, RegFieldPair(0, pregIdx)));
    }
    for (auto *parameter : candidate.GetParameters()) {
      (void)stmt->GetNopnd().emplace_back(parameter->CloneTree(alloc));
    }
    block->InsertBefore(region->GetStart()->GetStmtNode(), stmt);
  }
}

void OutlineGroup::CompleteOutlineFunction() {
  auto &candidate = regionGroup.front();
  auto *region = candidate.GetRegionCandidate();
  auto extractor = OutlineRegionExtractor(&candidate, outlineFunc);
  region->TraverseRegion([&extractor, this] (const StmtIterator &it) {
    ForEachOpnd(*to_ptr(it), nullptr, extractor);
    auto *newStmt = (it)->CloneTree(outlineFunc->GetCodeMempoolAllocator());
    outlineFunc->GetBody()->AddStatement(newStmt);
  });
  extractor.GenerateInputAndOutput();
}

void OutlineGroup::EraseOutlineRegion() {
  for (auto &candidate : regionGroup) {
    auto *region = candidate.GetRegionCandidate();
    auto *block = region->GetStart()->GetCurrBlock();
    region->TraverseRegion([block] (const StmtIterator &it) {
      block->RemoveStmt(to_ptr(it));
    });
  }
}

static size_t GetBenefit(OutlineGroup &group, InlineAnalyzer &inlineAnalyzer) {
  auto &outlineCandidate = group.GetOutlineRegions().front();
  auto *region = outlineCandidate.GetRegionCandidate();
  inlineAnalyzer.SetFunction(region->GetFunction());
  size_t cost = 0;
  for (auto *currStmt = region->GetStart()->GetStmtNode(); currStmt != nullptr; currStmt = currStmt->GetNext()) {
    if (currStmt->GetStmtInfoId() > region->GetEndId()) {
      break;
    }
    cost += static_cast<size_t>(inlineAnalyzer.GetStmtCost(currStmt));
  }
  return cost * (group.GetOutlineRegions().size() - 1);
}

static size_t GetCost(OutlineGroup &group, InlineAnalyzer &inlineAnalyzer) {
  auto &outlineCandidate = group.GetOutlineRegions().front();
  auto *region = outlineCandidate.GetRegionCandidate();
  inlineAnalyzer.SetFunction(region->GetFunction());
  size_t cost = kOneInsn;
  for (auto *expr : outlineCandidate.GetParameters()) {
    cost += static_cast<size_t>(inlineAnalyzer.GetExprCost(expr));
  }
  return cost * group.GetOutlineRegions().size();
}

static bool RegionGroupComparator(RegionGroup &lhs, RegionGroup &rhs) {
  auto leftPotentialBenefit = lhs.GetGroups().size() * lhs.GetGroups().front().GetLength();
  auto rightPotentialBenefit = rhs.GetGroups().size() * rhs.GetGroups().front().GetLength();
  return leftPotentialBenefit > rightPotentialBenefit;
}

void OutLine::PruneCandidateRegionsOverlapWithOutlinedRegion(RegionGroup &group) {
  auto &groupVector = group.GetGroups();
  auto it = groupVector.begin();
  while (it != groupVector.end()) {
    bool isOverlaped = false;
    for (auto *region : outlinedRegion) {
      if (!it->IsOverlapWith(*region)) {
        continue;
      }
      isOverlaped = true;
      (void)groupVector.erase(it);
      break;
    }
    if (!isOverlaped) {
      ++it;
    }
  }
  groupVector.shrink_to_fit();
}

void OutLine::Run() {
  RegionIdentify identifier(ipaInfo);
  identifier.RegionInit();
  auto &candidateGroups = identifier.GetRegionGroups();
  auto inlineAnalyzer = InlineAnalyzer(memPool);
  std::sort(candidateGroups.begin(), candidateGroups.end(), RegionGroupComparator);
  for (auto &group : candidateGroups) {
    PruneCandidateRegionsOverlapWithOutlinedRegion(group);
    if (group.GetGroups().size() < kGroupSizeLimit) {
      continue;
    }

    auto outlineGroup = OutlineGroup(group);
    outlineGroup.PrepareParameterLists();
    if (outlineGroup.GetParameterSize() > kParameterNumberLimit) {
      continue;
    }

    auto cost = GetCost(outlineGroup, inlineAnalyzer);
    auto benefit = GetBenefit(outlineGroup, inlineAnalyzer);
    if (benefit - cost < Options::outlineThreshold) {
      continue;
    }

    for (auto &region : group.GetGroups()) {
      (void)outlinedRegion.emplace_back(&region);
    }

    if (Options::dumpPhase == kOutlinePhase) {
      outlineGroup.Dump();
    }

    auto *outlineFunc = CreateNewOutlineFunction(group.GetGroups().front().GetOutputPrimType());
    outlineGroup.SetOutlineFunction(outlineFunc);
    outlineGroup.ReplaceOutlineRegions();
  }
}

MIRFunction *OutLine::CreateNewOutlineFunction(PrimType returnType) {
  auto newFuncName = std::string("outlined_func." + std::to_string(++newFuncIndex));
  auto *newFunc = module->GetMIRBuilder()->GetOrCreateFunction(newFuncName, TyIdx(returnType));
  (void)module->GetFunctionList().emplace_back(newFunc);
  newFunc->SetBody(newFunc->GetCodeMempool()->New<BlockNode>());
  newFunc->SetAttr(FUNCATTR_static);
  newFunc->SetAttr(FUNCATTR_noinline);
  newFunc->AllocSymTab();
  newFunc->AllocPregTab();
  newFunc->AllocLabelTab();
  return newFunc;
}

bool M2MOutline::PhaseRun(MIRModule &m) {
  auto *ipaInfo = GET_ANALYSIS(IpaSccPM, m);
  MemPool *memPool = ApplyTempMemPool();
  OutLine outline(ipaInfo, &m, memPool);
  outline.Run();
  return true;
}

void M2MOutline::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<IpaSccPM>();
}
}
