/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#include "me_verify.h"
#include "me_cfg.h"
#include "me_dominance.h"

namespace maple {
bool MeVerify::enableDebug = false;

void MeVerify::VerifyFunction() {
  if (enableDebug) {
    LogInfo::MapleLogger() << meFunc.GetName() << "\n";
  }
  VerifyCommonExitBB();
  uint64 i = 0;
  for (auto &bb : meFunc.GetLaidOutBBs()) {
    ++i;
    if (bb->GetSucc().empty() && bb->GetPred().empty()) {
      continue;
    }
    if (bb->GetPred().empty() && !bb->GetAttributes(kBBAttrIsEntry)) {
      continue;
    }
    VerifyBBKind(*bb);
    VerifySuccAndPredOfBB(*bb);
    if (bb->GetAttributes(kBBAttrIsTry) && !bb->GetMeStmts().empty() &&
        bb->GetMeStmts().front().GetOp() == OP_try) {
      VerifyAttrTryBB(*bb, i - 1);
    }
  }
}

void DealWithPoninterType(const MIRType &type, std::string &str);

void AddTypeKindToStr(const MIRType &type, std::string &str) {
  if (type.GetKind() != kTypeClass && type.GetKind() != kTypeStruct && type.GetKind() != kTypeInterface &&
      type.GetKind() != kTypeClassIncomplete && type.GetKind() != kTypeStructIncomplete &&
      type.GetKind() != kTypeInterfaceIncomplete) {
    str += "_" + std::to_string(type.GetKind());
  }
}

void DealWithFuncType(const MIRType &type, std::string &str) {
  auto funcType = static_cast<const MIRFuncType&>(type);
  str += "_" + std::to_string(funcType.GetRetTyIdx().GetIdx());
  str += "_" + std::to_string(funcType.IsVarargs());
  for (auto &item : funcType.GetParamTypeList()) {
    str += "_" + std::to_string(item.GetIdx());
  }
  for (auto &item : funcType.GetParamAttrsList()) {
    str += "_" + std::to_string(item.GetAttrFlag()) + std::to_string(item.GetAlignValue());
  }
}

void GetAttrOfType(const MIRType &type, std::string &str) {
  str += "_" + std::to_string(static_cast<int>(type.GetPrimType()));
  AddTypeKindToStr(type, str);
  switch (type.GetKind()) {
    case kTypePointer: {
      DealWithPoninterType(type, str);
      break;
    }
    case kTypeFArray:
    case kTypeJArray: {
      auto farrayType = static_cast<const MIRFarrayType&>(type);
      GetAttrOfType(*farrayType.GetElemType(), str);
      break;
    }
    case kTypeArray: {
      auto arryType = static_cast<const MIRArrayType&>(type);
      for (uint32 i = 0; i < arryType.GetDim(); ++i) {
        str += "_" + std::to_string(arryType.GetSizeArrayItem(i));
        break;
      }
      GetAttrOfType(*arryType.GetElemType(), str);
      break;
    }
    case kTypeBitField: {
      str += "_" + std::to_string(static_cast<const MIRBitFieldType&>(type).GetFieldSize());
      break;
    }
    case kTypeFunction: {
      DealWithFuncType(type, str);
      break;
    }
    case kTypeInstantVector: {
      for (auto &item : static_cast<const MIRInstantVectorType&>(type).GetInstantVec()) {
        str += "_" + std::to_string(item.first.GetIdx());
        str += "_" + std::to_string(item.second.GetIdx());
      }
      break;
    }
    case kTypeGenericInstant: {
      str += "_" + std::to_string(static_cast<const MIRGenericInstantType&>(type).GetGenericTyIdx());
      break;
    }
    case kTypeByName: {
      str += "_" + std::to_string(type.IsNameIsLocal());
      break;
    }
    default:
      break;
  }
}

void DealWithPoninterType(const MIRType &type, std::string &str) {
  auto ptrType = static_cast<const MIRPtrType&>(type);
  str += "_" + std::to_string(ptrType.GetTypeAttrs().GetAlignValue()) + "_" +
         std::to_string(ptrType.GetTypeAttrs().GetAttrFlag());
  GetAttrOfType(*ptrType.GetPointedType(), str);
}

// The type must exited in typeHashTable, ptrTypeTable or refTypeTable.
bool FindTypeInOtherTable(MIRType &mirType) {
  const auto &typeHashTable = GlobalTables::GetTypeTable().GetTypeHashTable();
  const auto &ptrTypeTable = GlobalTables::GetTypeTable().GetPtrTypeMap();
  const auto &refTypeTable = GlobalTables::GetTypeTable().GetRefTypeMap();
  if (typeHashTable.find(&mirType) != typeHashTable.end()) {
    return true;
  }
  bool inTypeHash = false;
  for (auto &iter : typeHashTable) {
    if (iter->GetTypeIndex() == mirType.GetTypeIndex()) {
      inTypeHash = true;
    }
  }
  if (inTypeHash) {
    return true;
  }
  if (mirType.IsMIRPtrType() &&
      ptrTypeTable.find(static_cast<const MIRPtrType&>(mirType).GetPointedTyIdx()) != ptrTypeTable.end()) {
    return true;
  }
  if (mirType.IsMIRPtrType() &&
      refTypeTable.find(static_cast<const MIRPtrType&>(mirType).GetPointedTyIdx()) != refTypeTable.end()) {
    return true;
  }
  return false;
}

void FindTypeInTypeTable() {
  const auto &typeHashTable = GlobalTables::GetTypeTable().GetTypeHashTable();
  const auto &ptrTypeTable = GlobalTables::GetTypeTable().GetPtrTypeMap();
  const auto &refTypeTable = GlobalTables::GetTypeTable().GetRefTypeMap();
  std::set<TyIdx> tyIdxs;
  for (auto &iter : typeHashTable) {
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(iter->GetTypeIndex()) == nullptr) {
      CHECK_FATAL(false, "%s %d is not exit in typeTable",
                  iter->GetMplTypeName().c_str(), iter->GetTypeIndex().GetIdx());
    }
    size_t size = tyIdxs.size();
    tyIdxs.insert(iter->GetTypeIndex());
    if (tyIdxs.size() == size) {
      CHECK_FATAL(false, "%s %d repeated in typeHashTable",
                  iter->GetMplTypeName().c_str(), iter->GetTypeIndex().GetIdx());
    }
  }
  tyIdxs.clear();
  for (auto &iter : ptrTypeTable) {
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(iter.first) == nullptr) {
      CHECK_FATAL(false, "%d is not exit in typeTable", iter.first.GetIdx());
    }
    size_t size = tyIdxs.size();
    tyIdxs.insert(iter.first);
    if (tyIdxs.size() == size) {
      CHECK_FATAL(false, "%d repeated in ptrTypeTable", iter.first.GetIdx());
    }
  }
  tyIdxs.clear();
  for (auto &iter : refTypeTable) {
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(iter.first) == nullptr) {
      CHECK_FATAL(false, "%d is not exit in typeTable", iter.first.GetIdx());
    }
    size_t size = tyIdxs.size();
    tyIdxs.insert(iter.first);
    if (tyIdxs.size() == size) {
      CHECK_FATAL(false, "%d is repeated in refTypeTable", iter.first.GetIdx());
    }
  }
}

void VerifyGlobalTypeTable() {
  const auto &typeTable = GlobalTables::GetTypeTable().GetTypeTable();
  const auto &typeHashTable = GlobalTables::GetTypeTable().GetTypeHashTable();
  const auto &ptrTypeTable = GlobalTables::GetTypeTable().GetPtrTypeMap();
  const auto &refTypeTable = GlobalTables::GetTypeTable().GetRefTypeMap();
  FindTypeInTypeTable();
  auto sizeOfMIRTypes = typeHashTable.size() + ptrTypeTable.size() + refTypeTable.size();
  // nullptr and struct mirtype classmeta in typetable but not in other tables.
  if (sizeOfMIRTypes != typeTable.size() - 1) {
    if (MeVerify::enableDebug) {
      LogInfo::MapleLogger() << typeTable.size() << " " << typeHashTable.size() << " " << ptrTypeTable.size() << " " <<
          refTypeTable.size() << " " << (typeHashTable.size() + ptrTypeTable.size() + refTypeTable.size()) << "\n";
    }
    CHECK_FATAL(false, "verify failed, sizeOfMIRTypes: %d, typeTable.size(): %d", sizeOfMIRTypes, typeTable.size());
  }
  int i = 0;
  std::set<std::string> primTypeAndTypeName;
  std::set<TyIdx> tyIdxs;
  for (auto &mirType : typeTable) {
    if (mirType == nullptr) {
      continue;
    }
    size_t idx = tyIdxs.size();
    tyIdxs.insert(mirType->GetTypeIndex());
    CHECK_FATAL(idx != tyIdxs.size(), "the same tyidx in type table");
    size_t size = primTypeAndTypeName.size();
    std::string currName = mirType->GetMplTypeName();
    GetAttrOfType(*mirType, currName);
    primTypeAndTypeName.insert(currName);
    if (MeVerify::enableDebug) {
      LogInfo::MapleLogger() << currName << "\n";
    }
    if (size == primTypeAndTypeName.size()) {
      ++i;
      CHECK_FATAL(false, "the type has been created %s %d", currName.c_str(), mirType->GetTypeIndex().GetIdx());
    }
    if (FindTypeInOtherTable(*mirType)) {
      continue;
    }
    if (mirType->GetMplTypeName().find(namemangler::kClassMetadataTypeName) != std::string::npos) {
      continue;
    }
    CHECK_FATAL(false, "%d %s is in typetable,", mirType->GetTypeIndex().GetIdx(), mirType->GetMplTypeName().c_str());
  }
  // the first elem of type table is nullptr
  CHECK_FATAL(tyIdxs.size() == typeTable.size() - 1, "the size of tyIdxs is not equal to typeTable");
  if (MeVerify::enableDebug) {
    LogInfo::MapleLogger() << i << "\n";
  }
}

void MeVerify::VerifyPhiNode(const BB &bb, Dominance &dom) const {
  if (enableDebug) {
    meFunc.GetCfg()->DumpToFile("meverify");
  }
  for (auto &it : bb.GetMePhiList()) {
    auto *phiNode = it.second;
    if (phiNode == nullptr) {
      continue;
    }
    if (enableDebug) {
      LogInfo::MapleLogger() << bb.GetBBId() << " " <<  phiNode->GetLHS()->GetExprID() << "\n";
    }
    if (!phiNode->GetIsLive()) {
      continue;
    }
    auto *lhs = phiNode->GetLHS();
    if (lhs == nullptr) {
      continue;
    }
    CHECK_FATAL(phiNode->GetDefBB() == &bb, "current bb must be def bb");
    CHECK_FATAL(lhs->GetDefBy() == kDefByPhi, "must be kDefByPhi");
    CHECK_FATAL(&(lhs->GetDefPhi()) == phiNode, "must be the def phinode");
    CHECK_FATAL(bb.GetPred().size() == phiNode->GetOpnds().size(), "must be equal");
    for (size_t i = 0; i < phiNode->GetOpnds().size(); ++i) {
      auto *opnd = phiNode->GetOpnd(i);
      if (opnd->IsVolatile()) {
        continue;
      }
      MeStmt *stmt = nullptr;
      BB *defBB = opnd->GetDefByBBMeStmt(dom, stmt);
      if (bb.GetPred(i) != defBB && !dom.Dominate(*defBB, *bb.GetPred(i))) {
        CHECK_FATAL(false, "the defBB of opnd must be the predBB or dominate the current bb");
      }
    }
  }
}

void MeVerify::VerifySuccAndPredOfBB(const BB &bb) const {
  std::set<BBId> temp;
  for (auto &pred : bb.GetPred()) {
    size_t size = temp.size();
    temp.insert(pred->GetBBId());
    CHECK_FATAL(size != temp.size(), "the bb is already existed");
  }
  temp.clear();
  for (auto &succ : bb.GetSucc()) {
    size_t size = temp.size();
    temp.insert(succ->GetBBId());
    CHECK_FATAL(size != temp.size(), "the bb is already existed");
  }
}

void MeVerify::VerifyBBKind(const BB &bb) const {
  switch (bb.GetKind()) {
    case kBBCondGoto: {
      VerifyCondGotoBB(bb);
      break;
    }
    case kBBGoto: {
      VerifyGotoBB(bb);
      break;
    }
    case kBBFallthru: {
      VerifyFallthruBB(bb);
      break;
    }
    case kBBReturn: {
      VerifyReturnBB(bb);
      break;
    }
    case kBBSwitch: {
      VerifySwitchBB(bb);
      break;
    }
    case kBBUnknown: {
      if (bb.GetBBId() != 0 && bb.GetBBId() != 1) {
        CHECK_FATAL(false, "Verify: must be Entry or Exit bb");
      }
      break;
    }
    default: {
      CHECK_FATAL(false, "Verify: can not support this kind of bb");
    }
  }
}

// Filter the special case which created in the function named RemoveEhEdgesInSyncRegion of mefunction.
void MeVerify::DealWithSpecialCase(const BB &currBB, const BB &tryBB) const {
  if (meFunc.GetCfg()->GetEndTryBB2TryBB().size() != 1) {
    CHECK_FATAL(false, "must be try");
  }
  if (currBB.GetAttributes(kBBAttrIsTryEnd)) {
    auto endTryBB = currBB;
    if (!(endTryBB.GetAttributes(kBBAttrIsCatch) || endTryBB.GetAttributes(kBBAttrIsJSCatch)) ||
        endTryBB.GetKind() != kBBFallthru || !endTryBB.GetAttributes(kBBAttrIsTryEnd) ||
        !endTryBB.GetAttributes(kBBAttrIsJavaFinally) || endTryBB.GetMeStmts().back().GetOp() != OP_syncexit ||
        tryBB.GetMeStmts().back().GetOp() != OP_try) {
      CHECK_FATAL(false, "must be try");
    }
  }
  if (&currBB != &tryBB && !currBB.GetAttributes(kBBAttrIsTryEnd)) {
    for (auto &stmt : currBB.GetMeStmts()) {
      if (stmt.GetOp() == OP_try || stmt.GetOp() == OP_catch || stmt.GetOp() == OP_throw) {
        CHECK_FATAL(false, "must be try");
      }
    }
  }
}

void MeVerify::VerifyNestedTry(const BB &tryBB, const BB &currBB) const {
  if (&currBB != &tryBB && !currBB.GetAttributes(kBBAttrIsTryEnd)) {
    for (auto &stmt : currBB.GetMeStmts()) {
      if (stmt.GetOp() == OP_try) {
        CHECK_FATAL(false, "nested try");
      }
    }
  }
}

void MeVerify::VerifyAttrTryBB(BB &tryBB, uint64 index) {
  auto tryStmt = static_cast<TryMeStmt&>(tryBB.GetMeStmts().front());
  int i = 0;
  for (auto offsetIt = tryStmt.GetOffsets().rbegin(), offsetEIt = tryStmt.GetOffsets().rend();
       offsetIt != offsetEIt; ++offsetIt) {
    auto offsetBBId = meFunc.GetCfg()->GetLabelBBAt(*offsetIt)->GetBBId();
    bool needExit = false;
    for (size_t j = index; j < meFunc.GetLaidOutBBs().size() && !needExit; ++j) {
      auto currBB = meFunc.GetLaidOutBBs().at(j);
      if (currBB == nullptr) {
        continue;
      }
      if (currBB->GetAttributes(kBBAttrIsTryEnd)) {
        needExit = true;
      }
      // bb_layout move artificial bb without try-attribute into try
      if (currBB->GetAttributes(kBBAttrArtificial)) {
        continue;
      }
      // cannot appear nested try
      VerifyNestedTry(tryBB, *currBB);
      if (!currBB->GetAttributes(kBBAttrIsTry)) {
        DealWithSpecialCase(*currBB, tryBB);
        continue;
      }
      // verify try attribute
      CHECK_FATAL(currBB->GetAttributes(kBBAttrIsTry), "must be try");
      if (currBB->GetKind() == kBBReturn && currBB->GetAttributes(kBBAttrIsExit) &&
          currBB->GetMeStmts().back().GetOp() == OP_return) {
        continue;
      }
      // When the curr bb is catch, the preds could not include the eh edge.
      if (currBB->GetAttributes(kBBAttrIsCatch) && currBB->GetAttributes(kBBAttrIsJavaFinally)) {
        continue;
      }
      // When the size of gotoBB's succs is more than two, one is targetBB, one is  wontExitBB and the other is catchBB.
      if (currBB->GetKind() == kBBGoto && currBB->GetAttributes(kBBAttrWontExit)) {
        if (offsetBBId != (*(currBB->GetSucc().rbegin() + i + 1))->GetBBId() &&
            offsetBBId != (*(currBB->GetSucc().rbegin() + i))->GetBBId()) {
          CHECK_FATAL(false, "must be equal");
        }
      } else {
        CHECK_FATAL(offsetBBId == (*(currBB->GetSucc().rbegin() + i))->GetBBId(), "must be equal");
      }
      if (enableDebug) {
        LogInfo::MapleLogger() << currBB->GetBBId() << " " <<  (*(currBB->GetSucc().rbegin() + i))->GetBBId() << "\n";
      }
    }
    CHECK_FATAL(needExit, "no tryend bb");
    ++i;
  }
}

void MeVerify::VerifyPredBBOfSuccBB(const BB &bb, const MapleVector<BB*> &succs) const {
  bool find = false;
  for (auto &succ : succs) {
    for (auto &pred : succ->GetPred()) {
      if (pred->GetBBId() == bb.GetBBId()) {
        find = true;
        break;
      }
    }
    CHECK_FATAL(find, "Verify: bb is not pred of succ bb");
    find = false;
  }
}

void MeVerify::VerifyCondGotoBB(const BB &bb) const {
  if (bb.GetAttributes(kBBAttrIsTry) || bb.GetAttributes(kBBAttrWontExit)) {
    CHECK_FATAL(bb.GetSucc().size() >= 2, "Verify: condgoto bb must have more than two succ bb");
  } else {
    CHECK_FATAL(bb.GetSucc().size() == 2, "Verify: condgoto bb must have two succ bb");
  }
  CHECK_FATAL(!bb.GetMeStmts().empty(), "Verify: meStmts of bb should not be empty");
  if (bb.GetMeStmts().back().GetOp() != OP_brfalse && bb.GetMeStmts().back().GetOp() != OP_brtrue) {
    CHECK_FATAL(false, "Verify: the opcode of last stmt must be OP_brfalse or OP_brtrue");
  }
  CHECK_FATAL(static_cast<const CondGotoMeStmt&>(bb.GetMeStmts().back()).GetOffset() == bb.GetSucc(1)->GetBBLabel(),
              "Verify: offset of condgoto stmt must equal to the label of second succ bb");
  VerifyPredBBOfSuccBB(bb, bb.GetSucc());
}

void MeVerify::VerifyGotoBB(const BB &bb) const {
  if (bb.GetAttributes(kBBAttrIsTry) || bb.GetAttributes(kBBAttrWontExit)) {
    CHECK_FATAL(bb.GetSucc().size() >= 1, "Verify: goto bb must have more than one succ bb");
  } else {
    CHECK_FATAL(bb.GetSucc().size() == 1, "Verify: goto bb must have one succ bb");
  }
  CHECK_FATAL(!bb.GetMeStmts().empty(), "Verify: meStmts of bb should not be empty");
  if (bb.GetMeStmts().back().GetOp() == OP_goto) {
    CHECK_FATAL(static_cast<const GotoMeStmt&>(bb.GetMeStmts().back()).GetOffset() == bb.GetSucc(0)->GetBBLabel(),
                "Verify: offset of goto stmt must equal to the label of succ bb");
  }
  VerifyPredBBOfSuccBB(bb, bb.GetSucc());
}

void MeVerify::VerifyFallthruBB(const BB &bb) const {
  VerifyPredBBOfSuccBB(bb, bb.GetSucc());
  if (bb.GetAttributes(kBBAttrIsTry) || bb.GetAttributes(kBBAttrWontExit)) {
    CHECK_FATAL(bb.GetSucc().size() >= 1, "Verify: fallthru bb must have more than one succ bb");
  } else {
    CHECK_FATAL(bb.GetSucc().size() == 1, "Verify: fallthru bb must have one succ bb");
  }
}

bool MeVerify::IsOnlyHaveReturnOrThrowStmt(const BB &bb, Opcode op) const {
  auto stmt = &bb.GetMeStmts().front();
  CHECK_NULL_FATAL(stmt);
  while (stmt != nullptr) {
    if (stmt->GetOp() == op || stmt->GetOp() == OP_decref || stmt->GetOp() == OP_incref ||
        stmt->GetOp() == OP_decrefreset || stmt->GetOp() == OP_comment) {
      stmt = stmt->GetNextMeStmt();
      continue;
    }
    return false;
  }
  return true;
}

void MeVerify::VerifyCommonExitBB() const {
  for (auto &pred : meFunc.GetCfg()->GetCommonExitBB()->GetPred()) {
    if (pred->GetKind() == kBBReturn) {
      continue;
    }
    if (pred->GetKind() == kBBGoto && IsOnlyHaveReturnOrThrowStmt(*pred, OP_throw)) {
      continue;
    }
    CHECK_FATAL(false, "verify CommonExitBB failed");
  }
}

void MeVerify::VerifyReturnBB(const BB &bb) const {
  for (auto &pred : meFunc.GetCfg()->GetCommonExitBB()->GetPred()) {
    if (pred == &bb) {
      return;
    }
  }
  CHECK_FATAL(false, "verify return bb failed");
}

void MeVerify::VerifySwitchBB(const BB &bb) const {
  CHECK_FATAL(!bb.GetMeStmts().empty(), "Verify: meStmts of bb should not be empty");
  auto switchMeStmt = static_cast<const SwitchMeStmt&>(bb.GetMeStmts().back());
  auto switchTable = switchMeStmt.GetSwitchTable();
  CHECK_FATAL(switchMeStmt.GetDefaultLabel() == bb.GetSucc(0)->GetBBLabel(),
              "Verify: defaultlabel of switchstmt must be the label of first succ bb");
  for (auto &casePair : switchTable) {
    bool isExit = false;
    for (auto &succBB : bb.GetSucc()) {
      if (casePair.second == succBB->GetBBLabel()) {
        isExit = true;
        break;
      }
    }
    CHECK_FATAL(isExit, "Verify: case label of switchtable must equal to the succ bb");
  }
  VerifyPredBBOfSuccBB(bb, bb.GetSucc());
}

bool MEVerify::PhaseRun(maple::MeFunction &f) {
  MeVerify meVerify(f);
  meVerify.VerifyFunction();
  return false;
}
} // namespace maple
