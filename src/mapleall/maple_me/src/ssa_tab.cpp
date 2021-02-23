/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ssa_tab.h"
#include <iostream>
#include <fstream>
#include "ssa_mir_nodes.h"
#include "opcode_info.h"
#include "mir_function.h"

// Allocate data structures to store SSA information. Only statement nodes and
// tree nodes that incur defs and uses are relevant. Tree nodes are made larger
// by allocating the larger SSANodes.  Statement nodes' SSA information is
// stored in class SSATab's StmtsSSAPart, which has an array of pointers indexed
// by the stmtID field of each statement node.
namespace maple {
BaseNode *SSATab::CreateSSAExpr(BaseNode &expr) {
  if (expr.GetOpCode() == OP_addrof || expr.GetOpCode() == OP_dread) {
    if (expr.IsSSANode()) {
      return mirModule.CurFunction()->GetCodeMemPool()->New<AddrofSSANode>(static_cast<AddrofSSANode&>(expr));
    }
    auto &addrofNode = static_cast<AddrofNode&>(expr);
    AddrofSSANode *ssaNode = mirModule.CurFunction()->GetCodeMemPool()->New<AddrofSSANode>(addrofNode);
    MIRSymbol *st = mirModule.CurFunction()->GetLocalOrGlobalSymbol(ssaNode->GetStIdx());
    OriginalSt *ost = FindOrCreateSymbolOriginalSt(*st, mirModule.CurFunction()->GetPuidx(), ssaNode->GetFieldID());
    versionStTable.CreateZeroVersionSt(ost);
    ssaNode->SetSSAVar(*versionStTable.GetZeroVersionSt(ost));
    return ssaNode;
  } else if (expr.GetOpCode() == OP_regread) {
    auto &regReadNode = static_cast<RegreadNode&>(expr);
    RegreadSSANode *ssaNode = mirModule.CurFunction()->GetCodeMemPool()->New<RegreadSSANode>(regReadNode);
    OriginalSt *ost =
        originalStTable.FindOrCreatePregOriginalSt(ssaNode->GetRegIdx(), mirModule.CurFunction()->GetPuidx());
    versionStTable.CreateZeroVersionSt(ost);
    ssaNode->SetSSAVar(*versionStTable.GetZeroVersionSt(ost));
    return ssaNode;
  } else if (expr.GetOpCode() == OP_iread) {
    auto &ireadNode = static_cast<IreadNode&>(expr);
    IreadSSANode *ssaNode = mirModule.CurFunction()->GetCodeMempool()->New<IreadSSANode>(ireadNode);
    BaseNode *newOpnd = CreateSSAExpr(*ireadNode.Opnd(0));
    if (newOpnd != nullptr) {
      ssaNode->SetOpnd(newOpnd, 0);
    }
    return ssaNode;
  }
  for (size_t i = 0; i < expr.NumOpnds(); ++i) {
    BaseNode *newOpnd = CreateSSAExpr(*expr.Opnd(i));
    if (newOpnd != nullptr) {
      expr.SetOpnd(newOpnd, i);
    }
  }
  return nullptr;
}

void SSATab::CreateSSAStmt(StmtNode &stmt, BB *curbb) {
  for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
    BaseNode *newOpnd = CreateSSAExpr(*stmt.Opnd(i));
    if (newOpnd != nullptr) {
      stmt.SetOpnd(newOpnd, i);
    }
  }
  switch (stmt.GetOpCode()) {
    case OP_maydassign:
    case OP_dassign: {
      MayDefPartWithVersionSt *theSSAPart =
          stmtsSSAPart.GetSSAPartMp()->New<MayDefPartWithVersionSt>(&stmtsSSAPart.GetSSAPartAlloc());
      stmtsSSAPart.SetSSAPartOf(stmt, theSSAPart);
      auto &dNode = static_cast<DassignNode&>(stmt);
      MIRSymbol *st = mirModule.CurFunction()->GetLocalOrGlobalSymbol(dNode.GetStIdx());
      CHECK_FATAL(st != nullptr, "null ptr check");

      OriginalSt *ost = FindOrCreateSymbolOriginalSt(*st, mirModule.CurFunction()->GetPuidx(), dNode.GetFieldID());
      AddDefBB4Ost(ost->GetIndex(), curbb->GetBBId());
      versionStTable.CreateZeroVersionSt(ost);
      theSSAPart->SetSSAVar(*versionStTable.GetZeroVersionSt(ost));
      // if the rhs may throw exception, we insert MayDef of the lhs var
      if (stmt.GetOpCode() == OP_maydassign) {
        theSSAPart->InsertMayDefNode(theSSAPart->GetSSAVar(), &dNode);
      }
      return;
    }
    case OP_regassign: {
      auto &regNode = static_cast<RegassignNode&>(stmt);
      OriginalSt *ost =
          originalStTable.FindOrCreatePregOriginalSt(regNode.GetRegIdx(), mirModule.CurFunction()->GetPuidx());
      AddDefBB4Ost(ost->GetIndex(), curbb->GetBBId());
      versionStTable.CreateZeroVersionSt(ost);
      VersionSt *vst = versionStTable.GetZeroVersionSt(ost);
      stmtsSSAPart.SetSSAPartOf(stmt, vst);
      return;
    }
    case OP_return:
    case OP_throw:
    case OP_gosub:
    case OP_retsub:
      stmtsSSAPart.SetSSAPartOf(stmt, stmtsSSAPart.GetSSAPartMp()->New<MayUsePart>(&stmtsSSAPart.GetSSAPartAlloc()));
      return;
    case OP_syncenter:
    case OP_syncexit:
      stmtsSSAPart.SetSSAPartOf(stmt,
                                stmtsSSAPart.GetSSAPartMp()->New<MayDefMayUsePart>(&stmtsSSAPart.GetSSAPartAlloc()));
      return;
    case OP_iassign:
      stmtsSSAPart.SetSSAPartOf(stmt, stmtsSSAPart.GetSSAPartMp()->New<MayDefPart>(&stmtsSSAPart.GetSSAPartAlloc()));
      return;
    default: {
      if (kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
        MayDefMayUseMustDefPart *theSSAPart =
            stmtsSSAPart.GetSSAPartMp()->New<MayDefMayUseMustDefPart>(&stmtsSSAPart.GetSSAPartAlloc());
        stmtsSSAPart.SetSSAPartOf(stmt, theSSAPart);
        // insert the mustdefs
        CallReturnVector *nrets = static_cast<NaryStmtNode&>(stmt).GetCallReturnVector();
        CHECK_FATAL(nrets != nullptr, "CreateSSAStmt: failed to retrieve call return vector");
        if (nrets->empty()) {
          return;
        }
        for (CallReturnPair &retPair : *nrets) {
          OriginalSt *ost = nullptr;
          if (!retPair.second.IsReg()) {
            StIdx stidx = retPair.first;
            MIRSymbolTable *symTab = mirModule.CurFunction()->GetSymTab();
            MIRSymbol *st = symTab->GetSymbolFromStIdx(stidx.Idx());
            ost = FindOrCreateSymbolOriginalSt(*st, mirModule.CurFunction()->GetPuidx(), retPair.second.GetFieldID());
          } else {
            ost = originalStTable.FindOrCreatePregOriginalSt(retPair.second.GetPregIdx(), mirModule.CurFunction()->GetPuidx());
          }
          versionStTable.CreateZeroVersionSt(ost);
          VersionSt *vst = versionStTable.GetZeroVersionSt(ost);
          AddDefBB4Ost(ost->GetIndex(), curbb->GetBBId());
          theSSAPart->InsertMustDefNode(vst, &stmt);
        }
      } else if (kOpcodeInfo.IsCall(stmt.GetOpCode())) {
        stmtsSSAPart.SetSSAPartOf(stmt,
                                  stmtsSSAPart.GetSSAPartMp()->New<MayDefMayUsePart>(&stmtsSSAPart.GetSSAPartAlloc()));
      }
    }
  }
}
}  // namespace maple
