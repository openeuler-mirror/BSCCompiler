/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <iostream>
#include <algorithm>
#include "me_option.h"
#include "mir_module.h"
#include "mir_lower.h"
#include "mir_builder.h"
#include "lfo_loop_vec.h"

namespace maple {

void LoopVecInfo::UpdatePrimType(PrimType cptype) {
  if (GetPrimTypeSize(largestPrimType) < GetPrimTypeSize(cptype)) {
    largestPrimType = cptype;
  }
}

// generate new bound for vectorization loop and epilog loop
// original bound info <initnode, uppernode, incrnode>, condNode doesn't include equal
// limitation now:  initNode and incrNode are const and initnode is vectorLane aligned.
// vectorization loop: <initnode, uppernode/vectorFactor * vectorFactor, incrNode*vectFact>
// epilog loop: < uppernode/vectorFactor*vectorFactor, uppernode, incrnode>
void LoopTransPlan::GenerateBoundInfo(DoloopNode *doloop, DoloopInfo *li) {
  BaseNode *initNode = doloop->GetStartExpr();
  BaseNode *incrNode = doloop->GetIncrExpr();
  BaseNode *condNode = doloop->GetCondExpr();
  bool condOpHasEqual = ((condNode->GetOpCode() == OP_le) || (condNode->GetOpCode() == OP_ge));
  ConstvalNode *constOnenode = nullptr;
  MIRType *typeInt = GlobalTables::GetTypeTable().GetInt32();
  if (condOpHasEqual) {
    MIRIntConst *constOne = GlobalTables::GetIntConstTable().GetOrCreateIntConst(1, *typeInt);
    constOnenode = codeMP->New<ConstvalNode>(PTY_i32, constOne);
  }
  ASSERT(incrNode->IsConstval(), "too complex, incrNode should be const");
  ConstvalNode *icn = static_cast<ConstvalNode*>(incrNode);
  MIRIntConst *incrConst = static_cast<MIRIntConst*>(icn->GetConstVal());
  ASSERT(condNode->IsBinaryNode(), "cmp node should be binary node");
  BaseNode *upNode = condNode->Opnd(1); // TODO:: verify opnd(1) is upper while opnd(0) is index variable

  MIRIntConst *newIncr = GlobalTables::GetIntConstTable().GetOrCreateIntConst(vecFactor*incrConst->GetValue(), *typeInt);
  ConstvalNode *newIncrNode = codeMP->New<ConstvalNode>(PTY_i32, newIncr);
  // check initNode is alignment
  if (initNode->IsConstval()) {
    ConstvalNode *lcn = static_cast<ConstvalNode*>(initNode);
    MIRIntConst *lowConst = static_cast<MIRIntConst*>(lcn->GetConstVal());
    int64 lowvalue = lowConst->GetValue();
    // upNode is constant
    if (upNode->IsConstval()) {
      ConstvalNode *ucn = static_cast<ConstvalNode*>(upNode);
      MIRIntConst *upConst = static_cast<MIRIntConst*>(ucn->GetConstVal());
      int64 upvalue = upConst->GetValue();
      if (condOpHasEqual) {
        upvalue += 1;
      }
      if (((upvalue - lowvalue) % (newIncr->GetValue())) == 0) {
        // early return, change vbound->stride only
        vBound = localMP->New<LoopBound>(nullptr, nullptr, newIncrNode);
      } else {
        // trip count is not vector lane aligned
        // vectorized loop < initnode, (up - low)/newIncr *newincr + init, newincr>
        // TODO: the vectorized loop need unalignment instruction.
        int32_t newupval = (upvalue - lowvalue) / (newIncr->GetValue()) * (newIncr->GetValue()) + lowvalue;
        MIRIntConst *newUpConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(newupval, *typeInt);
        ConstvalNode *newUpNode = codeMP->New<ConstvalNode>(PTY_i32, newUpConst);
        vBound = localMP->New<LoopBound>(nullptr, newUpNode, newIncrNode);
        // generate epilog
        eBound = localMP->New<LoopBound>(newUpNode, nullptr, nullptr);
      }
    } else if (upNode->GetOpCode() == OP_dread) {
      // upNode is symbol variable, TODO::op_regread
      // step 1: generate vectorized loop bound
      AddrofNode *dreadnode = static_cast<AddrofNode*>(upNode);
      // upNode of vBound is uppnode / newIncr * newIncr
      BinaryNode *divnode;
      BaseNode *addnode = upNode;
      if (condOpHasEqual) {
        addnode = codeMP->New<BinaryNode>(OP_add, PTY_i32, dreadnode, constOnenode);
      }
      if (lowvalue != 0) {
        BinaryNode *subnode = codeMP->New<BinaryNode>(OP_sub, PTY_i32, addnode, initNode);
        divnode = codeMP->New<BinaryNode>(OP_div, PTY_i32, subnode, newIncrNode);
      } else {
        divnode = codeMP->New<BinaryNode>(OP_div, PTY_i32, addnode, newIncrNode);
      }
      BinaryNode *mulnode = codeMP->New<BinaryNode>(OP_mul, PTY_i32, divnode, newIncrNode);
      vBound = localMP->New<LoopBound>(nullptr, mulnode, newIncrNode);
      // step2:  generate epilog bound
      eBound = localMP->New<LoopBound>(mulnode, nullptr, nullptr);
    } else {
      ASSERT(0, "upper bound is complex, NIY");
    }
  } else if (initNode->GetOpCode() == OP_dread) {
    // initnode is not constant
    // set bound of vectorized loop
    BinaryNode *subnode;
    if (condOpHasEqual) {
      BinaryNode *addnode = codeMP->New<BinaryNode>(OP_add, PTY_i32, upNode, constOnenode);
      subnode = codeMP->New<BinaryNode>(OP_sub, PTY_i32, addnode, initNode);
    } else {
      subnode = codeMP->New<BinaryNode>(OP_sub, PTY_i32, upNode, initNode);
    }
    BinaryNode *divnode = codeMP->New<BinaryNode>(OP_div, PTY_i32, subnode, newIncrNode);
    BinaryNode *mulnode = codeMP->New<BinaryNode>(OP_mul, PTY_i32, divnode, newIncrNode);
    vBound = localMP->New<LoopBound>(nullptr, mulnode, newIncrNode);
    // set bound of epilog loop
    eBound = localMP->New<LoopBound>(mulnode, nullptr, nullptr);
  } else {
    ASSERT(0, "low bound is complex, NIY");
  }
}

// generate best plan for current doloop
void LoopTransPlan::Generate(DoloopNode *doloop, DoloopInfo* li) {
  // hack values of vecFactor and vecLanes
  vecLanes = 128 / ((GetPrimTypeSize(vecInfo->largestPrimType)) * 8);
  vecFactor = vecLanes; // vectory length / type
  // generate bound information
  GenerateBoundInfo(doloop, li);
}

MIRType* LoopVectorization::GenVecType(PrimType sPrimType, uint8 lanes) {
   MIRType *vecType = nullptr;
   CHECK_FATAL(IsPrimitiveInteger(sPrimType), "primtype should be integer");
   switch(sPrimType) {
     case PTY_i32:  {
       if (lanes == 4) {
         vecType = GlobalTables::GetTypeTable().GetV4Int32();
       } else if (lanes == 2) {
         vecType = GlobalTables::GetTypeTable().GetV2Int32();
       } else {
         ASSERT(0, "unsupported int32 vectory lanes");
       }
       break;
     }
     case PTY_u32: {
       if (lanes == 4) {
         vecType = GlobalTables::GetTypeTable().GetV4UInt32();
       } else if (lanes == 2) {
         vecType = GlobalTables::GetTypeTable().GetV2UInt32();
       } else {
         ASSERT(0, "unsupported uint32 vectory lanes");
       }
       break;
     }
     case PTY_i16: {
       if (lanes == 4) {
         vecType = GlobalTables::GetTypeTable().GetV4Int16();
       } else if (lanes == 8) {
         vecType = GlobalTables::GetTypeTable().GetV8Int16();
       } else {
         ASSERT(0, "unsupported int16 vector lanes");
       }
       break;
     }
     case PTY_u16: {
       if (lanes == 4) {
         vecType = GlobalTables::GetTypeTable().GetV4UInt16();
       } else if (lanes == 8) {
         vecType = GlobalTables::GetTypeTable().GetV8UInt16();
       } else {
         ASSERT(0, "unsupported uint16 vector lanes");
       }
       break;
     }
     case PTY_i8: {
       if (lanes == 16) {
         vecType = GlobalTables::GetTypeTable().GetV16Int8();
       } else if (lanes == 8) {
         vecType = GlobalTables::GetTypeTable().GetV8Int8();
       } else {
         ASSERT(0, "unsupported int8 vector lanes");
       }
       break;
     }
     case PTY_u8: {
       if (lanes == 16) {
         vecType = GlobalTables::GetTypeTable().GetV16UInt8();
       } else if (lanes == 8) {
         vecType = GlobalTables::GetTypeTable().GetV8UInt8();
       } else {
         ASSERT(0, "unsupported uint8 vector lanes");
       }
       break;
     }
     case PTY_i64:  {
       if (lanes == 2) {
         vecType = GlobalTables::GetTypeTable().GetV2Int64();
       } else {
         ASSERT(0, "unsupported int64 vector lanes");
       }
       break;
     }
     case PTY_u64:  {
       if (lanes == 2) {
         vecType = GlobalTables::GetTypeTable().GetV2UInt64();
       } else {
         ASSERT(0, "unsupported uint64 vector lanes");
       }
       break;
     }
     default:
       ASSERT(0, "NIY");
   }
   return vecType;
}

// generate instrinsic node to copy scalar to vector type
StmtNode *LoopVectorization::GenIntrinNode(BaseNode *scalar, PrimType vecPrimType) {
  PrimType intrnPrimtype = PTY_v4i32;
  MIRIntrinsicID intrnID = INTRN_vector_from_scalar_v4i32;
  MIRType *vecType = nullptr;
  switch(vecPrimType) {
    case PTY_v4i32: {
      intrnPrimtype = PTY_v4i32;
      intrnID = INTRN_vector_from_scalar_v4i32;
      vecType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    default: {
      ASSERT(0, "NIY");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicopwithtype, PTY_v4i32);
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(1);
  rhs->SetNOpndAt(0, scalar);
  rhs->SetTyIdx(vecType->GetTypeIndex());
  PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(intrnPrimtype, vecType);
  RegassignNode *stmtNode = codeMP->New<RegassignNode>(PTY_v4i32, regIdx, rhs);
  return stmtNode;
}

// iterate tree node to wide scalar type to vector type
// following opcode can be vectorized directly
//  +, -, *, &, |, <<, >>, compares, ~, !
// iassign, iread, dassign, dread
void LoopVectorization::VectorizeNode(BaseNode *node, uint8 count) {
  if (enableDebug) {
    node->Dump(0);
  }
  switch (node->GetOpCode()) {
    case OP_iassign: {
      IassignNode *iassign = static_cast<IassignNode *>(node);
      // change lsh type to vector type
      MIRType &mirType = GetTypeFromTyIdx(iassign->GetTyIdx());
      CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
      MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
      MIRType *vecType = GenVecType(ptrType->GetPointedType()->GetPrimType(), count);
      ASSERT(vecType != nullptr, "vector type should not be null");
      MIRType *pvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType, PTY_ptr);
      // update lhs type
      iassign->SetTyIdx(pvecType->GetTypeIndex());
      // visit rsh
      VectorizeNode(iassign->GetRHS(), count);
      break;
    }
    case OP_iread: {
      IreadNode *ireadnode = static_cast<IreadNode *>(node);
      // update primtype
      MIRType *primVecType = GenVecType(ireadnode->GetPrimType(), count);
      node->SetPrimType(primVecType->GetPrimType());
      // update tyidx
      MIRType &mirType = GetTypeFromTyIdx(ireadnode->GetTyIdx());
      CHECK_FATAL(mirType.GetKind() == kTypePointer, "iread must have pointer type");
      MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
      MIRType *vecType = GenVecType(ptrType->GetPointedType()->GetPrimType(), count);
      ASSERT(vecType != nullptr, "vector type should not be null");
      MIRType *pvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType, PTY_ptr);
      // update lhs type
      ireadnode->SetTyIdx(pvecType->GetTypeIndex());
      break;
    }
    // scalar related: widen type directly or unroll instructions
    case OP_dassign:
    case OP_dread:
      ASSERT(0, "NIY");
      break;
    // vector type support in opcode  +, -, *, &, |, <<, >>, compares, ~, !
    case OP_add:
    case OP_sub:
    case OP_mul:
    case OP_band:
    case OP_bior:
    case OP_shl:
    case OP_lshr:
    case OP_ashr:
    // compare
    case OP_eq:
    case OP_ne:
    case OP_lt:
    case OP_gt:
    case OP_le:
    case OP_ge:
    case OP_cmpg:
    case OP_cmpl: {
      ASSERT(node->IsBinaryNode(), "should be binarynode");
      BinaryNode *binNode = static_cast<BinaryNode *>(node);
      MIRType *vecType = GenVecType(node->GetPrimType(), count);
      node->SetPrimType(vecType->GetPrimType()); // update primtype of binary op
      VectorizeNode(binNode->Opnd(0), count);
      VectorizeNode(binNode->Opnd(1), count);
      break;
    }
    // unary op
    case OP_neg:
    case OP_bnot:
    case OP_lnot: {
      ASSERT(node->IsUnaryNode(), "should be unarynode");
      UnaryNode *unaryNode = static_cast<UnaryNode *>(node);
      MIRType *vecType = GenVecType(node->GetPrimType(), count);
      node->SetPrimType(vecType->GetPrimType()); // update primtype of unary op
      VectorizeNode(unaryNode->Opnd(0), count);
      break;
    }
    case OP_constval: {
      LfoPart *lfoP = (*lfoExprParts)[node];
      ASSERT(lfoP != nullptr, "nullptr check");
      // constval could be used in binary op without widen directly
      if (!lfoP->GetParent()->IsBinaryNode()) {
        // use intrinsicop vdupq_n_i32 to move const to tmp variable
        ASSERT(0, "constval need to extended NIY");
      }
      break;
    }
    default:
      ASSERT(0, "can't be vectorized");
  }
}

// update init/stride/upper nodes of doloop
// now hack code to widen const stride with value "vecFactor * original stride"
void LoopVectorization::widenDoloop(DoloopNode *doloop, LoopTransPlan *tp) {
  if (tp->vBound) {
    if (tp->vBound->incrNode) {
      doloop->SetIncrExpr(tp->vBound->incrNode);
    }
    if (tp->vBound->lowNode) {
      doloop->SetStartExpr(tp->vBound->lowNode);
    }
    if (tp->vBound->upperNode) {
      BinaryNode *cmpn = static_cast<BinaryNode*>(doloop->GetCondExpr());
      cmpn->SetBOpnd(tp->vBound->upperNode, 1);
    }
  }
}

void LoopVectorization::VectorizeDoLoop(DoloopNode *doloop, LoopTransPlan *tp) {
  // LogInfo::MapleLogger() << "\n**** dump doloopnode ****\n";
  // doloop->Dump(0);
  // step 1: handle loop low/upper/stride
  widenDoloop(doloop, tp);

  // step 2: widen vectorizable stmt in doloop
  BlockNode *loopbody = doloop->GetDoBody();
  for (auto &stmt : loopbody->GetStmtNodes()) {
    //if (stmt need to be vectoried in vectorized list) {
    VectorizeNode(&stmt, tp->vecFactor);
    //} else {
     // stmt could not be widen directly, unroll instruction with vecFactor
     // move value from vector type if need (need def-use information from plan)
     //}
  }
}

// generate remainder loop
DoloopNode *LoopVectorization::GenEpilog(DoloopNode *doloop) {
  // new doloopnode
  // copy doloop body
  // insert newdoloopnode after doloop
  return doloop;
}

// generate prolog/epilog blocknode if needed
// return doloop need to be vectorized
DoloopNode *LoopVectorization::PrepareDoloop(DoloopNode *doloop, LoopTransPlan *tp) {
  bool needPeel = false;
  // generate peel code if need
  if (needPeel) {
    // peel code here
    // udpate loop lower of doloop if need
    // copy loop body
  }
  // generate epilog
  if (tp->eBound) {
    // copy doloop
    DoloopNode *edoloop = doloop->CloneTree(*codeMPAlloc);
    ASSERT(tp->eBound->lowNode, "nullptr check");
    // update loop low bound
    edoloop->SetStartExpr(tp->eBound->lowNode);
    // add epilog after doloop
    LfoPart *lfoInfo = (*lfoStmtParts)[doloop->GetStmtID()];
    ASSERT(lfoInfo, "nullptr check");
    BaseNode *parent = lfoInfo->GetParent();
    ASSERT(parent && (parent->GetOpCode() ==  OP_block), "nullptr check");
    BlockNode *pblock = static_cast<BlockNode *>(parent);
    pblock->InsertAfter(doloop, edoloop);
  }
  return doloop;
}

void LoopVectorization::TransformLoop() {
  auto it = vecPlans.begin();
  for (; it != vecPlans.end(); it++) {
    // generate prilog/epilog according to vectorization plan
    DoloopNode *donode = it->first;
    LoopTransPlan *tplan = it->second;
    DoloopNode *vecDoloopNode = PrepareDoloop(donode, tplan);
    VectorizeDoLoop(vecDoloopNode, tplan);
  }
}

bool LoopVectorization::ExprVectorizable(DoloopInfo *doloopInfo, BaseNode *x) {
  if (!IsPrimitiveInteger(x->GetPrimType())) {
    return false;
  }
  switch (x->GetOpCode()) {
    // supported leaf ops
    case OP_constval:
    case OP_dread:
    case OP_addrof: {
      LfoPart* lfopart = (*lfoExprParts)[x];
      CHECK_FATAL(lfopart, "nullptr check");
      BaseNode *parent = lfopart->GetParent();
      if (parent && parent->GetOpCode() == OP_array) {
        return true;
      }
      return false; // TODO::NIY
    }
    // supported binary ops
    case OP_add:
    case OP_sub:
    case OP_mul:
    case OP_band:
    case OP_bior:
    case OP_shl:
    case OP_lshr:
    case OP_ashr:
    case OP_eq:
    case OP_ne:
    case OP_lt:
    case OP_gt:
    case OP_le:
    case OP_ge:
    case OP_cmpg:
    case OP_cmpl:
      return ExprVectorizable(doloopInfo, x->Opnd(0)) && ExprVectorizable(doloopInfo, x->Opnd(1));
    // supported unary ops
    case OP_bnot:
    case OP_lnot:
    case OP_neg:
      return ExprVectorizable(doloopInfo, x->Opnd(0));
    case OP_iread: {
      bool canVec = ExprVectorizable(doloopInfo, x->Opnd(0));
      if (canVec) {
        IreadNode *iread = static_cast<IreadNode *>(x);
        if (iread->GetFieldID() != 0 && iread->Opnd(0)->GetOpCode() == OP_array) {
          MeExpr *meExpr = depInfo->preEmit->GetLfoExprPart(iread->Opnd(0))->GetMeExpr();
          canVec = doloopInfo->IsLoopInvariant(meExpr);
        }
      }
      return canVec;
    }
    // supported n-ary ops
    case OP_array: {
      for (size_t i = 0; i < x->NumOpnds(); i++) {
        if (!ExprVectorizable(doloopInfo, x->Opnd(i))) {
          return false;
        }
      }
      return true;
    }
    default: ;
  }
  return false;
}

// assumed to be inside innermost loop
bool LoopVectorization::Vectorizable(DoloopInfo *doloopInfo, BlockNode *block, LoopVecInfo* vecInfo) {
  StmtNode *stmt = block->GetFirst();
  while (stmt != nullptr) {
    switch (stmt->GetOpCode()) {
      case OP_doloop:
      case OP_dowhile:
      case OP_while: {
        CHECK_FATAL(false, "Vectorizable: cannot handle non-innermost loop");
        break;
      }
      case OP_block:
        return Vectorizable(doloopInfo, static_cast<DoloopNode *>(stmt)->GetDoBody(), vecInfo);
      case OP_iassign: {
        IassignNode *iassign = static_cast<IassignNode *>(stmt);
        bool canVec = ExprVectorizable(doloopInfo, iassign->GetRHS());
        if (canVec && iassign->GetFieldID() != 0) {  // check base of iassign
          MeExpr *meExpr = depInfo->preEmit->GetLfoExprPart(iassign->Opnd(0))->GetMeExpr();
          canVec = doloopInfo->IsLoopInvariant(meExpr);
        }
        if (canVec) {
          MIRType &mirType = GetTypeFromTyIdx(iassign->GetTyIdx());
          CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
          MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
          PrimType stmtpt = ptrType->GetPointedType()->GetPrimType();
          CHECK_FATAL(IsPrimitiveInteger(stmtpt) && (!IsPrimitivePoint(stmtpt)), "iassign ptr type should be integer now");
          vecInfo->UpdatePrimType(stmtpt);
          vecInfo->vecStmtIDs.insert((stmt)->GetStmtID());
        }
        return canVec;
      }
      default: return false;
    }
    stmt = stmt->GetNext();
  }
  return false;
}

void LoopVectorization::Perform() {
  // step 2: collect information, legality check and generate transform plan
  MapleMap<DoloopNode *, DoloopInfo *>::iterator mapit = depInfo->doloopInfoMap.begin();
  for (; mapit != depInfo->doloopInfoMap.end(); mapit++) {
    if (!mapit->second->children.empty() || !mapit->second->Parallelizable()) {
      continue;
    }
    LoopVecInfo *vecInfo = localMP->New<LoopVecInfo>(localAlloc);
    bool vectorizable = Vectorizable(mapit->second, mapit->first->GetDoBody(), vecInfo);
    if (enableDebug) {
      LogInfo::MapleLogger() << "\nInnermost Doloop:";
      if (!vectorizable) {
        LogInfo::MapleLogger() << " NOT";
      }
      LogInfo::MapleLogger() << " VECTORIZABLE\n";
      mapit->first->Dump(0);
    }
    if (!vectorizable) {
      continue;
    }
    // generate vectorize plan;
    LoopTransPlan *tplan = localMP->New<LoopTransPlan>(codeMP, localMP, vecInfo);
    tplan->Generate(mapit->first, mapit->second);
    vecPlans[mapit->first] = tplan;
  }
  // step 3: do transform
  // transform plan map to each doloop
  TransformLoop();
}

AnalysisResult *DoLfoLoopVectorization::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*) {
  // generate lfo IR
  LfoPreEmitter *lfoemit = static_cast<LfoPreEmitter *>(m->GetAnalysisResult(MeFuncPhase_LFOPREEMIT, func));
  CHECK_NULL_FATAL(lfoemit);
  // step 1: get dependence graph for each loop
  auto *lfodepInfo = static_cast<LfoDepInfo *>(m->GetAnalysisResult(MeFuncPhase_LFODEPTEST, func));
  CHECK_NULL_FATAL(lfodepInfo);

  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "\n**** Before loop vectorization phase ****\n";
    func->GetMirFunc()->Dump(false);
  }

  // run loop vectorization
  LoopVectorization loopVec(NewMemPool(), lfoemit, lfodepInfo, DEBUGFUNC(func));
  loopVec.Perform();

  // invalid analysis result
  m->InvalidAllResults();

  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "\n\n\n**** After loop vectorization phase ****\n";
    func->GetMirFunc()->Dump(false);
  }

  // lower lfoIR for other mapleme phases
  MIRLower mirlowerer(func->GetMIRModule(), func->GetMirFunc());
  mirlowerer.SetLowerME();
  mirlowerer.SetLowerExpandArray();
  mirlowerer.LowerFunc(*(func->GetMirFunc()));

  return nullptr;
}
}  // namespace maple
