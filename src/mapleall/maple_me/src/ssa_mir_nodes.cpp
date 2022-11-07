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
#include "ssa_mir_nodes.h"
#include "opcode_info.h"
#include "mir_function.h"
#include "printing.h"
#include "ssa_tab.h"

namespace maple {
void GenericSSAPrint(const MIRModule &mod, const StmtNode &stmtNode, int32 indent, StmtsSSAPart &stmtsSSAPart) {
  stmtNode.Dump(indent);
  // print SSAPart
  Opcode op = stmtNode.GetOpCode();
  AccessSSANodes *ssaPart = stmtsSSAPart.SSAPartOf(stmtNode);
  switch (op) {
    case OP_maydassign:
    case OP_dassign: {
      mod.GetOut() << " ";
      CHECK_NULL_FATAL(ssaPart->GetSSAVar());
      ssaPart->GetSSAVar()->Dump();
      LogInfo::MapleLogger() << "\n";
      ssaPart->DumpMayDefNodes(mod);
      return;
    }
    case OP_regassign: {
      mod.GetOut() << "  ";
      CHECK_NULL_FATAL(ssaPart->GetSSAVar());
      ssaPart->GetSSAVar()->Dump();
      LogInfo::MapleLogger() << "\n";
      return;
    }
    case OP_iassign: {
      ssaPart->DumpMayDefNodes(mod);
      return;
    }
    case OP_throw:
    case OP_retsub:
    case OP_return: {
      ssaPart->DumpMayUseNodes(mod);
      mod.GetOut() << '\n';
      return;
    }
    default: {
      if (kOpcodeInfo.IsCallAssigned(op)) {
        ssaPart->DumpMayUseNodes(mod);
        ssaPart->DumpMustDefNodes(mod);
        ssaPart->DumpMayDefNodes(mod);
      } else if (kOpcodeInfo.HasSSADef(op) && kOpcodeInfo.HasSSAUse(op)) {
        ssaPart->DumpMayUseNodes(mod);
        mod.GetOut() << '\n';
        ssaPart->DumpMayDefNodes(mod);
      }
      return;
    }
  }
}

static TypeOfMayDefList *SSAGenericGetMayDefsFromVersionSt(const VersionSt &vst,
    StmtsSSAPart &stmtsSSAPart, std::unordered_set<const VersionSt*> &visited) {
  if (vst.IsInitVersion() || visited.find(&vst) != visited.end()) {
    return nullptr;
  }
  visited.insert(&vst);
  if (vst.GetDefType() == VersionSt::kPhi) {
    const PhiNode *phi = vst.GetPhi();
    for (size_t i = 0; i < phi->GetPhiOpnds().size(); ++i) {
      const VersionSt *vSym = phi->GetPhiOpnd(i);
      TypeOfMayDefList *mayDefs = SSAGenericGetMayDefsFromVersionSt(*vSym, stmtsSSAPart, visited);
      if (mayDefs != nullptr) {
        return mayDefs;
      }
    }
  } else if (vst.GetDefType() == VersionSt::kMayDef) {
    const MayDefNode *mayDef = vst.GetMayDef();
    return &stmtsSSAPart.GetMayDefNodesOf(*mayDef->GetStmt());
  }
  return nullptr;
}

TypeOfMayDefList *SSAGenericGetMayDefsFromVersionSt(const VersionSt &sym, StmtsSSAPart &stmtsSSAPart) {
  std::unordered_set<const VersionSt*> visited;
  return SSAGenericGetMayDefsFromVersionSt(sym, stmtsSSAPart, visited);
}

bool HasMayUseOpnd(const BaseNode &baseNode, SSATab &func) {
  const auto &stmtNode = static_cast<const StmtNode&>(baseNode);
  if (kOpcodeInfo.HasSSAUse(stmtNode.GetOpCode())) {
    TypeOfMayUseList &mayUses = func.GetStmtsSSAPart().GetMayUseNodesOf(stmtNode);
    if (!mayUses.empty()) {
      return true;
    }
  }
  for (size_t i = 0; i < baseNode.NumOpnds(); ++i) {
    if (HasMayUseOpnd(*baseNode.Opnd(i), func)) {
      return true;
    }
  }
  return false;
}

bool IsSameContent(const BaseNode *exprA, const BaseNode *exprB, bool isZeroVstEqual) {
  if (exprA == nullptr || exprB == nullptr) {
    return false;
  }
  if (exprA == exprB) {
    return true;
  }
  Opcode opA = exprA->GetOpCode();
  Opcode opB = exprB->GetOpCode();
  PrimType ptypA = exprA->GetPrimType();
  PrimType ptypB = exprB->GetPrimType();
  size_t opndNumA = exprA->NumOpnds();
  size_t opndNumB = exprB->NumOpnds();
  // 1. compare common field for all kinds of expr
  if (opA != opB || ptypA != ptypB || opndNumA != opndNumB) {
    return false;
  }
  if (exprA->IsSSANode()) {
    const VersionSt *vstA = static_cast<const SSANode *>(exprA)->GetSSAVar();
    const VersionSt *vstB = static_cast<const SSANode *>(exprB)->GetSSAVar();
    if (vstA != nullptr && vstB != nullptr) {
      return vstA == vstB && (isZeroVstEqual || !vstA->IsInitVersion());
    }
  } else if (exprA->IsLeaf()) {
    // leaf node has no SSANode inside it, we can compare them by IsSameContent.
    return exprA->IsSameContent(exprB);
  }
  // if any leaf nodes of these two expr are different, return false
  for (size_t i = 0; i < opndNumA; ++i) {
    if (!IsSameContent(exprA->Opnd(i), exprB->Opnd(i), isZeroVstEqual)) {
      return false;
    }
  }
  // 2. compare extra fields of some kinds of expr
  switch (opA) {
    case OP_extractbits:
    case OP_zext:
    case OP_sext: {
      auto *nodeA = static_cast<const ExtractbitsNode *>(exprA);
      auto *nodeB = static_cast<const ExtractbitsNode *>(exprB);
      return nodeA->GetBitsOffset() == nodeB->GetBitsOffset() && nodeA->GetBitsSize() == nodeB->GetBitsSize();
    }
    case OP_depositbits: {
      auto *nodeA = static_cast<const DepositbitsNode *>(exprA);
      auto *nodeB = static_cast<const DepositbitsNode *>(exprB);
      return nodeA->GetBitsOffset() == nodeB->GetBitsOffset() && nodeA->GetBitsSize() == nodeB->GetBitsSize();
    }
    case OP_array: {
      auto *nodeA = static_cast<const ArrayNode *>(exprA);
      auto *nodeB = static_cast<const ArrayNode *>(exprB);
      return nodeA->GetBoundsCheck() == nodeB->GetBoundsCheck() && nodeA->GetTyIdx() == nodeB->GetTyIdx();
    }
    case OP_iread: {
      if (!exprA->IsSSANode()) { // no ssa version has been set (i.e. ssa_tab not run)
        return exprA->IsSameContent(exprB);
      }
      BaseNode *nodeA = const_cast<IreadSSANode *>(static_cast<const IreadSSANode *>(exprA))->GetNoSSANode();
      BaseNode *nodeB = const_cast<IreadSSANode *>(static_cast<const IreadSSANode *>(exprB))->GetNoSSANode();
      auto *ireadA = static_cast<IreadNode *>(nodeA);
      auto *ireadB = static_cast<IreadNode *>(nodeB);
      return ireadA->GetTyIdx() == ireadB->GetTyIdx() && ireadA->GetFieldID() == ireadB->GetFieldID();
    }
    case OP_iaddrof: {
      auto *ireadA = static_cast<const IreadNode *>(exprA);
      auto *ireadB = static_cast<const IreadNode *>(exprB);
      return ireadA->GetTyIdx() == ireadB->GetTyIdx() && ireadA->GetFieldID() == ireadB->GetFieldID();
    }
    case OP_ireadoff: {
      return static_cast<const IreadoffNode *>(exprA)->GetOffset() ==
             static_cast<const IreadoffNode *>(exprB)->GetOffset();
    }
    case OP_ireadfpoff: {
      return static_cast<const IreadFPoffNode *>(exprA)->GetOffset() ==
             static_cast<const IreadFPoffNode *>(exprB)->GetOffset();
    }
    case OP_ireadpcoff: {
      return static_cast<const IreadPCoffNode *>(exprA)->GetOffset() ==
             static_cast<const IreadPCoffNode *>(exprB)->GetOffset();
    }
    case OP_intrinsicop:
    case OP_intrinsicopwithtype: {
      auto *nodeA = static_cast<const IntrinsicopNode *>(exprA);
      auto *nodeB = static_cast<const IntrinsicopNode *>(exprB);
      return nodeA->GetIntrinsic() == nodeB->GetIntrinsic() && nodeA->GetTyIdx() == nodeB->GetTyIdx();
    }
    case OP_resolveinterfacefunc:
    case OP_resolvevirtualfunc: {
      auto *nodeA = static_cast<const ResolveFuncNode *>(exprA);
      auto *nodeB = static_cast<const ResolveFuncNode *>(exprB);
      return nodeA->GetPuIdx() == nodeB->GetPuIdx();
    }
    default: {
      if (kOpcodeInfo.IsTypeCvt(opA)) {
        if (opA == OP_retype) {
          if (static_cast<const RetypeNode *>(exprA)->GetTyIdx() !=
              static_cast<const RetypeNode *>(exprB)->GetTyIdx()) {
            return false;
          }
        }
        return static_cast<const TypeCvtNode *>(exprA)->FromType() ==
               static_cast<const TypeCvtNode *>(exprB)->FromType();
      }
      if (kOpcodeInfo.IsCompare(opA)) {
        return static_cast<const CompareNode *>(exprA)->GetOpndType() ==
               static_cast<const CompareNode *>(exprB)->GetOpndType();
      }
      return true;
    }
  }
}
}  // namespace maple
