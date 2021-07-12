/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_function.h"
#include "lfo_dep_test.h"

namespace maple {

void LfoDepInfo::CreateDoloopInfo(BlockNode *block, DoloopInfo *parent) {
  StmtNode *stmt = block->GetFirst();
  while (stmt) {
    switch (stmt->GetOpCode()) {
      case OP_doloop: {
        DoloopNode *doloop = static_cast<DoloopNode *>(stmt);
        DoloopInfo *doloopInfo = memPool->New<DoloopInfo>(&alloc, this, doloop, parent);
        doloopInfoMap.insert(std::pair<DoloopNode *, DoloopInfo *>(doloop, doloopInfo));
        if (parent) {
          parent->children.push_back(doloopInfo);
        } else {
          outermostDoloopInfoVec.push_back(doloopInfo);
        }
        CreateDoloopInfo(doloop->GetDoBody(), doloopInfo);
        break;
      }
      case OP_block: {
        CreateDoloopInfo(static_cast<BlockNode *>(stmt), parent);
        break;
      }
      case OP_if: {
        IfStmtNode *ifstmtnode = static_cast<IfStmtNode *>(stmt);
        if (ifstmtnode->GetThenPart())
          CreateDoloopInfo(ifstmtnode->GetThenPart(), parent);
        if (ifstmtnode->GetElsePart())
          CreateDoloopInfo(ifstmtnode->GetElsePart(), parent);
        break;
      }
      case OP_dowhile:
      case OP_while: {
        CreateDoloopInfo(static_cast<WhileStmtNode *>(stmt)->GetBody(), parent);
        break;
      }
      default:
        break;
    }
    stmt = stmt->GetNext();
  }
}

SubscriptDesc *DoloopInfo::BuildOneSubscriptDesc(BaseNode *subsX) {
  SubscriptDesc *subsDesc = alloc->GetMemPool()->New<SubscriptDesc>();
  Opcode op = subsX->GetOpCode();
  BaseNode *mainTerm = nullptr;
  if (op != OP_add && op != OP_sub) {
    mainTerm = subsX;
  } else {  // get addtiveConst
    BinaryNode *binnode = static_cast<BinaryNode *>(subsX);
    BaseNode *opnd0 = binnode->Opnd(0);
    BaseNode *opnd1 = binnode->Opnd(1);
    if (opnd1->op != OP_constval) {
      subsDesc->tooMessy = true;
      return subsDesc;
    }
    MIRConst *mirconst = static_cast<ConstvalNode *>(opnd1)->GetConstVal();
    if (mirconst->GetKind() != kConstInt) {
      subsDesc->tooMessy = true;
      return subsDesc;
    }
    subsDesc->additiveConst = static_cast<MIRIntConst *>(mirconst)->GetValue();
    if (op == OP_sub)
      subsDesc->additiveConst = - subsDesc->additiveConst;
    mainTerm = opnd0;
  }
  // process mainTerm
  BaseNode *varNode = nullptr;
  if (op != OP_mul) {
    varNode = mainTerm;
  } else {
    BinaryNode *mulbinnode = static_cast<BinaryNode *>(mainTerm);
    BaseNode *mulopnd0 = mulbinnode->Opnd(0);
    BaseNode *mulopnd1 = mulbinnode->Opnd(1);
    if (mulopnd0->GetOpCode() != OP_dread) {
      subsDesc->tooMessy = true;
      return subsDesc;
    }
    varNode = mulopnd0;
    if (mulopnd1->GetOpCode() != OP_constval) {
      subsDesc->tooMessy = true;
      return subsDesc;
    }
    MIRConst *mirconst = static_cast<ConstvalNode *>(mulopnd1)->GetConstVal();
    if (mirconst->GetKind() != kConstInt) {
      subsDesc->tooMessy = true;
      return subsDesc;
    }
    subsDesc->coeff = static_cast<MIRIntConst *>(mirconst)->GetValue();
  } 
  // process varNode
  if (varNode->GetOpCode() == OP_dread) {
    DreadNode *dnode = static_cast<DreadNode *>(varNode);
    if (dnode->GetStIdx() == doloop->GetDoVarStIdx()) {
      subsDesc->iv = dnode;
      return subsDesc;
    }
  }
  subsDesc->tooMessy = true;
  return subsDesc;
}

void DoloopInfo::BuildOneArrayAccessDesc(ArrayNode *arr, bool isRHS) {
#if 0
  MIRType *atype =  arr->GetArrayType(GlobalTables::GetTypeTable());
  ASSERT(atype->GetKind() == kTypeArray, "type was wrong");
  MIRArrayType *arryty = static_cast<MIRArrayType *>(atype);
  size_t dim = arryty->GetDim();
  CHECK_FATAL(dim == arr->NumOpnds() - 1, "BuildOneArrayAccessDesc: inconsistent array dimension");
#else
  size_t dim = arr->NumOpnds() - 1;
#endif
  ArrayAccessDesc *arrDesc = alloc->GetMemPool()->New<ArrayAccessDesc>(alloc, arr);
  if (isRHS) {
    rhsArrays.push_back(arrDesc);
  } else {
    lhsArrays.push_back(arrDesc);
  }
  for (size_t i = 0; i < dim; i++) {
    SubscriptDesc *subs = BuildOneSubscriptDesc(arr->GetIndex(i));
    arrDesc->subscriptVec.push_back(subs);
  }
}

void DoloopInfo::CreateRHSArrayAccessDesc(BaseNode *x) {
  if (x->GetOpCode() == OP_array) {
    BuildOneArrayAccessDesc(static_cast<ArrayNode *>(x), true/*isRHS*/);
  }
  for (size_t i = 0; i < x->NumOpnds(); i++) {
    CreateRHSArrayAccessDesc(x->Opnd(i));
  }
}

void DoloopInfo::CreateArrayAccessDesc(BlockNode *block) {
  StmtNode *stmt = block->GetFirst();
  while (stmt) {
    switch (stmt->GetOpCode()) {
      case OP_doloop: {
        CHECK_FATAL(false, "CreateArrayAccessDesc: non-innermost doloop NYI");
        break;
      }
      case OP_block: {
        CreateArrayAccessDesc(static_cast<BlockNode *>(stmt));
        break;
      }
      case OP_if: {
        CreateRHSArrayAccessDesc(stmt->Opnd(0));
        IfStmtNode *ifstmtnode = static_cast<IfStmtNode *>(stmt);
        if (ifstmtnode->GetThenPart())
          CreateArrayAccessDesc(ifstmtnode->GetThenPart());
        if (ifstmtnode->GetElsePart())
          CreateArrayAccessDesc(ifstmtnode->GetElsePart());
        break;
      }
      case OP_dowhile:
      case OP_while: {
        CreateRHSArrayAccessDesc(stmt->Opnd(0));
        CreateArrayAccessDesc(static_cast<WhileStmtNode *>(stmt)->GetBody());
        break;
      }
      case OP_iassign: {
        IassignNode *iass = static_cast<IassignNode *>(stmt);
        if (iass->addrExpr->GetOpCode() == OP_array) {
          BuildOneArrayAccessDesc(static_cast<ArrayNode *>(iass->addrExpr), false/*isRHS*/);
        } else {
          hasPtrAccess = true;
        }
        CreateRHSArrayAccessDesc(iass->rhs);
        break;
      }
      case OP_call:
      case OP_callassigned:
      case OP_icall:
      case OP_icallassigned: {
        hasCall = true;
        // fall thru
      }
      default: {
        for (size_t i = 0; i < stmt->NumOpnds(); i++) {
          CreateRHSArrayAccessDesc(stmt->Opnd(i));
        }
        break;
      }
    }
    stmt = stmt->GetNext();
  }
}

void LfoDepInfo::CreateArrayAccessDesc(MapleMap<DoloopNode *, DoloopInfo *> *doloopInfoMap) {
  MapleMap<DoloopNode *, DoloopInfo *>::iterator mapit = doloopInfoMap->begin();
  for (; mapit != doloopInfoMap->end(); mapit++) {
    DoloopInfo *doloopInfo = mapit->second;
    if (!doloopInfo->children.empty()) {
      continue;  // only handling innermost doloops
    }
    doloopInfo->CreateArrayAccessDesc(doloopInfo->doloop->GetDoBody());
    if (DEBUGFUNC(lfoFunc->meFunc)) {
      LogInfo::MapleLogger() << "Innermost Doloop:";
      if (doloopInfo->hasPtrAccess) {
        LogInfo::MapleLogger() << " hasPtrAccess";
      }
      if (doloopInfo->hasCall) {
        LogInfo::MapleLogger() << " hasCall";
      }
      LogInfo::MapleLogger() << std::endl;
      doloopInfo->doloop->Dump(0);
      LogInfo::MapleLogger() << "LHS arrays:\n";
      for (ArrayAccessDesc *arrAcc : doloopInfo->lhsArrays) {
        arrAcc->theArray->Dump(0);
        LogInfo::MapleLogger() << " subscripts:";
        for (SubscriptDesc *subs : arrAcc->subscriptVec) {
          if (subs->tooMessy) {
            LogInfo::MapleLogger() << " [messy]";
          } else {
            LogInfo::MapleLogger() << " [" << subs->coeff << "*";
            LfoPart *lfopart = preEmit->GetLfoExprPart(subs->iv);
            ScalarMeExpr *scalar = static_cast<ScalarMeExpr *>(lfopart->GetMeExpr());
            scalar->GetOst()->Dump();
            LogInfo::MapleLogger() << "+" << subs->additiveConst << "]";
          }
        }
        LogInfo::MapleLogger() << std::endl;
      }
      LogInfo::MapleLogger() << "RHS arrays: ";
      for (ArrayAccessDesc *arrAcc : doloopInfo->rhsArrays) {
        arrAcc->theArray->Dump(0);
        LogInfo::MapleLogger() << " subscripts:";
        for (SubscriptDesc *subs : arrAcc->subscriptVec) {
          if (subs->tooMessy) {
            LogInfo::MapleLogger() << " [messy]";
          } else {
            LogInfo::MapleLogger() << " [" << subs->coeff << "*";
            LfoPart *lfopart = preEmit->GetLfoExprPart(subs->iv);
            ScalarMeExpr *scalar = static_cast<ScalarMeExpr *>(lfopart->GetMeExpr());
            scalar->GetOst()->Dump();
            LogInfo::MapleLogger() << "+" << subs->additiveConst << "]";
          }
        }
        LogInfo::MapleLogger() << std::endl;
      }
      LogInfo::MapleLogger() << std::endl;
    }
  }
}

AnalysisResult *DoLfoDepTest::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*) {
  LfoPreEmitter *preEmit = static_cast<LfoPreEmitter *>(m->GetAnalysisResult(MeFuncPhase_LFOPREEMIT, func));
  LfoFunction *lfoFunc = func->GetLfoFunc();
  MemPool *depTestMp = NewMemPool();
  LfoDepInfo *depInfo = depTestMp->New<LfoDepInfo>(depTestMp, lfoFunc, preEmit);
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "\n============== LFO_DEP_TEST =============" << '\n';
  }
  depInfo->CreateDoloopInfo(func->GetMirFunc()->GetBody(), nullptr);
  depInfo->CreateArrayAccessDesc(&depInfo->doloopInfoMap);
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "________________" << std::endl;
    lfoFunc->meFunc->GetMirFunc()->Dump();
  }
  return depInfo;
}
}  // namespace maple
