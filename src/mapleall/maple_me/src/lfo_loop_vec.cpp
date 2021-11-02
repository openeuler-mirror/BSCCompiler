/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
uint32_t LoopVectorization::vectorizedLoop = 0;

void LoopVecInfo::UpdateWidestTypeSize(uint32_t newtypesize) {
  if (largestTypeSize < newtypesize) {
    largestTypeSize = newtypesize;
  }
}

bool LoopVecInfo::UpdateRHSTypeSize(PrimType ptype) {
  uint32_t newSize = GetPrimTypeSize(ptype) * 8;
  if (currentRHSTypeSize == 0) {
    currentRHSTypeSize = newSize;
    return true;
  } else if (newSize > currentRHSTypeSize) {
    currentRHSTypeSize = newSize;
    return false; // skip vectorize now since type is not consistent
  } else if (newSize < currentRHSTypeSize) {
    return false;
  }
  return true;
}

// generate new bound for vectorization loop and epilog loop
// original bound info <initnode, uppernode, incrnode>, condNode doesn't include equal
// limitation now:  initNode and incrNode are const and initnode is vectorLane aligned.
// vectorization loop: <initnode, (uppernode-initnode)/vectorFactor * vectorFactor + initnode, incrNode*vectFact>
// epilog loop: < (uppernode-initnode)/vectorFactor*vectorFactor+initnode, uppernode, incrnode>
void LoopTransPlan::GenerateBoundInfo(DoloopNode *doloop, DoloopInfo *li) {
  (void) li;
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
  ConstvalNode *icn = static_cast<ConstvalNode *>(incrNode);
  MIRIntConst *incrConst = static_cast<MIRIntConst *>(icn->GetConstVal());
  ASSERT(condNode->IsBinaryNode(), "cmp node should be binary node");
  BaseNode *upNode = condNode->Opnd(1);

  MIRIntConst *newIncr = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
      vecFactor * incrConst->GetValue(), *typeInt);
  ConstvalNode *newIncrNode = codeMP->New<ConstvalNode>(PTY_i32, newIncr);
  if (initNode->IsConstval()) {
    ConstvalNode *lcn = static_cast<ConstvalNode *>(initNode);
    MIRIntConst *lowConst = static_cast<MIRIntConst *>(lcn->GetConstVal());
    int64 lowvalue = lowConst->GetValue();
    // upNode is constant
    if (upNode->IsConstval()) {
      ConstvalNode *ucn = static_cast<ConstvalNode *>(upNode);
      MIRIntConst *upConst = static_cast<MIRIntConst *>(ucn->GetConstVal());
      int64 upvalue = upConst->GetValue();
      if (condOpHasEqual) {
        upvalue += 1;
      }
      if (((upvalue - lowvalue) % (newIncr->GetValue())) == 0) {
        // early return, change vbound->stride only
        vBound = localMP->New<LoopBound>(nullptr, nullptr, newIncrNode);
      } else {
        // trip count is not vector lane aligned
        int32_t newupval = (upvalue - lowvalue) / (newIncr->GetValue()) * (newIncr->GetValue()) + lowvalue;
        MIRIntConst *newUpConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(newupval, *typeInt);
        ConstvalNode *newUpNode = codeMP->New<ConstvalNode>(PTY_i32, newUpConst);
        vBound = localMP->New<LoopBound>(nullptr, newUpNode, newIncrNode);
        // generate epilog
        eBound = localMP->New<LoopBound>(newUpNode, nullptr, nullptr);
      }
    } else if (upNode->GetOpCode() == OP_dread || upNode->GetOpCode() == OP_regread) {
      // step 1: generate vectorized loop bound
      // upNode of vBound is (uppnode - initnode) / newIncr * newIncr + initnode
      BinaryNode *divnode;
      BaseNode *addnode = upNode;
      if (condOpHasEqual) {
        addnode = codeMP->New<BinaryNode>(OP_add, PTY_i32, upNode, constOnenode);
      }
      if (lowvalue != 0) {
        BinaryNode *subnode = codeMP->New<BinaryNode>(OP_sub, PTY_i32, addnode, initNode);
        divnode = codeMP->New<BinaryNode>(OP_div, PTY_i32, subnode, newIncrNode);
      } else {
        divnode = codeMP->New<BinaryNode>(OP_div, PTY_i32, addnode, newIncrNode);
      }
      BinaryNode *mulnode = codeMP->New<BinaryNode>(OP_mul, PTY_i32, divnode, newIncrNode);
      addnode = codeMP->New<BinaryNode>(OP_add, PTY_i32, mulnode, initNode);
      vBound = localMP->New<LoopBound>(nullptr, addnode, newIncrNode);
      // step2:  generate epilog bound
      eBound = localMP->New<LoopBound>(addnode, nullptr, nullptr);
    } else {
      ASSERT(0, "upper bound is complex, NIY");
    }
  } else if (initNode->GetOpCode() == OP_dread || initNode->GetOpCode() == OP_regread) {
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
    BinaryNode *addnode = codeMP->New<BinaryNode>(OP_add, PTY_i32, mulnode, initNode);
    vBound = localMP->New<LoopBound>(nullptr, addnode, newIncrNode);
    // set bound of epilog loop
    eBound = localMP->New<LoopBound>(addnode, nullptr, nullptr);
  } else {
    ASSERT(0, "low bound is complex, NIY");
  }
}

// generate best plan for current doloop
void LoopTransPlan::Generate(DoloopNode *doloop, DoloopInfo* li) {
  // vector length / type size
  vecLanes = 128 / (vecInfo->largestTypeSize);
  vecFactor = vecLanes;
  // generate bound information
  GenerateBoundInfo(doloop, li);
}

MIRType* LoopVectorization::GenVecType(PrimType sPrimType, uint8 lanes) {
  MIRType *vecType = nullptr;
  CHECK_FATAL(IsPrimitiveInteger(sPrimType), "primtype should be integer");
  switch (sPrimType) {
    case PTY_i32: {
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
    case PTY_i64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2Int64();
      } else {
        ASSERT(0, "unsupported int64 vector lanes");
      }
      break;
    }
    case PTY_u64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2UInt64();
      } else {
        ASSERT(0, "unsupported uint64 vector lanes");
      }
      break;
    }
    case PTY_a64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2UInt64();
      } else {
        ASSERT(0, "unsupported a64 vector lanes");
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
          ASSERT(0, "unsupported ptr vector lanes");
        }
      } else if (GetPrimTypeSize(sPrimType) == 8) {
        if (lanes == 2) {
          vecType = GlobalTables::GetTypeTable().GetV2UInt64();
        } else {
          ASSERT(0, "unsupported ptr vector lanes");
        }
      }
      break;
    }
    default:
      ASSERT(0, "NIY");
  }
  return vecType;
}

// generate instrinsic node to sum all elements of a vector type
IntrinsicopNode *LoopVectorization::GenSumVecStmt(BaseNode *vecTemp, PrimType vecPrimType) {
  MIRIntrinsicID intrnID = INTRN_vector_sum_v4i32;
  MIRType *retType = nullptr;
  switch (vecPrimType) {
    case PTY_v4i32: {
      intrnID = INTRN_vector_sum_v4i32;
      retType = GlobalTables::GetTypeTable().GetInt32();
      break;
    }
    case PTY_v2i32: {
      intrnID = INTRN_vector_sum_v2i32;
      retType = GlobalTables::GetTypeTable().GetInt32();
      break;
    }
    case PTY_v4u32: {
      intrnID = INTRN_vector_sum_v4u32;
      retType = GlobalTables::GetTypeTable().GetUInt32();
      break;
    }
    case PTY_v2u32: {
      intrnID = INTRN_vector_sum_v2u32;
      retType = GlobalTables::GetTypeTable().GetUInt32();
      break;
    }
    case PTY_v2i64: {
      intrnID = INTRN_vector_sum_v2i64;
      retType = GlobalTables::GetTypeTable().GetInt64();
      break;
    }
    case PTY_v2u64: {
      intrnID = INTRN_vector_sum_v2u64;
      retType = GlobalTables::GetTypeTable().GetUInt64();
      break;
    }
    case PTY_v8i16: {
      intrnID = INTRN_vector_sum_v8i16;
      retType = GlobalTables::GetTypeTable().GetInt16();
      break;
    }
    case PTY_v8u16: {
      intrnID = INTRN_vector_sum_v8u16;
      retType = GlobalTables::GetTypeTable().GetUInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = INTRN_vector_sum_v4i16;
      retType = GlobalTables::GetTypeTable().GetInt16();
      break;
    }
    case PTY_v4u16: {
      intrnID = INTRN_vector_sum_v4u16;
      retType = GlobalTables::GetTypeTable().GetUInt16();
      break;
    }
    case PTY_v16i8: {
      intrnID = INTRN_vector_sum_v16i8;
      retType = GlobalTables::GetTypeTable().GetInt8();
      break;
    }
    case PTY_v16u8: {
      intrnID = INTRN_vector_sum_v16u8;
      retType = GlobalTables::GetTypeTable().GetUInt8();
      break;
    }
    case PTY_v8i8: {
      intrnID = INTRN_vector_sum_v8i8;
      retType = GlobalTables::GetTypeTable().GetInt8();
      break;
    }
    case PTY_v8u8: {
      intrnID = INTRN_vector_sum_v8u8;
      retType = GlobalTables::GetTypeTable().GetUInt8();
      break;
    }
    default:
      ASSERT(0, "NIY");
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, retType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(1);
  rhs->GetNopnd().push_back(vecTemp);
  rhs->SetTyIdx(retType->GetTypeIndex());
  return rhs;
}

// check opcode is reduction, +/-/*///min/max
bool LoopVectorization::IsReductionOp(Opcode op) {
  if (op == OP_add || op == OP_sub) return true;
  return false;
}

// generate instrinsic node to copy scalar to vector type
IntrinsicopNode *LoopVectorization::GenDupScalarExpr(BaseNode *scalar, PrimType vecPrimType) {
  MIRIntrinsicID intrnID = INTRN_vector_from_scalar_v4i32;
  MIRType *vecType = nullptr;
  switch (vecPrimType) {
    case PTY_v4i32: {
      intrnID = INTRN_vector_from_scalar_v4i32;
      vecType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v2i32: {
      intrnID = INTRN_vector_from_scalar_v2i32;
      vecType = GlobalTables::GetTypeTable().GetV2Int32();
      break;
    }
    case PTY_v4u32: {
      intrnID = INTRN_vector_from_scalar_v4u32;
      vecType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2u32: {
      intrnID = INTRN_vector_from_scalar_v2u32;
      vecType = GlobalTables::GetTypeTable().GetV2UInt32();
      break;
    }
    case PTY_v8i16: {
      intrnID = INTRN_vector_from_scalar_v8i16;
      vecType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u16: {
      intrnID = INTRN_vector_from_scalar_v8u16;
      vecType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = INTRN_vector_from_scalar_v4i16;
      vecType = GlobalTables::GetTypeTable().GetV4Int16();
      break;
    }
    case PTY_v4u16: {
      intrnID = INTRN_vector_from_scalar_v4u16;
      vecType = GlobalTables::GetTypeTable().GetV4UInt16();
      break;
    }
    case PTY_v16i8: {
      intrnID = INTRN_vector_from_scalar_v16i8;
      vecType = GlobalTables::GetTypeTable().GetV16Int8();
      break;
    }
    case PTY_v16u8: {
      intrnID = INTRN_vector_from_scalar_v16u8;
      vecType = GlobalTables::GetTypeTable().GetV16UInt8();
      break;
    }
    case PTY_v8i8: {
      intrnID = INTRN_vector_from_scalar_v8i8;
      vecType = GlobalTables::GetTypeTable().GetV8Int8();
      break;
    }
    case PTY_v8u8: {
      intrnID = INTRN_vector_from_scalar_v8u8;
      vecType = GlobalTables::GetTypeTable().GetV8UInt8();
      break;
    }
    case PTY_v2i64: {
      intrnID = INTRN_vector_from_scalar_v2i64;
      vecType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v2u64: {
      intrnID = INTRN_vector_from_scalar_v2u64;
      vecType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      ASSERT(0, "NIY");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, vecPrimType);
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(1);
  rhs->GetNopnd().push_back(scalar);
  rhs->SetTyIdx(vecType->GetTypeIndex());
  return rhs;
}

// iterate tree node to wide scalar type to vector type
// following opcode can be vectorized directly
//  +, -, *, &, |, <<, >>, compares, ~, !
// iassign, iread, dassign, dread
void LoopVectorization::VectorizeNode(BaseNode *node, LoopTransPlan *tp) {
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
      MIRType *vecType = GenVecType(ptrType->GetPointedType()->GetPrimType(), tp->vecFactor);
      ASSERT(vecType != nullptr, "vector type should not be null");
      MIRType *pvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType, PTY_ptr);
      // update lhs type
      iassign->SetTyIdx(pvecType->GetTypeIndex());
      // visit rsh
      BaseNode *rhs = iassign->GetRHS();
      if (tp->vecInfo->uniformVecNodes.find(rhs) != tp->vecInfo->uniformVecNodes.end()) {
        // rhs replaced scalar node with vector node
        iassign->SetRHS(tp->vecInfo->uniformVecNodes[rhs]);
      } else {
        VectorizeNode(iassign->GetRHS(), tp);
        // insert CVT if lsh type is not same as rhs type
        if (vecType->GetPrimType() !=  rhs->GetPrimType()) {
          BaseNode *newrhs = codeMP->New<TypeCvtNode>(OP_cvt, vecType->GetPrimType(), rhs->GetPrimType(), rhs);
          iassign->SetRHS(newrhs);
        }
      }
      break;
    }
    case OP_iread: {
      IreadNode *ireadnode = static_cast<IreadNode *>(node);
      // update tyidx
      MIRType &mirType = GetTypeFromTyIdx(ireadnode->GetTyIdx());
      CHECK_FATAL(mirType.GetKind() == kTypePointer, "iread must have pointer type");
      MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
      MIRType *vecType = nullptr;
      // update lhs type
      if (ptrType->GetPointedType()->GetPrimType() == PTY_agg) {
        // iread variable from a struct, use iread type
        vecType = GenVecType(ireadnode->GetPrimType(), tp->vecFactor);
        ASSERT(vecType != nullptr, "vector type should not be null");
      } else {
        vecType = GenVecType(ptrType->GetPointedType()->GetPrimType(), tp->vecFactor);
        ASSERT(vecType != nullptr, "vector type should not be null");
        MIRType *pvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType, PTY_ptr);
        ireadnode->SetTyIdx(pvecType->GetTypeIndex());
      }
      node->SetPrimType(vecType->GetPrimType());
      break;
    }
    // scalar related: widen type directly or unroll instructions
    case OP_dassign: {
      // now only support reduction scalar
      // sum = sum +/- vectorizable_expr
      // =>
      // Example: vec t1 = dup_scalar(sum);
      // Example: doloop {
      // Example:   t1 = t1 + vectorized_node;
      // Example: }
      //  sum = sum +/- intrinsic_op vec_sum(t1)
      // Example:sum = intrinsic_op vec_sum(vectorized_node);
      DassignNode *dassign = static_cast<DassignNode *>(node);
      StIdx lhsStIdx = dassign->GetStIdx();
      MIRSymbol *lhsSym = mirFunc->GetLocalOrGlobalSymbol(lhsStIdx);
      MIRType &lhsType = GetTypeFromTyIdx(lhsSym->GetTyIdx());
      MIRType *vecType = GenVecType(lhsType.GetPrimType(), tp->vecFactor);
      ASSERT(vecType != nullptr, "vector type should not be null");
      BaseNode *vecNode = dassign->GetRHS()->Opnd(1);
      // skip vecNode is uniform
      if (tp->vecInfo->uniformNodes.find(vecNode) == tp->vecInfo->uniformNodes.end()) {
        VectorizeNode(vecNode, tp);
        if (vecType->GetPrimType() != vecNode->GetPrimType()) {
          BaseNode *newVecNode = codeMP->New<TypeCvtNode>(OP_cvt, vecType->GetPrimType(), vecNode->GetPrimType(), vecNode);
          dassign->GetRHS()->SetOpnd(newVecNode, 1);
        }
      }
      BaseNode *redNewVar = tp->vecInfo->redVecNodes[lhsStIdx];
      ASSERT((redNewVar != nullptr && redNewVar->GetOpCode() == OP_dread), "nullptr check");
      StIdx vecStIdx = (static_cast<AddrofNode *>(redNewVar))->GetStIdx();
      dassign->SetStIdx(vecStIdx);
      dassign->SetPrimType(vecType->GetPrimType());
      dassign->GetRHS()->SetOpnd(redNewVar, 0);
      dassign->GetRHS()->SetPrimType(vecType->GetPrimType());
      break;
    }
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
      for (int32_t i = 0; i < 2; i++) {
        if (tp->vecInfo->uniformVecNodes.find(binNode->Opnd(i)) != tp->vecInfo->uniformVecNodes.end()) {
          BaseNode *newnode = tp->vecInfo->uniformVecNodes[binNode->Opnd(i)];
          binNode->SetOpnd(newnode, i);
        } else {
          VectorizeNode(binNode->Opnd(i), tp);
        }
      }
      node->SetPrimType(binNode->Opnd(0)->GetPrimType()); // update primtype of binary op with opnd's type
      break;
    }
    // unary op
    case OP_abs:
    case OP_neg:
    case OP_bnot:
    case OP_lnot: {
      ASSERT(node->IsUnaryNode(), "should be unarynode");
      UnaryNode *unaryNode = static_cast<UnaryNode *>(node);
      if (tp->vecInfo->uniformVecNodes.find(unaryNode->Opnd(0)) != tp->vecInfo->uniformVecNodes.end()) {
        BaseNode *newnode = tp->vecInfo->uniformVecNodes[unaryNode->Opnd(0)];
        unaryNode->SetOpnd(newnode, 0);
      } else {
        VectorizeNode(unaryNode->Opnd(0), tp);
      }
      node->SetPrimType(unaryNode->Opnd(0)->GetPrimType()); // update primtype of unary op with opnd's type
      break;
    }
    case OP_dread:
    case OP_constval: {
      // donothing
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
      BinaryNode *cmpn = static_cast<BinaryNode *>(doloop->GetCondExpr());
      cmpn->SetBOpnd(tp->vBound->upperNode, 1);
    }
  }
}


void LoopVectorization::VectorizeDoLoop(DoloopNode *doloop, LoopTransPlan *tp) {
  // LogInfo::MapleLogger() << "\n**** dump doloopnode ****\n";
  // step 1: handle loop low/upper/stride
  widenDoloop(doloop, tp);

  // step 2: insert dup stmt before doloop
  if (!tp->vecInfo->uniformNodes.empty()) {
    LfoPart* lfopart = (*lfoStmtParts)[doloop->GetStmtID()];
    BaseNode *parent = lfopart->GetParent();
    ASSERT(parent && (parent->GetOpCode() == OP_block), "nullptr check");
    BlockNode *pblock = static_cast<BlockNode *>(parent);
    auto it = tp->vecInfo->uniformNodes.begin();
    for (; it != tp->vecInfo->uniformNodes.end(); it++) {
      BaseNode *node = *it;
      LfoPart *lfoP = (*lfoExprParts)[node];
      // check node's parent, if they are binary node, skip the duplication
      if ((!lfoP->GetParent()->IsBinaryNode()) || (node->GetOpCode() == OP_iread)) {
        PrimType ptype = node->GetPrimType();
        if (tp->vecInfo->constvalTypes.count(node) > 0) {
          ptype = tp->vecInfo->constvalTypes[node];
        }
        MIRType *vecType = GenVecType(ptype, tp->vecFactor);
        IntrinsicopNode *dupscalar = GenDupScalarExpr(node, vecType->GetPrimType());
        PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(vecType->GetPrimType());
        RegassignNode *dupScalarStmt = codeMP->New<RegassignNode>(vecType->GetPrimType(), regIdx, dupscalar);
        pblock->InsertBefore(doloop, dupScalarStmt);
        RegreadNode *regreadNode = codeMP->New<RegreadNode>(vecType->GetPrimType(), regIdx);
        tp->vecInfo->uniformVecNodes[node] = regreadNode;
      }
    }
  }
  // step 2.2 reduction variable
  if (!tp->vecInfo->reductionVars.empty()) {
    LfoPart* lfopart = (*lfoStmtParts)[doloop->GetStmtID()];
    BaseNode *parent = lfopart->GetParent();
    ASSERT(parent && (parent->GetOpCode() == OP_block), "nullptr check");
    BlockNode *pblock = static_cast<BlockNode *>(parent);
    auto it = tp->vecInfo->reductionVars.begin();
    int count = 0;
    MIRType &typeInt = *GlobalTables::GetTypeTable().GetPrimType(PTY_i32);
    MIRIntConst *constZero = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, typeInt);
    ConstvalNode *zeroNode = codeMP->New<ConstvalNode>(PTY_i32, constZero);
    for (; it != tp->vecInfo->reductionVars.end(); it++) {
      StIdx stIdx = it->first;
      MIRSymbol *redSym = mirFunc->GetLocalOrGlobalSymbol(stIdx);
      PrimType ptype = GetTypeFromTyIdx(redSym->GetTyIdx()).GetPrimType();
      MIRType *vecType = GenVecType(ptype, tp->vecFactor);
      // before loop: vec = dup_scalar(0)
      IntrinsicopNode *dupscalar = GenDupScalarExpr(zeroNode, vecType->GetPrimType());
      // new stidx
      std::string redName("red");
      redName.append(std::to_string(doloop->GetStmtID()));
      redName.append("_");
      redName.append(std::to_string(count++));
      GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(redName);
      MIRSymbol *st = mirFunc->GetModule()->GetMIRBuilder()->CreateSymbol(vecType->GetTypeIndex(), strIdx, kStVar, kScAuto, mirFunc, kScopeLocal);
      DassignNode *redInitStmt = codeMP->New<DassignNode>(ptype, dupscalar, st->GetStIdx(), 0);
      pblock->InsertBefore(doloop, redInitStmt);
      AddrofNode *dreadNode = codeMP->New<AddrofNode>(OP_dread, vecType->GetPrimType(), st->GetStIdx(), 0);
      tp->vecInfo->redVecNodes[stIdx] = dreadNode;
      // after loop:  reduction +/-= vec_sum(vec)
      AddrofNode *redScalarNode = codeMP->New<AddrofNode>(OP_dread, ptype, stIdx, 0);
      IntrinsicopNode *intrnNode = GenSumVecStmt(dreadNode, vecType->GetPrimType());
      // reduction op is +/- now, use OP_add to sum all elements
      BinaryNode *binaryNode = codeMP->New<BinaryNode>(OP_add, ptype, redScalarNode, intrnNode);
      DassignNode *redDassign = codeMP->New<DassignNode>(ptype, binaryNode, stIdx, 0);
      pblock->InsertAfter(doloop, redDassign);
    }
  }
  // step 3: widen vectorizable stmt in doloop
  BlockNode *loopbody = doloop->GetDoBody();
  for (auto &stmt : loopbody->GetStmtNodes()) {
    if (tp->vecInfo->vecStmtIDs.count(stmt.GetStmtID()) > 0) {
      VectorizeNode(&stmt, tp);
    } else {
      // stmt could not be widen directly, unroll instruction with vecFactor
      // move value from vector type if need (need def-use information from plan)
      CHECK_FATAL(0, "NIY:: unvectorized stmt");
    }
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

bool LoopVectorization::ExprVectorizable(DoloopInfo *doloopInfo, LoopVecInfo* vecInfo, BaseNode *x) {
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
      if (x->GetOpCode() == OP_constval) {
        vecInfo->uniformNodes.insert(x);
        return true;
      }
      MeExpr *expr = lfopart->GetMeExpr();
      if (expr && doloopInfo->IsLoopInvariant(expr)) {
        // no vectorize if rhs operands types are not same
        if (!vecInfo->UpdateRHSTypeSize(x->GetPrimType())) {
          return false;
        }
        vecInfo->uniformNodes.insert(x);
        return true;
      }
      return false;
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
    case OP_cmpl: {
      // check two operands are constant
      MeExpr *r0MeExpr = (*lfoExprParts)[x->Opnd(0)]->GetMeExpr();
      MeExpr *r1MeExpr = (*lfoExprParts)[x->Opnd(1)]->GetMeExpr();
      bool r0Uniform = doloopInfo->IsLoopInvariant(r0MeExpr);
      bool r1Uniform = doloopInfo->IsLoopInvariant(r1MeExpr);
      if (r0Uniform && r1Uniform) {
        vecInfo->uniformNodes.insert(x->Opnd(0));
        vecInfo->uniformNodes.insert(x->Opnd(1));
        return true;
      } else if (r0Uniform) {
        vecInfo->uniformNodes.insert(x->Opnd(0));
        return ExprVectorizable(doloopInfo, vecInfo, x->Opnd(1));
      } else if (r1Uniform) {
        vecInfo->uniformNodes.insert(x->Opnd(1));
        return ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0));
      }
      return ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0)) && ExprVectorizable(doloopInfo, vecInfo, x->Opnd(1));
    }
    // supported unary ops
    case OP_bnot:
    case OP_lnot:
    case OP_neg:
    case OP_abs: {
      MeExpr *r0MeExpr = (*lfoExprParts)[x->Opnd(0)]->GetMeExpr();
      bool r0Uniform = doloopInfo->IsLoopInvariant(r0MeExpr);
      if (r0Uniform) {
        vecInfo->uniformNodes.insert(x->Opnd(0));
        return true;
      }
      return ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0));
    }
    case OP_iread: {
      bool canVec = ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0));
      if (canVec) {
        IreadNode* ireadnode = static_cast<IreadNode *>(x);
        MIRType &mirType = GetTypeFromTyIdx(ireadnode->GetTyIdx());
        CHECK_FATAL(mirType.GetKind() == kTypePointer, "iread must have pointer type");
        MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
        if (!vecInfo->UpdateRHSTypeSize(ptrType->GetPointedType()->GetPrimType())) {
          canVec = false; // skip if rhs type is not consistent
        } else {
          IreadNode *iread = static_cast<IreadNode *>(x);
          if ((iread->GetFieldID() != 0 || MustBeAddress(iread->GetPrimType())) &&
              iread->Opnd(0)->GetOpCode() == OP_array) {
            MeExpr *meExpr = depInfo->preEmit->GetLfoExprPart(iread->Opnd(0))->GetMeExpr();
            canVec = doloopInfo->IsLoopInvariant(meExpr);
          }
        }
      }
      return canVec;
    }
    // supported n-ary ops
    case OP_array: {
      for (size_t i = 0; i < x->NumOpnds(); i++) {
        if (!ExprVectorizable(doloopInfo, vecInfo, x->Opnd(i))) {
          return false;
        }
      }
      return true;
    }
    default: ;
  }
  return false;
}

// only handle one level convert
// <short, int> or <int, short> pairs
bool LoopVectorization::CanConvert(uint32_t lshtypeSize, uint32_t rhstypeSize) {
  if (lshtypeSize >= rhstypeSize) {
    return ((lshtypeSize / rhstypeSize) <= 2);
  }
  return ((rhstypeSize / lshtypeSize) <= 2);
}

bool LoopVectorization::CanAdjustRhsType(PrimType targetType, ConstvalNode *rhs) {
  MIRIntConst *intConst = static_cast<MIRIntConst*>(rhs->GetConstVal());
  int64 v = intConst->GetValue();
  bool res = false;
  switch (targetType) {
    case PTY_i32: {
      res = (v >= INT_MIN && v <= INT_MAX);
      break;
    }
    case PTY_u32: {
      res = (v >= 0 && v <= UINT_MAX);
      break;
    }
    case PTY_i16: {
      res = (v >= SHRT_MIN && v <= SHRT_MAX);
      break;
    }
    case PTY_u16: {
      res = (v >= 0 && v <=  USHRT_MAX);
      break;
    }
    case PTY_i8: {
      res = (v >= SCHAR_MIN && v <= SCHAR_MAX);
      break;
    }
    case PTY_u8: {
      res = (v >= 0 && v <= UCHAR_MAX);
      break;
    }
    case PTY_i64:
    case PTY_u64: {
      res = true;
      break;
    }
    default: {
      break;
    }
  }
  return res;
}

// assumed to be inside innermost loop
bool LoopVectorization::Vectorizable(DoloopInfo *doloopInfo, LoopVecInfo* vecInfo, BlockNode *block) {
  StmtNode *stmt = block->GetFirst();
  while (stmt != nullptr) {
    // reset vecInfo
    vecInfo->ResetStmtRHSTypeSize();
    switch (stmt->GetOpCode()) {
      case OP_doloop:
      case OP_dowhile:
      case OP_while: {
        CHECK_FATAL(false, "Vectorizable: cannot handle non-innermost loop");
        break;
      }
      case OP_block:
        return Vectorizable(doloopInfo, vecInfo, static_cast<DoloopNode *>(stmt)->GetDoBody());
      case OP_iassign: {
        IassignNode *iassign = static_cast<IassignNode *>(stmt);
        // no vectorize lsh is complex or constant subscript
        if (iassign->addrExpr->GetOpCode() == OP_array) {
          ArrayNode *lhsArr = static_cast<ArrayNode *>(iassign->addrExpr);
          ArrayAccessDesc *accessDesc = doloopInfo->GetArrayAccessDesc(lhsArr, false /* isRHS */);
          ASSERT(accessDesc != nullptr, "nullptr check");
          size_t dim = lhsArr->NumOpnds() - 1;
          // check innest loop dimension is complex
          // case like a[abs(i-1)] = 1; depth test will report it's parallelize
          // or a[4*i+1] =; address is not continous if coeff > 1
          if (accessDesc->subscriptVec[dim - 1]->tooMessy ||
              accessDesc->subscriptVec[dim - 1]->loopInvariant ||
              accessDesc->subscriptVec[dim - 1]->coeff != 1) {
            return false;
          }
        }
        // check rsh
        bool canVec = ExprVectorizable(doloopInfo, vecInfo, iassign->GetRHS());
        if (canVec) {
          if (iassign->GetFieldID() != 0) {  // check base of iassign
            MeExpr *meExpr = (*lfoExprParts)[iassign->Opnd(0)]->GetMeExpr();
            canVec = doloopInfo->IsLoopInvariant(meExpr);
          }
        }
        if (canVec) {
          MIRType &mirType = GetTypeFromTyIdx(iassign->GetTyIdx());
          CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
          MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
          PrimType stmtpt = ptrType->GetPointedType()->GetPrimType();
          CHECK_FATAL(IsPrimitiveInteger(stmtpt), "iassign ptr type should be integer now");
          // now check lsh type size should be same as rhs typesize
          uint32_t lshtypesize = GetPrimTypeSize(stmtpt) * 8;
          if (iassign->GetRHS()->IsConstval() &&
              (stmtpt != iassign->GetRHS()->GetPrimType()) &&
              CanAdjustRhsType(stmtpt, static_cast<ConstvalNode *>(iassign->GetRHS()))) {
            vecInfo->constvalTypes[iassign->GetRHS()] = stmtpt;
            vecInfo->UpdateRHSTypeSize(stmtpt);
          } else if (iassign->GetRHS()->GetOpCode() != OP_iread) {
            // iread will update in exprvectorizable with its pointto type
            vecInfo->UpdateRHSTypeSize(iassign->GetRHS()->GetPrimType());
          }
          if (!CanConvert(lshtypesize, vecInfo->currentRHSTypeSize)) {
             // use one cvt could handle the type difference
            return false; // need cvt instruction
          }
          // rsh is loop invariant
          MeExpr *meExpr = (*lfoExprParts)[iassign->GetRHS()]->GetMeExpr();
          if (meExpr && doloopInfo->IsLoopInvariant(meExpr)) {
            vecInfo->uniformNodes.insert(iassign->GetRHS());
          }
          vecInfo->vecStmtIDs.insert((stmt)->GetStmtID());
          // update largest type size
          uint32_t maxSize = vecInfo->currentRHSTypeSize > lshtypesize ?
              vecInfo->currentRHSTypeSize : lshtypesize;
          vecInfo->UpdateWidestTypeSize(maxSize);
        } else {
          // early return
          return false;
        }
        break;
      }
      case OP_dassign: {
        DassignNode *dassign = static_cast<DassignNode *>(stmt);
        StIdx lhsStIdx = dassign->GetStIdx();
        MIRSymbol *lhsSym = mirFunc->GetLocalOrGlobalSymbol(lhsStIdx);
        MIRType &lhsType = GetTypeFromTyIdx(lhsSym->GetTyIdx());
        BaseNode *rhs = dassign->GetRHS();
        uint32_t lshtypesize = GetPrimTypeSize(lhsType.GetPrimType()) * 8;
        if (IsReductionOp(rhs->GetOpCode()) && doloopInfo->IsReductionVar(lhsStIdx)) {
          BaseNode *opnd0 = rhs->Opnd(0);
          BaseNode *opnd1 = rhs->Opnd(1);
          CHECK_FATAL((opnd0->GetOpCode() == OP_dread) && ((static_cast<AddrofNode *>(opnd0))->GetStIdx() == lhsStIdx),
                      "opnd0 is reduction variable");
          if (ExprVectorizable(doloopInfo, vecInfo, opnd1)) {
            // there's iread in rhs
            if ((vecInfo->currentRHSTypeSize != 0) && (lshtypesize != vecInfo->currentRHSTypeSize)) {
              return false;
            }
            vecInfo->vecStmtIDs.insert((stmt)->GetStmtID());
            vecInfo->UpdateWidestTypeSize(lshtypesize);
            vecInfo->reductionVars.insert(std::make_pair((static_cast<AddrofNode *>(opnd0))->GetStIdx(), rhs->GetOpCode()));
          } else {
            return false; // only handle reduction scalar
          }
        } else {
          return false;
        }
        break;
      }
      default: return false;
    }
    stmt = stmt->GetNext();
  }
  return true;
}

void LoopVectorization::Perform() {
  // step 2: collect information, legality check and generate transform plan
  MapleMap<DoloopNode *, DoloopInfo *>::iterator mapit = depInfo->doloopInfoMap.begin();
  for (; mapit != depInfo->doloopInfoMap.end(); mapit++) {
    if (!mapit->second->children.empty() ||
        ((!mapit->second->Parallelizable()) && (!mapit->second->CheckReductionLoop()))) {
      continue;
    }
    LoopVecInfo *vecInfo = localMP->New<LoopVecInfo>(localAlloc);
    bool vectorizable = Vectorizable(mapit->second, vecInfo, mapit->first->GetDoBody());
    if (vectorizable) {
      LoopVectorization::vectorizedLoop++;
    }
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
}  // namespace maple
