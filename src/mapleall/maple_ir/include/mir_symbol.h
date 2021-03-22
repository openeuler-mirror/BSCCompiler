/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_SYMBOL_H
#define MAPLE_IR_INCLUDE_MIR_SYMBOL_H
#include <sstream>
#include "mir_const.h"
#include "mir_preg.h"
#include "src_position.h"

constexpr int kScopeLocal = 2;   // the default scope level for function variables
constexpr int kScopeGlobal = 1;  // the scope level for global variables

namespace maple {
enum MIRSymKind {
  kStInvalid,
  kStVar,
  kStFunc,
  kStConst,
  kStJavaClass,
  kStJavaInterface,
  kStPreg
};

enum MIRStorageClass : uint8 {
  kScInvalid,
  kScAuto,
  kScAliased,
  kScFormal,
  kScExtern,
  kScGlobal,
  kScPstatic,  // PU-static
  kScFstatic,  // file-static
  kScText,
  kScTypeInfo,      // used for eh type st
  kScTypeInfoName,  // used for eh type st name
  kScTypeCxxAbi,    // used for eh inherited from c++ __cxxabiv1
  kScEHRegionSupp,  // used for tables that control C++ exception handling
  kScUnused
};

// to represent a single symbol
class MIRSymbol {
 public:
  union SymbolType {  // a symbol can either be a const or a function or a preg which currently used for formal
    MIRConst *konst;
    MIRFunction *mirFunc;
    MIRPreg *preg;  // the MIRSymKind must be kStPreg
  };

  MIRSymbol() = default;
  MIRSymbol(uint32 idx, uint8 scp) : stIdx(scp, idx) {}
  ~MIRSymbol() = default;

  void SetIsTmp(bool temp) {
    isTmp = temp;
  }

  bool GetIsTmp() const {
    return isTmp;
  }

  void SetNeedForwDecl() {
    needForwDecl = true;
  }

  bool IsNeedForwDecl() const {
    return needForwDecl;
  }

  void SetInstrumented() {
    instrumented = true;
  }

  bool IsInstrumented() const {
    return instrumented;
  }

  void SetIsImported(bool imported) {
    isImported = imported;
  }

  bool GetIsImported() const {
    return isImported;
  }

  void SetWPOFakeParm() {
    wpoFakeParam = true;
  }

  bool IsWpoFakeParm() const {
    return wpoFakeParam;
  }

  bool IsWpoFakeRet() const {
    return wpoFakeRet;
  }

  void SetWPOFakeRet() {
    wpoFakeRet = true;
  }

  void SetIsTmpUnused(bool used) {
    isTmpUnused = used;
  }
  void SetIsImportedDecl(bool imported) {
    isImportedDecl = imported;
  }

  bool GetIsImportedDecl() const {
    return isImportedDecl;
  }

  bool IsTmpUnused() const {
    return isTmpUnused;
  }

  void SetAppearsInCode(bool appears) {
    appearsInCode = appears;
  }

  bool GetAppearsInCode() const {
    return appearsInCode;
  }

  void SetTyIdx(TyIdx tyIdx) {
    this->tyIdx = tyIdx;
  }

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  void SetInferredTyIdx(TyIdx inferredTyIdx) {
    this->inferredTyIdx = inferredTyIdx;
  }

  TyIdx GetInferredTyIdx() const {
    return inferredTyIdx;
  }

  void SetStIdx(StIdx stIdx) {
    this->stIdx = stIdx;
  }

  StIdx GetStIdx() const {
    return stIdx;
  }

  void SetSKind(MIRSymKind m) {
    sKind = m;
  }

  MIRSymKind GetSKind() const {
    return sKind;
  }

  uint32 GetScopeIdx() const {
    return stIdx.Scope();
  }

  uint32 GetStIndex() const {
    return stIdx.Idx();
  }

  bool IsLocal() const {
    return stIdx.Islocal();
  }

  bool IsGlobal() const {
    return stIdx.IsGlobal();
  }

  const TypeAttrs &GetAttrs() const {
    return typeAttrs;
  }

  TypeAttrs &GetAttrs() {
    return typeAttrs;
  }

  void SetAttrs(TypeAttrs attr) {
    typeAttrs = attr;
  }

  // AddAttrs adds more attributes instead of overrides the current one
  void AddAttrs(TypeAttrs attr) {
    typeAttrs.SetAttrFlag(typeAttrs.GetAttrFlag() | attr.GetAttrFlag());
  }

  bool GetAttr(AttrKind attrKind) const {
    return typeAttrs.GetAttr(attrKind);
  }

  void SetAttr(AttrKind attrKind) {
    typeAttrs.SetAttr(attrKind);
  }

  void ResetAttr(AttrKind attrKind) {
    typeAttrs.ResetAttr(attrKind);
  }

  bool IsVolatile() const {
    return typeAttrs.GetAttr(ATTR_volatile);
  }

  bool IsTypeVolatile(int fieldID) const;

  bool IsStatic() const {
    return typeAttrs.GetAttr(ATTR_static);
  }

  bool IsFinal() const {
    return ((typeAttrs.GetAttr(ATTR_final) || typeAttrs.GetAttr(ATTR_readonly)) &&
            staticFinalBlackList.find(GetName()) == staticFinalBlackList.end()) ||
           IsLiteral() || IsLiteralPtr();
  }

  bool IsWeak() const {
    return typeAttrs.GetAttr(ATTR_weak);
  }

  bool IsPrivate() const {
    return typeAttrs.GetAttr(ATTR_private);
  }

  bool IsRefType() const {
    return typeAttrs.GetAttr(ATTR_localrefvar);
  }

  void SetNameStrIdx(GStrIdx strIdx) {
    nameStrIdx = strIdx;
  }

  void SetNameStrIdx(const std::string &name);

  GStrIdx GetNameStrIdx() const {
    return nameStrIdx;
  }

  MIRStorageClass GetStorageClass() const {
    return storageClass;
  }

  void SetStorageClass(MIRStorageClass cl) {
    storageClass = cl;
  }

  bool IsReadOnly() const {
    return kScFstatic == storageClass && kStConst == sKind;
  }

  bool IsConst() const {
    return sKind == kStConst || (sKind == kStVar && value.konst != nullptr);
  }

  MIRType *GetType() const;

  const std::string &GetName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(nameStrIdx);
  }

  MIRConst *GetKonst() const {
    ASSERT((sKind == kStConst || sKind == kStVar), "must be const symbol");
    return value.konst;
  }

  void SetKonst(MIRConst *mirconst) {
    ASSERT((sKind == kStConst || sKind == kStVar), "must be const symbol");
    value.konst = mirconst;
  }

  void SetIsDeleted() {
    isDeleted = true;
  }

  void ResetIsDeleted() {
    isDeleted = false;
  }

  bool IsDeleted() const {
    return isDeleted;
  }

  bool IsPreg() const {
    return sKind == kStPreg;
  }

  bool IsJavaClassInterface() const {
    return sKind == kStJavaClass || sKind == kStJavaInterface;
  }

  SymbolType GetValue() const {
    return value;
  }

  void SetValue(SymbolType value) {
    this->value = value;
  }

  SrcPosition &GetSrcPosition() {
    return srcPosition;
  }

  void SetSrcPosition(SrcPosition &position) {
    srcPosition = position;
  }

  MIRPreg *GetPreg() {
    ASSERT(IsPreg(), "must be Preg");
    return value.preg;
  }

  const MIRPreg *GetPreg() const {
    CHECK_FATAL(IsPreg(), "must be Preg");
    return value.preg;
  }

  void SetPreg(MIRPreg *preg) {
    CHECK_FATAL(IsPreg(), "must be Preg");
    value.preg = preg;
  }

  bool CanBeIgnored() const {
    return isDeleted;
  }

  void SetLocalRefVar() {
    SetAttr(ATTR_localrefvar);
  }

  void ResetLocalRefVar() {
    ResetAttr(ATTR_localrefvar);
  }

  MIRFunction *GetFunction() const {
    ASSERT(sKind == kStFunc, "must be function symbol");
    return value.mirFunc;
  }

  void SetFunction(MIRFunction *func) {
    ASSERT(sKind == kStFunc, "must be function symbol");
    value.mirFunc = func;
  }

  bool IsEhIndex() const {
    return GetName() == "__eh_index__";
  }

  bool HasAddrOfValues() const;
  bool IsLiteral() const;
  bool IsLiteralPtr() const;
  bool PointsToConstString() const;
  bool IsConstString() const;
  bool IsClassInitBridge() const;
  bool IsReflectionStrTab() const;
  bool IsReflectionHashTabBucket() const;
  bool IsReflectionInfo() const;
  bool IsReflectionFieldsInfo() const;
  bool IsReflectionFieldsInfoCompact() const;
  bool IsReflectionSuperclassInfo() const;
  bool IsReflectionFieldOffsetData() const;
  bool IsReflectionMethodAddrData() const;
  bool IsReflectionMethodSignature() const;
  bool IsReflectionClassInfo() const;
  bool IsReflectionArrayClassInfo() const;
  bool IsReflectionClassInfoPtr() const;
  bool IsReflectionClassInfoRO() const;
  bool IsITabConflictInfo() const;
  bool IsVTabInfo() const;
  bool IsITabInfo() const;
  bool IsReflectionPrimitiveClassInfo() const;
  bool IsReflectionMethodsInfo() const;
  bool IsReflectionMethodsInfoCompact() const;
  bool IsRegJNITab() const;
  bool IsRegJNIFuncTab() const;
  bool IsMuidTab() const;
  bool IsMuidRoTab() const;
  bool IsCodeLayoutInfo() const;
  std::string GetMuidTabName() const;
  bool IsMuidFuncDefTab() const;
  bool IsMuidFuncDefOrigTab() const;
  bool IsMuidFuncInfTab() const;
  bool IsMuidFuncUndefTab() const;
  bool IsMuidDataDefTab() const;
  bool IsMuidDataDefOrigTab() const;
  bool IsMuidDataUndefTab() const;
  bool IsMuidFuncDefMuidTab() const;
  bool IsMuidFuncUndefMuidTab() const;
  bool IsMuidDataDefMuidTab() const;
  bool IsMuidDataUndefMuidTab() const;
  bool IsMuidFuncMuidIdxMuidTab() const;
  bool IsMuidRangeTab() const;
  bool IsArrayClassCache() const;
  bool IsArrayClassCacheName() const;
  bool IsForcedGlobalFunc() const;
  bool IsForcedGlobalClassinfo() const;
  bool IsGctibSym() const;
  bool IsPrimordialObject() const;
  bool IgnoreRC() const;
  void Dump(bool isLocal, int32 indent, bool suppressinit = false) const;
  void DumpAsLiteralVar() const;
  bool operator==(const MIRSymbol &msym) const {
    return nameStrIdx == msym.nameStrIdx;
  }

  bool operator!=(const MIRSymbol &msym) const {
    return nameStrIdx != msym.nameStrIdx;
  }

  bool operator<(const MIRSymbol &msym) const {
    return nameStrIdx < msym.nameStrIdx;
  }

  static uint32 LastPrintedLineNum() {
    return lastPrintedLineNum;
  }

  static void SetLastPrintedLineNum(uint32 val) {
    lastPrintedLineNum = val;
  }

  // Please keep order of the fields, avoid paddings.
 private:
  TyIdx tyIdx{ 0 };
  TyIdx inferredTyIdx{ kInitTyIdx };
  MIRStorageClass storageClass{ kScInvalid };
  MIRSymKind sKind{ kStInvalid };
  bool isTmp = false;
  bool needForwDecl = false;  // addrof of this symbol used in initialization, NOT serialized
  bool wpoFakeParam = false;  // fake symbol introduced in wpo phase for a parameter, NOT serialized
  bool wpoFakeRet = false;    // fake symbol introduced in wpo phase for return value, NOT serialized
  bool isDeleted = false;     // tell if it is deleted, NOT serialized
  bool instrumented = false;  // a local ref pointer instrumented by RC opt, NOT serialized
  bool isImported = false;
  bool isImportedDecl = false;
  bool isTmpUnused = false;  // when parse the mplt_inline file, mark all the new symbol as tmpunused
  bool appearsInCode = false;  // only used for kStFunc
  StIdx stIdx { 0, 0 };
  TypeAttrs typeAttrs;
  GStrIdx nameStrIdx{ 0 };
  SymbolType value = { nullptr };
  SrcPosition srcPosition;      // where the symbol is defined
  // following cannot be assumed final even though they are declared final
  static const std::set<std::string> staticFinalBlackList;
  static GStrIdx reflectClassNameIdx;
  static GStrIdx reflectMethodNameIdx;
  static GStrIdx reflectFieldNameIdx;
  static uint32 lastPrintedLineNum;     // used during printing ascii output
};

class MIRSymbolTable {
 public:
  explicit MIRSymbolTable(MapleAllocator &allocator)
      : mAllocator(allocator),
        strIdxToStIdxMap(mAllocator.Adapter()),
        symbolTable({ nullptr }, mAllocator.Adapter()) {}

  ~MIRSymbolTable() = default;

  bool IsValidIdx(uint32 idx) const {
    return idx < symbolTable.size();
  }

  MIRSymbol *GetSymbolFromStIdx(uint32 idx, bool checkFirst = false) const {
    if (checkFirst && idx >= symbolTable.size()) {
      return nullptr;
    }
    CHECK_FATAL(IsValidIdx(idx), "symbol table index out of range");
    return symbolTable[idx];
  }

  MIRSymbol *CreateSymbol(uint8 scopeID) {
    auto *st = mAllocator.GetMemPool()->New<MIRSymbol>(symbolTable.size(), scopeID);
    symbolTable.push_back(st);
    return st;
  }

  void PushNullSymbol() {
    symbolTable.push_back(nullptr);
  }

  // add sym from other symbol table, happens in inline
  bool AddStOutside(MIRSymbol *sym) {
    if (sym == nullptr) {
      return false;
    }
    sym->SetStIdx(StIdx(sym->GetScopeIdx(), symbolTable.size()));
    symbolTable.push_back(sym);
    return AddToStringSymbolMap(*sym);
  }

  bool AddToStringSymbolMap(const MIRSymbol &st) {
    GStrIdx strIdx = st.GetNameStrIdx();
    if (strIdxToStIdxMap[strIdx].FullIdx() != 0) {
      return false;
    }
    strIdxToStIdxMap[strIdx] = st.GetStIdx();
    return true;
  }

  StIdx GetStIdxFromStrIdx(GStrIdx idx) const {
    auto it = strIdxToStIdxMap.find(idx);
    return (it == strIdxToStIdxMap.end()) ? StIdx() : it->second;
  }

  MIRSymbol *GetSymbolFromStrIdx(GStrIdx idx, bool checkFirst = false) {
    return GetSymbolFromStIdx(GetStIdxFromStrIdx(idx).Idx(), checkFirst);
  }

  void Dump(bool isLocal, int32 indent = 0, bool printDeleted = false) const;
  size_t GetSymbolTableSize() const {
    return symbolTable.size();
  }

  void Clear() {
    symbolTable.clear();
  }

  MIRSymbol *CloneLocalSymbol(const MIRSymbol &oldSym) const {
    auto *memPool = mAllocator.GetMemPool();
    auto *newSym = memPool->New<MIRSymbol>(oldSym);
    if (oldSym.GetSKind() == kStConst) {
      newSym->SetKonst(oldSym.GetKonst()->Clone(*memPool));
    } else if (oldSym.GetSKind() == kStPreg) {
      newSym->SetPreg(memPool->New<MIRPreg>(*oldSym.GetPreg()));
    } else if (oldSym.GetSKind() == kStFunc) {
      CHECK_FATAL(false, "%s has unexpected local func symbol", oldSym.GetName().c_str());
    }
    return newSym;
  }

 private:
  MapleAllocator mAllocator;
  // hash table mapping string index to st index
  MapleMap<GStrIdx, StIdx> strIdxToStIdxMap;
  // map symbol idx to symbol node
  MapleVector<MIRSymbol*> symbolTable;
};

class MIRLabelTable {
 public:
  explicit MIRLabelTable(MapleAllocator &allocator)
      : mAllocator(allocator),
        strIdxToLabIdxMap(std::less<GStrIdx>(), mAllocator.Adapter()),
        labelTable(mAllocator.Adapter()),
        addrTakenLabels(mAllocator.Adapter()) {
    labelTable.push_back(GStrIdx(kDummyLabel));  // push dummy label index 0
  }

  ~MIRLabelTable() = default;

  LabelIdx CreateLabel() {
    LabelIdx labelIdx = labelTable.size();
    labelTable.push_back(GStrIdx(0));  // insert dummy global string index for anonymous label
    return labelIdx;
  }

  LabelIdx CreateLabelWithPrefix(char c);

  LabelIdx AddLabel(GStrIdx nameIdx) {
    LabelIdx labelIdx = labelTable.size();
    labelTable.push_back(nameIdx);
    strIdxToLabIdxMap[nameIdx] = labelIdx;
    return labelIdx;
  }

  LabelIdx GetLabelIdxFromStrIdx(GStrIdx idx) const {
    auto it = strIdxToLabIdxMap.find(idx);
    if (it == strIdxToLabIdxMap.end()) {
      return LabelIdx();
    }
    return it->second;
  }

  bool AddToStringLabelMap(LabelIdx lidx);
  size_t GetLabelTableSize() const {
    return labelTable.size();
  }

  const std::string &GetName(LabelIdx labelIdx) const;

  size_t Size() const {
    return labelTable.size();
  }

  static uint32 GetDummyLabel() {
    return kDummyLabel;
  }

  GStrIdx GetSymbolFromStIdx(LabelIdx idx) const {
    CHECK_FATAL(idx < labelTable.size(), "label table index out of range");
    return labelTable[idx];
  }

  void SetSymbolFromStIdx(LabelIdx idx, GStrIdx strIdx) {
    CHECK_FATAL(idx < labelTable.size(), "label table index out of range");
    labelTable[idx] = strIdx;
  }

  MapleVector<GStrIdx> GetLabelTable() {
    return labelTable;
  }

  const MapleUnorderedSet<LabelIdx> &GetAddrTakenLabels() const {
    return addrTakenLabels;
  }

  MapleUnorderedSet<LabelIdx> &GetAddrTakenLabels() {
    return addrTakenLabels;
  }

  const MapleMap<GStrIdx, LabelIdx> &GetStrIdxToLabelIdxMap() const {
    return strIdxToLabIdxMap;
  }
  void EraseStrIdxToLabelIdxElem(GStrIdx idx) {
    strIdxToLabIdxMap.erase(idx);
  }

 private:
  static constexpr uint32 kDummyLabel = 0;
  MapleAllocator mAllocator;
  MapleMap<GStrIdx, LabelIdx> strIdxToLabIdxMap;
  MapleVector<GStrIdx> labelTable;  // map label idx to label name
 public:
  MapleUnorderedSet<LabelIdx> addrTakenLabels; // those appeared in addroflabel or MIRLblConst
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_SYMBOL_H
