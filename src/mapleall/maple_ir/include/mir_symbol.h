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

namespace maple {
constexpr int kScopeLocal = 2;   // the default scope level for function variables
constexpr int kScopeGlobal = 1;  // the scope level for global variables

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

  void SetIsTmpUnused(bool unused) {
    isTmpUnused = unused;
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

  void SetTyIdx(const TyIdx &newTyIdx) {
    this->tyIdx = newTyIdx;
  }

  TyIdx GetTyIdx() const {
    return tyIdx;
  }

  void SetInferredTyIdx(const TyIdx &newInferredTyIdx) {
    this->inferredTyIdx = newInferredTyIdx;
  }

  TyIdx GetInferredTyIdx() const {
    return inferredTyIdx;
  }

  void SetStIdx(const StIdx &newStIdx) {
    this->stIdx = newStIdx;
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
    typeAttrs.AddAttrBoundary(attr.GetAttrBoundary());
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

  // for checking if a symbol need GOT for relocation or not
  // if any functionality needs to check GOT usage, should use this function
  bool NeedGOT(bool isPIC, bool isPIE) const;

  bool IsThreadLocal() const {
    return typeAttrs.GetAttr(ATTR_tls_static) || typeAttrs.GetAttr(ATTR_tls_dynamic);
  }

  bool IsStatic() const {
    return typeAttrs.GetAttr(ATTR_static);
  }

  bool IsPUStatic() const {
    return GetStorageClass() == kScPstatic;
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
    return storageClass == kScFstatic && sKind == kStConst;
  }

  bool IsConst() const {
    return sKind == kStConst || (sKind == kStVar && value.konst != nullptr);
  }

  bool IsHiddenVisibility() const {
    return typeAttrs.GetAttr(ATTR_visibility_hidden);
  }

  bool IsDefaultVisibility() const {
    return !typeAttrs.GetAttr(ATTR_visibility_hidden) && !typeAttrs.GetAttr(ATTR_visibility_protected);
  }

  bool IsDefaultTLSModel() const {
    return IsThreadLocal() && !typeAttrs.GetAttr(ATTR_local_exec) && !typeAttrs.GetAttr(ATTR_initial_exec) &&
        !typeAttrs.GetAttr(ATTR_local_dynamic) && !typeAttrs.GetAttr(ATTR_global_dynamic);
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

  bool IsVar() const {
    return sKind == kStVar;
  }

  bool IsFunction() const {
    return sKind == kStFunc;
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

  void SetValue(SymbolType newValue) {
    this->value = newValue;
  }

  SrcPosition &GetSrcPosition() {
    return srcPosition;
  }

  const SrcPosition &GetSrcPosition() const {
    return srcPosition;
  }

  void SetSrcPosition(const SrcPosition &position) {
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

  // check symbol is a retVar
  bool IsReturnVar() const {
    if (storageClass != kScAuto) {
      return false;
    }
    // the prefix of the retVar symbol is 'retVar_'
    constexpr uint32 kRetVarPrefixLength = 7;
    if (GetName().compare(0, kRetVarPrefixLength, "retVar_") != 0) {
      return false;
    }
    // retVar symbol has only one underscope
    constexpr uint32 kRetVarUnderscopeNum = 1;
    if (std::count(GetName().begin(), GetName().end(), '_') != kRetVarUnderscopeNum) {
      return false;
    }
    return true;
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
  void Dump(bool isLocal, int32 indent, bool suppressInit = false, const MIRSymbolTable *localsymtab = nullptr) const;
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

  static uint32 &LastPrintedLineNumRef() {
    return lastPrintedLineNum;
  }

  static uint16 &LastPrintedColumnNumRef() {
    return lastPrintedColumnNum;
  }

  bool HasPotentialAssignment() const {
    return hasPotentialAssignment;
  }

  void SetHasPotentialAssignment() {
    hasPotentialAssignment = true;
  }

  void SetAsmAttr(const UStrIdx &idx) {
    asmAttr = idx;
  }

  const UStrIdx &GetAsmAttr() const {
    return asmAttr;
  }

  void SetWeakrefAttr(const std::pair<bool, UStrIdx> &idx) {
    weakrefAttr = idx;
  }

  const std::pair<bool, UStrIdx> &GetWeakrefAttr() const {
    return weakrefAttr;
  }

  bool IsFormal() const {
    return storageClass == kScFormal;
  }

  bool LMBCAllocateOffSpecialReg() const {
    if (isDeleted) {
      return false;
    }
    switch (storageClass) {
      case kScAuto:
        return true;
      case kScPstatic:
      case kScFstatic:
        return value.konst == nullptr && !hasPotentialAssignment;
      default:
        return false;
    }
  }
  UStrIdx asmAttr { 0 }; // if not 0, the string for the name in C's asm attribute
  UStrIdx sectionAttr { 0 }; // if not 0, the string for the name in C's section attribute

  bool GetAccessByMem() const {
    return accessByMem;
  }

  void SetAccessByMem(bool val) {
    accessByMem = val;
  }

  bool IsUnionReplaceCand() const {
    return unionReplaceCand;
  }

  void SeUnionReplaceCand(bool set) {
    unionReplaceCand = set;
  }

  uint8 GetSymbolAlign(bool isArm64ilp32) const {
    uint8 align = GetAttrs().GetAlignValue();
    if (align == 0) {
      if (GetType()->GetKind() == kTypeStruct || GetType()->GetKind() == kTypeClass ||
          GetType()->GetKind() == kTypeArray || GetType()->GetKind() == kTypeUnion) {
#if (defined(TARGX86) && TARGX86) || (defined(TARGX86_64) && TARGX86_64)
        return align;
#else
        uint8 alignMin = 0;
        if (GetType()->GetAlign() > 0) {
          alignMin = static_cast<uint8>(log2(GetType()->GetAlign()));
        }
        align = std::max<uint8>(3, alignMin); // 3: alignment in bytes of uint8
#endif
      } else {
        align = static_cast<uint8>(GetType()->GetAlign());
#if (defined(TARGAARCH64) && TARGAARCH64) || (defined(TARGARM32) && TARGARM32) ||\
    (defined(TARGARK) && TARGARK) || (defined(TARGRISCV64) && TARGRISCV64)
        if (isArm64ilp32 && GetType()->GetPrimType() == PTY_a32) {
          align = 3; // 3: alignment in bytes of uint8
        } else {
          align = static_cast<uint8>(log2(align));
        }
#endif
      }
    }
    return align;
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
  bool hasPotentialAssignment = false; // for global static vars, init as false and will be set true
                                       // if assigned by stmt or the address of itself is taken
  bool accessByMem = false;  // temporary used for tls var the access of which have been transformed to mem offset
  bool unionReplaceCand = false;
  StIdx stIdx { 0, 0 };
  TypeAttrs typeAttrs;
  GStrIdx nameStrIdx{ 0 };
  std::pair<bool, UStrIdx> weakrefAttr { false, 0 };
  SymbolType value = { nullptr };
  SrcPosition srcPosition;      // where the symbol is defined
  // following cannot be assumed final even though they are declared final
  static const std::set<std::string> staticFinalBlackList;
  static GStrIdx reflectClassNameIdx;
  static GStrIdx reflectMethodNameIdx;
  static GStrIdx reflectFieldNameIdx;
  static uint32 lastPrintedLineNum;     // used during printing ascii output
  static uint16 lastPrintedColumnNum;
};

class MIRSymbolTable {
 public:
  explicit MIRSymbolTable(const MapleAllocator &allocator)
      : mAllocator(allocator),
        strIdxToStIdxMap(mAllocator.Adapter()),
        symbolTable({ nullptr }, mAllocator.Adapter()) {}

  ~MIRSymbolTable() = default;

  bool IsValidIdx(uint32 idx) const {
    return idx < symbolTable.size();
  }

  MIRSymbol *GetSymbolFromStIdx(uint32 idx, bool checkFirst = false) const {
    if (checkFirst && static_cast<size_t>(idx) >= symbolTable.size()) {
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

  MIRSymbol *GetSymbolFromStrIdx(const GStrIdx &idx, bool checkFirst = false) const {
    return GetSymbolFromStIdx(GetStIdxFromStrIdx(idx).Idx(), checkFirst);
  }

  void Dump(bool isLocal, int32 indent = 0, bool printDeleted = false,
            MIRFlavor flavor = kFlavorUnknown) const;
  size_t GetSymbolTableSize() const {
    return symbolTable.size();
  }

  void Clear() {
    symbolTable.clear();
    strIdxToStIdxMap.clear();
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
      : addrTakenLabels(allocator.Adapter()),
        caseLabelSet(allocator.Adapter()),
        mAllocator(allocator),
        strIdxToLabIdxMap(std::less<GStrIdx>(), mAllocator.Adapter()),
        labelTable(mAllocator.Adapter()) {
    labelTable.push_back(GStrIdx(kDummyLabel));  // push dummy label index 0
  }

  ~MIRLabelTable() = default;

  LabelIdx CreateLabel() {
    LabelIdx labelIdx = labelTable.size();
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(std::to_string(labelIdx));
    labelTable.push_back(strIdx);
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

  void AddToStringLabelMap(LabelIdx labelIdx);
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

  MapleUnorderedSet<LabelIdx> addrTakenLabels; // those appeared in addroflabel or MIRLblConst
  MapleUnorderedSet<LabelIdx> caseLabelSet;    // labels marking starts of switch cases

 private:
  static constexpr uint32 kDummyLabel = 0;
  MapleAllocator mAllocator;
  MapleMap<GStrIdx, LabelIdx> strIdxToLabIdxMap;
  MapleVector<GStrIdx> labelTable;  // map label idx to label name
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_SYMBOL_H
