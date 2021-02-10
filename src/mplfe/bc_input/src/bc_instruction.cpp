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
#include "bc_instruction.h"
#include <algorithm>
#include "bc_class.h"
#include "feir_builder.h"
#include "fe_manager.h"
#include "fe_options.h"

namespace maple {
namespace bc {
void BCInstruction::InitBCInStruction(uint16 kind, bool wide, bool throwable) {
  instKind = static_cast<BCInstructionKind>(instKind | kind);
  isWide = wide;
  isThrowable = throwable;
}

void BCInstruction::SetWidth(uint8 size) {
  width = size;
}

uint8 BCInstruction::GetWidth() const {
  return width;
}

void BCInstruction::SetCatchable() {
  isCatchable = isThrowable;
}

bool BCInstruction::IsCatchable() const {
  return isCatchable;
}

BCInstructionKind BCInstruction::GetInstKind() const {
  return instKind;
}

bool BCInstruction::IsWide() const {
  return isWide;
}

void BCInstruction::SetInstructionKind(BCInstructionKind kind) {
  instKind = static_cast<BCInstructionKind>(static_cast<uint32>(instKind) | static_cast<uint32>(kind));
}

void BCInstruction::Parse(const BCClassMethod &method) {
  ParseImpl(method);
}

void BCInstruction::SetExceptionType(const GStrIdx &typeNameIdx) {
  catchedExTypeNamesIdx.emplace(typeNameIdx);
  SetBCRegType(*this);
}

std::list<UniqueFEIRStmt> BCInstruction::EmitToFEIRStmts() {
  std::list<UniqueFEIRStmt> stmts;
  // Do not modify following stmt order
  GenCommentStmt(stmts);
  if ((instKind & kTarget) || (instKind & kCatch)) {
    if (instKind & kCatch) {
      UniqueFEIRStmt stmt = GenCatchStmt();
      stmts.emplace_back(std::move(stmt));;
    } else {
      UniqueFEIRStmt stmt = GenLabelStmt();
      stmts.emplace_back(std::move(stmt));
    }
  }
  if (instKind & kTryStart) {
    UniqueFEIRStmt stmt = GenTryLabelStmt();
    stmts.emplace_back(std::move(stmt));
  }
  std::list<UniqueFEIRStmt> instStmts = EmitToFEIRStmtsImpl();
  for (auto it = instStmts.begin(); it != instStmts.end(); ++it) {
    it->get()->SetThrowable(isThrowable);
    if (FEOptions::GetInstance().IsAOT()) {
      it->get()->SetHexPC(pc);
    }
    stmts.emplace_back(std::move(*it));
  }
  std::list<UniqueFEIRStmt> retypeStmts = GenRetypeStmtsAfterDef();
  for (auto it = retypeStmts.begin(); it != retypeStmts.end(); ++it) {
    stmts.emplace_back(std::move(*it));
  }
  if (instKind & kTryEnd) {
    UniqueFEIRStmt stmt = GenTryEndLabelStmt();
    stmts.emplace_back(std::move(stmt));
  }
  SetSrcFileInfo(stmts);
  return stmts;
}

void BCInstruction::SetSrcFileInfo(std::list<UniqueFEIRStmt> &stmts) const {
#ifdef DEBUG
  if (FEOptions::GetInstance().IsDumpLOC() && !stmts.empty()) {
    (*stmts.begin())->SetSrcFileInfo(srcFileIdx, srcFileLineNum);
  }
#endif
}

bool BCInstruction::IsFallThru() const {
  return instKind & kFallThru;
}

bool BCInstruction::IsConditionBranch() const {
  return instKind & kConditionBranch;
}

bool BCInstruction::IsGoto() const {
  return instKind & kGoto;
}

bool BCInstruction::IsSwitch() const {
  return instKind & kSwitch;
}

bool BCInstruction::IsTarget() const {
  return instKind & kTarget;
}

bool BCInstruction::IsTryStart() const {
  return instKind & kTryStart;
}

bool BCInstruction::IsTryEnd() const {
  return instKind & kTryEnd;
}

bool BCInstruction::IsCatch() const {
  return instKind & kCatch;
}

void BCInstruction::SetReturnInst(BCInstruction *inst) {
  inst->SetBCRegType(*this);
  returnInst = inst;
}

void BCInstruction::SetBCRegType(const BCInstruction &inst) {
  SetBCRegTypeImpl(inst);
}

bool BCInstruction::HasReturn() const {
  return returnInst != nullptr;
}

bool BCInstruction::IsReturn() const {
  return isReturn;
}

uint32 BCInstruction::GetPC() const {
  return pc;
}

uint8 BCInstruction::GetOpcode() const {
  return opcode;
}

std::vector<uint32> BCInstruction::GetTargets() const {
  return GetTargetsImpl();
}

void BCInstruction::SetDefultTarget(BCInstruction *inst) {
  defaultTarget = inst;
}

void BCInstruction::AddHandler(BCInstruction *handler) {
  auto it = std::find(handlers.begin(), handlers.end(), handler);
  if (it == handlers.end()) {
    handlers.emplace_back(handler);
  }
}

void BCInstruction::AddHandlerTarget(uint32 target) {
  handlerTargets.emplace_back(target);
}

MapleVector<uint32> BCInstruction::GetHandlerTargets() const {
  return handlerTargets;
}

MapleList<BCReg*> *BCInstruction::GetDefedRegs() {
  return &defedRegs;
}

MapleList<BCReg*> *BCInstruction::GetUsedRegs() {
  return &usedRegs;
}

void BCInstruction::SetRegTypeInTypeInfer() {
  SetRegTypeInTypeInferImpl();
}

std::vector<uint32> BCInstruction::GetTargetsImpl() const {
  return std::vector<uint32>{};  // Default empty, means invalid
}

void BCInstruction::GenCommentStmt(std::list<UniqueFEIRStmt> &stmts) const {
#ifdef DEBUG
  if (FEOptions::GetInstance().IsDumpComment()) {
    std::ostringstream oss;
    // 4 means 4-character width
    oss << "LINE " << FEManager::GetManager().GetSourceFileNameFromIdx(srcFileIdx) << " : " << srcFileLineNum <<
        ", INST_IDX : " << pc << " ||" << std::setfill('0') << std::setw(4) << std::hex << pc << ": " <<
        (opName == nullptr ? "invalid op" : opName);
    stmts.emplace_back(FEIRBuilder::CreateStmtComment(oss.str()));
  }
#endif
}

UniqueFEIRStmt BCInstruction::GenLabelStmt() const {
  return std::make_unique<FEIRStmtPesudoLabel2>(funcNameIdx, pc);
}

UniqueFEIRStmt BCInstruction::GenCatchStmt() const {
  std::unique_ptr<FEIRStmtPesudoCatch2> stmt = std::make_unique<FEIRStmtPesudoCatch2>(funcNameIdx, pc);
  for (const auto &exTypeNameIdx : catchedExTypeNamesIdx) {
    stmt->AddCatchTypeNameIdx(exTypeNameIdx);
  }
  return stmt;
}

UniqueFEIRStmt BCInstruction::GenTryLabelStmt() const {
  std::unique_ptr<FEIRStmtPesudoJavaTry2>  javaTry = std::make_unique<FEIRStmtPesudoJavaTry2>(funcNameIdx);
  for (const auto &handler : handlers) {
    javaTry->AddCatchLabelIdx(handler->GetPC());
  }
  return javaTry;
}

UniqueFEIRStmt BCInstruction::GenTryEndLabelStmt() const {
  return std::make_unique<FEIRStmtPesudoEndTry>();
}

std::list<UniqueFEIRStmt> BCInstruction::GenRetypeStmtsAfterDef() const {
  std::list<UniqueFEIRStmt> stmts;
  for (BCReg *reg : defedRegs) {
    std::list<UniqueFEIRStmt> stmts0 = reg->GenRetypeStmtsAfterDef();
    for (auto &stmt : stmts0) {
      stmts.emplace_back(std::move(stmt));
    }
  }
  return stmts;
}

std::list<UniqueFEIRStmt> BCInstruction::GenRetypeStmtsBeforeUse() const {
  std::list<UniqueFEIRStmt> stmts;
  for (BCReg *reg : usedRegs) {
    std::list<UniqueFEIRStmt> stmts0 = reg->GenRetypeStmtsBeforeUse();
    for (auto &stmt : stmts0) {
      stmts.emplace_back(std::move(stmt));
    }
  }
  return stmts;
}

void BCInstruction::SetFuncNameIdx(const GStrIdx &methodIdx) {
  funcNameIdx = methodIdx;
}

void BCInstruction::SetSrcPositionInfo(uint32 fileIdxIn, uint32 lineNumIn) {
  srcFileIdx = fileIdxIn;
  srcFileLineNum = lineNumIn;
}


void BCInstruction::SetOpName(const char *name) {
#ifdef DEBUG
  opName = name;
#endif
}

const char *BCInstruction::GetOpName() const {
#ifdef DEBUG
  return opName;
#else
  return nullptr;
#endif
}

// ========== BCRegTypeItem ==========
PrimType BCRegTypeItem::GetPrimType() const {
  if (isPrimPtr) {
    return PTY_ref;
  } else {
    return GetBasePrimType();
  }
}

PrimType BCRegTypeItem::GetBasePrimType() const {
  return BCUtil::GetPrimType(typeNameIdx);
}

bool BCRegTypeItem::IsMorePreciseType(const BCRegTypeItem &typeItemIn) const {
  if (IsRef() && !typeItemIn.IsRef()) {
    return true;
  } else if (!IsRef() && typeItemIn.IsRef()) {
    return false;
  } else if (IsRef() && typeItemIn.IsRef()) {
    const std::string &name0 = GlobalTables::GetStrTable().GetStringFromStrIdx(typeNameIdx);
    const std::string &name1 = GlobalTables::GetStrTable().GetStringFromStrIdx(typeItemIn.typeNameIdx);
    uint8 dim0 = FEUtils::GetDim(name0);
    uint8 dim1 = FEUtils::GetDim(name1);
    if (dim0 == dim1) {
      return name0.substr(dim0).compare(BCUtil::kJavaObjectName) != 0;
    } else {
      return dim0 > dim1;
    }
  } else {
    if (!isIndeterminate && typeItemIn.isIndeterminate) {
      return true;
    } else if (isIndeterminate && !typeItemIn.isIndeterminate) {
      return false;
    } else {
      return BCUtil::IsMorePrecisePrimitiveType(typeNameIdx, typeItemIn.typeNameIdx);
    }
  }
}

// ========== BCRegType ==========
BCRegType::BCRegType(MapleAllocator &allocatorIn, BCReg &reg, const GStrIdx &typeNameIdxIn,
                     bool isPrimPtrIn, bool isIndeterminateIn)
    : allocator(allocatorIn), curReg(reg),
      regTypeItem(allocator.GetMemPool()->New<BCRegTypeItem>(typeNameIdxIn, isPrimPtrIn, isIndeterminateIn)),
      typesUsedAs(allocator.Adapter()),
      elemTypes(allocator.Adapter()),
      arrayTypes(allocator.Adapter()) {
  curReg.regTypeItem = regTypeItem;
}

void BCRegType::UpdateDefTypeFromUse(BCRegTypeItem *typeItem) {
  UpdateUsedSet(typeItem);
}

void BCRegType::UpdateDefTypeThroughPhi(BCReg &defedReg, const std::set<BCRegTypeItem*> &usedTypes) {
  UpdateTypeSetFromPhi(defedReg, usedTypes);
}

void BCRegType::UpdateTypeSetFromPhi(BCReg &defedReg, const std::set<BCRegTypeItem*> &usedTypes) {
  bool isIndeterminate = true;
  for (auto &elem : usedTypes) {
    if (!elem->isIndeterminate) {
      isIndeterminate = false;
    }
    InsertUniqueTypeItem(typesUsedAs, elem);
  }
  if (isIndeterminate) {
    defedReg.regType->UpdateUsedSet(regTypeItem);
  }
}

void BCRegType::PrecisifyTypes() {
  if (precisified) {
    return;
  }
  precisified = true;
  if (regTypeItem->isIndeterminate) {
    BCRegTypeItem *realType = GetMostPreciseType(typesUsedAs);
    if (realType != nullptr) {
      regTypeItem->Copy(*realType);
    }
    if (regTypeItem->isIndeterminate) {
      // Get type from array elem
      for (auto elem : elemTypes) {
        elem->PrecisifyTypes();
        std::string arrTypeName = "A" + GlobalTables::GetStrTable().GetStringFromStrIdx(elem->regTypeItem->typeNameIdx);
        GStrIdx arrayTypeIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrTypeName);
        regTypeItem->typeNameIdx = arrayTypeIdx;
        regTypeItem->isIndeterminate = false;
      }
    } else {
      const std::string &arrTypeName = GlobalTables::GetStrTable().GetStringFromStrIdx(regTypeItem->typeNameIdx);
      for (auto elem : elemTypes) {
        if (arrTypeName.size() > 1 && arrTypeName.at(0) == 'A') {
          elem->regTypeItem->typeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrTypeName.substr(1));
          elem->regTypeItem->isIndeterminate = false;
        } else {
          INFO(kLncInfo, "Could not determine the array type thouth its USE, set by its element.");
        }
      }
    }
  }
  for (auto elem : arrayTypes) {
    elem->PrecisifyTypes();
    // Generally, array type could be determined by it USE.
    // Otherwise, set it though its element type.
    if (elem->IsIndeterminate() || !BCUtil::IsArrayType(elem->regTypeItem->typeNameIdx)) {
      std::string arrayTypeName = "A" + GlobalTables::GetStrTable().GetStringFromStrIdx(regTypeItem->typeNameIdx);
      GStrIdx arrayTypeIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrayTypeName);
      elem->regTypeItem->typeNameIdx = arrayTypeIdx;
      elem->regTypeItem->isIndeterminate = false;
    }
  }
}

BCRegTypeItem *BCRegType::GetMostPreciseType(const MapleVector<BCRegTypeItem*> &types) {
  BCRegTypeItem *retType = nullptr;
  if (types.empty()) {
    return retType;
  }
  auto it = types.begin();
  retType = *it;
  ++it;
  while (it != types.end()) {
    if ((*it)->IsMorePreciseType(*retType)) {
      retType = *it;
    }
    ++it;
  }
  return retType;
}

// ========== BCReg ==========
PrimType BCReg::GetPrimType() const {
  return regTypeItem->GetPrimType();
}

PrimType BCReg::GetBasePrimType() const {
  return regTypeItem->GetBasePrimType();
}

bool BCReg::IsConstZero() const {
  if (regValue != nullptr) {
    return regValue->primValue.raw32 == 0;
  }
  return false;
}

UniqueFEIRType BCReg::GenFEIRType() const {
  return GenFEIRTypeImpl();
}

UniqueFEIRVar BCReg::GenFEIRVarReg() const {
  return GenFEIRVarRegImpl();
}

UniqueFEIRVar BCReg::GenFEIRVarRegImpl() const {
  return std::make_unique<FEIRVarReg>(regNum, GenFEIRTypeImpl());
}

UniqueFEIRType BCReg::GenFEIRTypeImpl() const {
  PrimType pty = GetBasePrimType();
  if (regTypeItem->isPrimPtr && pty != PTY_ref) {
    return std::make_unique<FEIRTypePointer>(std::make_unique<FEIRTypeDefault>(pty, regTypeItem->typeNameIdx));
  } else {
    return std::make_unique<FEIRTypeDefault>(pty, regTypeItem->typeNameIdx);
  }
}

std::list<UniqueFEIRStmt> BCReg::GenRetypeStmtsAfterDef() const {
  std::list<UniqueFEIRStmt> retypeStmts;
  // Not gen retype stmt for use reg, same def-use type reg.
  if (!isDef) {
    return retypeStmts;
  }
  std::unique_ptr<BCReg> dstReg = this->Clone();
  std::unique_ptr<BCRegTypeItem> tmpItem = std::make_unique<BCRegTypeItem>(GStrIdx(0), false);
  dstReg->regTypeItem = tmpItem.get();
  PrimType ptyDef = regTypeItem->GetPrimType();

  std::list<BCRegTypeItem*> unqTypeItems;
  for (const auto &usedType : *(regType->GetUsedTypes())) {
    bool exist = false;
    for (const auto &elem : unqTypeItems) {
      if ((*usedType) == (*elem)) {
        exist = true;
        break;
      }
    }
    if (exist == false) {
      unqTypeItems.emplace_back(usedType);
    }
  }

  for (const auto &usedType : unqTypeItems) {
    PrimType ptyUsed = usedType->GetPrimType();
    if ((*usedType) == (*regTypeItem) ||
        (usedType->isIndeterminate &&
         ((ptyUsed == PTY_i64 && ptyDef == PTY_f64) || (ptyUsed == PTY_i32 && ptyDef == PTY_f32)))) {
      continue;
    }
    // Create retype stmt
    dstReg->regTypeItem->Copy(*usedType);
    UniqueFEIRStmt retypeStmt =
        FEIRBuilder::CreateStmtRetype(dstReg->GenFEIRVarReg(), this->GenFEIRVarReg());
    if (retypeStmt != nullptr) {
      retypeStmts.emplace_back(std::move(retypeStmt));
    }
  }
  return retypeStmts;
}

std::list<UniqueFEIRStmt> BCReg::GenRetypeStmtsBeforeUse() const {
  std::list<UniqueFEIRStmt> retypeStmts;
  // Not gen retype stmt for def reg, same def-use type reg.
  // And Not gen retype stmt, if the defiend type is indeterminate.
  const BCRegTypeItem *defed = regType->GetRegTypeItem();
  if (isDef) {
    return retypeStmts;
  }
  if (!((*regTypeItem) == (*defed))) {
    BCReg srcReg;
    std::unique_ptr<BCRegTypeItem> tmpItem = std::make_unique<BCRegTypeItem>(GStrIdx(0), false);
    srcReg.regTypeItem = tmpItem.get();
    // Create retype stmt
    srcReg.regNum = regNum;
    srcReg.regTypeItem->Copy(*defed);
    UniqueFEIRStmt retypeStmt =
        FEIRBuilder::CreateStmtRetype(this->GenFEIRVarReg(), srcReg.GenFEIRVarReg());
    if (retypeStmt != nullptr) {
      retypeStmts.emplace_back(std::move(retypeStmt));
    }
  }
  return retypeStmts;
}

std::unique_ptr<BCReg> BCReg::CloneImpl() const {
  auto reg = std::make_unique<BCReg>();
  *reg = *this;
  return reg;
}
}  // namespace bc
}  // namespace maple
