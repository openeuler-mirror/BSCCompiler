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

#ifndef MAPLE_IR_INCLUDE_DBG_INFO_H
#define MAPLE_IR_INCLUDE_DBG_INFO_H
#include <iostream>

#include "mpl_logging.h"
#include "types_def.h"
#include "prim_types.h"
#include "mir_nodes.h"
#include "mir_scope.h"
#include "namemangler.h"
#include "lexer.h"
#include "dwarf.h"

namespace maple {
// for more color code: http://ascii-table.com/ansi-escape-sequences.php
#define RESET "\x1B[0m"
#define BOLD "\x1B[1m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"

const uint32 kDbgDefaultVal = 0xdeadbeef;
#define HEX(val) std::hex << "0x" << (val) << std::dec

class MIRModule;
class MIRType;
class MIRSymbol;
class MIRSymbolTable;
class MIRTypeNameTable;
class DBGBuilder;
class DBGCompileMsgInfo;
class MIRLexer;
class MIREnum;

// for compiletime warnings
class DBGLine {
 public:
  DBGLine(uint32 lnum, const char *l) : lineNum(lnum), codeLine(l) {}
  virtual ~DBGLine() {}

  void Dump() const {
    LogInfo::MapleLogger() << "LINE: " << lineNum << " " << codeLine << std::endl;
  }

 private:
  uint32 lineNum;
  const char *codeLine;
};

#define MAXLINELEN 4096

class DBGCompileMsgInfo {
 public:
  DBGCompileMsgInfo();
  virtual ~DBGCompileMsgInfo() {}
  void ClearLine(uint32 n);
  void SetErrPos(uint32 lnum, uint32 cnum);
  void UpdateMsg(uint32 lnum, const char *line);
  void EmitMsg();

 private:
  uint32 startLine;  // mod 3
  uint32 errLNum;
  uint32 errCNum;
  uint32 errPos;
  uint32 lineNum[3];
  uint8 codeLine[3][MAXLINELEN];  // 3 round-robin line buffers
};

enum DBGDieKind { kDwTag, kDwAt, kDwOp, kDwAte, kDwForm, kDwCfa };

using DwTag = uint32;   // for DW_TAG_*
using DwAt = uint32;    // for DW_AT_*
using DwOp = uint32;    // for DW_OP_*
using DwAte = uint32;   // for DW_ATE_*
using DwForm = uint32;  // for DW_FORM_*
using DwCfa = uint32;   // for DW_CFA_*

class DBGDieAttr;

class DBGExpr {
 public:
  explicit DBGExpr(MIRModule *m) : dwOp(0), value(kDbgDefaultVal), opnds(m->GetMPAllocator().Adapter()) {}

  DBGExpr(MIRModule *m, DwOp op) : dwOp(op), value(kDbgDefaultVal), opnds(m->GetMPAllocator().Adapter()) {}

  virtual ~DBGExpr() {}

  void AddOpnd(uint64 val) {
    opnds.push_back(val);
  }

  int GetVal() const {
    return value;
  }

  void SetVal(int v) {
    value = v;
  }

  DwOp GetDwOp() const {
    return dwOp;
  }

  void SetDwOp(DwOp op) {
    dwOp = op;
  }

  MapleVector<uint64> &GetOpnd() {
    return opnds;
  }

  size_t GetOpndSize() const {
    return opnds.size();
  }

  void Clear() {
    return opnds.clear();
  }

 private:
  DwOp dwOp;
  // for local var fboffset, global var strIdx
  int value;
  MapleVector<uint64> opnds;
};

class DBGExprLoc {
 public:
  explicit DBGExprLoc(MIRModule *m) : module(m), exprVec(m->GetMPAllocator().Adapter()), symLoc(nullptr) {
    simpLoc = m->GetMemPool()->New<DBGExpr>(module);
  }

  DBGExprLoc(MIRModule *m, DwOp op) : module(m), exprVec(m->GetMPAllocator().Adapter()), symLoc(nullptr) {
    simpLoc = m->GetMemPool()->New<DBGExpr>(module, op);
  }

  virtual ~DBGExprLoc() {}

  bool IsSimp() const {
    return (exprVec.size() == 0 && simpLoc->GetVal() != static_cast<int>(kDbgDefaultVal));
  }

  int GetFboffset() const {
    return simpLoc->GetVal();
  }

  void SetFboffset(int offset) {
    simpLoc->SetVal(offset);
  }

  int GetGvarStridx() const {
    return simpLoc->GetVal();
  }

  void SetGvarStridx(int idx) {
    simpLoc->SetVal(idx);
  }

  DwOp GetOp() const {
    return simpLoc->GetDwOp();
  }

  uint32 GetSize() const {
    return static_cast<uint32>(simpLoc->GetOpndSize());
  }

  void ClearOpnd() {
    simpLoc->Clear();
  }

  void AddSimpLocOpnd(uint64 val) {
    simpLoc->AddOpnd(val);
  }

  DBGExpr *GetSimpLoc() const {
    return simpLoc;
  }

  void *GetSymLoc() {
    return symLoc;
  }

  void SetSymLoc(void *loc) {
    symLoc = loc;
  }

  void Dump() const;

 private:
  MIRModule *module;
  DBGExpr *simpLoc;
  MapleVector<DBGExpr> exprVec;
  void *symLoc;
};

class DBGDieAttr {
 public:
  explicit DBGDieAttr(DBGDieKind k) : dieKind(k), dwAttr(DW_AT_deleted), dwForm(DW_FORM_GNU_strp_alt) {
    value.u = kDbgDefaultVal;
  }

  virtual ~DBGDieAttr() = default;

  size_t SizeOf(DBGDieAttr *attr) const;

  void AddSimpLocOpnd(uint64 val) {
    value.ptr->AddSimpLocOpnd(val);
  }

  void ClearSimpLocOpnd() {
    value.ptr->ClearOpnd();
  }

  void Dump(int indent);

  DBGDieKind GetKind() const {
    return dieKind;
  }

  void SetKind(DBGDieKind kind) {
    dieKind = kind;
  }

  DwAt GetDwAt() const {
    return dwAttr;
  }

  void SetDwAt(DwAt at) {
    dwAttr = at;
  }

  DwForm GetDwForm() const {
    return dwForm;
  }

  void SetDwForm(DwForm form) {
    dwForm = form;
  }

  int32 GetI() const {
    return value.i;
  }

  void SetI(int32 val) {
    value.i = val;
  }

  uint32 GetId() const {
    return value.id;
  }

  void SetId(uint32 val) {
    value.id = val;
  }

  int64 GetJ() const {
    return value.j;
  }

  void SetJ(int64 val) {
    value.j = val;
  }

  uint64 GetU() const {
    return value.u;
  }

  void SetU(uint64 val) {
    value.u = val;
  }

  float GetF() const {
    return value.f;
  }

  void SetF(float val) {
    value.f = val;
  }

  double GetD() const {
    return value.d;
  }

  void SetD(double val) {
    value.d = val;
  }

  DBGExprLoc *GetPtr() {
    return value.ptr;
  }

  void SetPtr(DBGExprLoc *val) {
    value.ptr = val;
  }

  void SetKeep(bool flag) {
    keep = flag;
  }

  bool GetKeep() const {
    return keep;
  }

 private:
  DBGDieKind dieKind;
  DwAt dwAttr;
  DwForm dwForm;  // type for the attribute value
  union {
    int32 i;
    uint32 id;    // dieId when dwForm is of DW_FORM_ref
                  // strIdx when dwForm is of DW_FORM_string
    int64 j;
    uint64 u;
    float f;
    double d;

    DBGExprLoc *ptr;
  } value;
  bool keep = true;
};

class DBGDie {
 public:
  DBGDie(MIRModule *m, DwTag tag);
  virtual ~DBGDie() {}
  void AddSubVec(DBGDie *die);
  void AddAttr(DBGDieAttr *attr);
  void AddAttr(DwAt at, DwForm form, uint64 val, bool keepFlag = true);
  void AddSimpLocAttr(DwAt at, DwForm form, DwOp op, uint64 val);
  void AddGlobalLocAttr(DwAt at, DwForm form, uint64 val);
  void AddFrmBaseAttr(DwAt at, DwForm form);
  DBGExprLoc *GetExprLoc();
  bool SetAttr(DwAt attr, uint64 val);
  bool SetAttr(DwAt attr, int64 val);
  bool SetAttr(DwAt attr, uint32 val);
  bool SetAttr(DwAt attr, int32 val);
  bool SetAttr(DwAt attr, float val);
  bool SetAttr(DwAt attr, double val);
  bool SetAttr(DwAt attr, DBGExprLoc *ptr);
  bool SetSimpLocAttr(DwAt attr, int64 val);
  void ResetParentDie() const;
  void Dump(int indent);

  uint32 GetId() const {
    return id;
  }

  void SetId(uint32 val) {
    id = val;
  }

  DwTag GetTag() const {
    return tag;
  }

  void SetTag(DwTag val) {
    tag = val;
  }

  bool GetWithChildren() const {
    return withChildren;
  }

  void SetWithChildren(bool val) {
    withChildren = val;
  }

  bool GetKeep() const {
    return keep;
  }

  void SetKeep(bool val) {
    keep = val;
  }

  DBGDie *GetParent() const {
    return parent;
  }

  void SetParent(DBGDie *val) {
    parent = val;
  }

  DBGDie *GetSibling() const {
    return sibling;
  }

  void SetSibling(DBGDie *val) {
    sibling = val;
  }

  DBGDie *GetFirstChild() const {
    return firstChild;
  }

  void SetFirstChild(DBGDie *val) {
    firstChild = val;
  }

  uint32 GetAbbrevId() const {
    return abbrevId;
  }

  void SetAbbrevId(uint32 val) {
    abbrevId = val;
  }

  uint32 GetTyIdx() const {
    return tyIdx;
  }

  void SetTyIdx(uint32 val) {
    tyIdx = val;
  }

  uint32 GetOffset() const {
    return offset;
  }

  void SetOffset(uint32 val) {
    offset = val;
  }

  uint32 GetSize() const {
    return size;
  }

  void SetSize(uint32 val) {
    size = val;
  }

  const MapleVector<DBGDieAttr *> &GetAttrVec() const {
    return attrVec;
  }

  MapleVector<DBGDieAttr *> &GetAttrVec() {
    return attrVec;
  }

  const MapleVector<DBGDie *> &GetSubDieVec() const {
    return subDieVec;
  }

  MapleVector<DBGDie *> &GetSubDieVec() {
    return subDieVec;
  }

  uint32 GetSubDieVecSize() const {
    return static_cast<uint32>(subDieVec.size());
  }

  DBGDie *GetSubDieVecAt(uint32 i) const {
    return subDieVec[i];
  }

  // link ExprLoc to die's
  void LinkExprLoc(DBGDie *die) {
    for (auto &at : attrVec) {
      if (at->GetDwAt() == DW_AT_location) {
        DBGExprLoc *loc = die->GetExprLoc();
        at->SetPtr(loc);
      }
    }
  }

 private:
  MIRModule *module;
  DwTag tag;
  uint32 id;         // starts from 1 which is root die compUnit
  bool withChildren;
  bool keep;         // whether emit into .s
  DBGDie *parent;
  DBGDie *sibling;
  DBGDie *firstChild;
  uint32 abbrevId;   // id in .debug_abbrev
  uint32 tyIdx;      // for type TAG
  uint32 offset;     // Dwarf CU relative offset
  uint32 size;       // DIE Size in .debug_info
  MapleVector<DBGDieAttr *> attrVec;
  MapleVector<DBGDie *> subDieVec;
};

class DBGAbbrevEntry {
 public:
  DBGAbbrevEntry(MIRModule *m, DBGDie *die);
  virtual ~DBGAbbrevEntry() {}
  bool Equalto(DBGAbbrevEntry *entry);
  void Dump(int indent);

  DwTag GetTag() const {
    return tag;
  }

  void SetTag(DwTag val) {
    tag = val;
  }

  uint32 GetAbbrevId() const {
    return abbrevId;
  }

  void SetAbbrevId(uint32 val) {
    abbrevId = val;
  }

  bool GetWithChildren() const {
    return withChildren;
  }

  void SetWithChildren(bool val) {
    withChildren = val;
  }

  MapleVector<uint32> &GetAttrPairs() {
    return attrPairs;
  }

 private:
  DwTag tag;
  uint32 abbrevId;
  bool withChildren;
  MapleVector<uint32> attrPairs;  // kDwAt kDwForm pairs
};

class DBGAbbrevEntryVec {
 public:
  DBGAbbrevEntryVec(MIRModule *m, DwTag tag) : tag(tag), entryVec(m->GetMPAllocator().Adapter()) {}

  virtual ~DBGAbbrevEntryVec() {}

  uint32 GetId(MapleVector<uint32> &attrs);
  void Dump(int indent);

  DwTag GetTag() const {
    return tag;
  }

  void SetTag(DwTag val) {
    tag = val;
  }

  const MapleVector<DBGAbbrevEntry *> &GetEntryvec() const {
    return entryVec;
  }

  MapleVector<DBGAbbrevEntry *> &GetEntryvec() {
    return entryVec;
  }

 private:
  DwTag tag;
  MapleVector<DBGAbbrevEntry *> entryVec;
};

struct ScopePos {
  uint32 id;
  SrcPosition pos;
};

enum EmitStatus : uint8 {
  kBeginEmited = 0,
  kEndEmited = 1,
};
constexpr uint8 kAllEmited = 3;

class DebugInfo {
 public:
  explicit DebugInfo(MIRModule *m)
      : module(m),
        compUnit(nullptr),
        dummyTypeDie(nullptr),
        lexer(nullptr),
        maxId(1),
        mplSrcIdx(0),
        debugInfoLength(0),
        curFunction(nullptr),
        compileMsg(nullptr),
        parentDieStack(m->GetMPAllocator().Adapter()),
        idDieMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        abbrevVec(m->GetMPAllocator().Adapter()),
        tagAbbrevMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        baseTypeMap(std::less<GStrIdx>(), m->GetMPAllocator().Adapter()),
        tyIdxDieIdMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        stridxDieIdMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        globalStridxDieIdMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        funcDefStrIdxDieIdMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        typeDefTyIdxMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        pointedPointerMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        funcLstrIdxDieIdMap(std::less<MIRFunction *>(), m->GetMPAllocator().Adapter()),
        funcLstrIdxLabIdxMap(std::less<MIRFunction *>(), m->GetMPAllocator().Adapter()),
        funcScopeLows(std::less<MIRFunction *>(), m->GetMPAllocator().Adapter()),
        funcScopeHighs(std::less<MIRFunction *>(), m->GetMPAllocator().Adapter()),
        funcScopeIdStatus(std::less<MIRFunction *>(), m->GetMPAllocator().Adapter()),
        globalTypeAliasMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        constTypeDieMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        volatileTypeDieMap(std::less<uint32>(), m->GetMPAllocator().Adapter()),
        strps(std::less<uint32>(), m->GetMPAllocator().Adapter()) {
    /* valid entry starting from index 1 as abbrevid starting from 1 as well */
    abbrevVec.push_back(nullptr);
    InitMsg();
    varPtrPrefix = std::string(namemangler::kPtrPrefixStr);
  }

  virtual ~DebugInfo() {}

  void BuildDebugInfo();
  void ClearDebugInfo();
  void Dump(int indent);

  DBGDie *GetFuncDie(const MIRFunction *func, bool isDeclDie = false);
  DBGDie *GetLocalDie(MIRFunction *func, GStrIdx strIdx);
  DBGDieAttr *CreateAttr(DwAt at, DwForm form, uint64 val) const;

  bool IsScopeIdEmited(MIRFunction *func, uint32 scopeId);

  void GetCrossScopeId(MIRFunction *func,
                       std::unordered_set<uint32> &idSet,
                       bool isLow,
                       const SrcPosition &oldSrcPos,
                       const SrcPosition &newSrcPos);

  void ComputeSizeAndOffsets();

  DBGDie *GetDie(uint32 id) {
    return idDieMap[id];
  }

  void SetIdDieMap(uint32 i, DBGDie *die) {
    idDieMap[i] = die;
  }

  DBGDie *GetDummyTypeDie() {
    return dummyTypeDie;
  }

  DBGDie *GetParentDie() {
    return parentDieStack.top();
  }

  void ResetParentDie() {
    parentDieStack.clear();
    parentDieStack.push(compUnit);
  }

  uint32 GetDebugInfoLength() const {
    return debugInfoLength;
  }

  void SetFuncScopeIdStatus(MIRFunction *func, uint32 scopeId, EmitStatus status) {
    if (funcScopeIdStatus[func].find(scopeId) == funcScopeIdStatus[func].end()) {
      funcScopeIdStatus[func][scopeId] = 0;
    }
    funcScopeIdStatus[func][scopeId] |= (1U << status);
  }

  MapleVector<DBGAbbrevEntry *> &GetAbbrevVec() {
    return abbrevVec;
  }

  void AddStrps(uint32 val) {
    strps.insert(val);
  }

  MapleSet<uint32> &GetStrps() {
    return strps;
  }

  uint32 GetMaxId() const {
    return maxId;
  }

  uint32 GetIncMaxId() {
    return maxId++;
  }

  DBGDie *GetCompUnit() const {
    return compUnit;
  }

  size_t GetParentDieSize() const {
    return parentDieStack.size();
  }

  void SetErrPos(uint32 lnum, uint32 cnum) const {
    compileMsg->SetErrPos(lnum, cnum);
  }

  void UpdateMsg(uint32 lnum, const char *line) {
    compileMsg->UpdateMsg(lnum, line);
  }

  void EmitMsg() {
    compileMsg->EmitMsg();
  }

 private:
  MIRModule *module;
  DBGDie *compUnit;      // root die: compilation unit
  DBGDie *dummyTypeDie;  // workaround for unknown types
  MIRLexer *lexer;
  uint32 maxId;
  GStrIdx mplSrcIdx;
  uint32 debugInfoLength;
  MIRFunction *curFunction;

  // for compilation messages
  DBGCompileMsgInfo *compileMsg;

  MapleStack<DBGDie *> parentDieStack;
  MapleMap<uint32, DBGDie *> idDieMap;
  MapleVector<DBGAbbrevEntry *> abbrevVec;  // valid entry starting from index 1
  MapleMap<uint32, DBGAbbrevEntryVec *> tagAbbrevMap;
  MapleMap<GStrIdx, std::pair<GStrIdx, PrimType>> baseTypeMap;  // baseTypeMap: <InputName, OutputName, PrimType>

  // to be used when derived type references a base type die
  MapleMap<uint32, uint32> tyIdxDieIdMap;
  MapleMap<uint32, uint32> stridxDieIdMap;
  // save global var die, global var string idx to die id
  MapleMap<uint32, uint32> globalStridxDieIdMap;
  MapleMap<uint32, uint32> funcDefStrIdxDieIdMap;
  MapleMap<uint32, uint32> typeDefTyIdxMap;  // prevtyIdxtypidx_map
  MapleMap<uint32, uint32> pointedPointerMap;
  MapleMap<MIRFunction *, std::map<uint32, uint32>> funcLstrIdxDieIdMap;
  MapleMap<MIRFunction *, std::map<uint32, LabelIdx>> funcLstrIdxLabIdxMap;

  MapleMap<MIRFunction *, std::vector<ScopePos>> funcScopeLows;
  MapleMap<MIRFunction *, std::vector<ScopePos>> funcScopeHighs;

  /* save functions's scope id that has been emited */
  MapleMap<MIRFunction *, std::map<uint32, uint8>> funcScopeIdStatus;

  /* alias type */
  MapleMap<uint32, uint32> globalTypeAliasMap;
  MapleMap<uint32, uint32> constTypeDieMap;
  MapleMap<uint32, uint32> volatileTypeDieMap;

  MapleSet<uint32> strps;
  std::string varPtrPrefix;

  void InitMsg() {
    compileMsg = module->GetMemPool()->New<DBGCompileMsgInfo>();
  }

  void Init();
  void InitBaseTypeMap();
  void Finish();
  void SetupCU();

  void BuildDebugInfoEnums();
  void BuildDebugInfoContainers();
  void BuildDebugInfoGlobalSymbols();
  void BuildDebugInfoFunctions();

  // build tree to populate withChildren, sibling, firstChild
  // also insert DW_AT_sibling attributes when needed
  void BuildDieTree();

  void BuildAbbrev();

  uint32 GetAbbrevId(DBGAbbrevEntryVec *vec, DBGAbbrevEntry *entry) const;

  DBGDie *GetGlobalDie(const GStrIdx &strIdx);

  void SetLocalDie(GStrIdx strIdx, const DBGDie *die);
  void SetLocalDie(MIRFunction *func, GStrIdx strIdx, const DBGDie *die);
  DBGDie *GetLocalDie(GStrIdx strIdx);

  LabelIdx GetLabelIdx(GStrIdx strIdx);
  LabelIdx GetLabelIdx(MIRFunction *func, GStrIdx strIdx) const;
  void SetLabelIdx(const GStrIdx &strIdx, LabelIdx labIdx);
  void SetLabelIdx(MIRFunction *func, const GStrIdx &strIdx, LabelIdx labIdx);
  void InsertBaseTypeMap(const std::string &inputName, const std::string &outpuName, PrimType type);

  DBGDie *GetIdDieMapAt(uint32 i) {
    return idDieMap[i];
  }

  void PushParentDie(DBGDie *die) {
    parentDieStack.push(die);
  }

  void PopParentDie() {
    parentDieStack.pop();
  }

  MIRFunction *GetCurFunction() {
    return curFunction;
  }

  void SetCurFunction(MIRFunction *func) {
    curFunction = func;
  }

  void SetTyidxDieIdMap(const TyIdx tyIdx, const DBGDie *die) {
    tyIdxDieIdMap[tyIdx.GetIdx()] = die->GetId();
  }

  DBGDie *CreateVarDie(MIRSymbol *sym);
  DBGDie *CreateVarDie(MIRSymbol *sym, const GStrIdx &strIdx); // use alt name
  DBGDie *CreateFormalParaDie(MIRFunction *func, uint32 idx, bool isDef);
  DBGDie *CreateFieldDie(maple::FieldPair pair);
  DBGDie *CreateBitfieldDie(const MIRBitFieldType *type, const GStrIdx &sidx, uint32 &prevBits);
  void CreateStructTypeFieldsDies(const MIRStructType *structType, DBGDie *die);
  void CreateStructTypeParentFieldsDies(const MIRStructType *structType, DBGDie *die);
  void CreateStructTypeMethodsDies(const MIRStructType *structType, DBGDie *die);
  DBGDie *CreateStructTypeDie(GStrIdx strIdx, const MIRStructType *structType, bool update = false);
  DBGDie *CreateClassTypeDie(const GStrIdx &strIdx, const MIRClassType *classType);
  DBGDie *CreateInterfaceTypeDie(const GStrIdx &strIdx, const MIRInterfaceType *interfaceType);
  DBGDie *CreatePointedFuncTypeDie(MIRFuncType *fType);
  void CreateFuncLocalSymbolsDies(MIRFunction *func, DBGDie *die);

  DBGDie *GetOrCreateLabelDie(LabelIdx labid);
  DBGDie *GetOrCreateFuncDeclDie(MIRFunction *func);
  DBGDie *GetOrCreateFuncDefDie(MIRFunction *func);
  DBGDie *GetOrCreatePrimTypeDie(MIRType *ty);
  DBGDie *GetOrCreateBaseTypeDie(const MIRType *type);
  DBGDie *GetOrCreateTypeDie(TyIdx tyidx);
  DBGDie *GetOrCreateTypeDie(MIRType *type);
  DBGDie *GetOrCreateTypeDieWithAttr(AttrKind attr, DBGDie *typeDie);
  DBGDie *GetOrCreateTypeDieWithAttr(TypeAttrs attrs, DBGDie *typeDie);
  DBGDie *GetOrCreatePointTypeDie(const MIRPtrType *ptrType);
  DBGDie *GetOrCreateArrayTypeDie(const MIRArrayType *arrayType);
  DBGDie *GetOrCreateStructTypeDie(const MIRType *type);
  DBGDie *GetOrCreateTypedefDie(GStrIdx stridx, TyIdx tyidx);
  DBGDie *GetOrCreateEnumTypeDie(uint32 idx);
  DBGDie *GetOrCreateEnumTypeDie(const MIREnum *mirEnum);
  DBGDie *GetOrCreateTypeByNameDie(const MIRType &type);

  GStrIdx GetPrimTypeCName(PrimType pty);

  void AddScopeDie(MIRScope *scope);
  DBGDie *GetAliasVarTypeDie(const MIRAliasVars &aliasVar, TyIdx tyidx);
  void HandleTypeAlias(MIRScope &scope);
  void AddAliasDies(MIRScope &scope, bool isLocal);
  void CollectScopePos(MIRFunction *func, MIRScope *scope);

  // Functions for calculating the size and offset of each DW_TAG_xxx and DW_AT_xxx
  void ComputeSizeAndOffset(DBGDie *die, uint32 &cuOffset);
};
} // namespace maple
#endif // MAPLE_IR_INCLUDE_DBG_INFO_H
