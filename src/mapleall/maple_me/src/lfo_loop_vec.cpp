/*
 * Copyright (c) [2021-2022] Futurewei Technologies Co.,Ltd.All rights reserved.
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
#include "lfo_loop_vec.h"
#include <iostream>
#include <algorithm>
#include "me_option.h"
#include "mir_module.h"
#include "mir_builder.h"
#include "common_utils.h"

constexpr uint32_t maxVecSize = 128;

namespace maple {

void LoopVecInfo::UpdateDoloopProfData(MIRFunction *mirFunc, DoloopNode *doLoop, int32_t vecLanes, bool isRemainder) {
  auto *profData = mirFunc->GetFuncProfData();
  if (!profData) {
    return;
  }
  int64_t doloopFreq = profData->GetStmtFreq(doLoop->GetStmtID());
  int64_t tempFreq;
  BlockNode *body = doLoop->GetDoBody();
  int64_t bodyFreq = profData->GetStmtFreq(body->GetStmtID());
  if (isRemainder) {
    // update doLoop node in remainder
    tempFreq = (bodyFreq % vecLanes) + (doloopFreq - bodyFreq);
    profData->SetStmtFreq(doLoop->GetStmtID(), tempFreq);
    // update doLoop body freq in remainder
    profData->SetStmtFreq(body->GetStmtID(), (bodyFreq % vecLanes));
    // update stmtlist in doloopbody
    if (body->GetFirst()) {
      profData->SetStmtFreq(body->GetFirst()->GetStmtID(), (bodyFreq % vecLanes));
    }
    if (body->GetLast() && (body->GetFirst() != body->GetLast())) {
      profData->SetStmtFreq(body->GetLast()->GetStmtID(), (bodyFreq % vecLanes));
    }
  } else {
    // vectorized doLoop
    tempFreq = (bodyFreq / vecLanes) + (doloopFreq - bodyFreq);
    profData->SetStmtFreq(doLoop->GetStmtID(), tempFreq);
    // update doLoop body freq
    profData->SetStmtFreq(body->GetStmtID(), (bodyFreq / vecLanes));
    // update stmtlist in doloopbody
    if (body->GetFirst()) {
      profData->SetStmtFreq(body->GetFirst()->GetStmtID(), (tempFreq / vecLanes));
    }
    if (body->GetLast() && (body->GetFirst() != body->GetLast())) {
      profData->SetStmtFreq(body->GetLast()->GetStmtID(), (tempFreq / vecLanes));
    }
  }
}

uint32_t LoopVectorization::vectorizedLoop = 0;

void LoopVecInfo::UpdateWidestTypeSize(uint32_t newTypesize) {
  if (largestTypeSize < newTypesize) {
    largestTypeSize = newTypesize;
  }
}

bool LoopVecInfo::UpdateRHSTypeSize(PrimType ptype) {
  uint32_t newSize = GetPrimTypeSize(ptype) * 8;
  if (smallestTypeSize > newSize) {
    smallestTypeSize = newSize;
  }
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
void LoopTransPlan::GenerateBoundInfo(const DoloopNode *doLoop, const DoloopInfo *li) {
  (void) li;
  BaseNode *initNode = doLoop->GetStartExpr();
  BaseNode *incrNode = doLoop->GetIncrExpr();
  BaseNode *condNode = doLoop->GetCondExpr();
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
      vecFactor * incrConst->GetExtValue(), *typeInt);
  ConstvalNode *newIncrNode = codeMP->New<ConstvalNode>(PTY_i32, newIncr);
  PrimType newOpndtype = (static_cast<CompareNode *>(condNode))->GetOpndType() == PTY_ptr ? PTY_i64 : PTY_i32;
  if (initNode->IsConstval()) {
    ConstvalNode *lcn = static_cast<ConstvalNode *>(initNode);
    MIRIntConst *lowConst = static_cast<MIRIntConst *>(lcn->GetConstVal());
    int64 lowvalue = lowConst->GetExtValue();
    // upNode is constant
    if (upNode->IsConstval()) {
      ConstvalNode *ucn = static_cast<ConstvalNode *>(upNode);
      MIRIntConst *upConst = static_cast<MIRIntConst *>(ucn->GetConstVal());
      int64 upvalue = upConst->GetExtValue();
      if (condOpHasEqual) {
        upvalue += 1;
      }
      int64 newIncrVal = newIncr->GetExtValue();
      if (((upvalue - lowvalue) % newIncrVal) == 0) {
        // early return, change vbound->stride only
        vBound = localMP->New<LoopBound>(nullptr, nullptr, newIncrNode);
      } else {
        // trip count is not vector lane aligned
        int64 newupval = (upvalue - lowvalue) / newIncrVal * newIncrVal + lowvalue;
        MIRIntConst *newUpConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(newupval, *typeInt);
        ConstvalNode *newUpNode = codeMP->New<ConstvalNode>(PTY_i32, newUpConst);
        vBound = localMP->New<LoopBound>(nullptr, newUpNode, newIncrNode);
        // generate epilog
        eBound = localMP->New<LoopBound>(newUpNode, nullptr, nullptr);
      }
    } else {
      // step 1: generate vectorized loop bound
      // upNode of vBound is (uppnode - initnode) / newIncr * newIncr + initnode
      BinaryNode *divnode = nullptr;
      BaseNode *addnode = upNode;
      if (condOpHasEqual) {
        addnode = codeMP->New<BinaryNode>(OP_add, newOpndtype, upNode, constOnenode);
      }
      if (lowvalue != 0) {
        BinaryNode *subnode = codeMP->New<BinaryNode>(OP_sub, newOpndtype, addnode, initNode);
        divnode = codeMP->New<BinaryNode>(OP_div, newOpndtype, subnode, newIncrNode);
      } else {
        divnode = codeMP->New<BinaryNode>(OP_div, newOpndtype, addnode, newIncrNode);
      }
      BinaryNode *mulnode = codeMP->New<BinaryNode>(OP_mul, newOpndtype, divnode, newIncrNode);
      addnode = codeMP->New<BinaryNode>(OP_add, newOpndtype, mulnode, initNode);
      vBound = localMP->New<LoopBound>(nullptr, addnode, newIncrNode);
      // step2:  generate epilog bound
      eBound = localMP->New<LoopBound>(addnode, nullptr, nullptr);
    }
  } else {
    // initnode is not constant
    // set bound of vectorized loop
    BinaryNode *subnode = nullptr;
    if (condOpHasEqual) {
      BinaryNode *addnode = codeMP->New<BinaryNode>(OP_add, newOpndtype, upNode, constOnenode);
      subnode = codeMP->New<BinaryNode>(OP_sub, newOpndtype, addnode, initNode);
    } else {
      subnode = codeMP->New<BinaryNode>(OP_sub, newOpndtype, upNode, initNode);
    }
    BinaryNode *divnode = codeMP->New<BinaryNode>(OP_div, newOpndtype, subnode, newIncrNode);
    BinaryNode *mulnode = codeMP->New<BinaryNode>(OP_mul, newOpndtype, divnode, newIncrNode);
    BinaryNode *addnode = codeMP->New<BinaryNode>(OP_add, newOpndtype, mulnode, initNode);
    vBound = localMP->New<LoopBound>(nullptr, addnode, newIncrNode);
    // set bound of epilog loop
    eBound = localMP->New<LoopBound>(addnode, nullptr, nullptr);
  }
}

// generate best plan for current doloop
bool LoopTransPlan::Generate(const DoloopNode *doLoop, const DoloopInfo* li, bool enableDebug) {
  // vector length / type size
  vecLanes = maxVecSize / (vecInfo->largestTypeSize);
  vecFactor = vecLanes;
  // return false if small type has no builtin vector type
  if (vecFactor * vecInfo->smallestTypeSize < maplebe::k64BitSize) {
    if (enableDebug) {
      LogInfo::MapleLogger() << "NOT VECTORIZABLE because no builtin vector type for smallestType in loop\n";
    }
    return false;
  }
  // if depdist is not zero
  if (vecInfo->minTrueDepDist > 0 || vecInfo->maxAntiDepDist < 0) {
    // true dep distance is less than vecLanes, return false
    if ((vecInfo->minTrueDepDist > 0) && (vecInfo->minTrueDepDist < vecLanes)) {
      if (enableDebug) {
        LogInfo::MapleLogger() << "NOT VECTORIZABLE because true dependence distance less than veclanes in loop\n";
      }
      return false;
    }
    // anti-dep distance doesn't break vectorization in case
    //  use before def like a[i] = a[i+1]
    // if use is after def as following, distance less than vecLanes will break vectorization
    //  a[i] =
    //       = a[i+1]
    // there's no extra information to describe sequence now
    // we only handle one stmt in loopbody without considering anti-dep distance
    if ((vecInfo->maxAntiDepDist < 0) && ((-vecInfo->maxAntiDepDist) < vecLanes) &&
        (doLoop->GetDoBody()->GetFirst() != doLoop->GetDoBody()->GetLast())) {
      if (enableDebug) {
        LogInfo::MapleLogger() << "NOT VECTORIZABLE because anti dependence distance less than veclanes in loop\n";
      }
      return false;
    }
  }
  // compare trip count if lanes is larger than tripcount
  {
    BaseNode *initNode = doLoop->GetStartExpr();
    BaseNode *incrNode = doLoop->GetIncrExpr();
    BaseNode *condNode = doLoop->GetCondExpr();
    BaseNode *upNode = condNode->Opnd(1);
    BaseNode *condOpnd0 = condNode->Opnd(0);

    // check opnd0 of condNode is an expression not a variable
    // upperbound formula doesn't handle this case now
    if ((condOpnd0->GetOpCode() != OP_dread) &&
        (condOpnd0->GetOpCode() != OP_regread)) {
      if (enableDebug) {
        LogInfo::MapleLogger() << "NOT VECTORIZABLE because of doLoop condition compare is complex \n";
      }
      return false;
    }

    bool condOpHasEqual = ((condNode->GetOpCode() == OP_le) || (condNode->GetOpCode() == OP_ge));
    if (initNode->IsConstval() && upNode->IsConstval() && incrNode->IsConstval()) {
      ConstvalNode *lcn = static_cast<ConstvalNode *>(initNode);
      MIRIntConst *lowConst = static_cast<MIRIntConst *>(lcn->GetConstVal());
      int64 lowvalue = lowConst->GetExtValue();
      ConstvalNode *ucn = static_cast<ConstvalNode *>(upNode);
      MIRIntConst *upConst = static_cast<MIRIntConst *>(ucn->GetConstVal());
      int64 upvalue = upConst->GetExtValue();
      ConstvalNode *icn = static_cast<ConstvalNode *>(incrNode);
      MIRIntConst *incrConst = static_cast<MIRIntConst *>(icn->GetConstVal());
      if (condOpHasEqual) {
        upvalue += 1;
      }
      int64 tripCount = (upvalue - lowvalue) / (incrConst->GetExtValue());
      if (static_cast<uint32>(tripCount) < vecLanes) {
        tripCount = (tripCount / 4 * 4); // get closest 2^n
        if (tripCount * vecInfo->smallestTypeSize < maplebe::k64BitSize) {
          if (enableDebug) {
            LogInfo::MapleLogger() << "NOT VECTORIZABLE because of doLoop trip count is small \n";
          }
          return false;
        } else {
          vecLanes = static_cast<uint8_t>(tripCount);
          vecFactor = static_cast<uint8_t>(tripCount);
        }
      }
    }
  }
  // generate bound information
  GenerateBoundInfo(doLoop, li);
  return true;
}

MIRType* LoopVectorization::GenVecType(PrimType sPrimType, uint8 lanes) const {
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
// now only support +/-
bool LoopVectorization::IsReductionOp(Opcode op) const {
  return op == OP_add || op == OP_sub;
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

IntrinsicopNode *LoopVectorization::GenVectorGetLow(BaseNode *vecNode, PrimType vecPrimType) {
  MIRIntrinsicID intrnID = INTRN_vector_get_low_v16i8;
  MIRType *retvecType = nullptr;
  switch (vecPrimType) {
    case PTY_v16i8: {
      intrnID = INTRN_vector_get_low_v16i8;
      retvecType = GlobalTables::GetTypeTable().GetV8Int8();
      break;
    }
    case PTY_v16u8: {
      intrnID = INTRN_vector_get_low_v16u8;
      retvecType = GlobalTables::GetTypeTable().GetV8UInt8();
      break;
    }
    case PTY_v8i16: {
      intrnID = INTRN_vector_get_low_v8i16;
      retvecType = GlobalTables::GetTypeTable().GetV4Int16();
      break;
    }
    case PTY_v8u16: {
      intrnID = INTRN_vector_get_low_v8u16;
      retvecType = GlobalTables::GetTypeTable().GetV4UInt16();
      break;
    }
    case PTY_v4i32: {
      intrnID = INTRN_vector_get_low_v4i32;
      retvecType = GlobalTables::GetTypeTable().GetV2Int32();
      break;
    }
    case PTY_v4u32: {
      intrnID = INTRN_vector_get_low_v4u32;
      retvecType = GlobalTables::GetTypeTable().GetV2UInt32();
      break;
    }
    case PTY_v2i64: {
      intrnID = INTRN_vector_get_low_v2i64;
      retvecType = GlobalTables::GetTypeTable().GetInt64();
      break;
    }
    case PTY_v2u64: {
      intrnID = INTRN_vector_get_low_v2u64;
      retvecType = GlobalTables::GetTypeTable().GetUInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_get_low");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, retvecType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(1);
  rhs->GetNopnd().push_back(vecNode);
  rhs->SetTyIdx(retvecType->GetTypeIndex());
  return rhs;
}

// vector add long oper0 and oper1 have same types
IntrinsicopNode *LoopVectorization::GenVectorAddw(BaseNode *oper0, BaseNode *oper1, PrimType op1Type, bool highPart) {
  MIRIntrinsicID intrnID = INTRN_vector_addw_low_v8i8;
  MIRType *resType = nullptr;
  switch (op1Type) {
    case PTY_v8i8: {
      intrnID = highPart ? INTRN_vector_addw_high_v8i8 : INTRN_vector_addw_low_v8i8;
      resType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u8: {
      intrnID = highPart ? INTRN_vector_addw_high_v8u8 : INTRN_vector_addw_low_v8u8;
      resType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = highPart ? INTRN_vector_addw_high_v4i16 : INTRN_vector_addw_low_v4i16;
      resType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v4u16: {
      intrnID = highPart ? INTRN_vector_addw_high_v4u16 : INTRN_vector_addw_low_v4u16;
      resType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2i32: {
      intrnID = highPart ? INTRN_vector_addw_high_v2i32 : INTRN_vector_addw_low_v2i32;
      resType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v2u32: {
      intrnID = highPart ? INTRN_vector_addw_high_v2u32 : INTRN_vector_addw_low_v2u32;
      resType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_addwh");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(2);
  rhs->GetNopnd().push_back(oper0);
  rhs->GetNopnd().push_back(oper1);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}

// Subtract Long
IntrinsicopNode *LoopVectorization::GenVectorSubl(BaseNode *oper0, BaseNode *oper1, PrimType op1Type, bool highPart) {
  MIRIntrinsicID intrnID = INTRN_vector_subl_low_v8i8;
  MIRType *resType = nullptr;
  switch (op1Type) {
    case PTY_v8i8: {
      intrnID = highPart ? INTRN_vector_subl_high_v8i8 : INTRN_vector_subl_low_v8i8;
      resType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u8: {
      intrnID = highPart ? INTRN_vector_subl_high_v8u8 : INTRN_vector_subl_low_v8u8;
      resType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = highPart ? INTRN_vector_subl_high_v4i16 : INTRN_vector_subl_low_v4i16;
      resType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v4u16: {
      intrnID = highPart ? INTRN_vector_subl_high_v4u16 : INTRN_vector_subl_low_v4u16;
      resType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2i32: {
      intrnID = highPart ? INTRN_vector_subl_high_v2i32 : INTRN_vector_subl_low_v2i32;
      resType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v2u32: {
      intrnID = highPart ? INTRN_vector_subl_high_v2u32 : INTRN_vector_subl_low_v2u32;
      resType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_subl");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(2);
  rhs->GetNopnd().push_back(oper0);
  rhs->GetNopnd().push_back(oper1);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}

// Vector Add long
IntrinsicopNode *LoopVectorization::GenVectorAddl(BaseNode *oper0,
    BaseNode *oper1, PrimType op1Type, bool highPart) {
  MIRIntrinsicID intrnID = INTRN_vector_addl_low_v8i8;
  MIRType *resType = nullptr;
  switch (op1Type) {
    case PTY_v8i8: {
      intrnID = highPart ? INTRN_vector_addl_high_v8i8 : INTRN_vector_addl_low_v8i8;
      resType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u8: {
      intrnID = highPart ? INTRN_vector_addl_high_v8u8 : INTRN_vector_addl_low_v8u8;
      resType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = highPart ? INTRN_vector_addl_high_v4i16 : INTRN_vector_addl_low_v4i16;
      resType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v4u16: {
      intrnID = highPart ? INTRN_vector_addl_high_v4u16 : INTRN_vector_addl_low_v4u16;
      resType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2i32: {
      intrnID = highPart ? INTRN_vector_addl_high_v2i32 : INTRN_vector_addl_low_v2i32;
      resType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v2u32: {
      intrnID = highPart ? INTRN_vector_addl_high_v2u32 : INTRN_vector_addl_low_v2u32;
      resType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_addl");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(2);
  rhs->GetNopnd().push_back(oper0);
  rhs->GetNopnd().push_back(oper1);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}

IntrinsicopNode *LoopVectorization::GenVectorMull(BaseNode *oper0,
    BaseNode *oper1, PrimType op1Type, bool highPart) {
  MIRIntrinsicID intrnID = INTRN_vector_mull_low_v2i32;
  MIRType *resType = nullptr;
  switch (op1Type) {
    case PTY_v8i8: {
      intrnID = highPart ? INTRN_vector_mull_high_v8i8 : INTRN_vector_mull_low_v8i8;
      resType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u8: {
      intrnID = highPart ? INTRN_vector_mull_high_v8u8 : INTRN_vector_mull_low_v8u8;
      resType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = highPart ? INTRN_vector_mull_high_v4i16 : INTRN_vector_mull_low_v4i16;
      resType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v4u16: {
      intrnID = highPart ? INTRN_vector_mull_high_v4u16 : INTRN_vector_mull_low_v4u16;
      resType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2i32: {
      intrnID = highPart ? INTRN_vector_mull_high_v2i32 : INTRN_vector_mull_low_v2i32;
      resType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v2u32: {
      intrnID = highPart ? INTRN_vector_mull_high_v2u32 : INTRN_vector_mull_low_v2u32;
      resType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_mull");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(2);
  rhs->GetNopnd().push_back(oper0);
  rhs->GetNopnd().push_back(oper1);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}


// return intrinsicID
MIRIntrinsicID LoopVectorization::GenVectorAbsSublID(MIRIntrinsicID intrnID) const {
  MIRIntrinsicID newIntrnID = INTRN_vector_labssub_low_v8i8;
  switch (intrnID)  {
    case INTRN_vector_subl_low_v8i8: {
      newIntrnID = INTRN_vector_labssub_low_v8i8;
      break;
    }
    case INTRN_vector_subl_high_v8i8: {
      newIntrnID = INTRN_vector_labssub_high_v8i8;
      break;
    }
    case INTRN_vector_subl_low_v8u8: {
      newIntrnID = INTRN_vector_labssub_low_v8u8;
      break;
    }
    case INTRN_vector_subl_high_v8u8: {
      newIntrnID =  INTRN_vector_labssub_high_v8u8;
      break;
    }
    case INTRN_vector_subl_low_v4i16: {
      newIntrnID = INTRN_vector_labssub_low_v4i16;
      break;
    }
    case INTRN_vector_subl_high_v4i16: {
      newIntrnID = INTRN_vector_labssub_high_v4i16;
      break;
    }
    case INTRN_vector_subl_low_v4u16: {
      newIntrnID = INTRN_vector_labssub_low_v4u16;
      break;
    }
    case INTRN_vector_subl_high_v4u16: {
      newIntrnID = INTRN_vector_labssub_high_v4u16;
      break;
    }
    case INTRN_vector_subl_low_v2i32: {
      newIntrnID = INTRN_vector_labssub_low_v2i32;
      break;
    }
    case INTRN_vector_subl_high_v2i32: {
      newIntrnID = INTRN_vector_labssub_high_v2i32;
      break;
    }
    case INTRN_vector_subl_low_v2u32: {
      newIntrnID = INTRN_vector_labssub_low_v2u32;
      break;
    }
    case INTRN_vector_subl_high_v2u32: {
      newIntrnID = INTRN_vector_labssub_high_v2u32;
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported change to vector_labssub");
    }
  }
  return newIntrnID;
}

// return widened vector by getting the abs value of subtracted arguments
IntrinsicopNode *LoopVectorization::GenVectorAbsSubl(BaseNode *oper0,
    BaseNode *oper1, PrimType op1Type, bool highPart) {
  MIRIntrinsicID intrnID = INTRN_vector_labssub_low_v8i8;
  MIRType *resType = nullptr;
  switch (op1Type) {
    case PTY_v8i8: {
      intrnID = highPart ? INTRN_vector_labssub_high_v8i8 : INTRN_vector_labssub_low_v8i8;
      resType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u8: {
      intrnID = highPart ? INTRN_vector_labssub_high_v8u8 : INTRN_vector_labssub_low_v8u8;
      resType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = highPart ? INTRN_vector_labssub_high_v4i16 : INTRN_vector_labssub_low_v4i16;
      resType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v4u16: {
      intrnID = highPart ? INTRN_vector_labssub_high_v4u16 : INTRN_vector_labssub_low_v4u16;
      resType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2i32: {
      intrnID = highPart ? INTRN_vector_labssub_high_v2i32 : INTRN_vector_labssub_low_v2i32;
      resType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v2u32: {
      intrnID = highPart ? INTRN_vector_labssub_high_v2u32 : INTRN_vector_labssub_low_v2u32;
      resType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_labssub");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(2);
  rhs->GetNopnd().push_back(oper0);
  rhs->GetNopnd().push_back(oper1);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}

IntrinsicopNode *LoopVectorization::GenVectorWidenOpnd(BaseNode *opnd, PrimType vecPrimType, bool highPart) {
  MIRIntrinsicID intrnID = INTRN_vector_widen_low_v8i8;
  MIRType *resType = nullptr;
  switch (vecPrimType) {
    case PTY_v8i8: {
      intrnID = highPart ? INTRN_vector_widen_high_v8i8 : INTRN_vector_widen_low_v8i8;
      resType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u8: {
      intrnID = highPart ? INTRN_vector_widen_high_v8u8 : INTRN_vector_widen_low_v8u8;
      resType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = highPart ? INTRN_vector_widen_high_v4i16 : INTRN_vector_widen_low_v4i16;
      resType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v4u16: {
      intrnID = highPart ? INTRN_vector_widen_high_v4u16 : INTRN_vector_widen_low_v4u16;
      resType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2i32: {
      intrnID = highPart ? INTRN_vector_widen_high_v2i32 : INTRN_vector_widen_low_v2i32;
      resType = GlobalTables::GetTypeTable().GetV2Int64(); // kArgTyV2I64
      break;
    }
    case PTY_v2u32: {
      intrnID = highPart ? INTRN_vector_widen_high_v2u32 : INTRN_vector_widen_low_v2u32;
      resType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_widen");
    }
  }
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(1);
  rhs->GetNopnd().push_back(opnd);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}

IntrinsicopNode *LoopVectorization::GenVectorPairWiseAccumulate(BaseNode *oper0,
    BaseNode *oper1, PrimType oper1Type) {
  MIRIntrinsicID intrnID = INTRN_vector_pairwise_adalp_v8i8;
  MIRType *resType = nullptr;
  switch (oper1Type) {
    case PTY_v8i8: {
      intrnID = INTRN_vector_pairwise_adalp_v8i8;
      resType = GlobalTables::GetTypeTable().GetV4Int16();
      break;
    }
    case PTY_v4i16: {
      intrnID = INTRN_vector_pairwise_adalp_v4i16;
      resType = GlobalTables::GetTypeTable().GetV2Int32();
      break;
    }
    case PTY_v2i32: {
      intrnID = INTRN_vector_pairwise_adalp_v2i32;
      resType = GlobalTables::GetTypeTable().GetInt64();
      break;
    }
    case PTY_v8u8: {
      intrnID = INTRN_vector_pairwise_adalp_v8u8;
      resType = GlobalTables::GetTypeTable().GetV4UInt16();
      break;
    }
    case PTY_v4u16: {
      intrnID = INTRN_vector_pairwise_adalp_v4u16;
      resType = GlobalTables::GetTypeTable().GetV2UInt32();
      break;
    }
    case PTY_v2u32: {
      intrnID = INTRN_vector_pairwise_adalp_v2u32;
      resType = GlobalTables::GetTypeTable().GetUInt64();
      break;
    }
    case PTY_v16i8: {
      intrnID = INTRN_vector_pairwise_adalp_v16i8;
      resType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8i16: {
      intrnID = INTRN_vector_pairwise_adalp_v8i16;
      resType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v4i32: {
      intrnID = INTRN_vector_pairwise_adalp_v4i32;
      resType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v16u8: {
      intrnID = INTRN_vector_pairwise_adalp_v16u8;
      resType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v8u16: {
      intrnID = INTRN_vector_pairwise_adalp_v8u16;
      resType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v4u32: {
      intrnID = INTRN_vector_pairwise_adalp_v4u32;
      resType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_widen");
    }
  }
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(2);
  rhs->GetNopnd().push_back(oper0);
  rhs->GetNopnd().push_back(oper1);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}

// vaddl,opnd0 and opnd1 are same type, target type are widen
IntrinsicopNode *LoopVectorization::GenVectorWidenIntrn(BaseNode *oper0,
    BaseNode *oper1, PrimType opndType, bool highPart, Opcode op) {
  if (op == OP_add) {
    return GenVectorAddl(oper0, oper1, opndType, highPart);
  } else if (op == OP_sub) {
    return GenVectorSubl(oper0, oper1, opndType, highPart);
  } else if (op == OP_mul) {
    return GenVectorMull(oper0, oper1, opndType, highPart);
  }
  ASSERT(0, "GenWidenIntrn : only support add and sub opcode"); // should not be here
  return nullptr;
}

bool LoopVectorization::CanWidenOpcode(const BaseNode *target, PrimType opndType) const {
  if ((target->GetPrimType() == opndType) ||
      (GetPrimTypeSize(target->GetPrimType()) < GetPrimTypeSize(opndType))) {
        return false;
  }
  Opcode op = target->GetOpCode();
  // we have add/sub widen intrinsic now
  if (op == OP_sub || op == OP_add || op == OP_mul) {
    return true;
  }
  // no other widen intrins supported now
  return false;
}

// generate addl/subl intrinsic
void LoopVectorization::GenWidenBinaryExpr(Opcode binOp,
                                           MapleVector<BaseNode *>& opnd0Vec,
                                           MapleVector<BaseNode *>& opnd1Vec,
                                           MapleVector<BaseNode *>& vectorizedNode) {
  auto op1veclen = opnd0Vec.size();
  auto op2veclen = opnd1Vec.size();
  auto lenMin = op1veclen < op2veclen ? op1veclen : op2veclen;
  for (size_t i = 0; i < lenMin; i++) {
    BaseNode *opnd0 = opnd0Vec[i];
    BaseNode *opnd1 = opnd1Vec[i];
    PrimType opnd0PrimType = opnd0->GetPrimType();
    PrimType opnd1PrimType = opnd1->GetPrimType();
    // widen type need high/low part
    if ((GetPrimTypeSize(opnd0PrimType) * 8) == maxVecSize) {
      IntrinsicopNode *getLowop0Intrn = GenVectorGetLow(opnd0, opnd0PrimType);
      IntrinsicopNode *getLowop1Intrn = GenVectorGetLow(opnd1, opnd1PrimType);
      IntrinsicopNode *widenOpLow = GenVectorWidenIntrn(getLowop0Intrn, getLowop1Intrn, getLowop0Intrn->GetPrimType(),
          false, binOp);
      IntrinsicopNode *widenOpHigh = GenVectorWidenIntrn(opnd0, opnd1, getLowop0Intrn->GetPrimType(), true, binOp);
      vectorizedNode.push_back(widenOpLow);
      vectorizedNode.push_back(widenOpHigh);
    } else {
      IntrinsicopNode *widenOp = GenVectorWidenIntrn(opnd0, opnd1, opnd0PrimType, false, binOp);
      vectorizedNode.push_back(widenOp);
    }
  }
}

// insert retype/cvt if sign/unsign
BaseNode *LoopVectorization::ConvertNodeType(bool cvtSigned, BaseNode *n) {
  MIRType *opcodetype = nullptr;
  MIRType *nodetype = GenVecType(GetVecElemPrimType(n->GetPrimType()),
                                 static_cast<uint8>(GetVecLanes(n->GetPrimType())));
  if (cvtSigned) {
    opcodetype = GenVecType(GetSignedPrimType(GetVecElemPrimType(n->GetPrimType())),
                            static_cast<uint8>(GetVecLanes(n->GetPrimType())));
  } else {
    opcodetype = GenVecType(GetUnsignedPrimType(GetVecElemPrimType(n->GetPrimType())),
                            static_cast<uint8>(GetVecLanes(n->GetPrimType())));
  }
  BaseNode *newnode = nullptr;
  if (GetPrimTypeSize(n->GetPrimType()) == GetPrimTypeSize(opcodetype->GetPrimType())) {
    newnode = codeMP->New<RetypeNode>(opcodetype->GetPrimType(), n->GetPrimType(), nodetype->GetTypeIndex(), n);
  } else {
    newnode = codeMP->New<TypeCvtNode>(OP_cvt, opcodetype->GetPrimType(), n->GetPrimType(), n);
  }
  return newnode;
}

IntrinsicopNode *LoopVectorization::GenVectorNarrowLowNode(BaseNode *opnd, PrimType opndPrimType) {
  MIRIntrinsicID intrnID = INTRN_vector_narrow_low_v2i64;
  MIRType *resType = nullptr;
  switch (opndPrimType) {
    case PTY_v2i64: {
      intrnID = INTRN_vector_narrow_low_v2i64;
      resType = GlobalTables::GetTypeTable().GetV2Int32();
      break;
    }
    case PTY_v4i32: {
      intrnID = INTRN_vector_narrow_low_v4i32;
      resType = GlobalTables::GetTypeTable().GetV4Int16();
      break;
    }
    case PTY_v8i16: {
      intrnID = INTRN_vector_narrow_low_v8i16;
      resType = GlobalTables::GetTypeTable().GetV8Int8();
      break;
    }
    case PTY_v2u64: {
      intrnID = INTRN_vector_narrow_low_v2u64;
      resType = GlobalTables::GetTypeTable().GetV2UInt32();
      break;
    }
    case PTY_v4u32: {
      intrnID = INTRN_vector_narrow_low_v4u32;
      resType = GlobalTables::GetTypeTable().GetV4UInt16();
      break;
    }
    case PTY_v8u16: {
      intrnID = INTRN_vector_narrow_low_v8u16;
      resType = GlobalTables::GetTypeTable().GetV8UInt8();
      break;
    }
    default: {
      CHECK_FATAL(0, "unsupported type in vector_narrowlow");
    }
  }
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, resType->GetPrimType());
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(1);
  rhs->GetNopnd().push_back(opnd);
  rhs->SetTyIdx(resType->GetTypeIndex());
  return rhs;
}

// create vectorized preg for reduction var and its init stmt vpreg = dup_scalar(0)
RegreadNode *LoopVectorization::GenVectorReductionVar(StmtNode *stmt, LoopTransPlan *tp) {
  PrimType lhsType;
  uint8 lhsLanes = tp->vecFactor;
  if (stmt->GetOpCode() == OP_dassign) {
    StIdx redStIdx = (static_cast<DassignNode *>(stmt))->GetStIdx();
    MIRSymbol *lhsSym = mirFunc->GetLocalOrGlobalSymbol(redStIdx);
    lhsType = GetTypeFromTyIdx(lhsSym->GetTyIdx()).GetPrimType();
    uint32_t lhstypesize = GetPrimTypeSize(lhsType) * 8;
    uint32_t lhsMaxLanes = ((maxVecSize / lhstypesize) < tp->vecFactor) ?
                            (maxVecSize / lhstypesize) : tp->vecFactor;
    lhsLanes = lhsMaxLanes;
  } else {
    IassignNode *iassign = static_cast<IassignNode *>(stmt);
    MIRType &mirType = GetTypeFromTyIdx(iassign->GetTyIdx());
    lhsType = (static_cast<MIRPtrType*>(&mirType))->GetPointedType()->GetPrimType();
  }
  MIRType *lhsvecType = GenVecType(lhsType, static_cast<uint8>(lhsLanes));
  PregIdx reglhsvec = mirFunc->GetPregTab()->CreatePreg(lhsvecType->GetPrimType());
  BaseNode *constZero = const0Node;
  if (GetPrimTypeSize(const0Node->GetPrimType()) != GetPrimTypeSize(lhsType)) {
    constZero = const0Node->CloneTree(*codeMPAlloc);
    constZero->SetPrimType(lhsType);
  }
  IntrinsicopNode *lhsvecIntrn = GenDupScalarExpr(constZero, lhsvecType->GetPrimType());
  RegassignNode *initlhsvec = codeMP->New<RegassignNode>(lhsvecType->GetPrimType(), reglhsvec, lhsvecIntrn);
  tp->vecInfo->beforeLoopStmts.push_back(initlhsvec);
  RegreadNode *regReadlhsvec = codeMP->New<RegreadNode>(lhsvecType->GetPrimType(), reglhsvec);
  tp->vecInfo->redVecNodes[stmt] = regReadlhsvec;
  return regReadlhsvec;
}

// check vectorized preg is generated for current stmt
RegreadNode *LoopVectorization::GetorNewVectorReductionVar(StmtNode *stmt, LoopTransPlan *tp) {
  RegreadNode *regReadlhsvec;
  if (tp->vecInfo->redVecNodes.find(stmt) != tp->vecInfo->redVecNodes.end()) {
    regReadlhsvec = static_cast<RegreadNode *>(tp->vecInfo->redVecNodes[stmt]);
  } else {
    regReadlhsvec = GenVectorReductionVar(stmt, tp);
  }
  return regReadlhsvec;
}

static bool IsCompareNode(Opcode op) {
  if (op == OP_eq || op == OP_ge || op == OP_gt || op == OP_le || op == OP_lt ||
      op == OP_ne || op == OP_cmp || op == OP_cmpl || op == OP_cmpg) {
    return true;
  }
  return false;
}

void LoopVectorization::VectorizeExpr(BaseNode *node, LoopTransPlan *tp, MapleVector<BaseNode *>& vectorizedNode,
    uint32_t depth) {
  switch (node->GetOpCode()) {
    case OP_iread: {
      IreadNode *ireadnode = static_cast<IreadNode *>(node);
      // update tyidx
      MIRType *mirType = ireadnode->GetType();
      MIRType *vecType = nullptr;
      // update lhs type
      if (mirType->GetPrimType() == PTY_agg) {
        // iread variable from a struct, use iread type
        vecType = GenVecType(ireadnode->GetPrimType(), tp->vecFactor);
        ASSERT(vecType != nullptr, "vector type should not be null");
      } else {
        vecType = GenVecType(mirType->GetPrimType(), tp->vecFactor);
        ASSERT(vecType != nullptr, "vector type should not be null");
        MIRType *pvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType, PTY_ptr);
        ireadnode->SetTyIdx(pvecType->GetTypeIndex());
      }
      PrimType optype = node->GetPrimType();
      node->SetPrimType(vecType->GetPrimType());
      if ((depth == 0) &&
          (tp->vecInfo->currentLHSTypeSize > GetPrimTypeSize(GetVecElemPrimType(vecType->GetPrimType()))) &&
          ((GetPrimTypeSize(optype) / GetPrimTypeSize(GetVecElemPrimType(vecType->GetPrimType()))) > 2)) {
        // widen 128 bit node type : split two nodes
        if (GetPrimTypeSize(vecType->GetPrimType()) == maplebe::k16BitSize) {
          IntrinsicopNode *getLowIntrn = GenVectorGetLow(node, vecType->GetPrimType());
          IntrinsicopNode *lowNode = GenVectorWidenOpnd(getLowIntrn, getLowIntrn->GetPrimType(), false);
          IntrinsicopNode *highNode = GenVectorWidenOpnd(node, getLowIntrn->GetPrimType(), true);
          vectorizedNode.push_back(lowNode);
          vectorizedNode.push_back(highNode);
        } else {
          // widen element type
          IntrinsicopNode *widenop = GenVectorWidenOpnd(node, vecType->GetPrimType(), false);
          vectorizedNode.push_back(widenop);
        }
      } else {
        vectorizedNode.push_back(node);
      }
      if ((IsSignedInteger(optype) && IsUnsignedInteger(vecType->GetPrimType())) ||
          (IsUnsignedInteger(optype) && IsSignedInteger(vecType->GetPrimType()))) {
        for (size_t i = 0; i < vectorizedNode.size(); i++) {
          vectorizedNode[i] = ConvertNodeType(IsSignedInteger(optype), vectorizedNode[i]);
        }
      }
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
    case OP_bxor:
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
      MapleVector<BaseNode *> vecopnd1(localAlloc.Adapter());
      MapleVector<BaseNode *> vecopnd2(localAlloc.Adapter());
      BaseNode *opnd1 = binNode->Opnd(0);
      BaseNode *opnd2 = binNode->Opnd(1);
      if (tp->vecInfo->uniformVecNodes.find(opnd1) != tp->vecInfo->uniformVecNodes.end()) {
        BaseNode *newnode = tp->vecInfo->uniformVecNodes[opnd1];
        vecopnd1.push_back(newnode);
      } else {
        VectorizeExpr(opnd1, tp, vecopnd1, depth + 1);
      }
      if (tp->vecInfo->uniformVecNodes.find(opnd2) != tp->vecInfo->uniformVecNodes.end()) {
        BaseNode *newnode = tp->vecInfo->uniformVecNodes[opnd2];
        vecopnd2.push_back(newnode);
      } else {
        VectorizeExpr(opnd2, tp, vecopnd2, depth + 1);
      }
      CHECK_FATAL(((!vecopnd1.empty()) && !vecopnd2.empty()), "check binary op vectrized node");
      PrimType opnd1PrimType = vecopnd1[0]->GetPrimType();
      PrimType opnd2PrimType = vecopnd2[0]->GetPrimType();
      // widen instruction (addl/subl) need two operands with same vectype
      if ((GetVecElemPrimType(opnd1PrimType) != PTY_begin) &&
          (GetVecElemPrimType(opnd2PrimType) != PTY_begin) &&
          (opnd1PrimType == opnd2PrimType) &&
          (tp->vecInfo->currentLHSTypeSize > GetPrimTypeSize(GetVecElemPrimType(opnd1PrimType))) &&
          CanWidenOpcode(node, GetVecElemPrimType(opnd1PrimType))) {
        GenWidenBinaryExpr(binNode->GetOpCode(), vecopnd1, vecopnd2, vectorizedNode);
      } else if ((PTY_begin != GetVecElemPrimType(opnd2PrimType)) &&
                 (GetPrimTypeSize(GetVecElemPrimType(opnd2PrimType)) >
                 GetPrimTypeSize(GetVecElemPrimType(opnd1PrimType)))) {
        // opnd2 is uniform scalar and type is different from opnd1
        // widen opnd1 with same element type as opnd2
        BaseNode *newopnd1 = vecopnd1[0];
        int opnd2ElemPrimTypeSize = GetPrimTypeSize(GetVecElemPrimType(opnd2PrimType));
        while (GetPrimTypeSize(GetVecElemPrimType(opnd1PrimType)) < opnd2ElemPrimTypeSize) {
          newopnd1 = GenVectorWidenOpnd(newopnd1, opnd1PrimType, false);
          opnd1PrimType = newopnd1->GetPrimType();
        }
        binNode->SetOpnd(newopnd1, 0);
        binNode->SetOpnd(vecopnd2[0], 1);
        binNode->SetPrimType(newopnd1->GetPrimType());
        if (IsCompareNode(binNode->GetOpCode())) {
          static_cast<CompareNode *>(binNode)->SetOpndType(binNode->GetPrimType());
        }
        vectorizedNode.push_back(binNode);
      } else {
        auto lenMax = vecopnd1.size() >= vecopnd2.size() ? vecopnd1.size() : vecopnd2.size();
        for (size_t i = 0; i < lenMax; i++) {
          BinaryNode *newbin = binNode->CloneTree(*codeMPAlloc);
          BaseNode *vecn1 = i < vecopnd1.size() ? vecopnd1[i] : vecopnd1[0];
          BaseNode *vecn2 = i < vecopnd2.size() ? vecopnd2[i] : vecopnd2[0];
          newbin->SetOpnd(vecn1, 0);
          newbin->SetOpnd(vecn2, 1);
          if (GetVecLanes(vecn1->GetPrimType()) > 0) {
            newbin->SetPrimType(vecn1->GetPrimType()); // update primtype of binary op with opnd's type
          } else {
            CHECK_FATAL(GetVecLanes(vecn2->GetPrimType()) > 0, "opnd2 should be vectype since opnd1 is scalar");
            newbin->SetPrimType(vecn2->GetPrimType()); // update primtype of binary op with opnd's type
          }
          // update opndtype
          if (IsCompareNode(newbin->GetOpCode())) {
            static_cast<CompareNode *>(newbin)->SetOpndType(newbin->GetPrimType());
          }
          if (tp->vecInfo->currentLHSTypeSize / GetPrimTypeSize(GetVecElemPrimType(newbin->GetPrimType())) > 2) {
            if (GetPrimTypeSize(newbin->GetPrimType()) == maplebe::k16BitSize) {
              // widen vectorized node to low and high part if newbin is already 128-bit
              IntrinsicopNode *getLowIntrn = GenVectorGetLow(newbin, newbin->GetPrimType());
              IntrinsicopNode *lowNode = GenVectorWidenOpnd(getLowIntrn, getLowIntrn->GetPrimType(), false);
              IntrinsicopNode *highNode = GenVectorWidenOpnd(newbin, getLowIntrn->GetPrimType(), true);
              vectorizedNode.push_back(lowNode);
              vectorizedNode.push_back(highNode);
            } else {
              // widen element type
              IntrinsicopNode *widenop = GenVectorWidenOpnd(newbin, newbin->GetPrimType(), false);
              vectorizedNode.push_back(widenop);
            }
          } else {
            vectorizedNode.push_back(newbin);
          }
        }
      }
      break;
    }
    // unary op
    case OP_abs:
    case OP_neg:
    case OP_bnot:
    case OP_lnot: {
      ASSERT(node->IsUnaryNode(), "should be unarynode");
      UnaryNode *unaryNode = static_cast<UnaryNode *>(node);
      MapleVector<BaseNode *> vecOpnd(localAlloc.Adapter());
      if (tp->vecInfo->uniformVecNodes.find(unaryNode->Opnd(0)) != tp->vecInfo->uniformVecNodes.end()) {
        BaseNode *newnode = tp->vecInfo->uniformVecNodes[unaryNode->Opnd(0)];
        unaryNode->SetOpnd(newnode, 0);
        node->SetPrimType(unaryNode->Opnd(0)->GetPrimType()); // update primtype of unary op with opnd's type
        vectorizedNode.push_back(node);
      } else {
        VectorizeExpr(unaryNode->Opnd(0), tp, vecOpnd, depth + 1);
        CHECK_FATAL(vecOpnd.size() >= 1, "vectorized node should be larger than 1");
        // use abssub to replace subl
        if ((node->GetOpCode() == OP_abs) && (vecOpnd[0]->GetOpCode() == OP_intrinsicop) &&
            ((static_cast<IntrinsicopNode *>(vecOpnd[0]))->GetIntrinsic() >= INTRN_vector_subl_low_v8i8 &&
             (static_cast<IntrinsicopNode *>(vecOpnd[0]))->GetIntrinsic() <= INTRN_vector_subl_high_v2u32)) {
          for (size_t i = 0; i < vecOpnd.size(); i++) {
            IntrinsicopNode *opnd0 = static_cast<IntrinsicopNode *>(vecOpnd[i]);
            PrimType opndPrimType = opnd0->GetPrimType();
            opnd0->SetIntrinsic(GenVectorAbsSublID(opnd0->GetIntrinsic()));
            BaseNode *newopnd = opnd0;
            if ((IsSignedInteger(node->GetPrimType()) && IsUnsignedInteger(opndPrimType)) ||
                (IsUnsignedInteger(node->GetPrimType()) && IsSignedInteger(opndPrimType))) {
              newopnd = ConvertNodeType(IsSignedInteger(node->GetPrimType()), opnd0);
            }
            vectorizedNode.push_back(newopnd);
          }
        } else {
          for (size_t i = 0; i < vecOpnd.size(); i++) {
            UnaryNode *cloneunaryNode = unaryNode->CloneTree(*codeMPAlloc);
            BaseNode *opnd0 = vecOpnd[i];
            PrimType opndPrimType = opnd0->GetPrimType();
            cloneunaryNode->SetOpnd(opnd0, 0);
            // insert cvt to change to sign or unsign
            if ((IsSignedInteger(node->GetPrimType()) && IsUnsignedInteger(opndPrimType)) ||
                (IsUnsignedInteger(node->GetPrimType()) && IsSignedInteger(opndPrimType))) {
              BaseNode *newnode = ConvertNodeType(IsSignedInteger(node->GetPrimType()), opnd0);
              cloneunaryNode->SetOpnd(newnode, 0);
            }
            cloneunaryNode->SetPrimType(cloneunaryNode->Opnd(0)->GetPrimType());
            vectorizedNode.push_back(cloneunaryNode);
          }
        }
      }
      break;
    }
    case OP_dread:
    case OP_constval: {
      if (tp->vecInfo->ivNodes.find(node) != tp->vecInfo->ivNodes.end()) {
        BaseNode *vecnode = tp->vecInfo->ivVecNodes[node];
        vectorizedNode.push_back(vecnode);
      } else {
        // donothing
        vectorizedNode.push_back(node);
      }
      break;
    }
    case OP_select: {
      TernaryNode *tnode = static_cast<TernaryNode *>(node);
      if (tp->vecInfo->uniformVecNodes.find(node) != tp->vecInfo->uniformVecNodes.end()) {
        BaseNode *vecNode = tp->vecInfo->uniformVecNodes[node];
        vectorizedNode.push_back(vecNode);
      } else {
        MapleVector<BaseNode *> vecopnd(localAlloc.Adapter());
        const uint8 tnodeSize = 3;
        for (int i = 0; i < tnodeSize; i++) {
          BaseNode *opnd = tnode->Opnd(i);
          if (tp->vecInfo->uniformVecNodes.find(opnd) != tp->vecInfo->uniformVecNodes.end()) {
            vecopnd.push_back(tp->vecInfo->uniformVecNodes[opnd]);
          } else {
            VectorizeExpr(opnd, tp, vecopnd, depth + 1);
          }
          ASSERT(vecopnd.size() == 1, "NYI::select opnd need expand");
          tnode->SetOpnd(vecopnd[0], i);
          vecopnd.clear();
        }
        // update node type
        PrimType vecType = node->Opnd(1)->GetPrimType();
        node->SetPrimType(vecType);
        vectorizedNode.push_back(node);
      }
      break;
    }
    default:
      ASSERT(0, "can't be vectorized");
  }
  return;
}

// set lhs type to vector type and return lhs pointto type
MIRType *LoopVectorization::VectorizeIassignLhs(IassignNode *iassign, LoopTransPlan *tp) {
  MIRType &mirType = GetTypeFromTyIdx(iassign->GetTyIdx());
  CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
  MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
  MIRType *lhsvecType = GenVecType(ptrType->GetPointedType()->GetPrimType(), tp->vecFactor);
  ASSERT(lhsvecType != nullptr, "vector type should not be null");
  tp->vecInfo->currentLHSTypeSize = GetPrimTypeSize(GetVecElemPrimType(lhsvecType->GetPrimType()));
  MIRType *pvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*lhsvecType, PTY_ptr);
  iassign->SetTyIdx(pvecType->GetTypeIndex());
  return lhsvecType;
}

void LoopVectorization::VectorizeReductionStmt(StmtNode *stmt, LoopTransPlan *tp) {
  MapleVector<BaseNode *> vecOpnd(localAlloc.Adapter());
  PreMeMIRExtension *lfopart = (*PreMeStmtExtensionMap)[stmt->GetStmtID()];
  BlockNode *doloopbody = static_cast<BlockNode *>(lfopart->GetParent());
  RegreadNode *regReadlhsvec = GetorNewVectorReductionVar(stmt, tp);
  tp->vecInfo->currentLHSTypeSize = GetPrimTypeSize(GetVecElemPrimType(regReadlhsvec->GetPrimType()));
  // skip vectorizing uniform node
  // rhsvecNode : vectorizable_expr
  BaseNode *rhsvecNode = stmt->GetRHS()->Opnd(1);
  if (tp->vecInfo->uniformNodes.find(rhsvecNode) == tp->vecInfo->uniformNodes.end()) {
    VectorizeExpr(rhsvecNode, tp, vecOpnd, 0);
  } else {
    // rhs is uniform
    vecOpnd.push_back(rhsvecNode);
  }
  // use widen intrinsic
  if ((GetPrimTypeSize(GetVecElemPrimType(regReadlhsvec->GetPrimType())) * 8 * tp->vecFactor) >
      maxVecSize) {
    BaseNode *currVecNode = nullptr;
    PrimType currVecType;
    for (size_t i = 0; i < vecOpnd.size(); i++) {
      currVecNode = vecOpnd[i];
      currVecType = currVecNode->GetPrimType();
      // need widen
      if (GetPrimTypeSize(GetVecElemPrimType(regReadlhsvec->GetPrimType())) >
          GetPrimTypeSize(GetVecElemPrimType(currVecType))) {
        ASSERT(((GetVecEleSize(regReadlhsvec->GetPrimType())) / GetVecEleSize(currVecType) == 2) &&
               (GetVecLanes(regReadlhsvec->GetPrimType()) * 2 == GetVecLanes(currVecType)), "type check");
        IntrinsicopNode *pairwiseWidenAddIntrn = GenVectorPairWiseAccumulate(regReadlhsvec, currVecNode, currVecType);
        RegassignNode *regassign3 =
            codeMP->New<RegassignNode>(regReadlhsvec->GetPrimType(), regReadlhsvec->GetRegIdx(), pairwiseWidenAddIntrn);
        doloopbody->InsertBefore(stmt, regassign3);
      } else {
        BinaryNode *binaryNode =
            codeMP->New<BinaryNode>(OP_add, regReadlhsvec->GetPrimType(), regReadlhsvec, currVecNode);
        RegassignNode *regassign =
            codeMP->New<RegassignNode>(regReadlhsvec->GetPrimType(), regReadlhsvec->GetRegIdx(), binaryNode);
        doloopbody->InsertBefore(stmt, regassign);
      }
    }
  } else {
    BinaryNode *binaryNode =
        codeMP->New<BinaryNode>(OP_add, regReadlhsvec->GetPrimType(), regReadlhsvec, vecOpnd[0]);
    RegassignNode *regassign1 =
        codeMP->New<RegassignNode>(regReadlhsvec->GetPrimType(), regReadlhsvec->GetRegIdx(), binaryNode);
    doloopbody->InsertBefore(stmt, regassign1);
  }
  // red = red +/- sum_vec(redvec)
  StmtNode *copyStmt = stmt->CloneTree(*codeMPAlloc);
  IntrinsicopNode *intrnvecSum = GenSumVecStmt(regReadlhsvec, regReadlhsvec->GetPrimType());
  copyStmt->GetRHS()->SetOpnd(intrnvecSum, 1);
  tp->vecInfo->afterLoopStmts.push_back(copyStmt);
  doloopbody->RemoveStmt(stmt);
}

// iterate tree node to vectorize scalar type to vector type
// following opcode can be vectorized directly
//  +, -, *, &, |, <<, >>, compares, ~, !
// iassign, iread, dassign, dread
void LoopVectorization::VectorizeStmt(BaseNode *node, LoopTransPlan *tp) {
  if (enableDebug) {
    node->Dump(0);
  }
  switch (node->GetOpCode()) {
    case OP_iassign: {
      IassignNode *iassign = static_cast<IassignNode *>(node);
      // node is reduction stmt
      if (tp->vecInfo->reductionStmts.find(iassign) != tp->vecInfo->reductionStmts.end()) {
        VectorizeReductionStmt(static_cast<StmtNode *>(node), tp);
      } else {
        MIRType *lhsptvecType = VectorizeIassignLhs(iassign, tp);
        BaseNode *rhs = iassign->GetRHS();
        BaseNode *newrhs;
        if (tp->vecInfo->uniformVecNodes.find(rhs) != tp->vecInfo->uniformVecNodes.end()) {
          // rhs replaced scalar node with vector node
          newrhs = tp->vecInfo->uniformVecNodes[rhs];
          if (GetPrimTypeSize(GetVecElemPrimType(newrhs->GetPrimType())) < tp->vecInfo->currentLHSTypeSize) {
            newrhs = GenVectorWidenOpnd(newrhs, newrhs->GetPrimType(), false);
          }
        } else {
          MapleVector<BaseNode *> vecRhs(localAlloc.Adapter());
          VectorizeExpr(rhs, tp, vecRhs, 0);
          ASSERT(vecRhs.size() == 1, "iassign doesn't handle complex type cvt now");
          // insert CVT if lsh type is not same as rhs type
          newrhs = vecRhs[0];
        }
        // if need cvt for sign/unsign
        if ((IsSignedInteger(lhsptvecType->GetPrimType()) && IsUnsignedInteger(newrhs->GetPrimType())) ||
            (IsUnsignedInteger(lhsptvecType->GetPrimType()) && IsUnsignedInteger(newrhs->GetPrimType()))) {
          newrhs = ConvertNodeType(IsSignedInteger(lhsptvecType->GetPrimType()), newrhs);
        }
        iassign->SetRHS(newrhs);
      }
      break;
    }
    // scalar related: widen type directly or unroll instructions
    case OP_dassign: {
      // handle reduction stmt
      // sum = sum +/- vectorizable_expr
      // =>
      // Example: vec t1 = dup_scalar(0);
      // Example: doloop {
      // Example:   t1 = t1 + vectorized_node;
      // Example: }
      //  sum = sum +/- intrinsic_op vec_sum(t1)
      StmtNode *dassign = static_cast<StmtNode *>(node);
      ASSERT(tp->vecInfo->reductionStmts.find(dassign) != tp->vecInfo->reductionStmts.end(),
             "dassign should be reduction stmt");
      VectorizeReductionStmt(dassign, tp);
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
      // remove equal condition included in compare
      // updated upper node consider equal condition
      if (cmpn->GetOpCode() == OP_le) {
        cmpn->SetOpCode(OP_lt);
      } else if (cmpn->GetOpCode() == OP_ge) {
        cmpn->SetOpCode(OP_gt);
      }
      cmpn->SetBOpnd(tp->vBound->upperNode, 1);
    }
  }
}


void LoopVectorization::VectorizeDoLoop(DoloopNode *doloop, LoopTransPlan *tp) {
  // LogInfo::MapleLogger() << "\n**** dump doloopnode ****\n";
  // step 1: handle loop low/upper/stride
  widenDoloop(doloop, tp);

  // step 2: insert dup stmt before doloop
  if ((!tp->vecInfo->uniformNodes.empty()) ||
      (!tp->vecInfo->ivNodes.empty())) {
    PreMeMIRExtension *lfopart = (*PreMeStmtExtensionMap)[doloop->GetStmtID()];
    BaseNode *parent = lfopart->GetParent();
    ASSERT(parent && (parent->GetOpCode() == OP_block), "nullptr check");
    BlockNode *pblock = static_cast<BlockNode *>(parent);
    auto it = tp->vecInfo->uniformNodes.begin();
    for (; it != tp->vecInfo->uniformNodes.end(); ++it) {
      BaseNode *node = *it;
      PrimType ptype =  (node->GetOpCode() == OP_iread) ?
          (static_cast<IreadNode *>(node))->GetType()->GetPrimType() : node->GetPrimType();
      if (tp->vecInfo->constvalTypes.count(node) > 0) {
        if (ptype != tp->vecInfo->constvalTypes[node]) {
          node->SetPrimType(tp->vecInfo->constvalTypes[node]);
        }
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
    if (!tp->vecInfo->ivNodes.empty()) {
      // initconst array -> [0, 1, 2... veclanes-1]
      // vtype reg <- iread array + initval
      // {
      //   reg <- reg + veclanes
      // }
      MIRType *elemType = static_cast<MIRArrayType *>(tp->vecInfo->ivConstArraySym->GetType())->GetElemType();
      MIRType *vecType = GenVecType(elemType->GetPrimType(), tp->vecLanes);
      MIRType *ptvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType);
      PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(vecType->GetPrimType());
      BaseNode *addr = codeMP->New<AddrofNode>(OP_addrof, PTY_ptr, tp->vecInfo->ivConstArraySym->GetStIdx(), 0);
      IreadNode *ireadNode = codeMP->New<IreadNode>(OP_iread, vecType->GetPrimType(),
                                                    ptvecType->GetTypeIndex(), 0, addr);
      BaseNode *rhs = ireadNode;
      if ((!doloop->GetStartExpr()->IsConstval()) ||
          (static_cast<MIRIntConst *>(static_cast<ConstvalNode *>(doloop->GetStartExpr())->GetConstVal())
              ->GetValue() != 0)) {
        rhs = codeMP->New<BinaryNode>(OP_add, vecType->GetPrimType(), rhs, doloop->GetStartExpr());
      }
      RegassignNode *ivInitStmt = codeMP->New<RegassignNode>(vecType->GetPrimType(), regIdx, rhs);
      pblock->InsertBefore(doloop, ivInitStmt);
      RegreadNode *regreadNode = codeMP->New<RegreadNode>(vecType->GetPrimType(), regIdx);
      it = tp->vecInfo->ivNodes.begin();
      for (; it != tp->vecInfo->ivNodes.end(); ++it) {
        tp->vecInfo->ivVecNodes[*it] = regreadNode;
      }
      // new increment stmt
      BaseNode *incrNode = tp->vBound->incrNode;
      if (GetPrimTypeSize(incrNode->GetPrimType()) != GetPrimTypeSize(elemType->GetPrimType())) {
        ConstvalNode *constnode = static_cast<ConstvalNode *>(incrNode);
        MIRIntConst *incrconst = static_cast<MIRIntConst *>(constnode->GetConstVal());
        MIRIntConst *newConst =
            GlobalTables::GetIntConstTable().GetOrCreateIntConst(incrconst->GetExtValue(), *elemType);
        incrNode = codeMP->New<ConstvalNode>(elemType->GetPrimType(), newConst);
      }
      BaseNode *incrhs = codeMP->New<BinaryNode>(OP_add, vecType->GetPrimType(), regreadNode, incrNode);
      tp->vecInfo->ivvecIncrStmt = codeMP->New<RegassignNode>(vecType->GetPrimType(), regIdx, incrhs);
    }
  }
  // step 3: widen vectorizable stmt in doloop
  BlockNode *loopbody = doloop->GetDoBody();
  for (auto &stmt : loopbody->GetStmtNodes()) {
    if (tp->vecInfo->vecStmtIDs.count(stmt.GetStmtID()) > 0) {
      VectorizeStmt(&stmt, tp);
    } else {
      // stmt could not be widen directly, unroll instruction with vecFactor
      // move value from vector type if need (need def-use information from plan)
      CHECK_FATAL(0, "NIY:: unvectorized stmt");
    }
  }
  // step 4: insert stmts before/after doloop
  if (!tp->vecInfo->beforeLoopStmts.empty() ||
      !tp->vecInfo->afterLoopStmts.empty()) {
    PreMeMIRExtension *lfopart = (*PreMeStmtExtensionMap)[doloop->GetStmtID()];
    BaseNode *parent = lfopart->GetParent();
    ASSERT(parent && (parent->GetOpCode() == OP_block), "nullptr check");
    BlockNode *pblock = static_cast<BlockNode *>(parent);
    for (auto *stmtNode : tp->vecInfo->beforeLoopStmts) {
      pblock->InsertBefore(doloop, stmtNode);
    }
    for (auto *stmtNode : tp->vecInfo->afterLoopStmts) {
      pblock->InsertAfter(doloop, stmtNode);
    }
  }
  if (tp->vecInfo->ivvecIncrStmt) {
    loopbody->InsertLast(tp->vecInfo->ivvecIncrStmt);
  }
}

// generate remainder loop
DoloopNode *LoopVectorization::GenEpilog(DoloopNode *doloop) const {
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
    PreMeMIRExtension *lfoInfo = (*PreMeStmtExtensionMap)[doloop->GetStmtID()];
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
  for (; it != vecPlans.end(); ++it) {
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
    case OP_iaddrof:
    case OP_addrof: {
      if (isArraySub) {
        return true;
      }
      if (x->GetOpCode() == OP_constval) {
        vecInfo->uniformNodes.insert(x);
        return true;
      }
      if (doloopInfo->IsLoopInvariant2(x)) {
        // no vectorize if rhs operands types are not same
        if (!vecInfo->UpdateRHSTypeSize(x->GetPrimType())) {
          return false;
        }
        vecInfo->uniformNodes.insert(x);
        return true;
      }
      // check scalar is induction variable
      if (x->GetOpCode() == OP_dread &&
          doloopInfo->IsLoopIVNode(x)) {
        vecInfo->ivNodes.insert(x);
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
    case OP_bxor:
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
      bool r0Uniform = doloopInfo->IsLoopInvariant2(x->Opnd(0));
      bool r1Uniform = doloopInfo->IsLoopInvariant2(x->Opnd(1));
      bool isvectorizable = false;
      if (r0Uniform && r1Uniform) {
        PreMeMIRExtension* lfopart = (*PreMeExprExtensionMap)[x];
        CHECK_FATAL(lfopart, "nullptr check");
        BaseNode *parent = lfopart->GetParent();
        if (parent && parent->GetOpCode() == OP_array && x == parent->Opnd(0)) {
          return true;
        }
        if (!isArraySub && !vecInfo->UpdateRHSTypeSize(x->GetPrimType())) {
          // no vectorize if rhs operands types are not same
          return false;
        } else if (!isArraySub) {
          vecInfo->uniformNodes.insert(x);
        }
        return true;
      } else if ((!isArraySub) && r0Uniform) {
        vecInfo->uniformNodes.insert(x->Opnd(0));
        isvectorizable = ExprVectorizable(doloopInfo, vecInfo, x->Opnd(1));
      } else if ((!isArraySub) && r1Uniform) {
        vecInfo->uniformNodes.insert(x->Opnd(1));
        isvectorizable = ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0));
        PrimType opnd1Type = x->Opnd(1)->GetPrimType();
        // update constval type if needed
        if (isvectorizable && (x->Opnd(1)->GetOpCode() == OP_constval)) {
          PrimType targetType;
          if (vecInfo->currentRHSTypeSize != 0 && vecInfo->currentRHSTypeSize < GetPrimTypeSize(opnd1Type)) {
            targetType = vecInfo->currentRHSTypeSize == 32 ? PTY_i32 :
                (vecInfo->currentRHSTypeSize == 16 ? PTY_i16 : PTY_i8);
          } else {
            targetType = x->Opnd(0)->GetPrimType();
          }
          if (GetPrimTypeSize(targetType) != GetPrimTypeSize(opnd1Type) &&
              CanAdjustRhsConstType(targetType, static_cast<ConstvalNode *>(x->Opnd(1)))) {
            vecInfo->constvalTypes[x->Opnd(1)] = targetType;
          }
        }
      } else if (ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0)) && ExprVectorizable(doloopInfo, vecInfo,
          x->Opnd(1))) {
        isvectorizable = true;
      }
      if (vecInfo->widenop > 0) {
        vecInfo->widenop = vecInfo->widenop << 1;
      }
      return isvectorizable;
    }
    // supported unary ops
    case OP_bnot:
    case OP_lnot:
    case OP_neg:
    case OP_abs: {
      bool r0Uniform = doloopInfo->IsLoopInvariant2(x->Opnd(0));
      if ((!isArraySub) && r0Uniform) {
        vecInfo->uniformNodes.insert(x->Opnd(0));
        return true;
      }
      return ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0));
    }
    case OP_cvt: {
      // skip cvt in array subscript
      return isArraySub;
    }
    case OP_iread: {
      IreadNode* ireadnode = static_cast<IreadNode *>(x);
      MIRType *mirType = ireadnode->GetType();
      if (GetPrimTypeSize(ireadnode->GetPrimType()) > GetPrimTypeSize(mirType->GetPrimType())) {
        vecInfo->widenop = (vecInfo->widenop | 1);
      }
      if ((!ireadnode->IsVolatile()) &&
          (ireadnode->Opnd(0)->GetOpCode() == OP_array) &&
          (doloopInfo->IsLoopInvariant2(ireadnode->Opnd(0)))) {
        if (vecInfo->UpdateRHSTypeSize(mirType->GetPrimType()) &&
          IsPrimitiveInteger(mirType->GetPrimType())) {
          vecInfo->uniformNodes.insert(x);
          return true;
        }
        return false;
      }
      if (isArraySub) {
        return false; // iread is not constant value and used in array subscript
      }
      bool canVec = ExprVectorizable(doloopInfo, vecInfo, x->Opnd(0));
      if (canVec) {
        if (!vecInfo->UpdateRHSTypeSize(mirType->GetPrimType()) ||
            (ireadnode->GetFieldID() != 0)) {
          canVec = false; // skip if rhs type is not consistent
        }
      }
      return canVec;
    }
    // supported n-ary ops
    case OP_array: {
      for (size_t i = 0; i < x->NumOpnds(); i++) {
        isArraySub = true;
        if (!ExprVectorizable(doloopInfo, vecInfo, x->Opnd(i))) {
          isArraySub = false;
          return false;
        }
        isArraySub = false;
      }
      return true;
    }
    // select
    case OP_select : {
      if (isArraySub) {
        return false;
      }
      if (doloopInfo->IsLoopInvariant2(x)) {
        vecInfo->uniformNodes.insert(x);
      }
      for (size_t i = 0; i < x->NumOpnds(); i++) {
        if (!ExprVectorizable(doloopInfo, vecInfo, x->Opnd(i))) {
          return false;
        }
      }
      //TODO:: enable OP_select after cg support vector type in OP_select
      return false;
    }
    default: ;
  }
  return false;
}

// only handle one level convert
// <short, int> or <int, short> pairs
bool LoopVectorization::CanConvert(uint32_t lshtypeSize, uint32_t rhstypeSize) const {
  if (lshtypeSize >= rhstypeSize) {
    return ((lshtypeSize / rhstypeSize) <= 2);
  }
  // skip narrow case : lhs is small than rhs
  return false;
}

bool LoopVectorization::CanAdjustRhsConstType(PrimType targetType, ConstvalNode *rhs) {
  MIRIntConst *intConst = static_cast<MIRIntConst*>(rhs->GetConstVal());
  int64 v = intConst->GetExtValue();
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

// handle iassign lhs ivar is constant and same as opnd0 of rhs
// a[3] += vectorizable(opnd1)
// a better way is to promote a[3] as a scalar before premeemit
// scalar will be identified as reduction variable
bool LoopVectorization::IassignIsReduction(IassignNode *iassign, LoopVecInfo* vecInfo) {
  if (IsReductionOp(iassign->GetRHS()->GetOpCode()) &&
      ((static_cast<BinaryNode *>(iassign->GetRHS()))->GetBOpnd(0)->GetOpCode() == OP_iread)) {
    auto *stmtP = (*PreMeStmtExtensionMap)[iassign->GetStmtID()];
    IassignMeStmt *iassMeStmt = static_cast<IassignMeStmt *>(stmtP->GetMeStmt());
    IvarMeExpr *ivarlhsExpr = iassMeStmt->GetLHSVal();
    auto *rhsOpnd0 = (*PreMeExprExtensionMap)[(static_cast<BinaryNode *>(iassign->GetRHS()))->GetBOpnd(0)];
    BaseNode *rhsOpnd1 = iassign->GetRHS()->Opnd(1);
    PrimType lhsmirType = (static_cast<MIRPtrType*>(&GetTypeFromTyIdx(iassign->GetTyIdx())))
        ->GetPointedType()->GetPrimType();
    PrimType rhsOpnd1Type = rhsOpnd1->GetPrimType();
    if (ivarlhsExpr && rhsOpnd0 && rhsOpnd0->GetMeExpr() &&
        (rhsOpnd0->GetMeExpr()->GetMeOp() == maple::kMeOpIvar)) {
      IvarMeExpr *ivarrhs0Expr = static_cast<IvarMeExpr *>(rhsOpnd0->GetMeExpr());
      if ((ivarlhsExpr->GetBase() == ivarrhs0Expr->GetBase()) &&
          (lhsmirType == rhsOpnd1Type)) {
        vecInfo->vecStmtIDs.insert((iassign)->GetStmtID());
        vecInfo->reductionStmts.insert(iassign);
        return true;
      }
    }
  }
  return false;
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
        bool lhsUseAsScalar = false;
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
              accessDesc->subscriptVec[dim - 1]->coeff != 1) {
            if (enableDebug) {
              LogInfo::MapleLogger() << "NOT VECTORIZABLE because of complex array subscript\n";
            }
            return false;
          } else if (accessDesc->subscriptVec[dim - 1]->loopInvariant) {
            lhsUseAsScalar = true;
          }
        }
        // check rsh
        bool canVec = ExprVectorizable(doloopInfo, vecInfo, iassign->GetRHS());
        if (!canVec) {
          // early return
          if (enableDebug) {
            LogInfo::MapleLogger() << "NOT VECTORIZABLE because of RHS is not vectorizable\n";
          }
          return false;
        }
        if (iassign->GetFieldID() != 0) {  // check base of iassign
          canVec = doloopInfo->IsLoopInvariant2(iassign->Opnd(0));
          if (!canVec) {
            if (enableDebug) {
              LogInfo::MapleLogger() << "NOT VECTORIZABLE because of baseaddress is not const with non-zero filedID\n";
            }
            return false;
          }
        }
        // check lhs is used like reduction variable
        if (lhsUseAsScalar && (!IassignIsReduction(iassign, vecInfo))) {
          if (enableDebug) {
            LogInfo::MapleLogger() << "NOT VECTORIZABLE because of lhs is scalar\n";
          }
          return false;
        }
        // check type
        {
          MIRType &mirType = GetTypeFromTyIdx(iassign->GetTyIdx());
          CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
          MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
          PrimType stmtpt = ptrType->GetPointedType()->GetPrimType();
          if (!IsPrimitiveInteger(stmtpt)) {
             // iassign ptr type should be integer now
            return false;
          }
          // now check lsh type size should be same as rhs typesize
          uint32_t lshtypesize = GetPrimTypeSize(stmtpt) * 8;
          if (iassign->GetRHS()->IsConstval() &&
              (stmtpt != iassign->GetRHS()->GetPrimType()) &&
              CanAdjustRhsConstType(stmtpt, static_cast<ConstvalNode *>(iassign->GetRHS()))) {
            vecInfo->constvalTypes[iassign->GetRHS()] = stmtpt;
            (void)vecInfo->UpdateRHSTypeSize(stmtpt);
          } else if (iassign->GetRHS()->GetOpCode() != OP_iread) {
            // iread will update rhs type in exprvectorizable
            (void)vecInfo->UpdateRHSTypeSize(iassign->GetRHS()->GetPrimType());
          }
          if (!CanConvert(lshtypesize, vecInfo->currentRHSTypeSize)) {
            // use one cvt could handle the type difference
            if (enableDebug) {
              LogInfo::MapleLogger() << "NOT VECTORIZABLE because of lhs and rhs type different\n";
            }
            return false; // need cvt instruction
          }
          // rsh is loop invariant
          if (doloopInfo->IsLoopInvariant2(iassign->GetRHS())) {
            vecInfo->uniformNodes.insert(iassign->GetRHS());
          }
          vecInfo->vecStmtIDs.insert((stmt)->GetStmtID());
          // update largest type size
          uint32_t maxSize = vecInfo->currentRHSTypeSize > lshtypesize ?
              vecInfo->currentRHSTypeSize : lshtypesize;
          vecInfo->UpdateWidestTypeSize(maxSize);
          // update smallest type
          if (lshtypesize < vecInfo->smallestTypeSize) {
            vecInfo->smallestTypeSize = lshtypesize;
          }
        }
        break;
      }
      case OP_dassign: {
        DassignNode *dassign = static_cast<DassignNode *>(stmt);
        StIdx lhsStIdx = dassign->GetStIdx();
        MIRSymbol *lhsSym = mirFunc->GetLocalOrGlobalSymbol(lhsStIdx);
        MIRType &lhsType = GetTypeFromTyIdx(lhsSym->GetTyIdx());
        BaseNode *rhs = dassign->GetRHS();
        uint32_t lhstypesize = GetPrimTypeSize(lhsType.GetPrimType()) * 8;
        if (IsReductionOp(rhs->GetOpCode()) && doloopInfo->IsReductionVar(lhsStIdx)) {
          BaseNode *opnd0 = rhs->Opnd(0);
          BaseNode *opnd1 = rhs->Opnd(1);
          CHECK_FATAL((opnd0->GetOpCode() == OP_dread) && ((static_cast<AddrofNode *>(opnd0))->GetStIdx() == lhsStIdx),
                      "opnd0 is reduction variable");
          if (ExprVectorizable(doloopInfo, vecInfo, opnd1)) {
            // there's iread in rhs
            if (vecInfo->currentRHSTypeSize != 0) {
              if (lhstypesize < vecInfo->currentRHSTypeSize ||
                  vecInfo->currentRHSTypeSize < 8) { // rsh typs size is less than i8/u8
                // narrow down the result, not handle now
                if (enableDebug) {
                  LogInfo::MapleLogger() <<
                     "NOT VECTORIZABLE because of different type between dassign reduction var and other opnd \n";
                }
                return false;
              }
              // return false if widen op in nested binary operation
              if (vecInfo->widenop >= 4) {
                return false;
              }
              vecInfo->UpdateWidestTypeSize(vecInfo->currentRHSTypeSize);
            } else {
              vecInfo->UpdateWidestTypeSize(lhstypesize);
            }
            vecInfo->vecStmtIDs.insert((stmt)->GetStmtID());
            vecInfo->reductionStmts.insert(stmt);
          } else {
            if (enableDebug) {
              LogInfo::MapleLogger() << "NOT VECTORIZABLE because of other opnd can't be vectorized\n";
            }
            return false; // only handle reduction scalar
          }
        } else {
          if (enableDebug) {
            LogInfo::MapleLogger() << "NOT VECTORIZABLE because of dassign variable is not reduction var\n";
          }
          return false;
        }
        break;
      }
      default: return false;
    }
    vecInfo->widenop = 0; // reset value
    stmt = stmt->GetNext();
  }
  return true;
}

// create constant node if need in vectorization
void LoopVectorization::GenConstVar(LoopVecInfo *vecInfo, uint8_t vecLanes) {
  // create zero node used in reduction stmt
  if ((!const0Node) && (!vecInfo->reductionStmts.empty())) {
    PrimType ptype = vecLanes == 4 ? PTY_i32 : (vecLanes == 2 ? PTY_i64 : PTY_i16);
    MIRType &typeInt = *GlobalTables::GetTypeTable().GetPrimType(ptype);
    MIRIntConst *constZero = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, typeInt);
    const0Node = codeMP->New<ConstvalNode>(ptype, constZero);
  }
  MIRSymbol *ivconstSym = (vecLanes == 4) ? initIVv4Sym : (vecLanes == 8 ? initIVv8Sym : initIVv2Sym);
  if ((!ivconstSym) && (!vecInfo->ivNodes.empty())) {
    std::string ivVecName("__ivvec");
    ivVecName.append(std::to_string(vecLanes));
    // add file name in case new constant array created in multi modules
    MIRModule *mirModule = mirFunc->GetModule();
    const std::string fileName = mirModule->GetFileName();
    std::string::size_type lastDot = fileName.find_last_of('.');
    std::string::size_type lastSlash = fileName.find_last_of('/');
    if (lastSlash != std::string::npos &&
        lastDot != std::string::npos) {
      ivVecName.append("_" + fileName.substr(lastSlash + 1, lastDot));
    } else if (lastSlash == std::string::npos) {
      ivVecName.append("_" + fileName.substr(0, lastDot));
    }
    ivVecName.erase(std::remove(ivVecName.begin(), ivVecName.end(), '-'), ivVecName.end());
    PrimType type = (vecLanes == 4) ? PTY_i32 : ((vecLanes == 8) ? PTY_i16 : PTY_i64);
    MIRType *elemType = GlobalTables::GetTypeTable().GetPrimType(type);
    MIRArrayType *arrayType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, 0);
    MIRAggConst *constval = mirModule->GetMemPool()->New<MIRAggConst>((*mirModule), *arrayType);
    for (uint32_t i = 0; i < vecLanes; i++) {
      MIRIntConst *intconst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(i, *elemType);
      constval->AddItem(intconst, 0);
    }
    ivconstSym = mirModule->GetMIRBuilder()->GetOrCreateGlobalDecl(ivVecName, *arrayType);
    ASSERT_NOT_NULL(ivconstSym);
    ivconstSym->SetKonst(constval);
  }
  vecInfo->ivConstArraySym = ivconstSym;
}

void LoopVectorization::Perform() {
  // step 2: collect information, legality check and generate transform plan
  MapleMap<DoloopNode *, DoloopInfo *>::iterator mapit = depInfo->doloopInfoMap.begin();
  for (; mapit != depInfo->doloopInfoMap.end(); ++mapit) {
    if (!mapit->second->children.empty() || mapit->second->NotParallel()) {
      continue;
    }
    // check in debug
    if (LoopVectorization::vectorizedLoop >= MeOption::vecLoopLimit) {
      break;
    }
    LoopVecInfo *vecInfo = localMP->New<LoopVecInfo>(localAlloc);
    if (mapit->second->HasTrueDepOnly() > 0) {
      vecInfo->minTrueDepDist = mapit->second->HasTrueDepOnly();
    }
    if (mapit->second->HasAntiDepOnly() < 0) {
      vecInfo->maxAntiDepDist = mapit->second->HasAntiDepOnly();
    }
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
    if (tplan->Generate(mapit->first, mapit->second, enableDebug)) {
      vecPlans[mapit->first] = tplan;
      GenConstVar(vecInfo, tplan->vecLanes);
      mapit->second->hasBeenVectorized = true;
    }
  }
  // step 3: do transform
  // transform plan map to each doloop
  TransformLoop();
}
}  // namespace maple
