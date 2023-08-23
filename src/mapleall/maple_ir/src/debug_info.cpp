/*
 * Copyright (C) [2021-2022] Futurewei Technologies, Inc. All rights reverved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */
#include "debug_info.h"
#include <cstring>
#include "mir_builder.h"
#include "printing.h"
#include "maple_string.h"
#include "global_tables.h"
#include "mir_type.h"
#include "securec.h"
#include "mpl_logging.h"
#include "version.h"
#include "triple.h"

namespace maple {
constexpr uint32 kIndx2 = 2;
constexpr uint32 kStructDBGSize = 8888;
const uint64 kDeadbeef = 0xdeadbeef;

// DBGDie methods
DBGDie::DBGDie(MIRModule *m, DwTag tag)
    : module(m),
      tag(tag),
      id(m->GetDbgInfo()->GetMaxId()),
      withChildren(false),
      keep(true),
      sibling(nullptr),
      firstChild(nullptr),
      abbrevId(0),
      tyIdx(0),
      offset(0),
      size(0),
      attrVec(m->GetMPAllocator().Adapter()),
      subDieVec(m->GetMPAllocator().Adapter()) {
  if (module->GetDbgInfo()->GetParentDieSize() != 0) {
    parent = module->GetDbgInfo()->GetParentDie();
  } else {
    parent = nullptr;
  }
  m->GetDbgInfo()->SetIdDieMap(m->GetDbgInfo()->GetIncMaxId(), this);
  attrVec.clear();
  subDieVec.clear();
}

void DBGDie::ResetParentDie() const {
  module->GetDbgInfo()->ResetParentDie();
}

void DBGDie::AddAttr(DwAt at, DwForm form, uint64 val, bool keepFlag) {
  // collect strps which need label
  if (form == DW_FORM_strp) {
    module->GetDbgInfo()->AddStrps(static_cast<uint32>(val));
  }
  DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form, val);
  attr->SetKeep(keepFlag);
  AddAttr(attr);
}

void DBGDie::AddSimpLocAttr(DwAt at, DwForm form, DwOp op, uint64 val) {
  DBGExprLoc *p = module->GetMemPool()->New<DBGExprLoc>(module, op);
  if (val != kDbgDefaultVal) {
    p->AddSimpLocOpnd(val);
  }
  DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form,
      reinterpret_cast<uintptr_t>(reinterpret_cast<void*>(p)));
  AddAttr(attr);
}

void DBGDie::AddGlobalLocAttr(DwAt at, DwForm form, uint64 val) {
  DBGExprLoc *p = module->GetMemPool()->New<DBGExprLoc>(module, DW_OP_addr);
  p->SetGvarStridx(static_cast<int>(val));
  DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form,
      reinterpret_cast<uintptr_t>(reinterpret_cast<void*>(p)));
  AddAttr(attr);
}

void DBGDie::AddFrmBaseAttr(DwAt at, DwForm form) {
  DBGExprLoc *p = module->GetMemPool()->New<DBGExprLoc>(module, DW_OP_call_frame_cfa);
  DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form,
      reinterpret_cast<uintptr_t>(reinterpret_cast<void*>(p)));
  AddAttr(attr);
}

DBGExprLoc *DBGDie::GetExprLoc() {
  for (auto it : attrVec) {
    if (it->GetDwAt() == DW_AT_location) {
      return it->GetPtr();
    }
  }
  return nullptr;
}

bool DBGDie::SetAttr(DwAt attr, uint64 val) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr) {
      it->SetU(val);
      return true;
    }
  }
  return false;
}

bool DBGDie::SetAttr(DwAt attr, int32 val) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr) {
      it->SetI(val);
      return true;
    }
  }
  return false;
}

bool DBGDie::SetAttr(DwAt attr, uint32 val) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr) {
      it->SetId(val);
      return true;
    }
  }
  return false;
}

bool DBGDie::SetAttr(DwAt attr, int64 val) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr) {
      it->SetJ(val);
      return true;
    }
  }
  return false;
}

bool DBGDie::SetAttr(DwAt attr, float val) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr) {
      it->SetF(val);
      return true;
    }
  }
  return false;
}

bool DBGDie::SetAttr(DwAt attr, double val) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr) {
      it->SetD(val);
      return true;
    }
  }
  return false;
}

bool DBGDie::SetAttr(DwAt attr, DBGExprLoc *ptr) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr) {
      it->SetPtr(ptr);
      return true;
    }
  }
  return false;
}

void DBGDie::AddAttr(DBGDieAttr *attr) {
  for (auto it : attrVec) {
    if (it->GetDwAt() == attr->GetDwAt()) {
      return;
    }
  }
  attrVec.push_back(attr);
}

void DBGDie::AddSubVec(DBGDie *die) {
  if (!die) {
    return;
  }
  for (auto it : subDieVec) {
    if (it->GetId() == die->GetId()) {
      return;
    }
  }
  subDieVec.push_back(die);
  die->parent = this;
}

// DBGAbbrevEntry methods
DBGAbbrevEntry::DBGAbbrevEntry(MIRModule *m, DBGDie *die) : attrPairs(m->GetMPAllocator().Adapter()) {
  tag = die->GetTag();
  abbrevId = 0;
  withChildren = die->GetWithChildren();
  for (auto it : die->GetAttrVec()) {
    if (!it->GetKeep()) {
      continue;
    }
    attrPairs.push_back(it->GetDwAt());
    attrPairs.push_back(it->GetDwForm());
  }
}

bool DBGAbbrevEntry::Equalto(DBGAbbrevEntry *entry) {
  if (attrPairs.size() != entry->attrPairs.size()) {
    return false;
  }
  if (withChildren != entry->GetWithChildren()) {
    return false;
  }
  for (uint32 i = 0; i < attrPairs.size(); i++) {
    if (attrPairs[i] != entry->attrPairs[i]) {
      return false;
    }
  }
  return true;
}

// DebugInfo methods
void DebugInfo::Init() {
  // strip file name
  std::string fileName = module->GetFileName();
  std::string::size_type pos = fileName.find_last_of('/') + 1;
  if (pos != std::string::npos) {
    (void)fileName.erase(0, pos);
  }

  mplSrcIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileName);
  compUnit = module->GetMemPool()->New<DBGDie>(module, DW_TAG_compile_unit);
  module->SetWithDbgInfo(true);
  ResetParentDie();
  if (module->GetSrcLang() == kSrcLangC) {
    varPtrPrefix = "";
  }
  InitBaseTypeMap();
}

GStrIdx DebugInfo::GetPrimTypeCName(PrimType pty) const {
  GStrIdx strIdx = GStrIdx(0);
  switch (pty) {
#define TYPECNAME(p, n) \
    case PTY_##p: \
      strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(n); \
      break

    TYPECNAME(i8,   "char");
    TYPECNAME(i16,  "short");
    TYPECNAME(i32,  "int");
    TYPECNAME(i64,  "long long");
    TYPECNAME(i128, "int128");
    TYPECNAME(u8,   "unsigned char");
    TYPECNAME(u16,  "unsigned short");
    TYPECNAME(u32,  "unsigned int");
    TYPECNAME(u64,  "unsigned long long");
    TYPECNAME(u128, "uint128");
    TYPECNAME(u1,   "bool");
    TYPECNAME(f32,  "float");
    TYPECNAME(f64,  "double");
    TYPECNAME(f128, "float128");
    TYPECNAME(c64,  "complex");
    TYPECNAME(c128, "double complex");
    default:
      break;
  }
  return strIdx;
}

void DebugInfo::InsertBaseTypeMap(const std::string &inputName, const std::string &outpuName, PrimType type) {
  baseTypeMap[GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(inputName)] = std::make_pair(
      GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(outpuName), type);
}

void DebugInfo::InitBaseTypeMap() {
  InsertBaseTypeMap(kDbgLong, "long", Triple::GetTriple().GetEnvironment() == Triple::kGnuIlp32 ? PTY_i32 : PTY_i64);
  InsertBaseTypeMap(kDbgULong, "unsigned long",
                    Triple::GetTriple().GetEnvironment() == Triple::kGnuIlp32 ? PTY_u32 : PTY_u64);
  InsertBaseTypeMap(kDbgLongDouble, "long double", PTY_f64);
  InsertBaseTypeMap(kDbgSignedChar, "signed char", PTY_i8);
  InsertBaseTypeMap(kDbgChar, "char", PTY_i8);
  InsertBaseTypeMap(kDbgUnsignedChar, "unsigned char", PTY_u8);
  InsertBaseTypeMap(kDbgUnsignedInt, "unsigned int", PTY_u32);
  InsertBaseTypeMap(kDbgShort, "short", PTY_i16);
  InsertBaseTypeMap(kDbgInt, "int", PTY_i32);
  InsertBaseTypeMap(kDbgLongLong, "long long", PTY_i64);
  InsertBaseTypeMap(kDbgInt128, "__int128", PTY_i128);
  InsertBaseTypeMap(kDbgUnsignedShort, "unsigned short", PTY_u16);
  InsertBaseTypeMap(kDbgUnsignedLongLong, "unsigned long long", PTY_u64);
  InsertBaseTypeMap(kDbgUnsignedInt128, "unsigned __int128", PTY_u128);
}

void DebugInfo::SetupCU() {
  compUnit->SetWithChildren(true);
  /* Add the Producer (Compiler) Information */
  std::string producer = "Maple Version " + Version::GetVersionStr();
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(producer);
  compUnit->AddAttr(DW_AT_producer, DW_FORM_strp, strIdx.GetIdx());

  /* Source Languate  */
  compUnit->AddAttr(DW_AT_language, DW_FORM_data4, DW_LANG_C99);

  /* Add the compiled source file information */
  compUnit->AddAttr(DW_AT_name, DW_FORM_strp, mplSrcIdx.GetIdx());
  compUnit->AddAttr(DW_AT_comp_dir, DW_FORM_strp, 0);

  compUnit->AddAttr(DW_AT_low_pc, DW_FORM_addr, kDbgDefaultVal);
  compUnit->AddAttr(DW_AT_high_pc, DW_FORM_data8, kDbgDefaultVal);

  compUnit->AddAttr(DW_AT_stmt_list, DW_FORM_sec_offset, kDbgDefaultVal);
}

void DebugInfo::AddScopeDie(MIRScope *scope) {
  if (!scope || scope->IsEmpty()) {
    return;
  }

  bool isLocal = scope->IsLocal();
  MIRFunction *func = GetCurFunction();
  // for non-function local scope, add a lexical block
  bool createBlock = isLocal && (scope != func->GetScope());
  if (createBlock) {
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_lexical_block);
    die->AddAttr(DW_AT_low_pc, DW_FORM_addr, scope->GetId());
    die->AddAttr(DW_AT_high_pc, DW_FORM_data8, scope->GetId());

    // add die to parent
    GetParentDie()->AddSubVec(die);

    PushParentDie(die);
  }

  // process type alias
  HandleTypeAlias(*scope);

  // process alias
  AddAliasDies(*scope, isLocal);

  if (scope->GetSubScopes().size() > 0) {
    // process subScopes
    for (auto it : scope->GetSubScopes()) {
      AddScopeDie(it);
    }
  }

  if (createBlock) {
    PopParentDie();
  }
}

DBGDie *DebugInfo::GetAliasVarTypeDie(const MIRAliasVars &aliasVar, TyIdx tyidx) {
  DBGDie *typeDie = nullptr;
  uint32 index = aliasVar.index;
  switch (aliasVar.atk) {
    case kATKType:
      typeDie = GetOrCreateTypeDie(TyIdx(index));
      break;
    case kATKString:
      typeDie = GetOrCreateTypedefDie(GStrIdx(index), tyidx);
      break;
    case kATKEnum:
      typeDie = GetOrCreateEnumTypeDie(index);
      break;
    default:
      ASSERT(false, "unknown alias type kind");
      break;
  }

  ASSERT(typeDie, "null alias type DIE");
  return GetOrCreateTypeDieWithAttr(aliasVar.attrs, typeDie);
}

DBGDie *DebugInfo::GetOrCreateTypeByNameDie(const MIRType &type) {
  ASSERT(type.GetKind() == kTypeByName, "must be a typename");
  DBGDie *die = GetOrCreateBaseTypeDie(&type);
  if (die != nullptr) {
    return die;
  }
  // look for the enum first
  for (auto &it : GlobalTables::GetEnumTable().enumTable) {
    if (it->GetNameIdx() == type.GetNameStrIdx()) {
      die = GetOrCreateEnumTypeDie(it);
      break;
    }
  }
  if (!die) {
    // look for the typedef
    TyIdx undlyingTypeIdx = module->GetScope()->GetTypeAlias()->GetTyIdxFromMap(type.GetNameStrIdx());
    CHECK_FATAL(undlyingTypeIdx != TyIdx(0), "typedef not found in TypeAliasTable");
    die = GetOrCreateTypedefDie(type.GetNameStrIdx(), undlyingTypeIdx);
  }
  return die;
}

void DebugInfo::HandleTypeAlias(MIRScope &scope) {
  const MIRTypeAlias *typeAlias = scope.GetTypeAlias();
  if (typeAlias == nullptr) {
    return;
  }

  if (scope.IsLocal()) {
    for (auto &i : typeAlias->GetTypeAliasMap()) {
      uint32 tid = i.second.GetIdx();
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tid);
      CHECK_NULL_FATAL(type);
      DBGDie *die = nullptr;
      if (type->GetKind() == kTypeByName) {
        die = GetOrCreateTypeByNameDie(*type);
      } else if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
        uint32 id = tyIdxDieIdMap[tid];
        die = idDieMap[id];
      } else {
        ASSERT(false, "type alias type not in tyIdxDieIdMap");
        continue;
      }
      (void)die->SetAttr(DW_AT_name, i.first.GetIdx());
      AddStrps(i.first.GetIdx());
    }
  } else {
    for (auto it : typeAlias->GetTypeAliasMap()) {
      globalTypeAliasMap[it.first.GetIdx()] = it.second.GetIdx();
    }

    for (auto it : globalTypeAliasMap) {
      (void)GetOrCreateTypeDie(TyIdx(it.second));
      DBGDie *die = GetOrCreateTypedefDie(GStrIdx(it.first), TyIdx(it.second));
      compUnit->AddSubVec(die);
      // associate typedef's type with die
      for (auto i : module->GetTypeNameTab()->GetGStrIdxToTyIdxMap()) {
        if (i.first.GetIdx() == it.first) {
          tyIdxDieIdMap[i.second.GetIdx()] = die->GetId();
          break;
        }
      }
    }
  }
}

void DebugInfo::AddAliasDies(const MIRScope &scope, bool isLocal) {
  MIRFunction *func = GetCurFunction();
  const std::string &funcName = func == nullptr ? "" : func->GetName();
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
  for (auto &i : scope.GetAliasVarMap()) {
    // maple var and die
    MIRSymbol *mplVar = nullptr;
    DBGDie *mplDie = nullptr;
    GStrIdx mplIdx = i.second.mplStrIdx;
    /* Update return type with function alias type. */
    if (!i.second.isLocal && mplIdx == strIdx) {
      mplVar = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(mplIdx);
      mplDie = GetFuncDie(func);
      DBGDie *typeDie = mplVar == nullptr ? nullptr : GetAliasVarTypeDie(i.second, mplVar->GetTyIdx());
      if (mplDie != nullptr && typeDie != nullptr) {
        (void)mplDie->SetAttr(DW_AT_type, typeDie->GetId());
      }
      continue;
    }
    if (i.second.isLocal) {
      mplVar = func->GetSymTab()->GetSymbolFromStrIdx(mplIdx);
      mplDie = GetLocalDie(mplIdx);
    } else {
      mplVar = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(mplIdx);
      mplDie = GetGlobalDie(mplIdx);
    }
    // some global vars are introduced by system, and
    // some local vars are discarded in O2, skip them
    if (mplVar == nullptr || mplDie == nullptr) {
      continue;
    }

    // for local scope, create alias die using maple var except name and type
    // for global scope, update type only if needed
    bool updateOnly = (isLocal == i.second.isLocal && i.first == mplIdx);
    DBGDie *vdie = updateOnly ? mplDie : CreateVarDie(mplVar, i.first);

    // get type from alias
    DBGDie *typeDie = GetAliasVarTypeDie(i.second, mplVar->GetTyIdx());
    vdie->SetAttr(DW_AT_type, typeDie->GetId());

    // for new var
    if (!updateOnly) {
      // link vdie's ExprLoc to mplDie's
      vdie->LinkExprLoc(mplDie);

      GetParentDie()->AddSubVec(vdie);

      // add alias var name to debug_str section
      strps.insert(i.first.GetIdx());
    }
  }
}

void DebugInfo::CollectScopePos(MIRFunction *func, MIRScope *scope) {
  if (scope != func->GetScope()) {
    ScopePos plow;
    plow.id = scope->GetId();
    plow.pos = scope->GetRangeLow();
    funcScopeLows[func].push_back(plow);

    ScopePos phigh;
    phigh.id = scope->GetId();
    phigh.pos = scope->GetRangeHigh();
    funcScopeHighs[func].push_back(phigh);
  }

  if (scope->GetSubScopes().size() > 0) {
    for (auto it : scope->GetSubScopes()) {
      CollectScopePos(func, it);
    }
  }
}

// result is in idSet,
// a set of scope ids which are crossed from oldSrcPos to newSrcPos
void DebugInfo::GetCrossScopeId(MIRFunction *func,
                                std::unordered_set<uint32> &idSet,
                                bool isLow,
                                const SrcPosition &oldSrcPos,
                                const SrcPosition &newSrcPos) {
  if (isLow) {
    for (auto &it : funcScopeLows[func]) {
      if (oldSrcPos.IsBf(it.pos) && (it.pos).IsBfOrEq(newSrcPos)) {
        idSet.insert(it.id);
      }
    }
  } else {
    if (oldSrcPos.IsEq(newSrcPos)) {
      return;
    }
    for (auto &it : funcScopeHighs[func]) {
      if (oldSrcPos.IsBfOrEq(it.pos) && (it.pos).IsBf(newSrcPos)) {
        idSet.insert(it.id);
      }
    }
  }
}

void DebugInfo::Finish() {
  SetupCU();
  // build tree from root DIE compUnit
  BuildDieTree();
  BuildAbbrev();
  ComputeSizeAndOffsets();
}

void DebugInfo::BuildDebugInfoEnums() {
  auto size = GlobalTables::GetEnumTable().enumTable.size();
  for (size_t i = 0; i < size; ++i) {
    DBGDie *die = GetOrCreateEnumTypeDie(static_cast<uint32>(i));
    compUnit->AddSubVec(die);
  }
}

void DebugInfo::BuildDebugInfoContainers() {
  for (auto it : module->GetTypeNameTab()->GetGStrIdxToTyIdxMap()) {
    TyIdx tyIdx = it.second;
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx.GetIdx());

    switch (type->GetKind()) {
      case kTypeClass:
      case kTypeClassIncomplete:
      case kTypeInterface:
      case kTypeInterfaceIncomplete:
      case kTypeStruct:
      case kTypeStructIncomplete:
      case kTypeUnion:
        (void)GetOrCreateStructTypeDie(type);
        break;
      case kTypeByName:
        if (globalTypeAliasMap.find(it.first.GetIdx()) == globalTypeAliasMap.end()) {
#ifdef DEBUG
          // not typedef
          LogInfo::MapleLogger() << "named type "
                                 << GlobalTables::GetStrTable().GetStringFromStrIdx(it.first).c_str()
                                 << "\n";
#endif /* DEBUG */
        }
        break;
      default:
        ASSERT(false, "unknown case in BuildDebugInfoContainers()");
        break;
    }
  }
}

void DebugInfo::BuildDebugInfoGlobalSymbols() {
  for (size_t i = 0; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(static_cast<uint32>(i));
    if (mirSymbol == nullptr || mirSymbol->IsDeleted() || mirSymbol->GetStorageClass() == kScUnused ||
        mirSymbol->GetStorageClass() == kScExtern) {
      continue;
    }
    if (module->IsCModule() && mirSymbol->IsGlobal() && mirSymbol->IsVar()) {
      DBGDie *vdie = CreateVarDie(mirSymbol);
      compUnit->AddSubVec(vdie);
    }
  }
}

void DebugInfo::BuildDebugInfoFunctions() {
  for (auto func : GlobalTables::GetFunctionTable().GetFuncTable()) {
    // the first one in funcTable is nullptr
    if (!func) {
      continue;
    }

    ASSERT(func->GetFuncSymbol() != nullptr, "nullptr check");
    if (func->GetFuncSymbol()->IsDeleted()) {
      continue;
    }

    SetCurFunction(func);
    // function decl
    if (stridxDieIdMap.find(func->GetNameStrIdx().GetIdx()) == stridxDieIdMap.end()) {
      if (func->GetClassTyIdx().GetIdx() == 0 && func->GetBody()) {
        DBGDie *funcDie = GetOrCreateFuncDeclDie(func);
        compUnit->AddSubVec(funcDie);
      }
    }
    // function def
    unsigned idx = func->GetNameStrIdx().GetIdx();
    if (func->GetBody() && funcDefStrIdxDieIdMap.find(idx) == funcDefStrIdxDieIdMap.end()) {
      DBGDie *funcDie = GetOrCreateFuncDefDie(func);
      if (func->GetClassTyIdx().GetIdx() == 0) {
        compUnit->AddSubVec(funcDie);
      }
    }
  }
}

void DebugInfo::BuildDebugInfo() {
  ASSERT(module->GetDbgInfo(), "null dbgInfo");

  Init();

  // setup debug info for enum types
  BuildDebugInfoEnums();

  // setup debug info for global type alias
  HandleTypeAlias(*module->GetScope());

  // containner types
  BuildDebugInfoContainers();

  // setup debug info for global symbols
  BuildDebugInfoGlobalSymbols();

  // handle global scope
  AddScopeDie(module->GetScope());

  // setup debug info for functions
  BuildDebugInfoFunctions();

  // finalize debug info
  Finish();
}

DBGDieAttr *DebugInfo::CreateAttr(DwAt at, DwForm form, uint64 val) const {
  DBGDieAttr *attr = module->GetMemPool()->New<DBGDieAttr>(kDwAt);
  attr->SetDwAt(at);
  attr->SetDwForm(form);
  attr->SetU(val);
  return attr;
}

void DebugInfo::SetLocalDie(MIRFunction *func, GStrIdx strIdx, const DBGDie *die) {
  (funcLstrIdxDieIdMap[func])[strIdx.GetIdx()] = die->GetId();
}

DBGDie *DebugInfo::GetGlobalDie(const GStrIdx &strIdx) {
  unsigned idx = strIdx.GetIdx();
  auto it = globalStridxDieIdMap.find(idx);
  if (it != globalStridxDieIdMap.end()) {
    return idDieMap.at(it->second);
  }
  return nullptr;
}

DBGDie *DebugInfo::GetLocalDie(MIRFunction *func, GStrIdx strIdx) {
  uint32 id = (funcLstrIdxDieIdMap[func])[strIdx.GetIdx()];
  auto it = idDieMap.find(id);
  if (it != idDieMap.end()) {
    return it->second;
  }
  return nullptr;
}

void DebugInfo::SetLocalDie(GStrIdx strIdx, const DBGDie *die) {
  (funcLstrIdxDieIdMap[GetCurFunction()])[strIdx.GetIdx()] = die->GetId();
}

DBGDie *DebugInfo::GetLocalDie(GStrIdx strIdx) {
  return GetLocalDie(GetCurFunction(), strIdx);
}

void DebugInfo::SetLabelIdx(MIRFunction *func, const GStrIdx &strIdx, LabelIdx labIdx) {
  (funcLstrIdxLabIdxMap[func])[strIdx.GetIdx()] = labIdx;
}

LabelIdx DebugInfo::GetLabelIdx(MIRFunction *func, const GStrIdx &strIdx) const {
  LabelIdx labidx = (funcLstrIdxLabIdxMap.at(func)).at(strIdx.GetIdx());
  return labidx;
}

void DebugInfo::SetLabelIdx(const GStrIdx &strIdx, LabelIdx labIdx) {
  (funcLstrIdxLabIdxMap[GetCurFunction()])[strIdx.GetIdx()] = labIdx;
}

LabelIdx DebugInfo::GetLabelIdx(GStrIdx strIdx) {
  LabelIdx labidx = (funcLstrIdxLabIdxMap[GetCurFunction()])[strIdx.GetIdx()];
  return labidx;
}

static DwOp GetBreg(unsigned i) {
  constexpr DwOp baseOpRegs[] = {
      DW_OP_breg0,
      DW_OP_breg1,
      DW_OP_breg2,
      DW_OP_breg3,
      DW_OP_breg4,
      DW_OP_breg5,
      DW_OP_breg6,
      DW_OP_breg7,
  };

  // struct parameters size larger than 8 are converted into pointers
  constexpr uint32 struct2PtrSize = 8;
  if (i < struct2PtrSize) {
    return baseOpRegs[i];
  }
  return DW_OP_breg0;
}

DBGDie *DebugInfo::CreateFormalParaDie(MIRFunction *func, uint32 idx, bool isDef) {
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_formal_parameter);

  TyIdx tyIdx = func->GetFormalDefAt(idx).formalTyIdx;
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  DBGDie *typeDie = GetOrCreateTypeDie(type);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());

  /* var */
  MIRSymbol *sym = func->GetFormalDefAt(idx).formalSym;
  if (isDef && sym) {
    die->AddAttr(DW_AT_name, DW_FORM_strp, sym->GetNameStrIdx().GetIdx());
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, sym->GetSrcPosition().FileNum());
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, sym->GetSrcPosition().LineNum());
    die->AddAttr(DW_AT_decl_column, DW_FORM_data4, sym->GetSrcPosition().Column());
    DwOp op = DW_OP_fbreg;
    if (type->IsStructType() && (static_cast<MIRStructType *>(type)->GetSize() > k64BitSize)) {
      op = GetBreg(idx);
    }
    die->AddSimpLocAttr(DW_AT_location, DW_FORM_exprloc, op, kDbgDefaultVal);
    SetLocalDie(func, sym->GetNameStrIdx(), die);
  }
  return die;
}

DBGDie *DebugInfo::GetOrCreateLabelDie(LabelIdx labid) {
  MIRFunction *func = GetCurFunction();
  CHECK(labid < func->GetLabelTab()->GetLabelTableSize(), "index out of range in DebugInfo::GetOrCreateLabelDie");
  GStrIdx strid = func->GetLabelTab()->GetSymbolFromStIdx(labid);
  if ((funcLstrIdxDieIdMap[func]).size() > 0 &&
      (funcLstrIdxDieIdMap[func]).find(strid.GetIdx()) != (funcLstrIdxDieIdMap[func]).end()) {
    return GetLocalDie(strid);
  }

  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_label);
  die->AddAttr(DW_AT_name, DW_FORM_strp, strid.GetIdx());
  die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx());
  die->AddAttr(DW_AT_decl_line, DW_FORM_data4, lexer->GetLineNum());
  die->AddAttr(DW_AT_low_pc, DW_FORM_addr, kDbgDefaultVal);
  GetParentDie()->AddSubVec(die);
  SetLocalDie(strid, die);
  SetLabelIdx(strid, labid);
  return die;
}

DBGDie *DebugInfo::CreateVarDie(MIRSymbol *sym) {
  // filter vtab
  if (sym->GetName().find(VTAB_PREFIX_STR) == 0) {
    return nullptr;
  }

  if (sym->GetName().find(GCTIB_PREFIX_STR) == 0) {
    return nullptr;
  }

  if (sym->GetStorageClass() == kScFormal) {
    return nullptr;
  }

  bool isLocal = sym->IsLocal();
  GStrIdx strIdx = sym->GetNameStrIdx();

  if (isLocal) {
    MIRFunction *func = GetCurFunction();
    if (!funcLstrIdxDieIdMap[func].empty() &&
        funcLstrIdxDieIdMap[func].find(strIdx.GetIdx()) != funcLstrIdxDieIdMap[func].end()) {
      return GetLocalDie(strIdx);
    }
  } else {
    if (globalStridxDieIdMap.find(strIdx.GetIdx()) != globalStridxDieIdMap.end()) {
      uint32 id = globalStridxDieIdMap[strIdx.GetIdx()];
      return idDieMap[id];
    }
  }

  DBGDie *die = CreateVarDie(sym, strIdx);
  if (!die) {
    return nullptr;
  }

  GetParentDie()->AddSubVec(die);
  if (isLocal) {
    SetLocalDie(strIdx, die);
  } else {
    globalStridxDieIdMap[strIdx.GetIdx()] = die->GetId();
  }

  return die;
}

DBGDie *DebugInfo::CreateVarDie(MIRSymbol *sym, const GStrIdx &strIdx) {
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_variable);

  /* var Name */
  die->AddAttr(DW_AT_name, DW_FORM_strp, strIdx.GetIdx());
  die->AddAttr(DW_AT_decl_file, DW_FORM_data4, sym->GetSrcPosition().FileNum());
  die->AddAttr(DW_AT_decl_line, DW_FORM_data4, sym->GetSrcPosition().LineNum());
  die->AddAttr(DW_AT_decl_column, DW_FORM_data4, sym->GetSrcPosition().Column());

  bool isLocal = sym->IsLocal();
  // TLS do not need normal DW_AT_location
  bool isThreadLocal = sym->IsThreadLocal();
  if (isLocal) {
    if (sym->IsPUStatic() && !isThreadLocal) {
      // Use actual internal sym by cg
      PUIdx pIdx = GetCurFunction()->GetPuidx();
      std::string ptrName = sym->GetName() + std::to_string(pIdx);
      uint64 idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(ptrName).GetIdx();
      die->AddGlobalLocAttr(DW_AT_location, DW_FORM_exprloc, idx);
    } else {
      die->AddSimpLocAttr(DW_AT_location, DW_FORM_exprloc, DW_OP_fbreg, kDbgDefaultVal);
    }
  } else if (!isThreadLocal) {
    // global var just use its name as address in .s
    uint64 idx = strIdx.GetIdx();
    if ((sym->IsReflectionClassInfo() && !sym->IsReflectionArrayClassInfo()) || sym->IsStatic()) {
      std::string ptrName = varPtrPrefix + sym->GetName();
      idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(ptrName).GetIdx();
    }
    die->AddGlobalLocAttr(DW_AT_location, DW_FORM_exprloc, idx);
  }

  MIRType *type = sym->GetType();
  DBGDie *typeDie = GetOrCreateTypeDie(type);
  DBGDie *newDie = GetOrCreateTypeDieWithAttr(sym->GetAttrs(), typeDie);
  if (!newDie) {
    return nullptr;
  }
  die->AddAttr(DW_AT_type, DW_FORM_ref4, newDie->GetId());

  return die;
}

DBGDie *DebugInfo::GetOrCreateFuncDeclDie(MIRFunction *func) {
  uint32 funcnameidx = func->GetNameStrIdx().GetIdx();
  if (stridxDieIdMap.find(funcnameidx) != stridxDieIdMap.end()) {
    uint32 id = stridxDieIdMap[funcnameidx];
    return idDieMap[id];
  }

  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subprogram);
  stridxDieIdMap[funcnameidx] = die->GetId();

  die->AddAttr(DW_AT_external, DW_FORM_flag_present, 1);

  // Function Name
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());

  die->AddAttr(DW_AT_name, DW_FORM_strp, funcnameidx);
  die->AddAttr(DW_AT_decl_file, DW_FORM_data4, sym->GetSrcPosition().FileNum());
  die->AddAttr(DW_AT_decl_line, DW_FORM_data4, sym->GetSrcPosition().LineNum());
  die->AddAttr(DW_AT_decl_column, DW_FORM_data4, sym->GetSrcPosition().Column());

  // Attributes for DW_AT_accessibility
  uint32 access = 0;
  if (func->IsPublic()) {
    access = DW_ACCESS_public;
  } else if (func->IsPrivate()) {
    access = DW_ACCESS_private;
  } else if (func->IsProtected()) {
    access = DW_ACCESS_protected;
  }
  if (access != 0) {
    die->AddAttr(DW_AT_accessibility, DW_FORM_data4, access);
  }

  die->AddAttr(DW_AT_GNU_all_tail_call_sites, DW_FORM_flag_present, kDbgDefaultVal);

  PushParentDie(die);

  // formal parameter
  for (uint32 i = 0; i < func->GetFormalCount(); i++) {
    DBGDie *param = CreateFormalParaDie(func, i, false);
    die->AddSubVec(param);
  }

  if (func->IsVarargs()) {
    DBGDie *varargDie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_unspecified_parameters);
    die->AddSubVec(varargDie);
  }

  PopParentDie();

  return die;
}

bool LIsCompilerGenerated(const MIRFunction *func) {
  return ((func->GetName().c_str())[0] != 'L');
}

void DebugInfo::CreateFuncLocalSymbolsDies(MIRFunction *func, DBGDie *die) {
  if (func->GetSymTab()) {
    // local variables, start from 1
    for (uint32 i = 1; i < func->GetSymTab()->GetSymbolTableSize(); i++) {
      MIRSymbol *var = func->GetSymTab()->GetSymbolFromStIdx(i);
      DBGDie *vdie = CreateVarDie(var);
      if (vdie == nullptr) {
        continue;
      }
      die->AddSubVec(vdie);
      // for C, source variable names will be used instead of mangloed maple variables
      if (module->IsCModule()) {
        vdie->SetKeep(false);
      }
    }
  }
}

DBGDie *DebugInfo::GetOrCreateFuncDefDie(MIRFunction *func) {
  uint32 funcnameidx = func->GetNameStrIdx().GetIdx();
  if (funcDefStrIdxDieIdMap.find(funcnameidx) != funcDefStrIdxDieIdMap.end()) {
    uint32 id = funcDefStrIdxDieIdMap[funcnameidx];
    return idDieMap[id];
  }

  DBGDie *funcdecldie = GetOrCreateFuncDeclDie(func);
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subprogram);
  // update funcDefStrIdxDieIdMap and leave stridxDieIdMap for the func decl
  funcDefStrIdxDieIdMap[funcnameidx] = die->GetId();

  die->AddAttr(DW_AT_specification, DW_FORM_ref4, funcdecldie->GetId());
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
  die->AddAttr(DW_AT_decl_line, DW_FORM_data4, sym->GetSrcPosition().LineNum());

  if (!func->IsReturnVoid()) {
    auto returnType = func->GetReturnType();
    DBGDie *typeDie = GetOrCreateTypeDie(returnType);
    die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());
  }

  die->AddAttr(DW_AT_low_pc, DW_FORM_addr, kDbgDefaultVal);
  die->AddAttr(DW_AT_high_pc, DW_FORM_data8, kDbgDefaultVal);
  die->AddFrmBaseAttr(DW_AT_frame_base, DW_FORM_exprloc);
  if (!func->IsStatic() && !LIsCompilerGenerated(func)) {
    die->AddAttr(DW_AT_object_pointer, DW_FORM_ref4, kDbgDefaultVal);
  }
  die->AddAttr(DW_AT_GNU_all_tail_call_sites, DW_FORM_flag_present, kDbgDefaultVal);

  PushParentDie(die);

  // formal parameter
  for (uint32 i = 0; i < func->GetFormalCount(); i++) {
    DBGDie *pdie = CreateFormalParaDie(func, i, true);
    die->AddSubVec(pdie);
  }

  CreateFuncLocalSymbolsDies(func, die);

  // add scope die
  AddScopeDie(func->GetScope());
  CollectScopePos(func, func->GetScope());

  PopParentDie();

  return die;
}

DBGDie *DebugInfo::GetOrCreatePrimTypeDie(MIRType *ty) {
  PrimType pty = ty->GetPrimType();
  uint32 tid = static_cast<uint32>(pty);
  if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
    uint32 id = tyIdxDieIdMap[tid];
    return idDieMap[id];
  }

  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_base_type);
  die->SetTyIdx(static_cast<uint32>(pty));

  GStrIdx strIdx = ty->GetNameStrIdx();
  if (strIdx.GetIdx() == 0) {
    std::string pname = std::string(GetPrimTypeName(ty->GetPrimType()));
    strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(pname);
    ty->SetNameStrIdx(strIdx);
  }

  die->AddAttr(DW_AT_byte_size, DW_FORM_data4, GetPrimTypeSize(pty));
  die->AddAttr(DW_AT_encoding, DW_FORM_data4, GetAteFromPTY(pty));

  // use C type name, int for i32 etc
  if (module->IsCModule()) {
    GStrIdx idx = GetPrimTypeCName(ty->GetPrimType());
    if (idx.GetIdx() != 0) {
      strIdx = idx;
    }
  }
  die->AddAttr(DW_AT_name, DW_FORM_strp, strIdx.GetIdx());

  compUnit->AddSubVec(die);
  tyIdxDieIdMap[static_cast<uint32>(pty)] = die->GetId();
  return die;
}

// At present, in order to solve the inaccurate expression of GetOrCreatePrimTypeDie for base types,
// e.g. long or long long. We can consider using this interface uniformly for base types in the future.
DBGDie *DebugInfo::GetOrCreateBaseTypeDie(const MIRType *type) {
  uint32 tid = type->GetTypeIndex().GetIdx();
  if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
    uint32 id = tyIdxDieIdMap[tid];
    return idDieMap[id];
  }

  auto iter = baseTypeMap.find(type->GetNameStrIdx());
  if (iter == baseTypeMap.cend()) {
    return nullptr;
  }
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_base_type);
  die->AddAttr(DW_AT_name, DW_FORM_strp, iter->second.first.GetIdx());
  die->AddAttr(DW_AT_byte_size, DW_FORM_data4, GetPrimTypeSize(iter->second.second));
  die->AddAttr(DW_AT_encoding, DW_FORM_data4, GetAteFromPTY(iter->second.second));

  compUnit->AddSubVec(die);
  tyIdxDieIdMap[type->GetTypeIndex().GetIdx()] = die->GetId();
  return die;
}

DBGDie *DebugInfo::CreatePointedFuncTypeDie(MIRFuncType *fType) {
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subroutine_type);

  die->AddAttr(DW_AT_prototyped, DW_FORM_data4, static_cast<int>(fType->GetParamTypeList().size() > 0));
  MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fType->GetRetTyIdx());
  DBGDie *retTypeDie = GetOrCreateTypeDie(retType);
  retTypeDie = GetOrCreateTypeDieWithAttr(fType->GetRetAttrs(), retTypeDie);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, retTypeDie->GetId());

  compUnit->AddSubVec(die);

  for (uint32 i = 0; i < fType->GetParamTypeList().size(); i++) {
    DBGDie *paramDie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_formal_parameter);
    MIRType *paramType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fType->GetNthParamType(i));
    DBGDie *paramTypeDie = GetOrCreateTypeDie(paramType);
    paramDie->AddAttr(DW_AT_type, DW_FORM_ref4, paramTypeDie->GetId());
    die->AddSubVec(paramDie);
  }

  tyIdxDieIdMap[fType->GetTypeIndex().GetIdx()] = die->GetId();
  return die;
}

DBGDie *DebugInfo::GetOrCreateTypeDieWithAttr(AttrKind attr, DBGDie *typeDie) {
  DBGDie *die = typeDie;
  uint32 dieId = typeDie->GetId();
  uint32 newId = 0;
  switch (attr) {
    case ATTR_const:
      if (constTypeDieMap.find(dieId) != constTypeDieMap.end()) {
        newId = constTypeDieMap[dieId];
        die = idDieMap[newId];
      } else {
        die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_const_type);
        die->AddAttr(DW_AT_type, DW_FORM_ref4, dieId);
        compUnit->AddSubVec(die);
        newId = die->GetId();
        constTypeDieMap[dieId] = newId;
      }
      break;
    case ATTR_volatile:
      if (volatileTypeDieMap.find(dieId) != volatileTypeDieMap.end()) {
        newId = volatileTypeDieMap[dieId];
        die = idDieMap[newId];
      } else {
        die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_volatile_type);
        die->AddAttr(DW_AT_type, DW_FORM_ref4, dieId);
        compUnit->AddSubVec(die);
        newId = die->GetId();
        volatileTypeDieMap[dieId] = newId;
      }
      break;
    default:
      break;
  }
  return die;
}

DBGDie *DebugInfo::GetOrCreateTypeDieWithAttr(const TypeAttrs &attrs, DBGDie *typeDie) {
  if (attrs.GetAttr(ATTR_const)) {
    typeDie = GetOrCreateTypeDieWithAttr(ATTR_const, typeDie);
  }
  if (attrs.GetAttr(ATTR_volatile)) {
    typeDie = GetOrCreateTypeDieWithAttr(ATTR_volatile, typeDie);
  }
  return typeDie;
}

DBGDie *DebugInfo::GetOrCreateTypeDie(TyIdx tyidx) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyidx);
  return GetOrCreateTypeDie(type);
}

DBGDie *DebugInfo::GetOrCreateTypeDie(MIRType *type) {
  if (type == nullptr) {
    return nullptr;
  }

  uint32 tid = type->GetTypeIndex().GetIdx();
  if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
    uint32 id = tyIdxDieIdMap[tid];
    return idDieMap[id];
  }

  uint32 sid = type->GetNameStrIdx().GetIdx();
  if (sid != 0 && stridxDieIdMap.find(sid) != stridxDieIdMap.end()) {
    uint32 id = stridxDieIdMap[sid];
    return idDieMap[id];
  }

  if (type->GetTypeIndex() == static_cast<uint32>(type->GetPrimType())) {
    return GetOrCreatePrimTypeDie(type);
  }

  DBGDie *die = nullptr;
  switch (type->GetKind()) {
    case kTypePointer:
      die = GetOrCreatePointTypeDie(static_cast<MIRPtrType *>(type));
      break;
    case kTypeFunction:
      die = CreatePointedFuncTypeDie(static_cast<MIRFuncType *>(type));
      break;
    case kTypeArray:
    case kTypeFArray:
    case kTypeJArray:
      die = GetOrCreateArrayTypeDie(static_cast<MIRArrayType *>(type));
      break;
    case kTypeUnion:
    case kTypeStruct:
    case kTypeStructIncomplete:
    case kTypeClass:
    case kTypeClassIncomplete:
    case kTypeInterface:
    case kTypeInterfaceIncomplete: {
      die = GetOrCreateStructTypeDie(type);
      break;
    }
    case kTypeBitField:
      break;
    case kTypeByName: {
      die = GetOrCreateTypeByNameDie(*type);
      break;
    }
    default:
      CHECK_FATAL(false, "TODO: support type");
      break;
  }

  if (die) {
    tyIdxDieIdMap[tid] = die->GetId();
  }

  return die;
}

DBGDie *DebugInfo::GetOrCreateTypedefDie(GStrIdx stridx, TyIdx tyidx) {
  uint32 sid = stridx.GetIdx();
  auto it = stridxDieIdMap.find(sid);
  if (it != stridxDieIdMap.end()) {
    return idDieMap[it->second];
  }

  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_typedef);
  compUnit->AddSubVec(die);

  die->AddAttr(DW_AT_name, DW_FORM_strp, sid);
  die->AddAttr(DW_AT_decl_file, DW_FORM_data1, 0);
  die->AddAttr(DW_AT_decl_line, DW_FORM_data1, 0);
  die->AddAttr(DW_AT_decl_column, DW_FORM_data1, 0);

  DBGDie *typeDie = GetOrCreateTypeDie(tyidx);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());

  stridxDieIdMap[sid] = die->GetId();
  return die;
}

DBGDie *DebugInfo::GetOrCreateEnumTypeDie(uint32 idx) {
  MIREnum *mirEnum = GlobalTables::GetEnumTable().enumTable[idx];
  return GetOrCreateEnumTypeDie(mirEnum);
}

DBGDie *DebugInfo::GetOrCreateEnumTypeDie(const MIREnum *mirEnum) {
  uint32 sid = mirEnum->GetNameIdx().GetIdx();
  auto it = stridxDieIdMap.find(sid);
  if (it != stridxDieIdMap.end()) {
    return idDieMap[it->second];
  }

  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_enumeration_type);

  PrimType pty = mirEnum->GetPrimType();
  // check if it is an anonymous enum
  const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(mirEnum->GetNameIdx());
  bool keep = (name.find("unnamed.") == std::string::npos);

  die->AddAttr(DW_AT_name, DW_FORM_strp, sid, keep);
  die->AddAttr(DW_AT_encoding, DW_FORM_data4, GetAteFromPTY(pty));
  die->AddAttr(DW_AT_byte_size, DW_FORM_data1, GetPrimTypeSize(pty));
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pty);
  DBGDie *typeDie = GetOrCreateTypeDie(type);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());

  for (auto &elemIt : mirEnum->GetElements()) {
    DBGDie *elem = module->GetMemPool()->New<DBGDie>(module, DW_TAG_enumerator);
    elem->AddAttr(DW_AT_name, DW_FORM_strp, elemIt.first.GetIdx());
    elem->AddAttr(DW_AT_const_value, DW_FORM_data8, static_cast<uint64>(elemIt.second.GetExtValue()));
    die->AddSubVec(elem);
  }

  stridxDieIdMap[sid] = die->GetId();
  return die;
}

DBGDie *DebugInfo::GetOrCreatePointTypeDie(const MIRPtrType *ptrType) {
  uint32 tid = ptrType->GetTypeIndex().GetIdx();
  if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
    uint32 id = tyIdxDieIdMap[tid];
    return idDieMap[id];
  }

  MIRType *type = ptrType->GetPointedType();
  DBGDie *typeDie = GetOrCreateTypeDie(type);
  typeDie = GetOrCreateTypeDieWithAttr(ptrType->GetTypeAttrs(), typeDie);
  // for <* void> and <func >
  if ((type != nullptr) &&
      (type->GetPrimType() == PTY_void || type->GetKind() == kTypeFunction)) {
    DBGDie *die = nullptr;
    if (type->GetKind() == kTypeFunction) {
      // for maple's function pointer type, function type should be used in dwarf
      die = typeDie;
      tyIdxDieIdMap[type->GetTypeIndex().GetIdx()] = die->GetId();
    } else {
      die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_pointer_type);
      die->AddAttr(DW_AT_byte_size, DW_FORM_data4, k8BitSize);
    }
    die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());
    tyIdxDieIdMap[ptrType->GetTypeIndex().GetIdx()] = die->GetId();
    compUnit->AddSubVec(die);
    return die;
  }

  if (typeDefTyIdxMap.find(type->GetTypeIndex().GetIdx()) != typeDefTyIdxMap.end()) {
    uint32 tyIdx = typeDefTyIdxMap[type->GetTypeIndex().GetIdx()];
    if (pointedPointerMap.find(tyIdx) != pointedPointerMap.end()) {
      uint32 tyid = pointedPointerMap[tyIdx];
      if (tyIdxDieIdMap.find(tyid) != tyIdxDieIdMap.end()) {
        uint32 dieid = tyIdxDieIdMap[tyid];
        DBGDie *die = idDieMap[dieid];
        return die;
      }
    }
  }

  // update incomplete type from stridxDieIdMap to tyIdxDieIdMap
  MIRStructType *stype = static_cast<MIRStructType*>(type);
  if ((stype != nullptr) && stype->IsIncomplete()) {
    uint32 sid = stype->GetNameStrIdx().GetIdx();
    if (stridxDieIdMap.find(sid) != stridxDieIdMap.end()) {
      uint32 dieid = stridxDieIdMap[sid];
      if (dieid != 0) {
        tyIdxDieIdMap[stype->GetTypeIndex().GetIdx()] = dieid;
      }
    }
  }

  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_pointer_type);
  die->AddAttr(DW_AT_byte_size, DW_FORM_data4, k8BitSize);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());
  tyIdxDieIdMap[ptrType->GetTypeIndex().GetIdx()] = die->GetId();

  compUnit->AddSubVec(die);

  return die;
}

DBGDie *DebugInfo::GetOrCreateArrayTypeDie(const MIRArrayType *arrayType) {
  uint32 tid = arrayType->GetTypeIndex().GetIdx();
  if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
    uint32 id = tyIdxDieIdMap[tid];
    return idDieMap[id];
  }

  MIRType *type = arrayType->GetElemType();
  DBGDie *typeDie = GetOrCreateTypeDie(type);
  typeDie = GetOrCreateTypeDieWithAttr(arrayType->GetTypeAttrs(), typeDie);

  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_array_type);
  die->AddAttr(DW_AT_byte_size, DW_FORM_data4, k8BitSize);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());
  tyIdxDieIdMap[arrayType->GetTypeIndex().GetIdx()] = die->GetId();

  compUnit->AddSubVec(die);

  // maple uses array of 1D array to represent 2D array
  // so only one DW_TAG_subrange_type entry is needed default
  uint16 dim = 1;
  if (theMIRModule->IsCModule() || theMIRModule->IsJavaModule()) {
    dim = arrayType->GetDim();
  }
  typeDie = GetOrCreateTypeDie(TyIdx(PTY_u32));
  bool keep = !arrayType->IsIncompleteArray();
  for (uint32 i = 0; i < dim; ++i) {
    DBGDie *rangeDie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subrange_type);
    rangeDie->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId(), keep);
    // The default lower bound value for C, C++, or Java is 0
    rangeDie->AddAttr(DW_AT_upper_bound, DW_FORM_data4, arrayType->GetSizeArrayItem(i) - 1UL, keep);
    die->AddSubVec(rangeDie);
  }

  return die;
}

DBGDie *DebugInfo::CreateFieldDie(const maple::FieldPair &pair) {
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_member);

  const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(pair.first);
  bool keep = (name.find("unnamed.") == std::string::npos);
  die->AddAttr(DW_AT_name, DW_FORM_strp, pair.first.GetIdx(), keep);
  die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx(), keep);

  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pair.second.first);
  DBGDie *typeDie = GetOrCreateTypeDie(type);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());

  die->AddAttr(DW_AT_data_member_location, DW_FORM_data4, kDbgDefaultVal);

  return die;
}

DBGDie *DebugInfo::CreateBitfieldDie(const MIRBitFieldType *type, const GStrIdx &sidx, uint32 &prevBits) {
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_member);

  die->AddAttr(DW_AT_name, DW_FORM_strp, sidx.GetIdx());
  die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx());

  MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetPrimType());
  DBGDie *typeDie = GetOrCreateTypeDie(ty);
  die->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());

  die->AddAttr(DW_AT_byte_size, DW_FORM_data4, GetPrimTypeSize(type->GetPrimType()));
  die->AddAttr(DW_AT_bit_size, DW_FORM_data4, type->GetFieldSize());

  uint32 typeSize = GetPrimTypeSize(type->GetPrimType()) * k8BitSize;
  prevBits = type->GetFieldSize() + prevBits > typeSize ? type->GetFieldSize() : type->GetFieldSize() + prevBits;
  uint32 offset = typeSize - prevBits;

  die->AddAttr(DW_AT_bit_offset, DW_FORM_data4, offset);
  die->AddAttr(DW_AT_data_member_location, DW_FORM_data4, 0);

  return die;
}

DBGDie *DebugInfo::GetOrCreateStructTypeDie(const MIRType *type) {
  ASSERT(type, "null struture type");
  GStrIdx strIdx = type->GetNameStrIdx();
  ASSERT(strIdx.GetIdx(), "struture type missing name");

  uint32 tid = type->GetTypeIndex().GetIdx();
  if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
    uint32 id = tyIdxDieIdMap[tid];
    return idDieMap[id];
  }

  DBGDie *die = nullptr;
  switch (type->GetKind()) {
    case kTypeClass:
    case kTypeClassIncomplete:
      die = CreateClassTypeDie(strIdx, static_cast<const MIRClassType *>(type));
      break;
    case kTypeInterface:
    case kTypeInterfaceIncomplete:
      die = CreateInterfaceTypeDie(strIdx, static_cast<const MIRInterfaceType *>(type));
      break;
    case kTypeStruct:
    case kTypeStructIncomplete:
    case kTypeUnion:
      die = CreateStructTypeDie(strIdx, static_cast<const MIRStructType *>(type), false);
      break;
    default:
#ifdef DEBUG
      LogInfo::MapleLogger() << "named type "
                             << GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx).c_str()
                             << "\n";
#endif /* DEBUG */
      break;
  }

  GlobalTables::GetTypeNameTable().SetGStrIdxToTyIdx(strIdx, type->GetTypeIndex());

  if (die) {
    tyIdxDieIdMap[type->GetTypeIndex().GetIdx()] = die->GetId();
  }
  return die;
}

void DebugInfo::CreateStructTypeFieldsDies(const MIRStructType *structType, DBGDie *die) {
  uint32 prevBits = 0;
  for (size_t i = 0; i < structType->GetFieldsSize(); i++) {
    MIRType *elemType = structType->GetElemType(static_cast<uint32>(i));
    FieldPair fp = structType->GetFieldsElemt(i);
    if (elemType->IsMIRBitFieldType()) {
      if (die->GetTag() == DW_TAG_union_type) {
        prevBits = 0;
      }
      MIRBitFieldType *bitFieldType = static_cast<MIRBitFieldType*>(elemType);
      DBGDie *bfDie = CreateBitfieldDie(bitFieldType, fp.first, prevBits);
      die->AddSubVec(bfDie);
    } else {
      if (die->GetTag() != DW_TAG_union_type) {
        prevBits = GetPrimTypeSize(elemType->GetPrimType()) * k8BitSize;
      }
      DBGDie *fieldDie = CreateFieldDie(fp);
      die->AddSubVec(fieldDie);

      // update field type with alias info
      const MIRAlias *alias = structType->GetAlias();
      if (!alias) {
        continue;
      }

      for (auto &aliasVar : alias->GetAliasVarMap()) {
        if (aliasVar.first == fp.first) {
          DBGDie *typeDie = GetAliasVarTypeDie(aliasVar.second, fp.second.first);
          fieldDie->SetAttr(DW_AT_type, typeDie->GetId());
          break;
        }
      }
    }
  }
}

void DebugInfo::CreateStructTypeParentFieldsDies(const MIRStructType *structType, DBGDie *die) {
  for (size_t i = 0; i < structType->GetParentFieldsSize(); i++) {
    FieldPair fp = structType->GetParentFieldsElemt(i);
    DBGDie *fieldDie = CreateFieldDie(fp);
    die->AddSubVec(fieldDie);
  }
}

void DebugInfo::CreateStructTypeMethodsDies(const MIRStructType *structType, DBGDie *die) {
  // member functions decl
  for (auto fp : structType->GetMethods()) {
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(fp.first.Idx());
    ASSERT((symbol != nullptr) && symbol->GetSKind() == kStFunc, "member function symbol not exist");
    MIRFunction *func = symbol->GetValue().mirFunc;
    ASSERT(func, "member function not exist");
    DBGDie *funDeclDie = GetOrCreateFuncDeclDie(func);
    die->AddSubVec(funDeclDie);
    if (func->GetBody()) {
      // member functions defination, these die are global
      DBGDie *funDefDie = GetOrCreateFuncDefDie(func);
      compUnit->AddSubVec(funDefDie);
    }
  }
}

// shared between struct and union, also used as part by class and interface
DBGDie *DebugInfo::CreateStructTypeDie(const GStrIdx &strIdx, const MIRStructType *structType, bool update) {
  DBGDie *die = nullptr;
  uint32 tid = structType->GetTypeIndex().GetIdx();

  if (update) {
    ASSERT(tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end(), "update type die not exist");
    uint32 id = tyIdxDieIdMap[tid];
    die = idDieMap[id];
    ASSERT(die, "update type die not exist");
  } else {
    DwTag tag = structType->GetKind() == kTypeUnion ? DW_TAG_union_type : DW_TAG_structure_type;
    die = module->GetMemPool()->New<DBGDie>(module, tag);
    tyIdxDieIdMap[tid] = die->GetId();
  }
  die = GetOrCreateTypeDieWithAttr(structType->GetTypeAttrs(), die);

  if (strIdx.GetIdx() != 0) {
    stridxDieIdMap[strIdx.GetIdx()] = die->GetId();
  }

  compUnit->AddSubVec(die);

  const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx);
  bool keep = (name.find("unnamed.") == std::string::npos);
  if (structType->GetKind() == kTypeStructIncomplete) {
    die->AddAttr(DW_AT_name, DW_FORM_strp, strIdx.GetIdx(), keep);
    die->AddAttr(DW_AT_declaration, DW_FORM_data4, 1);
  } else {
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, kStructDBGSize);
    die->AddAttr(DW_AT_name, DW_FORM_strp, strIdx.GetIdx(), keep);
    die->AddAttr(DW_AT_byte_size, DW_FORM_data4, kDbgDefaultVal);
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx());
  }
  
  // store tid for cg emitter
  die->AddAttr(DW_AT_type, DW_FORM_data4, tid, false);

  PushParentDie(die);

  // fields
  CreateStructTypeFieldsDies(structType, die);

  // parentFields
  CreateStructTypeParentFieldsDies(structType, die);

  // member functions
  CreateStructTypeMethodsDies(structType, die);

  PopParentDie();

  return die;
}

DBGDie *DebugInfo::CreateClassTypeDie(const GStrIdx &strIdx, const MIRClassType *classType) {
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_class_type);

  PushParentDie(die);

  // parent
  uint32 ptid = classType->GetParentTyIdx().GetIdx();
  if (ptid != 0) {
    MIRType *parenttype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(classType->GetParentTyIdx());
    DBGDie *parentDie = GetOrCreateTypeDie(parenttype);
    if (parentDie) {
      parentDie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_inheritance);
      parentDie->AddAttr(DW_AT_name, DW_FORM_strp, parenttype->GetNameStrIdx().GetIdx());
      DBGDie *typeDie = GetOrCreateStructTypeDie(classType);
      parentDie->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());

      // set to DW_ACCESS_public for now
      parentDie->AddAttr(DW_AT_accessibility, DW_FORM_data4, DW_ACCESS_public);
      die->AddSubVec(parentDie);
    }
  }

  PopParentDie();

  // update common fields
  tyIdxDieIdMap[classType->GetTypeIndex().GetIdx()] = die->GetId();
  DBGDie *die1 = CreateStructTypeDie(strIdx, classType, true);
  ASSERT(die == die1, "ClassTypeDie update wrong die");

  return die1;
}

DBGDie *DebugInfo::CreateInterfaceTypeDie(const GStrIdx &strIdx, const MIRInterfaceType *interfaceType) {
  DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_interface_type);

  PushParentDie(die);

  // parents
  for (auto it : interfaceType->GetParentsTyIdx()) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it);
    DBGDie *parentDie = GetOrCreateTypeDie(type);
    if (parentDie) {
      continue;
    }
    parentDie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_inheritance);
    parentDie->AddAttr(DW_AT_name, DW_FORM_strp, type->GetNameStrIdx().GetIdx());
    DBGDie *typeDie = GetOrCreateStructTypeDie(interfaceType);
    parentDie->AddAttr(DW_AT_type, DW_FORM_ref4, typeDie->GetId());
    parentDie->AddAttr(DW_AT_data_member_location, DW_FORM_data4, kDbgDefaultVal);

    // set to DW_ACCESS_public for now
    parentDie->AddAttr(DW_AT_accessibility, DW_FORM_data4, DW_ACCESS_public);
    die->AddSubVec(parentDie);
  }

  PopParentDie();

  // update common fields
  tyIdxDieIdMap[interfaceType->GetTypeIndex().GetIdx()] = die->GetId();
  DBGDie *die1 = CreateStructTypeDie(strIdx, interfaceType, true);
  ASSERT(die == die1, "InterfaceTypeDie update wrong die");

  return die1;
}

uint32 DebugInfo::GetAbbrevId(DBGAbbrevEntryVec *vec, DBGAbbrevEntry *entry) const {
  for (auto it : vec->GetEntryvec()) {
    if (it->Equalto(entry)) {
      return it->GetAbbrevId();
    }
  }
  return 0;
}

void DebugInfo::BuildAbbrev() {
  uint32 abbrevid = 1;
  for (uint32 i = 1; i < maxId; i++) {
    DBGDie *die = idDieMap[i];
    DBGAbbrevEntry *entry = module->GetMemPool()->New<DBGAbbrevEntry>(module, die);

    if (!tagAbbrevMap[die->GetTag()]) {
      tagAbbrevMap[die->GetTag()] = module->GetMemPool()->New<DBGAbbrevEntryVec>(module, die->GetTag());
    }

    uint32 id = GetAbbrevId(tagAbbrevMap[die->GetTag()], entry);
    if (id != 0) {
      // using existing abbrev id
      die->SetAbbrevId(id);
    } else {
      // add entry to vector
      entry->SetAbbrevId(abbrevid++);
      tagAbbrevMap[die->GetTag()]->GetEntryvec().push_back(entry);
      abbrevVec.push_back(entry);
      // update abbrevid in die
      die->SetAbbrevId(entry->GetAbbrevId());
    }
  }
  for (uint32 i = 1; i < maxId; i++) {
    DBGDie *die = idDieMap[i];
    if (die->GetAbbrevId() == 0) {
      LogInfo::MapleLogger() << "0 abbrevId i = " << i << " die->id = " << die->GetId() << std::endl;
    }
  }
}

void DebugInfo::BuildDieTree() {
  for (auto it : idDieMap) {
    if (it.first == 0) {
      continue;
    }
    DBGDie *die = it.second;
    uint32 size = die->GetSubDieVecSize();
    die->SetWithChildren(size > 0);
    if (size != 0) {
      die->SetFirstChild(die->GetSubDieVecAt(0));
      for (uint32 i = 0; i < size - 1; i++) {
        DBGDie *it0 = die->GetSubDieVecAt(i);
        DBGDie *it1 = die->GetSubDieVecAt(i + 1);
        if (it0->GetSubDieVecSize() != 0) {
          it0->SetSibling(it1);
          it0->AddAttr(DW_AT_sibling, DW_FORM_ref4, it1->GetId());
        }
      }
    }
  }
}

DBGDie *DebugInfo::GetFuncDie(const MIRFunction *func, bool isDeclDie) {
  uint32 id = isDeclDie ? stridxDieIdMap[func->GetNameStrIdx().GetIdx()] :
                          funcDefStrIdxDieIdMap[func->GetNameStrIdx().GetIdx()];
  if (id != 0) {
    return idDieMap[id];
  }
  return nullptr;
}

// Methods for calculating Offset and Size of DW_AT_xxx
size_t DBGDieAttr::SizeOf(DBGDieAttr *attr) const {
  DwForm form = attr->dwForm;
  switch (form) {
    // case DW_FORM_implicitconst:
    case DW_FORM_flag_present:
      return 0;  // Not handled yet.
    case DW_FORM_flag:
    case DW_FORM_ref1:
    case DW_FORM_data1:
      return sizeof(int8);
    case DW_FORM_ref2:
    case DW_FORM_data2:
      return sizeof(int16);
    case DW_FORM_ref4:
    case DW_FORM_data4:
      return sizeof(int32);
    case DW_FORM_ref8:
    case DW_FORM_ref_sig8:
    case DW_FORM_data8:
      return sizeof(int64);
    case DW_FORM_addr:
      return sizeof(int64);
    case DW_FORM_sec_offset:
    case DW_FORM_ref_addr:
    case DW_FORM_strp:
    case DW_FORM_GNU_ref_alt:
      // case DW_FORM_codeLinestrp:
      // case DW_FORM_strp_sup:
      // case DW_FORM_ref_sup:
      return k4BitSize;  // DWARF32, 8 if DWARF64

    case DW_FORM_string: {
      GStrIdx stridx(attr->value.id);
      const std::string &str = GlobalTables::GetStrTable().GetStringFromStrIdx(stridx);
      return str.length() + 1; /* terminal null byte */
    }
    case DW_FORM_exprloc: {
      CHECK_FATAL(attr->value.u != kDeadbeef, "wrong ptr");
      DBGExprLoc *ptr = attr->value.ptr;
      switch (ptr->GetOp()) {
        case DW_OP_call_frame_cfa:
          return k2BitSize;  // size 1 byte + DW_OP_call_frame_cfa 1 byte
        case DW_OP_fbreg: {
          // DW_OP_fbreg 1 byte
          size_t size = 1 + namemangler::GetSleb128Size(ptr->GetFboffset());
          return size + namemangler::GetUleb128Size(size);
        }
        case DW_OP_breg0:
        case DW_OP_breg1:
        case DW_OP_breg2:
        case DW_OP_breg3:
        case DW_OP_breg4:
        case DW_OP_breg5:
        case DW_OP_breg6:
        case DW_OP_breg7:
          return k3BitSize;
        case DW_OP_addr: {
          return namemangler::GetUleb128Size(k9BitSize) + k9BitSize;
        }
        default:
          return k4BitSize;
      }
    }
    default:
      LogInfo::MapleLogger() << "unhandled SizeOf: " << maple::GetDwFormName(form) << std::endl;
      CHECK_FATAL(maple::GetDwFormName(form) != nullptr, "null GetDwFormName(form)");
      return 0;
  }
}

void DebugInfo::ComputeSizeAndOffsets() {
  // CU-relative offset is reset to 0 here.
  uint32 cuOffset = sizeof(int32_t) +  // Length of Unit Info
                    sizeof(int16) +  // DWARF version number      : 0x0004
                    sizeof(int32) +  // Offset into Abbrev. Section : 0x0000
                    sizeof(int8);  // Pointer Size (in bytes)       : 0x08

  // After returning from this function, the length value is the size
  // of the .debug_info section
  ComputeSizeAndOffset(compUnit, cuOffset);
  debugInfoLength = cuOffset - sizeof(int32_t);
}

// Compute the size and offset of a DIE. The Offset is relative to start of the CU.
// It returns the offset after laying out the DIE.
void DebugInfo::ComputeSizeAndOffset(DBGDie *die, uint32 &cuOffset) {
  if (!die->GetKeep()) {
    return;
  }
  uint32 cuOffsetOrg = cuOffset;
  die->SetOffset(cuOffset);

  // Add the byte size of the abbreviation code
  cuOffset += static_cast<uint32>(namemangler::GetUleb128Size(uint64_t(die->GetAbbrevId())));

  // Add the byte size of all the DIE attributes.
  for (const auto &attr : die->GetAttrVec()) {
    if (!attr->GetKeep()) {
      continue;
    }
    cuOffset += static_cast<uint32>(attr->SizeOf(attr));
  }

  die->SetSize(cuOffset - cuOffsetOrg);

  // Let the children compute their offsets.
  if (die->GetWithChildren()) {
    uint32 size = die->GetSubDieVecSize();

    for (uint32 i = 0; i < size; i++) {
      DBGDie *childDie = die->GetSubDieVecAt(i);
      ComputeSizeAndOffset(childDie, cuOffset);
    }

    // Each child chain is terminated with a zero byte, adjust the offset.
    cuOffset += sizeof(int8);
  }
}

bool DebugInfo::IsScopeIdEmited(MIRFunction *func, uint32 scopeId) {
  auto it = funcScopeIdStatus.find(func);
  if (it == funcScopeIdStatus.end()) {
    return false;
  }

  auto scopeIdIt = it->second.find(scopeId);
  if (scopeIdIt == it->second.end()) {
    return false;
  }

  if (scopeIdIt->second != kAllEmited) {
    return false;
  }
  return true;
}

void DebugInfo::ClearDebugInfo() {
  for (auto &funcLstrIdxDieId : funcLstrIdxDieIdMap) {
    funcLstrIdxDieId.second.clear();
  }
  for (auto &funcLstrIdxLabIdx : funcLstrIdxLabIdxMap) {
    funcLstrIdxLabIdx.second.clear();
  }
  for (auto &funcScopeLow : funcScopeLows) {
    funcScopeLow.second.clear();
    funcScopeLow.second.shrink_to_fit();
  }
  for (auto &funcScopeHigh : funcScopeHighs) {
    funcScopeHigh.second.clear();
    funcScopeHigh.second.shrink_to_fit();
  }
  for (auto &status : funcScopeIdStatus) {
    status.second.clear();
  }
}

/* ///////////////
 * Dumps
 * ///////////////
 */
void DebugInfo::Dump(int indent) {
  LogInfo::MapleLogger() << "\n" << std::endl;
  LogInfo::MapleLogger() << "maple_debug_information {"
             << "  Length: " << HEX(debugInfoLength) << std::endl;
  compUnit->Dump(indent + 1);
  LogInfo::MapleLogger() << "}\n" << std::endl;
  LogInfo::MapleLogger() << "maple_debug_abbrev {" << std::endl;
  for (uint32 i = 1; i < abbrevVec.size(); i++) {
    abbrevVec[i]->Dump(indent + 1);
  }
  LogInfo::MapleLogger() << "}" << std::endl;
  return;
}

void DBGExprLoc::Dump() const {
  LogInfo::MapleLogger() << " " << HEX(GetOp());
  for (auto it : simpLoc->GetOpnd()) {
    LogInfo::MapleLogger() << " " << HEX(it);
  }
}

void DBGDieAttr::Dump(int indent) {
  PrintIndentation(indent);
  CHECK_FATAL(GetDwFormName(dwForm) && GetDwAtName(dwAttr), "null ptr check");
  LogInfo::MapleLogger() << GetDwAtName(dwAttr) << " " << GetDwFormName(dwForm);
  if (dwForm == DW_FORM_string || dwForm == DW_FORM_strp) {
    GStrIdx idx(value.id);
    LogInfo::MapleLogger() << " 0x" << std::hex << value.u << std::dec;
    LogInfo::MapleLogger() << " \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(idx).c_str() << "\"";
  } else if (dwForm == DW_FORM_ref4) {
    LogInfo::MapleLogger() << " <" << HEX(value.id) << ">";
  } else if (dwAttr == DW_AT_encoding) {
    CHECK_FATAL(GetDwAteName(static_cast<uint32>(value.u)), "null ptr check");
    LogInfo::MapleLogger() << " " << GetDwAteName(static_cast<uint32>(value.u));
  } else if (dwAttr == DW_AT_location) {
    value.ptr->Dump();
  } else {
    LogInfo::MapleLogger() << " 0x" << std::hex << value.u << std::dec;
  }
  LogInfo::MapleLogger() << std::endl;
}

void DBGDie::Dump(int indent) {
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "<" << HEX(id) << "><" << HEX(offset);
  LogInfo::MapleLogger() << "><" << HEX(size) << "><"
             << "> abbrev id: " << HEX(abbrevId);
  CHECK_FATAL(GetDwTagName(tag), "null ptr check");
  LogInfo::MapleLogger() << " (" << GetDwTagName(tag) << ") ";
  if (parent) {
    LogInfo::MapleLogger() << "parent <" << HEX(parent->GetId());
  }
  LogInfo::MapleLogger() << "> {";
  if (tyIdx != 0) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(tyIdx));
    if (type->GetKind() == kTypeStruct || type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface) {
      MIRStructType *stype = static_cast<MIRStructType *>(type);
      LogInfo::MapleLogger() << "           # " << stype->GetName();
    } else {
      LogInfo::MapleLogger() << "           # " << GetPrimTypeName(type->GetPrimType());
    }
  }
  LogInfo::MapleLogger() << std::endl;
  for (auto it : attrVec) {
    it->Dump(indent + 1);
  }
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "} ";
  if (subDieVec.size() != 0) {
    LogInfo::MapleLogger() << " {" << std::endl;
    for (auto it : subDieVec) {
      it->Dump(indent + 1);
    }
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "}";
  }
  LogInfo::MapleLogger() << std::endl;
  return;
}

void DBGAbbrevEntry::Dump(int indent) {
  PrintIndentation(indent);
  CHECK_FATAL(GetDwTagName(tag), "null ptr check ");
  LogInfo::MapleLogger() << "<" << HEX(abbrevId) << "> " << GetDwTagName(tag);
  if (GetWithChildren()) {
    LogInfo::MapleLogger() << " [with children] {" << std::endl;
  } else {
    LogInfo::MapleLogger() << " [no children] {" << std::endl;
  }
  for (uint32 i = 0; i < attrPairs.size(); i += k2BitSize) {
    PrintIndentation(indent + 1);
    CHECK_FATAL(GetDwAtName(attrPairs[i]) && GetDwFormName(attrPairs[i + 1]), "NULLPTR CHECK");

    LogInfo::MapleLogger() << " " << GetDwAtName(attrPairs[i]) << " " << GetDwFormName(attrPairs[i + 1])
        << " " << std::endl;
  }
  PrintIndentation(indent);
  LogInfo::MapleLogger() << "}" << std::endl;
  return;
}

void DBGAbbrevEntryVec::Dump(int indent) {
  for (auto it : entryVec) {
    PrintIndentation(indent);
    it->Dump(indent);
  }
  return;
}

// DBGCompileMsgInfo methods
void DBGCompileMsgInfo::ClearLine(uint32 n) {
  errno_t eNum = memset_s(codeLine[n], MAXLINELEN, 0, MAXLINELEN);
  if (eNum != 0) {
    FATAL(kLncFatal, "memset_s failed");
  }
}

DBGCompileMsgInfo::DBGCompileMsgInfo() : startLine(0), errPos(0) {
  lineNum[0] = 0;
  lineNum[1] = 0;
  lineNum[kIndx2] = 0;
  ClearLine(0);
  ClearLine(1);
  ClearLine(kIndx2);
  errLNum = 0;
  errCNum = 0;
}

void DBGCompileMsgInfo::SetErrPos(uint32 lnum, uint32 cnum) {
  errLNum = lnum;
  errCNum = cnum;
}

void DBGCompileMsgInfo::UpdateMsg(uint32 lnum, const char *line) {
  size_t size = strlen(line);
  if (size > MAXLINELEN - 1) {
    size = MAXLINELEN - 1;
  }
  startLine = (startLine + k2BitSize) % k3BitSize;
  ClearLine(startLine);
  errno_t eNum = memcpy_s(codeLine[startLine], MAXLINELEN, line, size);
  if (eNum != 0) {
    FATAL(kLncFatal, "memcpy_s failed");
  }
  codeLine[startLine][size] = '\0';
  lineNum[startLine] = lnum;
}

void DBGCompileMsgInfo::EmitMsg() {
  char str[MAXLINELEN + 1];

  errPos = errCNum;
  errPos = (errPos < k2BitSize) ? k2BitSize : errPos;
  errPos = (errPos > MAXLINELEN) ? MAXLINELEN : errPos;
  for (uint32 i = 0; i < errPos - 1; i++) {
    str[i] = ' ';
  }
  str[errPos - 1] = '^';
  str[errPos] = '\0';

  fprintf(stderr, "\n===================================================================\n");
  fprintf(stderr, "==================");
  fprintf(stderr, "\x1B[1m" "\x1B[31m" "  Compilation Error Diagnosis  " "\x1B[0m");
  fprintf(stderr, "==================\n");
  fprintf(stderr, "===================================================================\n");
  fprintf(stderr, "line %4u %s\n", lineNum[(startLine + k2BitSize) % k3BitSize],
          static_cast<unsigned char *>(codeLine[(startLine + k2BitSize) % k3BitSize]));
  fprintf(stderr, "line %4u %s\n", lineNum[(startLine + 1) % k3BitSize],
          static_cast<unsigned char *>(codeLine[(startLine + 1) % k3BitSize]));
  fprintf(stderr, "line %4u %s\n", lineNum[(startLine) % k3BitSize],
          static_cast<unsigned char *>(codeLine[(startLine) % k3BitSize]));
  fprintf(stderr, "\x1B[1m" "\x1B[31m" "          %s\n" "\x1B[0m", str);
  fprintf(stderr, "===================================================================\n");
}
}  // namespace maple
