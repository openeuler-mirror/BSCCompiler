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
#include "bc_class.h"
#include "global_tables.h"
#include "fe_utils_java.h"
#include "fe_utils.h"
#include "fe_manager.h"
#include "fe_options.h"

namespace maple {
namespace bc {
// ========== BCClassElem ==========
BCClassElem::BCClassElem(const BCClass &klassIn, uint32 acc, const std::string &nameIn, const std::string &descIn)
    : klass(klassIn),
      accessFlag(acc),
      name(nameIn),
      descriptor(descIn) {}

const std::string &BCClassElem::GetName() const {
  return name;
}

const std::string &BCClassElem::GetDescription() const {
  return descriptor;
}

uint32 BCClassElem::GetItemIdx() const {
  return GetItemIdxImpl();
}

uint32 BCClassElem::GetIdx() const {
  return GetIdxImpl();
}

uint32 BCClassElem::GetAccessFlag() const {
  return accessFlag;
}

bool BCClassElem::IsStatic() const {
  return IsStaticImpl();
}

const std::string &BCClassElem::GetClassName() const {
  return klass.GetClassName(false);
}

GStrIdx BCClassElem::GetClassNameMplIdx() const {
  return klass.GetClassNameMplIdx();
}

const BCClass &BCClassElem::GetBCClass() const {
  return klass;
}

// ========== BCTryInfo ==========
void BCTryInfo::DumpTryCatchInfo(const std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> &tryInfos) {
  DEBUG_STMT(
      for (const auto &tryInfo : *tryInfos) {
        LogInfo::MapleLogger(kLlDbg) << "tryBlock: [" << std::hex << "0x" << tryInfo->GetStartAddr() << ", 0x" <<
            tryInfo->GetEndAddr() << ")";
        LogInfo::MapleLogger(kLlDbg) << "  handler: ";
        for (const auto &e : (*tryInfo->GetCatches())) {
          LogInfo::MapleLogger(kLlDbg) << "0x" << e->GetHandlerAddr() << " ";
        }
        LogInfo::MapleLogger(kLlDbg) << std::endl;
      })
}

// ========== BCClassMethod ==========
void BCClassMethod::SetMethodInstOffset(const uint16 *pos) {
  instPos = pos;
}

void BCClassMethod::SetRegisterTotalSize(uint16 size) {
  registerTotalSize = size;
}

uint16 BCClassMethod::GetRegisterTotalSize() const{
  return registerTotalSize;
}

void BCClassMethod::SetCodeOff(uint32 off) {
  codeOff = off;
}

uint32 BCClassMethod::GetCodeOff() const {
  return codeOff;
}

void BCClassMethod::SetRegisterInsSize(uint16 size) {
  registerInsSize = size;
}

bool BCClassMethod::IsVirtual() const {
  return IsVirtualImpl();
}

bool BCClassMethod::IsNative() const {
  return IsNativeImpl();
}

bool BCClassMethod::IsInit() const {
  return IsInitImpl();
}

bool BCClassMethod::IsClinit() const {
  return IsClinitImpl();
}

std::string BCClassMethod::GetFullName() const {
  return GetClassName() + "|" + GetName() + "|" + GetDescription();
}

void BCClassMethod::SetSrcPosInfo() {
#ifdef DEBUG
  if ((FEOptions::GetInstance().IsDumpComment() || FEOptions::GetInstance().IsDumpLOC()) &&
      pSrcPosInfo != nullptr) {
    uint32 srcPos = 0;
    for (auto &[pos, inst] : *pcBCInstructionMap) {
      auto it = pSrcPosInfo->find(pos);
      if (it != pSrcPosInfo->end()) {
        srcPos = it->second;  // when pSrcPosInfo is valid, update srcPos
      }
      inst->SetSrcPositionInfo(klass.GetSrcFileIdx(), srcPos);
    }
  }
#endif
}

void BCClassMethod::ProcessInstructions() {
  if (pcBCInstructionMap->empty()) {
    return;
  }
  // some instructions depend on exception handler, so we process try-catch info first
  ProcessTryCatch();
  visitedPcSet.emplace(pcBCInstructionMap->begin()->first);
  GStrIdx methodIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(GetFullName()));
  for (auto itor = pcBCInstructionMap->begin(); itor != pcBCInstructionMap->end();) {
    BCInstruction *inst = itor->second;
    inst->SetFuncNameIdx(methodIdx);
    inst->Parse(*this);
    ++itor;
    if (inst->IsConditionBranch() || inst->IsGoto() || inst->IsSwitch()) {
      std::vector<uint32> targets = inst->GetTargets();
      for (uint32 target : targets) {
        auto it = pcBCInstructionMap->find(target);
        CHECK_FATAL(it != pcBCInstructionMap->end(), "Invalid branch target 0x%x in method: %s",
            target, GetFullName().c_str());
        it->second->SetInstructionKind(kTarget);
        if (visitedPcSet.emplace(target).second == false) {
          multiInDegreeSet.emplace(target);
        }
      }
      if (inst->IsSwitch() && itor != pcBCInstructionMap->end()) {
        itor->second->SetInstructionKind(kTarget);
        inst->SetDefaultTarget(itor->second);
      }
    }
    if (itor == pcBCInstructionMap->end()) {
      continue;
    }
    if (inst->IsFallThru()) {
      if (visitedPcSet.emplace(itor->first).second == false) {
        multiInDegreeSet.emplace(itor->first);
      }
    }
    if (itor->second->IsReturn()) {
      inst->SetReturnInst(itor->second);
    }
  }
  SetSrcPosInfo();
  if (FEOptions::GetInstance().GetTypeInferKind() == FEOptions::kLinearScan) {
    TypeInfer();
  }
}

void BCClassMethod::ProcessTryCatch() {
  if (tryInfos == nullptr) {
    return;
  }
  for (const auto &e : *tryInfos) {
    uint32 tryStart = e->GetStartAddr();
    uint32 tryEnd = e->GetEndAddr();
    auto it = pcBCInstructionMap->find(tryStart);
    CHECK_FATAL(it != pcBCInstructionMap->end(), "Invalid try start pos 0x%x in method: %s", tryStart,
        GetFullName().c_str());
    it->second->SetInstructionKind(kTryStart);
    BCInstruction *inst = it->second;
    auto it1 = pcBCInstructionMap->find(tryEnd);
    if (tryStart == tryEnd) {
      it->second->SetInstructionKind(kTryEnd);
    } else if (it1 != pcBCInstructionMap->end()) {
      --it1;
      it1->second->SetInstructionKind(kTryEnd);
    } else {
      // behind the last instruction
      pcBCInstructionMap->rbegin()->second->SetInstructionKind(kTryEnd);
    }
    // Calculate catchable instructions
    std::list<BCInstruction*> catchableInsts;
    while (it->second->GetPC() < tryEnd) {
      it->second->SetCatchable();
      if (it->second->IsCatchable()) {
        catchableInsts.emplace_back(it->second);
      }
      ++it;
      if (it == pcBCInstructionMap->end()) {
        break;
      }
    }
    for (const auto &handler: *(e->GetCatches())) {
      uint32 handlerPc = handler->GetHandlerAddr();
      auto elem = pcBCInstructionMap->find(handlerPc);
      CHECK_FATAL(elem != pcBCInstructionMap->end(), "Invalid catch pos 0x%x in method: %s", handlerPc,
          GetFullName().c_str());
      elem->second->SetInstructionKind(kCatch);
      elem->second->SetExceptionType(handler->GetExceptionNameIdx());
      inst->AddHandler(elem->second);
      for (auto catchableInst : catchableInsts) {
        catchableInst->AddHandlerTarget(handlerPc);
        if (visitedPcSet.emplace(handlerPc).second == false) {
          multiInDegreeSet.emplace(handlerPc);
        }
      }
    }
  }
}

// Use linear scan with SSA
// We insert phi behind the def in a edge before the dominance if the reg is alive under this dominance.
// A reg is default dead at its definition, we mark a reg alive only when it was used.
// And insert the reg def-type into `used alive types` if it is defined in a determinate type.
// We transfer reg live info through shared memory, record live-interval in dominancesMap and BCRegType::livesBegins.
// After a multi-in pos, we create a new TypeInferItem for a new live range.
// Insert `in-edge` into dominance `prevs`, for record complete reg use in circles by one pass.
void BCClassMethod::TypeInfer() {
  std::set<uint32> visitedSet;
  // [pc, [regNum, TypeInferItem*]]
  std::list<std::pair<uint32, std::vector<TypeInferItem*>>> pcDefedRegsList;
  // [pc, [regNum, TypeInferItem*]]
  std::vector<std::vector<TypeInferItem*>> dominances((*pcBCInstructionMap).rbegin()->first + 1);
  std::vector<TypeInferItem*> regTypeMap(registerTotalSize, nullptr);
  for (auto &reg : argRegs) {
    regTypeMap[reg->regNum] = ConstructTypeInferItem(allocator, 0, reg.get(), nullptr);
    regTypes.emplace_back(reg->regType);
  }
  visitedSet.emplace(0);
  if (multiInDegreeSet.find(0) != multiInDegreeSet.end()) {
    std::vector<TypeInferItem*> typeMap = ConstructNewRegTypeMap(allocator, 1, regTypeMap);
    pcDefedRegsList.emplace_front(0, typeMap);
    dominances[0] = typeMap;
  } else {
    pcDefedRegsList.emplace_front(0, regTypeMap);
  }
  Traverse(pcDefedRegsList, dominances, visitedSet);
  PrecisifyRegType();
}

void BCClassMethod::Traverse(std::list<std::pair<uint32, std::vector<TypeInferItem*>>> &pcDefedRegsList,
                             std::vector<std::vector<TypeInferItem*>> &dominances, std::set<uint32> &visitedSet) {
  while (!pcDefedRegsList.empty()) {
    auto head = pcDefedRegsList.front();
    pcDefedRegsList.pop_front();
    BCInstruction *currInst = (*pcBCInstructionMap)[head.first];
    std::vector<TypeInferItem*> nextRegTypeMap = head.second;
    auto usedRegs = currInst->GetUsedRegs();
    for (auto usedReg : *usedRegs) {
      auto defedItem = nextRegTypeMap[usedReg->regNum];
      CHECK_FATAL(defedItem != nullptr && defedItem->reg != nullptr,
          "Cannot find Reg%u defination at 0x%x:0x%x in method %s",
          usedReg->regNum, head.first, currInst->GetOpcode(), GetFullName().c_str());
      usedReg->regValue = defedItem->reg->regValue;
      usedReg->regType = defedItem->reg->regType;
      currInst->SetRegTypeInTypeInfer();
      // Make the reg alive when it used, live range [defPos, usePos]
      usedReg->regTypeItem->SetPos(currInst->GetPC());
      defedItem->InsertUniqueAliveType(nullptr, usedReg->regTypeItem);
    }
    auto defedRegs = currInst->GetDefedRegs();
    auto exHandlerTargets = currInst->GetHandlerTargets();
    uint32 next = currInst->GetPC() + currInst->GetWidth();
    for (auto exHandlerTarget : exHandlerTargets) {
      if (visitedSet.emplace(exHandlerTarget).second == false) {
        auto &domIt = dominances[exHandlerTarget];
        InsertPhi(domIt, nextRegTypeMap);
        continue;
      }
      if ((multiInDegreeSet.find(exHandlerTarget) != multiInDegreeSet.end())) {
        std::vector<TypeInferItem*> typeMap = ConstructNewRegTypeMap(allocator, exHandlerTarget + 1, nextRegTypeMap);
        pcDefedRegsList.emplace_front(exHandlerTarget, typeMap);
        dominances[exHandlerTarget] = typeMap;
      } else {
        pcDefedRegsList.emplace_front(exHandlerTarget, nextRegTypeMap);
      }
      Traverse(pcDefedRegsList, dominances, visitedSet);
    }
    for (auto defedReg : *defedRegs) {
      TypeInferItem *item = ConstructTypeInferItem(allocator, currInst->GetPC() + 1, defedReg, nullptr);
      nextRegTypeMap[defedReg->regNum] = item;
      defedReg->regType->SetPos(currInst->GetPC());
      regTypes.emplace_back(defedReg->regType);
    }
    if (currInst->IsFallThru() && next <= pcBCInstructionMap->rbegin()->first) {
      if (visitedSet.emplace(next).second == false) {
        auto &domIt = dominances[next];
        InsertPhi(domIt, nextRegTypeMap);
      } else {
        if (multiInDegreeSet.find(next) != multiInDegreeSet.end()) {
          std::vector<TypeInferItem*> typeMap = ConstructNewRegTypeMap(allocator, next + 1, nextRegTypeMap);
          pcDefedRegsList.emplace_front(next, typeMap);
          dominances[next] = typeMap;
        } else {
          pcDefedRegsList.emplace_front(next, nextRegTypeMap);
        }
        // Use stack to replace recursive call, avoid `call stack` overflow in long `fallthru` code.
        // And `fallthu` instructions can not disrupt visit order.
        if (currInst->IsConditionBranch() || currInst->IsSwitch()) {
          Traverse(pcDefedRegsList, dominances, visitedSet);
        }
      }
    }
    auto normalTargets = currInst->GetTargets();
    for (auto normalTarget : normalTargets) {
      if (visitedSet.emplace(normalTarget).second == false) {
        auto &domIt = dominances[normalTarget];
        InsertPhi(domIt, nextRegTypeMap);
        continue;
      }
      if (multiInDegreeSet.find(normalTarget) != multiInDegreeSet.end()) {
        std::vector<TypeInferItem*> typeMap = ConstructNewRegTypeMap(allocator, normalTarget + 1, nextRegTypeMap);
        pcDefedRegsList.emplace_front(normalTarget, typeMap);
        dominances[normalTarget] = typeMap;
      } else {
        pcDefedRegsList.emplace_front(normalTarget, nextRegTypeMap);
      }
      Traverse(pcDefedRegsList, dominances, visitedSet);
    }
  }
}

void BCClassMethod::InsertPhi(const std::vector<TypeInferItem*> &dom, std::vector<TypeInferItem*> &src) {
  for (auto &d : dom) {
    if (d == nullptr) {
      continue;
    }
    auto srcItem = src[d->reg->regNum];
    if (srcItem == d) {
      continue;
    }
    if (d->RegisterInPrevs(srcItem) == false) {
      continue;
    }

    if (d->isAlive == false) {
      continue;
    }
    CHECK_FATAL(srcItem != nullptr, "ByteCode RA error.");
    for (auto ty : *(d->aliveUsedTypes)) {
      ty->SetDom(true);
    }
    srcItem->InsertUniqueAliveTypes(d, d->aliveUsedTypes);
  }
}

TypeInferItem *BCClassMethod::ConstructTypeInferItem(
    MapleAllocator &alloc, uint32 pos, BCReg *bcReg, TypeInferItem *prev) {
  TypeInferItem *item = alloc.GetMemPool()->New<TypeInferItem>(alloc, pos, bcReg, prev);
  return item;
}

std::vector<TypeInferItem*> BCClassMethod::ConstructNewRegTypeMap(MapleAllocator &alloc, uint32 pos,
    const std::vector<TypeInferItem*> &regTypeMap) {
  std::vector<TypeInferItem*> res(regTypeMap.size(), nullptr);
  size_t i = 0;
  for (const auto &elem : regTypeMap) {
    if (elem != nullptr) {
      res[i] = ConstructTypeInferItem(alloc, pos, elem->reg, elem);
    }
    ++i;
  }
  return res;
}

std::list<UniqueFEIRStmt> BCClassMethod::GenReTypeStmtsThroughArgs() const {
  std::list<UniqueFEIRStmt> stmts;
  for (const auto &argReg : argRegs) {
    std::list<UniqueFEIRStmt> stmts0 = argReg->GenRetypeStmtsAfterDef();
    for (auto &stmt : stmts0) {
      stmts.emplace_back(std::move(stmt));
    }
  }
  return stmts;
}

void BCClassMethod::PrecisifyRegType() {
  for (auto elem : regTypes) {
    elem->PrecisifyTypes();
  }
}

std::list<UniqueFEIRStmt> BCClassMethod::EmitInstructionsToFEIR() const {
  std::list<UniqueFEIRStmt> stmts;
  if (!HasCode()) {
    return stmts; // Skip abstract and native method, not emit it to mpl but mplt.
  }
  if (IsStatic() && GetName().compare("<clinit>") != 0) { // Not insert JAVA_CLINIT_CHECK in <clinit>
    GStrIdx containerNameIdx = GetClassNameMplIdx();
    uint32 typeID = UINT32_MAX;
    if (FEOptions::GetInstance().IsAOT()) {
      const std::string &mplClassName = GetBCClass().GetClassName(true);
      int32 dexFileHashCode = GetBCClass().GetBCParser().GetFileNameHashId();
      typeID = FEManager::GetTypeManager().GetTypeIDFromMplClassName(mplClassName, dexFileHashCode);
    }
    UniqueFEIRStmt stmt =
        std::make_unique<FEIRStmtIntrinsicCallAssign>(INTRN_JAVA_CLINIT_CHECK,
                                                      std::make_unique<FEIRTypeDefault>(PTY_ref, containerNameIdx),
                                                      nullptr, typeID);
    stmts.emplace_back(std::move(stmt));
  }
  std::map<uint32, FEIRStmtPesudoLabel2*> targetFEIRStmtMap;
  std::list<FEIRStmtGoto2*> gotoFEIRStmts;
  std::list<FEIRStmtSwitch2*> switchFEIRStmts;
  std::list<UniqueFEIRStmt> retypeStmts = GenReTypeStmtsThroughArgs();
  for (auto &stmt : retypeStmts) {
    stmts.emplace_back(std::move(stmt));
  }
  for (const auto &elem : *pcBCInstructionMap) {
    std::list<UniqueFEIRStmt> instStmts = elem.second->EmitToFEIRStmts();
    for (auto &e : instStmts) {
      if (e->GetKind() == kStmtPesudoLabel) {
        targetFEIRStmtMap.emplace(static_cast<FEIRStmtPesudoLabel2*>(e.get())->GetPos(),
                                  static_cast<FEIRStmtPesudoLabel2*>(e.get()));
      }
      if (e->GetKind() == kStmtGoto || e->GetKind() == kStmtCondGoto) {
        gotoFEIRStmts.emplace_back(static_cast<FEIRStmtGoto2*>(e.get()));
      }
      if (e->GetKind() == kStmtSwitch) {
        switchFEIRStmts.emplace_back(static_cast<FEIRStmtSwitch2*>(e.get()));
      }
      stmts.emplace_back(std::move(e));
    }
  }
  LinkJumpTarget(targetFEIRStmtMap, gotoFEIRStmts, switchFEIRStmts); // Link jump target
  return stmts;
}

void BCClassMethod::LinkJumpTarget(const std::map<uint32, FEIRStmtPesudoLabel2*> &targetFEIRStmtMap,
                                   const std::list<FEIRStmtGoto2*> &gotoFEIRStmts,
                                   const std::list<FEIRStmtSwitch2*> &switchFEIRStmts) {
  for (auto &e : gotoFEIRStmts) {
    auto target = targetFEIRStmtMap.find(e->GetTarget());
    CHECK_FATAL(target != targetFEIRStmtMap.end(), "Cannot find the target for goto/condGoto");
    e->SetStmtTarget(*(target->second));
  }
  for (auto &e : switchFEIRStmts) {
    uint32 label = e->GetDefaultLabelIdx();
    if (label != UINT32_MAX) {
      auto defaultTarget = targetFEIRStmtMap.find(label);
      CHECK_FATAL(defaultTarget != targetFEIRStmtMap.end(), "Cannot find the default target for Switch");
      e->SetDefaultTarget(defaultTarget->second);
    }
    for (const auto &valueLabel : e->GetMapValueLabelIdx()) {
      auto target = targetFEIRStmtMap.find(valueLabel.second);
      CHECK_FATAL(target != targetFEIRStmtMap.end(), "Cannot find the target for Switch");
      e->AddTarget(valueLabel.first, target->second);
    }
  }
}

void BCClassMethod::DumpBCInstructionMap() const {
  // Only used in DEBUG manually
  DEBUG_STMT(
      uint32 idx = 0;
      for (const auto &[pos, instPtr] : *pcBCInstructionMap) {
        LogInfo::MapleLogger(kLlDbg) << "index: " << std::dec << idx++ << " pc: 0x" << std::hex << pos <<
            " opcode: 0x" << instPtr->GetOpcode() << std::endl;
      });
}

const uint16 *BCClassMethod::GetInstPos() const {
  return instPos;
}

std::vector<std::unique_ptr<FEIRVar>> BCClassMethod::GenArgVarList() const {
  return GenArgVarListImpl();
}

void BCClassMethod::GenArgRegs() {
  return GenArgRegsImpl();
}

// ========== BCCatchInfo ==========
BCCatchInfo::BCCatchInfo(uint32 handlerAddrIn, const GStrIdx &argExceptionNameIdx, bool iscatchAllIn)
    : handlerAddr(handlerAddrIn),
      exceptionNameIdx(argExceptionNameIdx),
      isCatchAll(iscatchAllIn) {}
// ========== BCClass ==========
void BCClass::SetSrcFileInfo(const std::string &name) {
  srcFileNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  srcFileIdx = FEManager::GetManager().RegisterSourceFileIdx(srcFileNameIdx);
}

void BCClass::SetSuperClasses(const std::list<std::string> &names) {
  if (names.empty()) {
    return;  // No parent class
  }
  superClassNameList = names;
}

void BCClass::SetInterface(const std::string &name) {
  interfaces.push_back(name);
}

void BCClass::SetAccFlag(uint32 flag) {
  accFlag = flag;
}

void BCClass::SetField(std::unique_ptr<BCClassField> field) {
  fields.push_back(std::move(field));
}

void BCClass::SetMethod(std::unique_ptr<BCClassMethod> method) {
  std::lock_guard<std::mutex> lock(bcClassMtx);
  methods.push_back(std::move(method));
}

void BCClass::InsertFinalStaticStringID(uint32 stringID) {
  finalStaticStringID.push_back(stringID);
}

void BCClass::SetClassName(const std::string &classNameOrin) {
  classNameOrinIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(classNameOrin);
  classNameMplIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(classNameOrin));
}

const std::unique_ptr<BCAnnotationsDirectory> &BCClass::GetAnnotationsDirectory() const {
  return annotationsDirectory;
}

void BCClass::SetAnnotationsDirectory(std::unique_ptr<BCAnnotationsDirectory> annotationsDirectoryIn) {
  annotationsDirectory = std::move(annotationsDirectoryIn);
}

std::vector<MIRConst*> BCClass::GetStaticFieldsConstVal() const {
  return staticFieldsConstVal;
}

void BCClass::InsertStaticFieldConstVal(MIRConst *cst) {
  staticFieldsConstVal.push_back(cst);
}

const std::string &BCClass::GetClassName(bool mapled) const {
  return mapled ? GlobalTables::GetStrTable().GetStringFromStrIdx(classNameMplIdx) :
                  GlobalTables::GetStrTable().GetStringFromStrIdx(classNameOrinIdx);
}

const std::list<std::string> &BCClass::GetSuperClassNames() const {
  return superClassNameList;
}

const std::vector<std::string> &BCClass::GetSuperInterfaceNames() const {
  return interfaces;
}

std::string BCClass::GetSourceFileName() const {
  return GlobalTables::GetStrTable().GetStringFromStrIdx(srcFileNameIdx);
}

GStrIdx BCClass::GetIRSrcFileSigIdx() const {
  return irSrcFileSigIdx;
}

int32 BCClass::GetFileNameHashId() const {
  return parser.GetFileNameHashId();
}

uint32 BCClass::GetAccessFlag() const {
  return accFlag;
}

const std::vector<std::unique_ptr<BCClassField>> &BCClass::GetFields() const {
  return fields;
}

std::vector<std::unique_ptr<BCClassMethod>> &BCClass::GetMethods() {
  return methods;
}
}  // namespace bc
}  // namespace maple
