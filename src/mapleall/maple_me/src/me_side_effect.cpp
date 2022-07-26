/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_side_effect.h"
#include "me_function.h"
#include "ver_symbol.h"
#include "me_ir.h"
#include "me_irmap.h"
#include "me_phase_manager.h"
#include "ipa_option.h"

// IPA sideeffect analysis
// Default value:
//   Mirfunc->attribute: not Pure, defArg, def, return Global, throw eh, return Arg, def private
//   ipasideeffect.cpp:  pure, no defArg, no Def, no Return Global, no throw eh, no return Arg, no def private
// For a close environment which consists of many modules:
// 1. Focus on Pure, DefGlobal, DefArg, ThrowException
// 2. Build SCC in one function, propgate Arg or Global attribute
// For a open environment with decouple and dynamic class load:
using namespace maple;
namespace {
const FuncWithSideEffect kMrtList[] {
#include "mrt_info.def"
};
size_t mrtListSz = 0;
std::map<PUIdx, uint8> mrtPuIdx;
// keep track of union of all side effects in a SCC
std::map<uint32, uint8> sccSe;
const SCCNode<CGNode> *currSccNode = nullptr;
size_t cgNodeCount = 0;
}

namespace maple {
IpaSideEffect::IpaSideEffect(MeFunction &mf, MemPool *memPool, CallGraph &callGraph, Dominance &dom)
    : isSeen(false),
      notPure(false),
      hasDefArg(false),
      hasDef(false),
      hasRetGlobal(false),
      hasThrException(false),
      hasRetArg(false),
      hasPrivateDef(false),
      isGlobal(false),
      isArg(false),
      meFunc(mf),
      alloc(memPool),
      callGraph(callGraph),
      sccId(0),
      dominance(dom) {}

void IpaSideEffect::SetEffectsTrue() {
  notPure = true;
  hasDefArg = true;
  hasDef = true;
  hasRetGlobal = true;
  hasThrException = true;
  hasRetArg = true;
  hasPrivateDef = true;
}

bool IpaSideEffect::IsIgnoreMethod(const MIRFunction &func) {
  // Ignore if virtual or no method
  if (func.IsAbstract()) {
    return true;
  }
  Klass *klass = callGraph.GetKlassh()->GetKlassFromFunc(&func);
  if (klass == nullptr) {
    // An array, must have method, but has all effects
    SetEffectsTrue();
    return true;
  }
  const auto &methods = klass->GetMethods();
  return std::find(methods.begin(), methods.end(), &func) == methods.end();
}

void IpaSideEffect::CopySccSideEffectToAllFunctions(SCCNode<CGNode> &scc, uint8 seMask) {
  // For all members of the SCC, copy the sum of the side effect of SCC to each member func.
  for (auto &sccIt : scc.GetNodes()) {
    CGNode *cgNode = sccIt;
    MIRFunction *func = cgNode->GetMIRFunction();
    if (mrtPuIdx.find(func->GetPuidx()) != mrtPuIdx.end()) {
      CHECK_FATAL(false, "Pay attention, manual defined func in a SCC. %s", func->GetName().c_str());
    }
    CHECK_FATAL(func->IsIpaSeen(), "Must have been processed.");
    CHECK_FATAL(func->IsPure() || (seMask & kNotPure) == kNotPure, "Must be true.");
    CHECK_FATAL(func->IsNoDefEffect() || (seMask & kHasDef) == kHasDef, "Must be true.");
    CHECK_FATAL(func->IsNoThrowException() || (seMask & kHasThrow) == kHasThrow, "Must be true.");
    CHECK_FATAL(func->IsNoPrivateDefEffect() || (seMask & kHasPrivateDef) == kHasPrivateDef, "Must be true.");
    if (seMask & kNotPure) {
      func->UnsetPure();
    }
    if (seMask & kHasDef) {
      func->UnsetNoDefEffect();
    }
    if (seMask & kHasThrow) {
      func->UnsetNoThrowException();
    }
    if (seMask & kHasPrivateDef) {
      func->UnsetNoPrivateDefEffect();
    }
  }
}

void IpaSideEffect::GetEffectFromCallee(MIRFunction &callee, const MIRFunction &caller) {
  uint32 calleeScc = GetOrSetSCCNodeId(callee);
  if (IsCallingIntoSCC(calleeScc)) {
    // Call graph ensures that all methods in SCC are visited before a call into the SCC.
    auto it = sccSe.find(calleeScc);
    CHECK_FATAL(it != sccSe.end(), "Sideeffect of scc must have been set.");
    uint8 mask = it->second;

    hasDefArg = hasDefArg || (mask & kHasDefArg);
    hasDef = hasDef || (mask & kHasDef);
    hasThrException = hasThrException || (mask & kHasThrow);
    hasRetArg = hasRetArg || (mask & kHasRetArg);
    hasPrivateDef = hasPrivateDef || (mask & kHasPrivateDef);
  } else if (callee.IsIpaSeen()) {
    notPure = notPure || !callee.IsPure();
    hasDefArg = hasDefArg || !callee.IsNoDefArgEffect();
    hasDef = hasDef || !callee.IsNoDefEffect();
    hasRetGlobal = hasRetGlobal || !callee.IsNoRetGlobal();
    hasThrException = hasThrException || !callee.IsNoThrowException();
    hasRetArg = hasRetArg || !callee.IsNoRetArg();
    hasPrivateDef = hasPrivateDef || !callee.IsNoPrivateDefEffect();
  } else if (!MatchPuidxAndSetSideEffects(callee.GetPuidx())) {
    // function was not compiled before and it is not one of the predetermined
    // sideeffect functions, then assume
    //  - native function : no side effects
    //  - non-native function : all side effects
    if (!callee.IsNative()) {
      SetEffectsTrue();
      return;
    }
    // If the caller and callee are in the same SCC, then its ok, since
    // the side effect of SCC is the sum of the methods in the SCC. The
    // side effect of the not yet visited method will be added in the
    // combined SCC side effects later when it is actually visited.
    if (callee.GetName() == caller.GetName()) {
      return;
    }
    // !!!!!!!!!!!!MUST PRINT WARNING!!!!!!!!!!!
    if (calleeScc != sccId && !MeOption::quiet) {
      LogInfo::MapleLogger() << "WARNING: cross scc default no side effects " << callee.GetName()
                             << " Native-function\n";
    }
  }
}

bool IpaSideEffect::MEAnalyzeDefExpr(MeExpr &baseExprMe, std::vector<MeExpr*> &varVector) {
  CHECK_FATAL(baseExprMe.GetOp() == OP_dread, "Must be dread");
  auto &baseExpr = static_cast<VarMeExpr&>(baseExprMe);
  const OriginalSt *ostSymbol = meFunc.GetMeSSATab()->GetOriginalStFromID(baseExpr.GetOstIdx());
  if (!ostSymbol->IsLocal()) {
    return true;
  }
  switch (baseExpr.GetDefBy()) {
    case kDefByStmt: {
      if (baseExpr.GetDefStmt() == nullptr) {
        return true;
      }
      MeStmt *defStmt = baseExpr.GetDefStmt();
      CHECK_FATAL(defStmt->GetOp() == OP_dassign, "Must be dassign");
      auto *dassignMeStmt = static_cast<DassignMeStmt*>(defStmt);
      switch (dassignMeStmt->GetRHS()->GetOp()) {
        case OP_dread: {
          for (auto it = varVector.begin(); it != varVector.end(); ++it) {
            // Cycle
            if (*it == dassignMeStmt->GetRHS()) {
              return true;
            }
          }
          varVector.push_back(dassignMeStmt->GetRHS());
          if (MEAnalyzeDefExpr(*dassignMeStmt->GetRHS(), varVector)) {
            return true;
          }
          break;
        }
        case OP_gcmallocjarray:
        case OP_gcmalloc:
        case OP_constval: {
          return false;
        }
        case OP_iread: {
          return true;
        }
        case OP_cvt:
        case OP_retype: {
          auto *node = static_cast<OpMeExpr*>(dassignMeStmt->GetRHS());
          CHECK_FATAL(node->GetOpnd(0)->GetOp() == OP_dread, "must be dread");
          for (auto it = varVector.begin(); it != varVector.end(); ++it) {
            // Cycle
            if (*it == node->GetOpnd(0)) {
              return true;
            }
          }
          varVector.push_back(node->GetOpnd(0));
          if (MEAnalyzeDefExpr(*node->GetOpnd(0), varVector)) {
            return true;
          }
          break;
        }
        default:
          CHECK_FATAL(false, "NYI");
      }
      break;
    }
    case kDefByPhi: {
      for (auto *expr : baseExpr.GetDefPhi().GetOpnds()) {
        if (expr->GetMeOp() != kMeOpVar) {
          continue;
        }
        if (std::find(varVector.begin(), varVector.end(), &baseExpr) != varVector.end()) {
          return true;
        }
        varVector.push_back(expr);
        if (MEAnalyzeDefExpr(*expr, varVector)) {
          return true;
        }
      }
      break;
    }
    case kDefByChi:
    case kDefByMustDef: {
      // Defined symbol is not a global
      return false;
    }
    default: {
      return true;
    }
  }
  return false;
}

// Def global variable or formal parameter
bool IpaSideEffect::AnalyzeDefExpr(VersionSt &baseVar, std::vector<VersionSt*> &varVector) {
  if (!baseVar.GetOst()->IsLocal()) {
    return true;
  }
  switch (baseVar.GetDefType()) {
    case VersionSt::kAssign: {
      if (baseVar.GetAssignNode() == nullptr) {
        return true;
      }
      BaseNode *rhs = baseVar.GetAssignNode()->GetRHS();
      ASSERT_NOT_NULL(rhs);
      switch (rhs->GetOpCode()) {
        case OP_dread: {
          VersionSt *st = (static_cast<AddrofSSANode*>(rhs))->GetSSAVar();
          for (auto it = varVector.begin(); it != varVector.end(); ++it) {
            // Cycle
            if (*it == st) {
              return true;
            }
          }
          varVector.push_back(st);
          if (AnalyzeDefExpr(*st, varVector)) {
            return true;
          }
          break;
        }
        case OP_gcmallocjarray:
        case OP_gcmalloc:
        case OP_constval: {
          return false;
        }
        case OP_iread: {
          return true;
        }
        case OP_cvt:
        case OP_retype: {
          auto *node = static_cast<TypeCvtNode*>(rhs);
          CHECK_FATAL(node->Opnd(0)->GetOpCode() == OP_dread, "must be dread");
          VersionSt *st = (static_cast<AddrofSSANode*>(node->Opnd(0)))->GetSSAVar();
          for (auto it = varVector.begin(); it != varVector.end(); ++it) {
            // Cycle
            if (*it == st) {
              return true;
            }
          }
          varVector.push_back(st);
          if (AnalyzeDefExpr(*st, varVector)) {
            return true;
          }
          break;
        }
        default:
          CHECK_FATAL(false, "NYI");
      }
      break;
    }
    // case VersionSt::Regassign:
    case VersionSt::kPhi: {
      for (size_t i = 0; i < baseVar.GetPhi()->GetPhiOpnds().size(); ++i) {
        VersionSt *st = baseVar.GetPhi()->GetPhiOpnd(i);
        for (auto it = varVector.begin(); it != varVector.end(); ++it) {
          // Cycle
          if (*it == st) {
            return true;
          }
        }
        varVector.push_back(st);
        if (AnalyzeDefExpr(*st, varVector)) {
          return true;
        }
      }
      break;
    }
    case VersionSt::kMayDef:
    case VersionSt::kMustDef: {
      return true;
    }
    default: {
      CHECK_FATAL(false, "NYI");
      break;
    }
  }
  return false;
}

bool IpaSideEffect::MatchPuidxAndSetSideEffects(PUIdx idx) {
  if (idx == 0) {
    return false;
  }
  auto mrtIt = mrtPuIdx.find(idx);
  if (mrtIt == mrtPuIdx.end()) {
    return false;
  }
  uint8 mrtSe = mrtIt->second;
  notPure = notPure || !(mrtSe & kPure);
  hasDefArg = hasDefArg || (mrtSe & kHasDefArg);
  hasDef = hasDef || (mrtSe & kHasDef);
  hasRetGlobal = hasRetGlobal || (mrtSe & kHasRetGlobal);
  hasThrException = hasThrException || (mrtSe & kHasThrow);
  hasRetArg = hasRetArg || (mrtSe & kHasRetArg);
  hasPrivateDef = hasPrivateDef || (mrtSe & kHasPrivateDef);
  return true;
}


bool IpaSideEffect::IsPureFromSummary(const MIRFunction &func) const {
  const std::string &funcName = func.GetName();
  return funcName.find("_7ChashCode_7C") != std::string::npos ||
         funcName.find("_7CgetClass_7C") != std::string::npos ||
         funcName.find("_7CgetName_7C") != std::string::npos ||
         funcName.find("_7CtoString_7C") != std::string::npos ||
         funcName.find("_7CvalueOf_7C") != std::string::npos;
}


void IpaSideEffect::ReadSummary() {
  if (mrtListSz != 0) {
    return;
  }
  mrtListSz = sizeof(kMrtList) / sizeof(FuncWithSideEffect);
  for (size_t i = 0; i < mrtListSz; i++) {
    MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(kMrtList[i].GetFuncName()));
    if (sym != nullptr) {
      MIRFunction *func = sym->GetFunction();
      uint8 seMask = 0;
      if (kMrtList[i].GetPure()) {
        seMask |= kPure;
        func->SetPure();
      }
      if (kMrtList[i].GetDefArg()) {
        seMask |= kHasDefArg;
      } else {
        func->SetNoDefArgEffect();
      }
      if (kMrtList[i].GetDef()) {
        seMask |= kHasDef;
      } else {
        func->SetNoDefEffect();
      }
      if (kMrtList[i].GetRetGlobal()) {
        seMask |= kHasRetGlobal;
      } else {
        func->SetNoRetGlobal();
      }
      if (kMrtList[i].GetException()) {
        seMask |= kHasThrow;
      } else {
        func->SetNoThrowException();
      }
      if (kMrtList[i].GetRetArg()) {
        seMask |= kHasRetArg;
      } else {
        func->SetNoRetArg();
      }
      if (kMrtList[i].GetPrivateDef()) {
        seMask |= kHasPrivateDef;
      } else {
        func->SetNoPrivateDefEffect();
      }
      func->SetIpaSeen();
      mrtPuIdx[func->GetPuidx()] = seMask;
    }
  }
}

uint32 IpaSideEffect::GetOrSetSCCNodeId(MIRFunction &mirfunc) {
  if (mirfunc.GetSCCId() != -1) {
    return static_cast<uint32>(mirfunc.GetSCCId());
  }
  auto it = callGraph.GetNodesMap().find(&mirfunc);
  if (it != callGraph.GetNodesMap().end()) {
    CGNode *callGraphNode = it->second;
    SCCNode<CGNode> *sccn = callGraphNode->GetSCCNode();
    if (sccn != nullptr && sccn->GetNodes().size() > 1) {
      mirfunc.SetSCCId(static_cast<int32>(sccn->GetID()));
      return sccn->GetID();
    }
  }
  mirfunc.SetSCCId(0);
  return 0;
}

bool IpaSideEffect::IsCallingIntoSCC(uint32 sccID) const {
  if (sccID == 0) {
    return false;
  }
  if (sccId == 0) {
    return false;
  }
  return (sccID != sccId);
}

void IpaSideEffect::UpdateExternalFuncSideEffects(MIRFunction &func) {
  if (!func.IsIpaSeen()) {
    return;
  }
  // This is an external caller read in from an already processed mplt file.
  // Need to process all callee and update the caller side effects.
  notPure = notPure || !func.IsPure();
  hasDefArg = hasDefArg || !func.IsNoDefArgEffect();
  hasDef = hasDef || !func.IsNoDefEffect();
  hasRetGlobal = hasRetGlobal || !func.IsNoRetGlobal();
  hasThrException = hasThrException || !func.IsNoThrowException();
  hasRetArg = hasRetArg || !func.IsNoRetArg();
  hasPrivateDef = hasPrivateDef || !func.IsNoPrivateDefEffect();
  auto callerIt = callGraph.GetNodesMap().find(&func);
  CHECK_FATAL(callerIt != callGraph.GetNodesMap().end(), "CGNode not found.");
  CGNode *cgNode = callerIt->second;
  for (auto &callSite : cgNode->GetCallee()) {
    // IPASEEN == true, body == NULL;
    // IPASEEN == true, body != NULL;
    // IPASEEN == false, body != NULL, ignore
    // IPASEEN == false, body == NULL, most conservative
    uint8 mask1 = 0;
    uint8 mask2 = 0;

    for (auto &cgIt : *callSite.second) {
      MIRFunction *callee = cgIt->GetMIRFunction();
      if (callee->IsIpaSeen()) {
        auto &mask = (callee->GetBody() == nullptr) ? mask1 : mask2;
        mask |= (!callee->IsNoDefArgEffect() ?     kHasDefArg :     0) |
                (!callee->IsNoDefEffect() ?        kHasDef :        0) |
                (!callee->IsNoRetGlobal() ?        kHasRetGlobal :  0) |
                (!callee->IsNoThrowException() ?   kHasThrow :      0) |
                (!callee->IsNoRetArg() ?           kHasRetArg :     0) |
                (!callee->IsNoPrivateDefEffect() ? kHasPrivateDef : 0);
      } else if (!callee->IsIpaSeen() && callee->GetBody() == nullptr) {
        if (!IsPureFromSummary(*callee)) { // Set pure
          hasPrivateDef = true;
          hasThrException = true;
          hasDef = true;
        }
      }
    }
    if ((((mask1 & kHasDef) == 0) && ((mask2 & kHasDef) == kHasDef)) ||
        (((mask1 & kHasDefArg) == 0) && ((mask2 & kHasDefArg) == kHasDefArg))) {
      CHECK_FATAL(false, "Need to update");
    }
  }
}

// Whether or not the expr is reading global or arg variables
void IpaSideEffect::AnalyzeExpr(MeExpr &expr,
                                std::set<VarMeExpr*> &globalExprs, std::set<VarMeExpr*> &argExprs,
                                std::set<VarMeExpr*> &nextLevelGlobalExprs, std::set<VarMeExpr*> &nextLevelArgExprs) {
  bool isGlobalTmp = false;
  bool isArgTmp = false;
  if (expr.GetPrimType() != PTY_ptr && expr.GetPrimType() != PTY_ref) {
    return;
  }
  Opcode op = expr.GetOp();
  for (size_t i = 0; i < expr.GetNumOpnds(); ++i) {
    AnalyzeExpr(*expr.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
  }
  switch (op) {
    case OP_iread: {
      hasThrException = true;
      IvarMeExpr &ivarMeExpr = static_cast<IvarMeExpr&>(expr);
      MeExpr *tmpBase = ivarMeExpr.GetBase();
      CHECK_FATAL(tmpBase->GetOp() == OP_dread || tmpBase->GetOp() == OP_array, "Must be dread or array");
      MeExpr *baseNode = nullptr;
      if (tmpBase->GetOp() == OP_array) {
        NaryMeExpr *arrayNode = static_cast<NaryMeExpr*>(tmpBase);
        CHECK_FATAL(arrayNode->GetOpnd(0)->GetOp() == OP_dread, "Must be dread");
        baseNode = arrayNode->GetOpnd(0);
      } else {
        baseNode = tmpBase;
      }
      CHECK_FATAL(baseNode->GetOp() == OP_dread, "must be dread");
      VarMeExpr *dread = static_cast<VarMeExpr*>(baseNode);
      if (nextLevelGlobalExprs.find(dread) != nextLevelGlobalExprs.end()) {
        isGlobalTmp = true;
      }
      if (nextLevelArgExprs.find(dread) != nextLevelArgExprs.end()) {
        isArgTmp = true;
      }
      break;
    }
    case OP_array: {
      hasThrException = true;
      break;
    }
    case OP_intrinsicop: {
      NaryMeExpr &intrnNode = static_cast<NaryMeExpr&>(expr);
      if (intrnNode.GetIntrinsic() != INTRN_JAVA_MERGE) {
        CHECK_FATAL(false, "NYI");
      }
      break;
    }
    case OP_div:
    case OP_rem:
    case OP_gcmalloc:
    case OP_gcmallocjarray:
    case OP_gcpermallocjarray:
    case OP_gcpermalloc: {
      hasThrException = true;
      isGlobalTmp = false;
      isArgTmp = false;
      break;
    }
    case OP_dread: {
      VarMeExpr &dread = static_cast<VarMeExpr&>(expr);
      const OriginalSt *ost = meFunc.GetMeSSATab()->GetSymbolOriginalStFromID(dread.GetOstIdx());
      if ((ost->GetMIRSymbol()->IsGlobal() && !ost->GetMIRSymbol()->IsConst() && !ost->GetMIRSymbol()->IsFinal()) ||
          globalExprs.find(&dread) != globalExprs.end()) {
        isGlobalTmp = true;
      } else if (meFunc.GetMirFunc()->IsAFormal(ost->GetMIRSymbol()) || argExprs.find(&dread) != argExprs.end()) {
        isArgTmp = true;
      }
      break;
    }
    case OP_constval:
    case OP_cvt:
    case OP_shl:
    case OP_lshr:
    case OP_band:
    case OP_add:
    case OP_ashr:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_cior:
    case OP_cmp:
    case OP_cmpg:
    case OP_cmpl:
    case OP_addrof:
    case OP_sub: {
      isGlobalTmp = false;
      isArgTmp = false;
      break;
    }
    case OP_retype:
    case OP_regread: {
      break;
    }
    case OP_intrinsicopwithtype: {
      NaryMeExpr &instrinsicExpr = static_cast<NaryMeExpr&>(expr);
      if (instrinsicExpr.GetIntrinsic() == INTRN_JAVA_CONST_CLASS ||
          instrinsicExpr.GetIntrinsic() == INTRN_JAVA_INSTANCE_OF) {
        isGlobalTmp = false;
        isArgTmp = false;
      } else {
        CHECK_FATAL(false, "NYI");
      }
      break;
    }
    default: {
      CHECK_FATAL(false, "NYI");
    }
  }
  isGlobal = isGlobal || isGlobalTmp;
  isArg = isArg || isArgTmp;
}

bool IpaSideEffect::UpdateSideEffectWithStmt(MeStmt &meStmt,
                                             std::set<VarMeExpr*> &globalExprs, std::set<VarMeExpr*> &argExprs,
                                             std::set<VarMeExpr*> &nextLevelGlobalExprs,
                                             std::set<VarMeExpr*> &nextLevelArgExprs) {
  isArg = false;
  isGlobal = false;
  Opcode op = meStmt.GetOp();
  switch (op) {
    case OP_return: {
      for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      if (isGlobal) {
        hasRetGlobal = true;
      }
      if (isArg) {
        hasRetArg = true;
      }
      break;
    }
    case OP_iassign: {
      for (size_t i = 1; i < meStmt.NumMeStmtOpnds(); ++i) { // Ignore base (LHR), both array or structure
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      hasThrException = true;
      IassignMeStmt &iasNode = static_cast<IassignMeStmt&>(meStmt);
      CHECK_FATAL(iasNode.GetLHSVal()->GetBase()->GetOp() == OP_dread ||
                  iasNode.GetLHSVal()->GetBase()->GetOp() == OP_array, "Must be dread");
      MeExpr *baseNode = nullptr;
      if (iasNode.GetLHSVal()->GetBase()->GetOp() == OP_array) {
        NaryMeExpr *arrayNode = static_cast<NaryMeExpr*>(iasNode.GetLHSVal()->GetBase());
        CHECK_FATAL(arrayNode->GetOpnd(0)->GetOp() == OP_dread, "Must be dread");
        baseNode = arrayNode->GetOpnd(0);
      } else {
        baseNode = iasNode.GetLHSVal()->GetBase();
      }
      VarMeExpr *dread = static_cast<VarMeExpr*>(baseNode);
      const OriginalSt *ost = meFunc.GetMeSSATab()->GetSymbolOriginalStFromID(dread->GetOstIdx());
      CHECK_FATAL(ost, "ost is nullptr!");
      if (ost->GetMIRSymbol()->IsGlobal() || globalExprs.find(dread) != globalExprs.end()) {
        SetHasDef();
      } else if (meFunc.GetMirFunc()->IsAFormal(ost->GetMIRSymbol()) || argExprs.find(dread) != argExprs.end()) {
        hasDefArg = true;
      }
      if (isGlobal) {
        (void)nextLevelGlobalExprs.insert(dread);
      }
      if (isArg) {
        (void)nextLevelArgExprs.insert(dread);
      }
      std::vector<MeExpr*> varVector;
      if (MEAnalyzeDefExpr(*dread, varVector)) {
        MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iasNode.GetTyIdx());
        MIRType *pType = static_cast<MIRPtrType*>(baseType)->GetPointedType();
        if (pType != nullptr && pType->IsInstanceOfMIRStructType()) {
          MIRStructType *structType = static_cast<MIRStructType*>(pType);
          FieldAttrs fattrs = structType->GetFieldAttrs(iasNode.GetLHSVal()->GetFieldID());
          if (fattrs.GetAttr(FLDATTR_private)) {
            hasPrivateDef = true;
          }
        }
      }
      break;
    }
    case OP_throw: {
      hasThrException = true;
      break;
    }
    case OP_intrinsiccall:
    case OP_intrinsiccallwithtype: {
      IntrinsiccallMeStmt &callNode = static_cast<IntrinsiccallMeStmt&>(meStmt);
      if (callNode.GetIntrinsic() != INTRN_JAVA_CLINIT_CHECK) {
        CHECK_FATAL(false, "NYI");
      }
      break;
    }
    case OP_intrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      IntrinsiccallMeStmt &callNode = static_cast<IntrinsiccallMeStmt&>(meStmt);
      if (callNode.GetIntrinsic() == INTRN_JAVA_CHECK_CAST) {
        hasThrException = true;
        MeExpr *lhs = callNode.GetAssignedLHS();
        CHECK_NULL_FATAL(lhs);
        CHECK_FATAL(lhs->GetOp() == OP_dread, "Must be dread.");
        VarMeExpr *lhsVar = static_cast<VarMeExpr*>(lhs);
        const OriginalSt *ost = meFunc.GetMeSSATab()->GetSymbolOriginalStFromID(lhsVar->GetOstIdx());
        if (ost->GetMIRSymbol()->IsGlobal()) {
          SetHasDef();
        }
        if (isGlobal) {
          (void)globalExprs.insert(lhsVar);
        }
        if (isArg) {
          (void)argExprs.insert(lhsVar);
        }
      } else if (callNode.GetIntrinsic() == INTRN_JAVA_ARRAY_FILL ||
                 callNode.GetIntrinsic() == INTRN_JAVA_FILL_NEW_ARRAY) {
        hasThrException = true;
        MeExpr *lhs = meStmt.GetOpnd(0);
        CHECK_FATAL(lhs->GetOp() == OP_dread, "Must be dread.");
        VarMeExpr *lhsVar = static_cast<VarMeExpr*>(lhs);
        const OriginalSt *ost = meFunc.GetMeSSATab()->GetSymbolOriginalStFromID(lhsVar->GetOstIdx());
        if (ost->GetMIRSymbol()->IsGlobal()) {
          SetHasDef();
        }
        if (isGlobal) {
          (void)globalExprs.insert(lhsVar);
        }
        if (isArg) {
          (void)argExprs.insert(lhsVar);
        }
      } else if (callNode.GetIntrinsic() == INTRN_JAVA_POLYMORPHIC_CALL) {
        hasPrivateDef = hasThrException = true;
        SetHasDef();
        CallMeStmt &callMeStmt = static_cast<CallMeStmt&>(meStmt);
        MeExpr *lhs = callMeStmt.GetAssignedLHS();
        if (lhs != nullptr) {
          CHECK_FATAL(lhs->GetOp() == OP_dread, "Must be dread.");
          VarMeExpr *lhsVar = static_cast<VarMeExpr*>(lhs);
          (void)globalExprs.insert(lhsVar);
          if (isArg) {
            (void)argExprs.insert(lhsVar);
          }
        }
        if (isArg) {
          hasDefArg = true;
        }
      } else {
        CHECK_FATAL(false, "NYI");
      }
      break;
    }
    case OP_dassign: { // Pass by value, special case is addrof C_str and const_array, in java only
      for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      DassignMeStmt &dassignNode = static_cast<DassignMeStmt&>(meStmt);
      VarMeExpr *lhsVar = static_cast<VarMeExpr*>(dassignNode.GetVarLHS());
      const OriginalSt *ost = meFunc.GetMeSSATab()->GetSymbolOriginalStFromID(lhsVar->GetOstIdx());
      if (ost->GetMIRSymbol()->IsGlobal()) {
        SetHasDef();
      }
      if (isGlobal) {
        (void)globalExprs.insert(lhsVar);
      }
      if (isArg) {
        (void)argExprs.insert(lhsVar);
      }
      break;
    }
    case OP_maydassign: {
      for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      MaydassignMeStmt &dassignNode = static_cast<MaydassignMeStmt&>(meStmt);
      if (dassignNode.GetMayDassignSym()->GetMIRSymbol()->IsGlobal()) {
        SetHasDef();
      }
      if (isGlobal) {
        (void)globalExprs.insert(static_cast<VarMeExpr*>(dassignNode.GetChiList()->begin()->second->GetLHS()));
      }
      if (isArg) {
        (void)argExprs.insert(static_cast<VarMeExpr*>(dassignNode.GetChiList()->begin()->second->GetLHS()));
      }
      break;
    }
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_interfacecallassigned: {
      CallMeStmt &callMeStmt = static_cast<CallMeStmt&>(meStmt);
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callMeStmt.GetPUIdx());
      if (callee->GetBaseClassName().find(JARRAY_PREFIX_STR) == 0) {
        // An array, must have method, but has all effects
        SetEffectsTrue();
        break;
      }
      // If callee is init, process the first argument only.
      size_t count = 0;
      if (callee->IsConstructor()) {
        count = 1;
      } else {
        count = meStmt.NumMeStmtOpnds();
      }
      for (size_t i = 0; i < count; ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      CGNode *callerNode = callGraph.GetCGNode(meFunc.GetMirFunc());
      CHECK_FATAL(callerNode != nullptr, "Must not be null");
      bool defArg = false;
      bool returnGlobal = false;
      bool returnArg = false;
      for (auto &callSite : callerNode->GetCallee()) {
        if (callSite.first->GetID() == callMeStmt.GetStmtID()) {
          for (auto *calleeNode : *callSite.second) {
            MIRFunction *calleeFunc = calleeNode->GetMIRFunction();
            if (calleeFunc->IsIpaSeen()) {
              if (IsPureFromSummary(*calleeFunc)) { // Set pure
                continue;
              }
              if (!calleeFunc->IsNoPrivateDefEffect()) {
                hasPrivateDef = true;
              }
              if (!calleeFunc->IsNoThrowException()) {
                hasThrException = true;
              }
              if (!calleeFunc->IsNoDefEffect()) {
                SetHasDef();
              }
              if (!calleeFunc->IsNoDefArgEffect()) {
                defArg = true;
              }
              if (!calleeFunc->IsNoRetGlobal()) {
                returnGlobal = true;
              }
              if (!calleeFunc->IsNoRetArg()) {
                returnArg = true;
              }
              if (hasPrivateDef && hasThrException && hasDef && defArg && returnGlobal && returnArg) {
                break;
              }
            } else {
              if (!IsPureFromSummary(*calleeFunc)) { // Set pure
                hasPrivateDef = true;
                hasThrException = true;
                defArg = true;
                returnGlobal = true;
                returnArg = true;
                SetHasDef();
              }
            }
          }
          if (!isGlobal && !isArg) {
            callSite.first->SetAllArgsLocal();
          }
          break;
        }
      }
      MeExpr *lhs = callMeStmt.GetAssignedLHS();
      if (lhs != nullptr) {
        CHECK_FATAL(lhs->GetOp() == OP_dread, "Must be dread.");
        VarMeExpr *lhsVar = static_cast<VarMeExpr*>(lhs);
        const OriginalSt *ost = meFunc.GetMeSSATab()->GetSymbolOriginalStFromID(lhsVar->GetOstIdx());
        if (ost->GetMIRSymbol()->IsGlobal()) {
          SetHasDef();
        }
        if (returnGlobal) {
          (void)globalExprs.insert(lhsVar);
        } else if (returnArg) {
          if (isGlobal) {
            (void)globalExprs.insert(lhsVar);
          }
          if (isArg) {
            (void)argExprs.insert(lhsVar);
          }
        }
      }
      if (defArg) {
        if (isGlobal) {
          SetHasDef();
        }
        if (isArg) {
          hasDefArg = true;
        }
      }
      break;
    }
    case OP_call:
    case OP_virtualcall:
    case OP_interfacecall: {
      for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      CallMeStmt &callMeStmt = static_cast<CallMeStmt&>(meStmt);
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callMeStmt.GetPUIdx());
      if (callee->GetBaseClassName().find(JARRAY_PREFIX_STR) == 0) {
        // An array, must have method, but has all effects
        SetEffectsTrue();
        break;
      }
      CGNode *callerNode = callGraph.GetCGNode(meFunc.GetMirFunc());
      CHECK_FATAL(callerNode != nullptr, "Must not be null");
      bool defArg = false;
      for (Callsite callSite : callerNode->GetCallee()) {
        if (callSite.first->GetID() == callMeStmt.GetStmtID()) {
          for (auto *calleeNode : *callSite.second) {
            MIRFunction *calleeFunc = calleeNode->GetMIRFunction();
            if (calleeFunc->IsIpaSeen()) {
              if (IsPureFromSummary(*calleeFunc)) { // Set pure
                continue;
              }
              if (!calleeFunc->IsNoPrivateDefEffect()) {
                hasPrivateDef = true;
              }
              if (!calleeFunc->IsNoThrowException()) {
                hasThrException = true;
              }
              if (!calleeFunc->IsNoDefEffect()) {
                SetHasDef();
              }
              if (!calleeFunc->IsNoDefArgEffect()) {
                defArg = true;
              }
              if (hasPrivateDef && hasThrException && hasDef && defArg) {
                break;
              }
            } else {
              if (!IsPureFromSummary(*calleeFunc)) { // Set pure
                hasPrivateDef = true;
                hasThrException = true;
                defArg = true;
                SetHasDef();
              }
            }
          }
          break;
        }
      }
      if (defArg) {
        if (isGlobal) {
          SetHasDef();
        }
        if (isArg) {
          hasDefArg = true;
        }
      }
      break;
    }
    case OP_virtualicallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_icallassigned:
    case OP_icallprotoassigned:
    case OP_superclasscallassigned:
    case OP_asm: {
      hasPrivateDef = hasThrException = true;
      SetHasDef();
      for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      CallMeStmt &callMeStmt = static_cast<CallMeStmt&>(meStmt);
      MeExpr *lhs = callMeStmt.GetAssignedLHS();
      if (lhs != nullptr) {
        CHECK_FATAL(lhs->GetOp() == OP_dread, "Must be dread.");
        VarMeExpr *lhsVar = static_cast<VarMeExpr*>(lhs);
        (void)globalExprs.insert(lhsVar);
        if (isArg) {
          (void)argExprs.insert(lhsVar);
        }
      }
      if (isArg) {
        hasDefArg = true;
      }
      break;
    }
    case OP_virtualicall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_icall:
    case OP_icallproto: {
      hasPrivateDef = hasThrException = true;
      SetHasDef();
      for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
        AnalyzeExpr(*meStmt.GetOpnd(i), globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs);
      }
      if (isArg) {
        hasDefArg = true;
      }
      break;
    }
    case OP_jscatch:
    case OP_finally:
    case OP_endtry:
    case OP_cleanuptry:
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstoreload:
    case OP_membarstorestore:
    case OP_retsub:
    case OP_gosub:
    case OP_goto:
    case OP_brfalse:
    case OP_brtrue:
    case OP_comment:
    case OP_jstry:
    case OP_try:
    case OP_catch:
    case OP_syncenter:
    case OP_syncexit:
    CASE_OP_ASSERT_NONNULL
    case OP_eval:
    case OP_free:
    case OP_switch:
    case OP_label:
      break;
    default: {
      CHECK_FATAL(false, "NYI");
    }
  }
  return isGlobal;
}

void IpaSideEffect::DoAnalysis() {
  ReadSummary(); // Read once
  MIRFunction *func = meFunc.GetMirFunc();
  if (func->IsAbstract() || mrtPuIdx.find(func->GetPuidx()) != mrtPuIdx.end()) {
    return;
  }
  // SCC check
  auto it = callGraph.GetNodesMap().find(func);
  CHECK_FATAL(it != callGraph.GetNodesMap().end(), "Must have cg node");
  CGNode *cgNode = it->second;
  SCCNode<CGNode> *sccNode = cgNode->GetSCCNode();
  CHECK_FATAL(sccNode != nullptr, "Must not be null.");
  if (currSccNode == nullptr) {
    currSccNode = sccNode;
    cgNodeCount = sccNode->GetNodes().size();
  } else if (currSccNode == sccNode) {
    CHECK_FATAL(cgNodeCount > 0, "cgNodeCount must be larger than 0");
  } else {
    currSccNode = sccNode;
    cgNodeCount = sccNode->GetNodes().size();
  }
  bool useGlobal = false;
  if (func->GetBody() == nullptr) { // External function from mplt, need to update effects
    UpdateExternalFuncSideEffects(*func);
  } else {
    auto cfg = meFunc.GetCfg();
    cfg->BuildSCC();
    std::set<VarMeExpr*> globalExprs;
    std::set<VarMeExpr*> argExprs;
    std::set<VarMeExpr*> nextLevelGlobalExprs;
    std::set<VarMeExpr*> nextLevelArgExprs;
    for (size_t i = 0; i < cfg->GetSccTopologicalVec().size(); ++i) {
      SCCOfBBs *scc = cfg->GetSccTopologicalVec()[i];
      CHECK_FATAL(scc != nullptr, "scc must not be null");
      if (scc->GetBBs().size() > 1) {
        cfg->BBTopologicalSort(*scc);
      }
      const uint32 maxLoopCount = 2;
      unsigned loopCount = scc->GetBBs().size() > 1 ? maxLoopCount : 1; // Loop count
      for (unsigned j = 0; j < loopCount; ++j) {
        for (BB *bb : scc->GetBBs()) {
          if (bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB()) {
            continue;
          }
          for (auto &meStmt : bb->GetMeStmts()) {
            if (UpdateSideEffectWithStmt(meStmt, globalExprs, argExprs, nextLevelGlobalExprs, nextLevelArgExprs)) {
              useGlobal = true;
            }
          }
        }
      }
    }
  }

  uint8 mask = 0;
  sccId = GetOrSetSCCNodeId(*func);
  if (sccId != 0) {
    auto itTmp = sccSe.find(sccId);
    if (itTmp != sccSe.end()) {
      mask = itTmp->second;
    }
  }
  if (hasDefArg || hasDef || useGlobal) {
    mask |= kNotPure;
  } else {
    func->SetPure();
  }
  if (!hasDefArg) {
    func->SetNoDefArgEffect();
  }
  if (hasDef) {
    mask |= kHasDef;
  } else {
    func->SetNoDefEffect();
  }
  if (!hasRetGlobal) {
    func->SetNoRetGlobal();
  }
  if (hasThrException) {
    mask |= kHasThrow;
  } else {
    func->SetNoThrowException();
  }
  if (!hasRetArg) {
    func->SetNoRetArg();
  }
  if (hasPrivateDef) {
    mask |= kHasPrivateDef;
  } else {
    func->SetNoPrivateDefEffect();
  }
  func->SetIpaSeen();
  --cgNodeCount;
  if (sccId != 0) {
    sccSe[sccId] = mask;
  }
  if (cgNodeCount == 0 && sccNode->GetNodes().size() > 1) {
    CopySccSideEffectToAllFunctions(*sccNode, mask);
  }
  // Delete redundant callinfo. func+callType+allargsarelocal
  std::unordered_set<uint64> callInfoHash;
  for (auto callSite = cgNode->GetCallee().begin(); callSite != cgNode->GetCallee().end(); ++callSite) {
    uint64 key = (static_cast<uint64>(callSite->first->GetFunc()->GetPuidx()) << 32) + // leftshift 32 bits
                 (static_cast<uint64>(callSite->first->GetCallType()) << 16) + // leftshift 16 bits
                 (callSite->first->AreAllArgsLocal() ? 1 : 0);
    if (callInfoHash.find(key) != callInfoHash.end()) {
      (void)cgNode->GetCallee().erase(callSite);
      --callSite;
    } else {
      (void)callInfoHash.insert(key);
    }
  }
}

void MESideEffect::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
}

bool MESideEffect::PhaseRun(MeFunction &f) {
  if (f.GetMirFunc()->IsNative()) {
    return false;
  }
  Dominance *dom = nullptr;
  if (f.GetMirFunc()->GetBody() != nullptr) {
    dom = GET_ANALYSIS(MEDominance, f);
  }
  CHECK_FATAL(dom != nullptr, "Dominance must be built.");
  MaplePhase *it = GetAnalysisInfoHook()->GetTopLevelAnalyisData<M2MCallGraph, MIRModule>(f.GetMIRModule());
  CallGraph *callGraph = static_cast<M2MCallGraph*>(it)->GetResult();
  CHECK_FATAL(callGraph != nullptr, "Call graph must be built.");
  IpaSideEffect ipaSideEffect(f, ApplyTempMemPool(), *callGraph, *dom);
  ipaSideEffect.DoAnalysis();
  return true;
}
}  // namespace maple
