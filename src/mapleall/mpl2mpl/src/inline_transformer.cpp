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
#include "inline_transformer.h"

#include <fstream>
#include <iostream>

#include "global_tables.h"
#include "mpl_logging.h"
#include "inline_summary.h"

namespace maple {
// This phase replaces a function call site with the body of the called function.
//   Step 0: See if CALLEE have been inlined to CALLER once.
//   Step 1: Clone CALLEE's body.
//   Step 2: Rename symbols, labels, pregs.
//   Step 3: Replace symbols, labels, pregs.
//   Step 4: Null check 'this' and assign actuals to formals.
//   Step 5: Insert the callee'return jump dest label.
//   Step 6: Handle return values.
//   Step 7: Remove the successive goto statement and label statement in some circumstances.
//   Step 8: Replace the call-stmt with new CALLEE'body.
//   Step 9: Update inlined_times.

// Common rename function
uint32 InlineTransformer::RenameSymbols(uint32 inlinedTimes) const {
  uint32 symTabSize = static_cast<uint32>(callee.GetSymbolTabSize());
  uint32 stIdxOff = static_cast<uint32>(caller.GetSymTab()->GetSymbolTableSize()) - 1;
  for (uint32 i = 0; i < symTabSize; ++i) {
    const MIRSymbol *sym = callee.GetSymbolTabItem(i);
    if (sym == nullptr) {
      continue;
    }
    CHECK_FATAL(sym->GetStorageClass() != kScPstatic, "pstatic symbols should have been converted to fstatic ones");
    std::string syName(kUnderlineStr);
    // Use puIdx here instead of func name because our mangled func name can be
    // really long.
    (void)syName.append(std::to_string(callee.GetPuidx()))
        .append(kVerticalLineStr)
        .append((sym->GetName() == "") ? std::to_string(i) : sym->GetName())
        .append(kUnderlineStr);
    if (!theMIRModule->firstInline) {
      syName += kSecondInlineSymbolSuffix;
    }
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(syName + std::to_string(inlinedTimes));
    if (sym->GetScopeIdx() == 0) {
      CHECK_FATAL(false, "sym->GetScopeIdx() should be != 0");
    }
    MIRSymbol *newSym = nullptr;
    newSym = caller.GetSymTab()->CloneLocalSymbol(*sym);
    newSym->SetNameStrIdx(strIdx);
    if (newSym->GetStorageClass() == kScFormal) {
      newSym->SetStorageClass(kScAuto);
    }
    newSym->SetIsTmp(true);
    newSym->ResetIsDeleted();
    if (!caller.GetSymTab()->AddStOutside(newSym)) {
      CHECK_FATAL(false, "Reduplicate names.");
    }
    if (newSym->GetStIndex() != (i + stIdxOff)) {
      CHECK_FATAL(false, "wrong symbol table index");
    }
    CHECK_FATAL(caller.GetSymTab()->IsValidIdx(newSym->GetStIndex()), "symbol table index out of range");
  }
  return stIdxOff;
}

static StIdx UpdateIdx(const StIdx &stIdx, uint32 stIdxOff, const std::vector<uint32> *oldStIdx2New) {
  // If the callee has pstatic symbols, we will save all symbol mapping info in the oldStIdx2New.
  // So if this oldStIdx2New is nullptr, we only use stIdxOff to update stIdx, otherwise we only use oldStIdx2New.
  StIdx newStIdx = stIdx;
  if (oldStIdx2New == nullptr) {
    newStIdx.SetIdx(newStIdx.Idx() + stIdxOff);
  } else {
    CHECK_FATAL(newStIdx.Idx() < oldStIdx2New->size(), "stIdx out of range");
    newStIdx.SetFullIdx((*oldStIdx2New)[newStIdx.Idx()]);
  }
  return newStIdx;
}

void InlineTransformer::ReplaceSymbols(BaseNode *baseNode, uint32 stIdxOff, const std::vector<uint32> *oldStIdx2New) {
  if (baseNode == nullptr) {
    return;
  }
  CallReturnVector *returnVector = baseNode->GetCallReturnVector();
  if (baseNode->GetOpCode() == OP_block) {
    BlockNode *blockNode = static_cast<BlockNode*>(baseNode);
    for (auto &stmt : blockNode->GetStmtNodes()) {
      ReplaceSymbols(&stmt, stIdxOff, oldStIdx2New);
    }
  } else if (baseNode->GetOpCode() == OP_dassign) {
    DassignNode *dassNode = static_cast<DassignNode*>(baseNode);
    // Skip globals.
    if (dassNode->GetStIdx().Islocal()) {
      dassNode->SetStIdx(UpdateIdx(dassNode->GetStIdx(), stIdxOff, oldStIdx2New));
    }
  } else if ((baseNode->GetOpCode() == OP_addrof || baseNode->GetOpCode() == OP_dread)) {
    AddrofNode *addrNode = static_cast<AddrofNode*>(baseNode);
    // Skip globals.
    if (addrNode->GetStIdx().Islocal()) {
      addrNode->SetStIdx(UpdateIdx(addrNode->GetStIdx(), stIdxOff, oldStIdx2New));
    }
  } else if (returnVector != nullptr) {
    // Skip globals.
    for (size_t i = 0; i < returnVector->size(); ++i) {
      if (!(*returnVector).at(i).second.IsReg() && (*returnVector).at(i).first.Islocal()) {
        (*returnVector)[i].first = UpdateIdx((*returnVector).at(i).first, stIdxOff, oldStIdx2New);
      }
    }
  } else if (baseNode->GetOpCode() == OP_foreachelem) {
    ForeachelemNode *forEachNode = static_cast<ForeachelemNode*>(baseNode);
    // Skip globals.
    if (forEachNode->GetElemStIdx().Idx() != 0) {
      forEachNode->SetElemStIdx(UpdateIdx(forEachNode->GetElemStIdx(), stIdxOff, oldStIdx2New));
    }
    if (forEachNode->GetArrayStIdx().Idx() != 0) {
      forEachNode->SetArrayStIdx(UpdateIdx(forEachNode->GetArrayStIdx(), stIdxOff, oldStIdx2New));
    }
  } else if (baseNode->GetOpCode() == OP_doloop) {
    DoloopNode *doLoopNode = static_cast<DoloopNode*>(baseNode);
    // Skip globals.
    if (!doLoopNode->IsPreg() && doLoopNode->GetDoVarStIdx().Idx()) {
      doLoopNode->SetDoVarStIdx(UpdateIdx(doLoopNode->GetDoVarStIdx(), stIdxOff, oldStIdx2New));
    }
  }
  // Search for nested dassign/dread/addrof node that may include a symbol index.
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    ReplaceSymbols(baseNode->Opnd(i), stIdxOff, oldStIdx2New);
  }
}

uint32 InlineTransformer::RenameLabels(uint32 inlinedTimes) const {
  size_t labelTabSize = callee.GetLabelTab()->GetLabelTableSize();
  uint32 labIdxOff = static_cast<uint32>(caller.GetLabelTab()->GetLabelTableSize()) - 1;
  // label table start at 1.
  for (size_t i = 1; i < labelTabSize; ++i) {
    std::string labelName = callee.GetLabelTabItem(static_cast<LabelIdx>(i));
    std::string newLableName(kUnderlineStr);
    (void)newLableName.append(std::to_string(callee.GetPuidx()))
        .append(kVerticalLineStr)
        .append(labelName)
        .append(kUnderlineStr)
        .append(std::to_string(inlinedTimes));
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(newLableName);
    LabelIdx labIdx = caller.GetLabelTab()->AddLabel(strIdx);
    CHECK_FATAL(labIdx == (i + labIdxOff), "wrong label index");
  }
  return labIdxOff;
}

#define MPLTOOL
void InlineTransformer::ReplaceLabels(BaseNode &baseNode, uint32 labIdxOff) const {
  // Now only mpltool would allow try in the callee.
#ifdef MPLTOOL
  if (baseNode.GetOpCode() == OP_try) {
    TryNode *tryNode = static_cast<TryNode*>(&baseNode);
    for (size_t i = 0; i < tryNode->GetOffsetsCount(); ++i) {
      tryNode->SetOffset(tryNode->GetOffset(i) + labIdxOff, i);
    }
  }
#else
  CHECK_FATAL(baseNode->op != OP_try, "Java `try` not allowed");
#endif
  if (baseNode.GetOpCode() == OP_block) {
    BlockNode *blockNode = static_cast<BlockNode*>(&baseNode);
    for (auto &stmt : blockNode->GetStmtNodes()) {
      ReplaceLabels(stmt, labIdxOff);
    }
  } else if (baseNode.IsCondBr()) {
    CondGotoNode *temp = static_cast<CondGotoNode*>(&baseNode);
    temp->SetOffset(temp->GetOffset() + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_label) {
    static_cast<LabelNode*>(&baseNode)->SetLabelIdx(static_cast<LabelNode*>(&baseNode)->GetLabelIdx() + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_addroflabel) {
    uint32 offset = static_cast<AddroflabelNode*>(&baseNode)->GetOffset();
    static_cast<AddroflabelNode*>(&baseNode)->SetOffset(offset + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_goto) {
    static_cast<GotoNode*>(&baseNode)->SetOffset(static_cast<GotoNode*>(&baseNode)->GetOffset() + labIdxOff);
  } else if (baseNode.GetOpCode() == OP_multiway || baseNode.GetOpCode() == OP_rangegoto) {
    ASSERT(false, "InlineTransformer::ReplaceLabels: OP_multiway and OP_rangegoto are not supported");
  } else if (baseNode.GetOpCode() == OP_switch) {
    SwitchNode *switchNode = static_cast<SwitchNode*>(&baseNode);
    switchNode->SetDefaultLabel(switchNode->GetDefaultLabel() + labIdxOff);
    for (uint32_t i = 0; i < switchNode->GetSwitchTable().size(); ++i) {
      LabelIdx idx = switchNode->GetSwitchTable()[i].second + labIdxOff;
      switchNode->UpdateCaseLabelAt(i, idx);
    }
  } else if (baseNode.GetOpCode() == OP_doloop) {
    ReplaceLabels(*static_cast<DoloopNode&>(baseNode).GetDoBody(), labIdxOff);
  } else if (baseNode.GetOpCode() == OP_foreachelem) {
    ReplaceLabels(*static_cast<ForeachelemNode&>(baseNode).GetLoopBody(), labIdxOff);
  } else if (baseNode.GetOpCode() == OP_dowhile || baseNode.GetOpCode() == OP_while) {
    ReplaceLabels(*static_cast<WhileStmtNode&>(baseNode).GetBody(), labIdxOff);
  } else if (baseNode.GetOpCode() == OP_if) {
    ReplaceLabels(*static_cast<IfStmtNode&>(baseNode).GetThenPart(), labIdxOff);
    if (static_cast<IfStmtNode*>(&baseNode)->GetElsePart()) {
      ReplaceLabels(*static_cast<IfStmtNode&>(baseNode).GetElsePart(), labIdxOff);
    }
  }
}

uint32 InlineTransformer::RenamePregs(std::unordered_map<PregIdx, PregIdx> &pregOld2new) const {
  const MapleVector<MIRPreg*> &tab = callee.GetPregTab()->GetPregTable();
  size_t tableSize = tab.size();
  uint32 regIdxOff = static_cast<uint32>(caller.GetPregTab()->Size()) - 1;
  for (size_t i = 1; i < tableSize; ++i) {
    MIRPreg *mirPreg = tab[i];
    PregIdx idx = 0;
    if (mirPreg->GetPrimType() == PTY_ptr || mirPreg->GetPrimType() == PTY_ref) {
      idx = caller.GetPregTab()->ClonePreg(*mirPreg);
    } else {
      idx = caller.GetPregTab()->CreatePreg(mirPreg->GetPrimType());
    }
    pregOld2new[i] = idx;
  }
  return regIdxOff;
}

static PregIdx GetNewPregIdx(PregIdx regIdx, std::unordered_map<PregIdx, PregIdx> &pregOld2New) {
  if (regIdx < 0) {
    return regIdx;
  }
  auto it = pregOld2New.find(regIdx);
  CHECK_FATAL(it != pregOld2New.end(), "Unable to find the regIdx to replace");
  return it->second;
}

void InlineTransformer::ReplacePregs(BaseNode *baseNode, std::unordered_map<PregIdx, PregIdx> &pregOld2New) const {
  if (baseNode == nullptr) {
    return;
  }
  auto op = baseNode->GetOpCode();
  if (op == OP_block) {
    for (auto &stmt : static_cast<BlockNode*>(baseNode)->GetStmtNodes()) {
      ReplacePregs(&stmt, pregOld2New);
    }
  } else if (op == OP_regassign) {
    RegassignNode *regassign = static_cast<RegassignNode*>(baseNode);
    regassign->SetRegIdx(GetNewPregIdx(regassign->GetRegIdx(), pregOld2New));
  } else if (op == OP_regread) {
    RegreadNode *regread = static_cast<RegreadNode*>(baseNode);
    regread->SetRegIdx(GetNewPregIdx(regread->GetRegIdx(), pregOld2New));
  } else if (op == OP_doloop) {
    DoloopNode *doloop = static_cast<DoloopNode*>(baseNode);
    if (doloop->IsPreg()) {
      PregIdx oldIdx = static_cast<PregIdx>(doloop->GetDoVarStIdx().FullIdx());
      doloop->SetDoVarStFullIdx(static_cast<uint32>(GetNewPregIdx(oldIdx, pregOld2New)));
    }
  } else if (op == OP_callassigned || op == OP_virtualcallassigned || op == OP_superclasscallassigned ||
             op == OP_interfacecallassigned || op == OP_customcallassigned || op == OP_polymorphiccallassigned ||
             op == OP_icallassigned || op == OP_intrinsiccallassigned || op == OP_xintrinsiccallassigned ||
             op == OP_intrinsiccallwithtypeassigned) {
    CallReturnVector *retVec = baseNode->GetCallReturnVector();
    CHECK_FATAL(retVec != nullptr, "retVec is nullptr in InlineTransformer::ReplacePregs");
    for (size_t i = 0; i < retVec->size(); ++i) {
      CallReturnPair &callPair = (*retVec).at(i);
      if (callPair.second.IsReg()) {
        PregIdx oldIdx = callPair.second.GetPregIdx();
        callPair.second.SetPregIdx(GetNewPregIdx(oldIdx, pregOld2New));
      }
    }
  }
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    ReplacePregs(baseNode->Opnd(i), pregOld2New);
  }
}

LabelIdx InlineTransformer::CreateReturnLabel(uint32 inlinedTimes) const {
  std::string labelName(kUnderlineStr);
  (void)labelName.append(std::to_string(callee.GetPuidx()))
      .append(kVerticalLineStr)
      .append(kReturnlocStr)
      .append(std::to_string(inlinedTimes));
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(labelName);
  LabelIdx labIdx = caller.GetLabelTab()->AddLabel(strIdx);
  return labIdx;
}

GotoNode *InlineTransformer::DoUpdateReturnStmts(BlockNode &newBody, StmtNode &stmt,
                                                 const CallReturnVector *callReturnVector, int &retCount) const {
  if (callReturnVector != nullptr && callReturnVector->size() > 1) {
    CHECK_FATAL(false, "multiple return values are not supported");
  }
  ++retCount;
  GotoNode *gotoNode = builder.CreateStmtGoto(OP_goto, returnLabelIdx);
  if (callReturnVector != nullptr && callReturnVector->size() == 1 && stmt.GetNumOpnds() == 1) {
    BaseNode *currBaseNode = static_cast<NaryStmtNode&>(stmt).Opnd(0);
    StmtNode *dStmt = nullptr;
    if (!callReturnVector->at(0).second.IsReg()) {
      dStmt = builder.CreateStmtDassign(callReturnVector->at(0).first, callReturnVector->at(0).second.GetFieldID(),
                                        currBaseNode);
    } else {
      PregIdx pregIdx = callReturnVector->at(0).second.GetPregIdx();
      const MIRPreg *mirPreg = caller.GetPregTab()->PregFromPregIdx(pregIdx);
      dStmt = builder.CreateStmtRegassign(mirPreg->GetPrimType(), pregIdx, currBaseNode);
    }
    dStmt->SetSrcPos(stmt.GetSrcPos());
    if (updateFreq) {
      caller.GetFuncProfData()->CopyStmtFreq(dStmt->GetStmtID(), stmt.GetStmtID());
      caller.GetFuncProfData()->CopyStmtFreq(gotoNode->GetStmtID(), stmt.GetStmtID());
      caller.GetFuncProfData()->EraseStmtFreq(stmt.GetStmtID());
    }
    newBody.ReplaceStmt1WithStmt2(&stmt, dStmt);
    newBody.InsertAfter(dStmt, gotoNode);
  } else {
    if (updateFreq) {
      caller.GetFuncProfData()->CopyStmtFreq(gotoNode->GetStmtID(), stmt.GetStmtID());
      caller.GetFuncProfData()->EraseStmtFreq(stmt.GetStmtID());
    }
    newBody.ReplaceStmt1WithStmt2(&stmt, gotoNode);
  }
  return gotoNode;
}

GotoNode *InlineTransformer::UpdateReturnStmts(BlockNode &newBody, const CallReturnVector *callReturnVector,
                                               int &retCount) const {
  // Examples:
  // For callee: return f ==>
  //             rval = f; goto label x;
  GotoNode *lastGoto = nullptr;
  for (auto &stmt : newBody.GetStmtNodes()) {
    switch (stmt.GetOpCode()) {
      case OP_foreachelem:
        lastGoto = UpdateReturnStmts(*static_cast<ForeachelemNode&>(stmt).GetLoopBody(), callReturnVector, retCount);
        break;
      case OP_doloop:
        lastGoto = UpdateReturnStmts(*static_cast<DoloopNode&>(stmt).GetDoBody(), callReturnVector, retCount);
        break;
      case OP_dowhile:
      case OP_while:
        lastGoto = UpdateReturnStmts(*static_cast<WhileStmtNode&>(stmt).GetBody(), callReturnVector, retCount);
        break;
      case OP_if: {
        IfStmtNode &ifStmt = static_cast<IfStmtNode&>(stmt);
        lastGoto = UpdateReturnStmts(*ifStmt.GetThenPart(), callReturnVector, retCount);
        if (ifStmt.GetElsePart() != nullptr) {
          auto *temp = UpdateReturnStmts(*ifStmt.GetElsePart(), callReturnVector, retCount);
          lastGoto = temp != nullptr ? temp : lastGoto;
        }
        break;
      }
      case OP_return: {
        lastGoto = DoUpdateReturnStmts(newBody, stmt, callReturnVector, retCount);
        break;
      }
      default:
        break;
    }
  }
  return lastGoto;
}

static void UpdateAddrofConstForPStatic(MIRConst &mirConst, MIRFunction &func,
                                        const std::vector<uint32> &oldStIdx2New) {
  auto constKind = mirConst.GetKind();
  if (constKind == kConstAddrof) {
    auto *addrofConst = static_cast<MIRAddrofConst*>(&mirConst);
    StIdx valueStIdx = addrofConst->GetSymbolIndex();
    if (!valueStIdx.IsGlobal()) {
      MIRSymbol *valueSym = func.GetSymbolTabItem(valueStIdx.Idx());
      if (valueSym->GetStorageClass() == kScPstatic) {
        valueStIdx.SetFullIdx(oldStIdx2New[valueStIdx.Idx()]);
        addrofConst->SetSymbolIndex(valueStIdx);
      }
    }
  } else if (constKind == kConstAggConst) {
    // Example code: static struct tag1 { struct tag1 *next; } s1 = {&s1}
    auto *aggConst = static_cast<MIRAggConst*>(&mirConst);
    for (auto *subMirConst : aggConst->GetConstVec()) {
      UpdateAddrofConstForPStatic(*subMirConst, func, oldStIdx2New);
    }
  }
}

static MIRSymbol *CreateFstaticFromPstatic(const MIRFunction &func, const MIRSymbol &oldSymbol) {
  // convert pu-static to file-static
  // pstatic symbol name mangling example: "foo_bar" --> "__pstatic__125__foo_bar"
  const auto &symNameOrig = oldSymbol.GetName();
  std::string symNameMangling = kPstatic + std::to_string(func.GetPuidx()) + kVerticalLineStr + symNameOrig;
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(symNameMangling);
  MIRSymbol *newSym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
  newSym->SetNameStrIdx(strIdx);
  newSym->SetStorageClass(kScFstatic);
  newSym->SetTyIdx(oldSymbol.GetTyIdx());
  newSym->SetSKind(oldSymbol.GetSKind());
  newSym->SetAttrs(oldSymbol.GetAttrs());
  newSym->SetValue(oldSymbol.GetValue());
  // avoid pstatic vars to be replaced by const
  newSym->SetHasPotentialAssignment();
  return newSym;
}

void InlineTransformer::ConvertPStaticToFStatic(MIRFunction &func) {
  bool hasPStatic = false;
  for (size_t i = 0; i < func.GetSymbolTabSize(); ++i) {
    MIRSymbol *sym = func.GetSymbolTabItem(static_cast<uint32>(i));
    if (sym != nullptr && sym->GetStorageClass() == kScPstatic) {
      hasPStatic = true;
      break;
    }
  }
  if (!hasPStatic) {
    return;  // No pu-static symbols, just return
  }
  std::vector<MIRSymbol*> localSymbols;
  std::vector<uint32> oldStIdx2New(func.GetSymbolTabSize(), 0);
  uint32 pstaticNum = 0;
  for (size_t i = 0; i < func.GetSymbolTabSize(); ++i) {
    MIRSymbol *sym = func.GetSymbolTabItem(static_cast<uint32>(i));
    if (sym == nullptr) {
      continue;
    }
    StIdx oldStIdx = sym->GetStIdx();
    if (sym->GetStorageClass() == kScPstatic) {
      ++pstaticNum;
      MIRSymbol *newSym = CreateFstaticFromPstatic(func, *sym);
      bool success = GlobalTables::GetGsymTable().AddToStringSymbolMap(*newSym);
      CHECK_FATAL(success, "Found repeated global symbols!");
      oldStIdx2New[i] = newSym->GetStIdx().FullIdx();
      // Examples:
      // If a pstatic symbol `foo` is initialized by address of another pstatic symbol `bar`, we need update the stIdx
      // of foo's initial value. Example code:
      //   static int bar = 42;
      //   static int *foo = &bar;
      if ((newSym->GetSKind() == kStConst || newSym->GetSKind() == kStVar) && newSym->GetKonst() != nullptr) {
        UpdateAddrofConstForPStatic(*newSym->GetKonst(), func, oldStIdx2New);
      }
    } else {
      StIdx newStIdx(oldStIdx);
      newStIdx.SetIdx(oldStIdx.Idx() - pstaticNum);
      oldStIdx2New[i] = newStIdx.FullIdx();
      sym->SetStIdx(newStIdx);
      localSymbols.push_back(sym);
    }
  }
  func.GetSymTab()->Clear();
  func.GetSymTab()->PushNullSymbol();
  for (MIRSymbol *sym : localSymbols) {
    (void)func.GetSymTab()->AddStOutside(sym);
  }
  // The stIdxOff will be ignored, 0 is just a placeholder
  ReplaceSymbols(func.GetBody(), 0, &oldStIdx2New);
}

BlockNode *InlineTransformer::CloneFuncBody(BlockNode &funcBody, bool recursiveFirstClone) {
  if (callee.IsFromMpltInline()) {
    return funcBody.CloneTree(theMIRModule->GetCurFuncCodeMPAllocator());
  }
  if (updateFreq) {
    auto *callerProfData = caller.GetFuncProfData();
    auto *calleeProfData = callee.GetFuncProfData();
    uint64_t callsiteFreq = callerProfData->GetStmtFreq(callStmt.GetStmtID());
    uint64_t calleeEntryFreq = calleeProfData->GetFuncFrequency();
    uint32_t updateOp = (kKeepOrigFreq | kUpdateFreqbyScale);
    BlockNode *blockNode;
    if (recursiveFirstClone) {
      blockNode = funcBody.CloneTreeWithFreqs(theMIRModule->GetCurFuncCodeMPAllocator(), callerProfData->GetStmtFreqs(),
                                              calleeProfData->GetStmtFreqs(), 1, 1, updateOp);
    } else {
      blockNode = funcBody.CloneTreeWithFreqs(theMIRModule->GetCurFuncCodeMPAllocator(), callerProfData->GetStmtFreqs(),
                                              calleeProfData->GetStmtFreqs(), callsiteFreq, calleeEntryFreq, updateOp);
      // update callee left entry Frequency
      uint64_t calleeFreq = calleeProfData->GetFuncRealFrequency();
      calleeProfData->SetFuncRealFrequency(calleeFreq - callsiteFreq);
    }
    return blockNode;
  } else {
    return funcBody.CloneTreeWithSrcPosition(*theMIRModule);
  }
}

BlockNode *InlineTransformer::GetClonedCalleeBody() {
  auto *calleeNode = cg->GetCGNode(&callee);
  CHECK_NULL_FATAL(calleeNode);
  if (&caller == &callee) {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "[INLINE Recursive]: " << caller.GetName() << '\n';
      constexpr int32 dumpIndent = 4;
      callStmt.Dump(dumpIndent);
    }
    auto *currFuncBody = calleeNode->GetOriginBody();
    if (currFuncBody == nullptr) {
      currFuncBody = CloneFuncBody(*callee.GetBody(), true);
      // For Inline recursive, we save the original function body before first inline.
      calleeNode->SetOriginBody(currFuncBody);
      // update inlining levels
      calleeNode->IncreaseRecursiveLevel();
    }
    return CloneFuncBody(*currFuncBody, false);
  } else {
    return CloneFuncBody(*callee.GetBody(), false);
  }
}

void InlineTransformer::AssignActualToFormal(BlockNode &newBody, uint32 stIdxOff, uint32 regIdxOff, BaseNode &oldActual,
                                             const MIRSymbol &formal) {
  BaseNode *actual = &oldActual;
  if (formal.IsPreg()) {
    PregIdx idx = callee.GetPregTab()->GetPregIdxFromPregno(static_cast<uint32>(formal.GetPreg()->GetPregNo()));
    RegassignNode *regAssign =
        builder.CreateStmtRegassign(formal.GetPreg()->GetPrimType(), idx + static_cast<int32>(regIdxOff), actual);
    newBody.InsertFirst(regAssign);
    return;
  }

  MIRSymbol *newFormal = caller.GetSymTab()->GetSymbolFromStIdx(formal.GetStIndex() + stIdxOff);
  ASSERT_NOT_NULL(newFormal);
  ASSERT_NOT_NULL(newFormal->GetType());
  PrimType formalPrimType = newFormal->GetType()->GetPrimType();
  PrimType realArgPrimType = actual->GetPrimType();
  // If realArg's type is different from formal's type, use cvt
  if (formalPrimType != realArgPrimType) {
    bool fpTrunc = (IsPrimitiveFloat(formalPrimType) && IsPrimitiveFloat(realArgPrimType) &&
                    GetPrimTypeSize(realArgPrimType) > GetPrimTypeSize(formalPrimType));
    // fpTrunc always needs explicit cvt, for example: cvt f32 f64 (dread f32 xxx)
    if (fpTrunc) {
      actual = builder.CreateExprTypeCvt(OP_cvt, formalPrimType, realArgPrimType, *actual);
    } else if (MustBeAddress(formalPrimType) != MustBeAddress(realArgPrimType)) {
      bool intTrunc = (IsPrimitiveInteger(formalPrimType) && IsPrimitiveInteger(realArgPrimType) &&
                       GetPrimTypeSize(realArgPrimType) > GetPrimTypeSize(formalPrimType));
      // For dassign, if rhs-expr is a primitive integer type, the assigned variable may be smaller, resulting in a
      // truncation. In this case, explicit cvt is not necessary
      if (!intTrunc) {
        actual = builder.CreateExprTypeCvt(OP_cvt, formalPrimType, realArgPrimType, *actual);
      }
    }
  }
  DassignNode *stmt = builder.CreateStmtDassign(*newFormal, 0, actual);
  newBody.InsertFirst(stmt);
  return;
}

void InlineTransformer::AssignActualsToFormals(BlockNode &newBody, uint32 stIdxOff, uint32 regIdxOff) {
  if (static_cast<uint32>(callStmt.NumOpnds()) != callee.GetFormalCount()) {
    LogInfo::MapleLogger() << "warning: # formal arguments != # actual arguments in the function " << callee.GetName()
                           << ". [formal count] " << callee.GetFormalCount() << ", "
                           << "[argument count] " << callStmt.NumOpnds() << std::endl;
  }
  if (callee.GetFormalCount() > 0 && callee.GetFormal(0)->GetName() == kThisStr) {
    UnaryStmtNode *nullCheck = theMIRModule->CurFuncCodeMemPool()->New<UnaryStmtNode>(OP_assertnonnull);
    nullCheck->SetOpnd(callStmt.Opnd(0), 0);
    newBody.InsertFirst(nullCheck);
  }
  // The number of formals and realArg are not always equal
  size_t realArgNum = std::min(callStmt.NumOpnds(), callee.GetFormalCount());
  for (size_t i = 0; i < realArgNum; ++i) {
    BaseNode *currBaseNode = callStmt.Opnd(i);
    MIRSymbol *formal = callee.GetFormal(i);
    CHECK_NULL_FATAL(currBaseNode);
    CHECK_NULL_FATAL(formal);
    AssignActualToFormal(newBody, stIdxOff, regIdxOff, *currBaseNode, *formal);
    if (updateFreq) {
      caller.GetFuncProfData()->CopyStmtFreq(newBody.GetFirst()->GetStmtID(), callStmt.GetStmtID());
    }
  }
}

void InlineTransformer::HandleReturn(BlockNode &newBody) {
  // Find the rval of call-stmt
  // calcute number of return stmt in CALLEE'body
  int retCount = 0;
  // record the last return stmt in CALLEE'body
  GotoNode *lastGoto = nullptr;
  CallReturnVector *currReturnVec = nullptr;
  if (callStmt.GetOpCode() == OP_callassigned || callStmt.GetOpCode() == OP_virtualcallassigned ||
      callStmt.GetOpCode() == OP_superclasscallassigned || callStmt.GetOpCode() == OP_interfacecallassigned) {
    currReturnVec = &callStmt.GetReturnVec();
    CHECK_FATAL(currReturnVec->size() <= 1, "multiple return values are not supported");
  }
  lastGoto = UpdateReturnStmts(newBody, currReturnVec, retCount);
  // Step 6.5: remove the successive goto statement and label statement in some circumstances.
  // There is no return stmt in CALLEE'body, if we have create a new label in Step5, remove it.
  if (retCount == 0) {
    if (labelStmt != nullptr) {
      newBody.RemoveStmt(labelStmt);
    }
  } else {
    // There are one or more goto stmt, remove the successive goto stmt and label stmt,
    // if there is only one return stmt, we can remove the label created in Step5, too.
    CHECK_FATAL(lastGoto != nullptr, "there should be at least one goto statement");
    if (labelStmt == nullptr) {
      // if we haven't created a new label in Step5, then newBody->GetLast == lastGoto means they are successive.
      if (newBody.GetLast() == lastGoto) {
        newBody.RemoveStmt(lastGoto);
      }
      return;
    }
    // if we have created a new label in Step5, then lastGoto->GetNext == labelStmt means they are successive.
    if (lastGoto->GetNext() == labelStmt) {
      newBody.RemoveStmt(lastGoto);
      if (retCount == 1) {
        newBody.RemoveStmt(labelStmt);
      }
    }
  }
  if (updateFreq && (labelStmt != nullptr) && (newBody.GetLast() == labelStmt)) {
    caller.GetFuncProfData()->CopyStmtFreq(labelStmt->GetStmtID(), callStmt.GetStmtID());
  }
}

void InlineTransformer::ReplaceCalleeBody(BlockNode &enclosingBlk, BlockNode &newBody) {
  // begin inlining function
  if (!Options::noComment) {
    MapleString beginCmt(theMIRModule->CurFuncCodeMemPool());
    if (theMIRModule->firstInline) {
      beginCmt += kInlineBeginComment;
    } else {
      beginCmt += kSecondInlineBeginComment;
    }
    beginCmt += callee.GetName();
    StmtNode *commNode = builder.CreateStmtComment(beginCmt.c_str());
    enclosingBlk.InsertBefore(&callStmt, commNode);
    if (updateFreq) {
      caller.GetFuncProfData()->CopyStmtFreq(commNode->GetStmtID(), callStmt.GetStmtID());
    }
    // end inlining function
    MapleString endCmt(theMIRModule->CurFuncCodeMemPool());
    if (theMIRModule->firstInline) {
      endCmt += kInlineEndComment;
    } else {
      endCmt += kSecondInlineEndComment;
    }
    endCmt += callee.GetName();
    if (enclosingBlk.GetLast() != nullptr && &callStmt != enclosingBlk.GetLast()) {
      CHECK_FATAL(callStmt.GetNext() != nullptr, "null ptr check");
    }
    commNode = builder.CreateStmtComment(endCmt.c_str());
    enclosingBlk.InsertAfter(&callStmt, commNode);
    if (updateFreq) {
      caller.GetFuncProfData()->CopyStmtFreq(commNode->GetStmtID(), callStmt.GetStmtID());
    }
    CHECK_FATAL(callStmt.GetNext() != nullptr, "null ptr check");
  }
  if (newBody.IsEmpty()) {
    enclosingBlk.RemoveStmt(&callStmt);
  } else {
    enclosingBlk.ReplaceStmtWithBlock(callStmt, newBody);
  }
}

void InlineTransformer::GenReturnLabel(BlockNode &newBody, uint32 inlinedTimes) {
  // For caller: a = foo() ==>
  //             a = foo(), label x.
  if (callStmt.GetNext() != nullptr && callStmt.GetNext()->GetOpCode() == OP_label) {
    // if the next stmt is a label, just reuse it
    LabelNode *nextLabel = static_cast<LabelNode*>(callStmt.GetNext());
    returnLabelIdx = nextLabel->GetLabelIdx();
  } else {
    returnLabelIdx = CreateReturnLabel(inlinedTimes);
    // record the created label
    labelStmt = builder.CreateStmtLabel(returnLabelIdx);
    newBody.AddStatement(labelStmt);
    if (updateFreq) {
      caller.GetFuncProfData()->CopyStmtFreq(labelStmt->GetStmtID(), callStmt.GetStmtID());
    }
  }
}

// Should be called before merge function body.
void RecordCallId(BaseNode &baseNode, std::map<uint32, uint32> &callMeStmtIdMap) {
  auto op = baseNode.GetOpCode();
  if (op == OP_call || op == OP_callassigned) {
    auto &callNode = static_cast<CallNode&>(baseNode);
    (void)callMeStmtIdMap.emplace(callNode.GetMeStmtID(), callNode.GetStmtID());
    return;
  }
  if (baseNode.GetOpCode() == OP_block) {
    BlockNode &blk = static_cast<BlockNode&>(baseNode);
    for (auto &stmt : blk.GetStmtNodes()) {
      RecordCallId(stmt, callMeStmtIdMap);
    }
  } else if (baseNode.GetOpCode() == OP_doloop) {
    BlockNode *blk = static_cast<DoloopNode&>(baseNode).GetDoBody();
    RecordCallId(*blk, callMeStmtIdMap);
  } else if (baseNode.GetOpCode() == OP_foreachelem) {
    BlockNode *blk = static_cast<ForeachelemNode&>(baseNode).GetLoopBody();
    RecordCallId(*blk, callMeStmtIdMap);
  } else if (baseNode.GetOpCode() == OP_dowhile || baseNode.GetOpCode() == OP_while) {
    BlockNode *blk = static_cast<WhileStmtNode&>(baseNode).GetBody();
    RecordCallId(*blk, callMeStmtIdMap);
  } else if (baseNode.GetOpCode() == OP_if) {
    BlockNode *blk = static_cast<IfStmtNode&>(baseNode).GetThenPart();
    RecordCallId(*blk, callMeStmtIdMap);
    blk = static_cast<IfStmtNode&>(baseNode).GetElsePart();
    if (blk != nullptr) {
      RecordCallId(*blk, callMeStmtIdMap);
    }
  }
}

// Inline CALLEE into CALLER.
bool InlineTransformer::PerformInline(BlockNode &enclosingBlk) {
  if (callee.IsEmpty()) {
    enclosingBlk.RemoveStmt(&callStmt);
    return false;
  }
  auto *calleeNode = cg->GetCGNode(&callee);
  CHECK_NULL_FATAL(calleeNode);
  // Record stmt right before and after the callStmt.
  StmtNode *stmtBeforeCall = callStmt.GetPrev();
  StmtNode *stmtAfterCall = callStmt.GetNext();
  // Step 0: See if CALLEE have been inlined to CALLER once.
  uint32 inlinedTimes = calleeNode->GetInlinedTimes();
  // If the callee has local static variables, We convert local pu-static symbols to global file-static symbol to avoid
  // multiple definition for these static symbols
  ConvertPStaticToFStatic(callee);
  // Step 1: Clone CALLEE's body.
  BlockNode *newBody = GetClonedCalleeBody();
  CHECK_NULL_FATAL(newBody);
  // Record callStmtId mapping from callee orig body to callee cloned body for merging inline summary
  std::map<uint32, uint32> callIdMap;
  if (Options::enableInlineSummary) {
    RecordCallId(*newBody, callIdMap);
  }

  // Step 2: Rename symbols, labels, pregs
  uint32 stIdxOff = RenameSymbols(inlinedTimes);
  uint32 labIdxOff = RenameLabels(inlinedTimes);
  std::unordered_map<PregIdx, PregIdx> pregOld2New;
  uint32 regIdxOff = RenamePregs(pregOld2New);
  // Step 3: Replace symbols, labels, pregs
  // Callee has no pu-static symbols now, so we only use stIdxOff to update stIdx, set oldStIdx2New nullptr
  ReplaceSymbols(newBody, stIdxOff, nullptr);
  ReplaceLabels(*newBody, labIdxOff);
  ReplacePregs(newBody, pregOld2New);
  // Step 4: Null check 'this' and assign actuals to formals.
  AssignActualsToFormals(*newBody, stIdxOff, regIdxOff);
  // Step 5: Insert the callee'return jump dest label.
  GenReturnLabel(*newBody, inlinedTimes);
  // Step 6: Handle return values.
  HandleReturn(*newBody);
  // Step 7: Replace the call-stmt with new CALLEE'body.
  ReplaceCalleeBody(enclosingBlk, *newBody);
  // Step 8: Update inlined_times.
  calleeNode->IncreaseInlinedTimes();
  // Step 9: After inlining, if there exists nested try-catch block, flatten them all.
  HandleTryBlock(stmtBeforeCall, stmtAfterCall);
  // Step 10: Merge inline summary
  if (Options::enableInlineSummary) {
    MergeInlineSummary(caller, callee, callStmt, callIdMap);
  }
  return true;
}

void InlineTransformer::HandleTryBlock(StmtNode *stmtBeforeCall, StmtNode *stmtAfterCall) {
  // Step 9.1 Find whether there is a javatry before callStmt.
  bool hasOuterTry = false;
  TryNode *outerTry = nullptr;
  StmtNode *outerEndTry = nullptr;
  for (StmtNode *stmt = stmtBeforeCall; stmt != nullptr; stmt = stmt->GetPrev()) {
    if (stmt->op == OP_endtry) {
      break;
    }
    if (stmt->op == OP_try) {
      hasOuterTry = true;
      outerTry = static_cast<TryNode*>(stmt);
      break;
    }
  }
  if (!hasOuterTry) {
    return;
  }
  for (StmtNode *stmt = stmtAfterCall; stmt != nullptr; stmt = stmt->GetNext()) {
    if (stmt->op == OP_endtry) {
      outerEndTry = stmt;
      break;
    }
    if (stmt->op == OP_try) {
      ASSERT(false, "Impossible, caller: [%s] callee: [%s]", caller.GetName().c_str(), callee.GetName().c_str());
      break;
    }
  }
  ASSERT(outerTry != nullptr, "Impossible, caller: [%s] callee: [%s]", caller.GetName().c_str(),
         callee.GetName().c_str());
  ASSERT(outerEndTry != nullptr, "Impossible, caller: [%s] callee: [%s]", caller.GetName().c_str(),
         callee.GetName().c_str());

  // Step 9.2 We have found outerTry and outerEndTry node, resolve the nested try-catch blocks between them.
  bool hasInnerTry = ResolveNestedTryBlock(*caller.GetBody(), *outerTry, outerEndTry);
  if (hasOuterTry && hasInnerTry) {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "[NESTED_TRY_CATCH]" << callee.GetName() << " to " << caller.GetName() << '\n';
    }
  }
}

bool InlineTransformer::ResolveNestedTryBlock(BlockNode &body, TryNode &outerTryNode,
                                              const StmtNode *outerEndTryNode) const {
  StmtNode *stmt = outerTryNode.GetNext();
  StmtNode *next = nullptr;
  bool changed = false;
  while (stmt != outerEndTryNode) {
    next = stmt->GetNext();
    switch (stmt->op) {
      case OP_try: {
        // Find previous meaningful stmt.
        StmtNode *last = stmt->GetPrev();
        while (last->op == OP_comment || last->op == OP_label) {
          last = last->GetPrev();
        }
        ASSERT(last->op != OP_endtry, "Impossible");
        ASSERT(last->op != OP_try, "Impossible");
        StmtNode *newEnd = theMIRModule->CurFuncCodeMemPool()->New<StmtNode>(OP_endtry);
        body.InsertAfter(last, newEnd);
        TryNode *tryNode = static_cast<TryNode*>(stmt);
        tryNode->OffsetsInsert(tryNode->GetOffsetsEnd(), outerTryNode.GetOffsetsBegin(), outerTryNode.GetOffsetsEnd());
        changed = true;
        break;
      }
      case OP_endtry: {
        // Find next meaningful stmt.
        StmtNode *first = stmt->GetNext();
        while (first->op == OP_comment || first->op == OP_label) {
          first = first->GetNext();
        }
        ASSERT(first != outerEndTryNode, "Impossible");
        next = first->GetNext();
        if (first->op == OP_try) {
          // In this case, there are no meaningful statements between last endtry and the next try,
          // we just solve the trynode and move the cursor to the statement right after the trynode.
          TryNode *tryNode = static_cast<TryNode*>(first);
          tryNode->OffsetsInsert(tryNode->GetOffsetsEnd(), outerTryNode.GetOffsetsBegin(),
                                 outerTryNode.GetOffsetsEnd());
          changed = true;
          break;
        } else {
          TryNode *node = outerTryNode.CloneTree(theMIRModule->GetCurFuncCodeMPAllocator());
          CHECK_FATAL(node != nullptr, "Impossible");
          body.InsertBefore(first, node);
          changed = true;
          break;
        }
      }
      default: {
        break;
      }
    }
    stmt = next;
  }
  return changed;
}
}  // namespace maple
