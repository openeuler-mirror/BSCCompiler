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
#ifndef MPL2MPL_INCLUDE_MUID_REPLACEMENT_H
#define MPL2MPL_INCLUDE_MUID_REPLACEMENT_H
#include "phase_impl.h"
#include "muid.h"
#include "version.h"
#include "maple_phase_manager.h"

namespace maple {
// For func def table.
constexpr uint32 kFuncDefAddrIndex = 0;
// For data def table.
constexpr uint32 kDataDefAddrIndex = 0;
// For func def info. table.
constexpr uint32 kFuncDefSizeIndex = 0;
constexpr uint32 kFuncDefNameIndex = 1;
constexpr uint32 kRangeBeginIndex = 0;
constexpr int32_t kDecoupleAndLazy = 3;
constexpr uint32_t kShiftBit16 = 16;
constexpr uint32_t kShiftBit15 = 15;

enum RangeIdx {
  // 0,1 entry is reserved for a stamp
  kVtabAndItab = 2,
  kItabConflict = 3,
  kVtabOffset = 4,
  kFieldOffset = 5,
  kValueOffset = 6,
  kLocalClassInfo = 7,
  kConststr = 8,
  kSuperclass = 9,
  kGlobalRootlist = 10,
  kClassmetaData = 11,
  kClassBucket = 12,
  kJavatext = 13,
  kJavajni = 14,
  kJavajniFunc = 15,
  kOldMaxNum = 16, // Old num
  kDataSection = 17,
  kDecoupleStaticKey = 18,
  kDecoupleStaticValue = 19,
  kBssStart = 20,
  kLinkerSoHash = 21,
  kArrayClassCache = 22,
  kArrayClassCacheName = 23,
  kNewMaxNum = 24 // New num
};

struct SourceFileMethod {
  uint32 sourceFileIndex;
  uint32 sourceClassIndex;
  uint32 sourceMethodIndex;
  bool isVirtual;
};

struct SourceFileField {
  uint32 sourceFileIndex;
  uint32 sourceClassIndex;
  uint32 sourceFieldIndex;
};

class MUIDReplacement : public FuncOptimizeImpl {
 public:
  MUIDReplacement(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~MUIDReplacement() override = default;

  FuncOptimizeImpl *Clone() override {
    return new MUIDReplacement(*this);
  }

  void ProcessFunc(MIRFunction *func) override;

  static void SetMplMd5(MUID muid) {
    mplMuid = muid;
  }

  static MUID &GetMplMd5() {
    return mplMuid;
  }

 private:
  using SymIdxPair = std::pair<MIRSymbol*, uint32>;
  using SourceIndexPair = std::pair<uint32, uint32>;
  enum LazyBindingOption : uint32 {
    kNoLazyBinding = 0,
    kConservativeLazyBinding = 1,
    kRadicalLazyBinding = 2
  };

  void InitRangeTabUseSym(std::vector<MIRSymbol*> &workList, MIRStructType &rangeTabEntryType,
                          MIRAggConst &rangeTabConst);
  void GenerateTables();
  void GenerateFuncDefTable();
  void GenerateDataDefTable();
  void GenerateUnifiedUndefTable();
  void GenerateRangeTable();
  uint32 FindIndexFromDefTable(const MIRSymbol &mirSymbol, bool isFunc);
  uint32 FindIndexFromUndefTable(const MIRSymbol &mirSymbol, bool isFunc);
  void ReplaceAddroffuncConst(MIRConst *&entry, uint32 fieldID, bool isVtab);
  void ReplaceFuncTable(const std::string &name);
  void ReplaceAddrofConst(MIRConst *&entry, bool muidIndex32Mod = false);
  void ReplaceAddrofConstByCreatingNewIntConst(MIRAggConst &aggrC, uint32 i);
  void ReplaceDataTable(const std::string &name);
  void ReplaceDirectInvokeOrAddroffunc(MIRFunction &currentFunc, StmtNode &stmt);
  ArrayNode *LoadSymbolPointer(const MIRSymbol &mirSymbol);
  void ReplaceDassign(MIRFunction &currentFunc, const DassignNode &dassignNode);
  void ReplaceDreadStmt(MIRFunction *currentFunc, StmtNode *stmt);
  void ClearVtabItab(const std::string &name);
  void ReplaceDecoupleKeyTable(MIRAggConst *oldConst);
  bool IsMIRAggConstNull(MIRSymbol *tabSym) const;
  void ReplaceFieldTypeTable(const std::string &name);
  BaseNode *ReplaceDreadExpr(MIRFunction *currentFunc, StmtNode *stmt, BaseNode *expr);
  BaseNode *ReplaceDread(MIRFunction &currentFunc, const StmtNode *stmt, BaseNode *opnd);
  void CollectFieldCallSite();
  void CollectDreadStmt(MIRFunction *currentFunc, BlockNode *block, StmtNode *stmt);
  BaseNode *CollectDreadExpr(MIRFunction *currentFunc, BlockNode &block, StmtNode *stmt, BaseNode *expr);
  void CollectDread(MIRFunction &currentFunc, StmtNode &stmt, BaseNode &opnd);
  void DumpMUIDFile(bool isFunc);
  void ReplaceStmts();
  void GenerateGlobalRootList();
  void CollectImplicitUndefClassInfo(StmtNode &stmt);
  void CollectFuncAndData();
  void InsertFunctionProfile(MIRFunction &currentFunc, uint64 index);
  void GenericSourceMuid();
  void GenCompilerMfileStatus();
  bool FindFuncNameInSimplfy(const std::string &name);
  bool CheckFunctionIsUsed(const MIRFunction &mirFunc) const;
  void ReplaceMethodMetaFuncAddr(const MIRSymbol &funcSymbol, uint64 index) const;
  void ReplaceFieldMetaStaticAddr(const MIRSymbol &mirSymbol, uint32 index) const;
  void CollectFuncAndDataFromKlasses();
  void CollectFuncAndDataFromGlobalTab();
  void HandleUndefFuncAndClassInfo(PUIdx puidx, StmtNode &stmt);
  void CollectFuncAndDataFromFuncList();
  void GenerateCompilerVersionNum();
  int64 GetDefOrUndefOffsetWithMask(uint64 offset, bool isDef, bool muidIndex32Mod = false) const;
  void CollectSuperClassArraySymbolData();
  void GenerateSourceInfo();
  static MIRSymbol *GetSymbolFromName(const std::string &name);
  ConstvalNode* GetConstvalNode(uint64 index) const;
  void InsertArrayClassSet(const MIRType &type);
  MIRType *GetIntrinsicConstArrayClass(StmtNode &stmt) const;
  void CollectArrayClass();
  void GenArrayClassCache();
  void ReleasePragmaMemPool() const;
  std::unordered_set<std::string> arrayClassSet;
  // The following sets are for internal uses. Sorting order does not matter here.
  std::unordered_set<MIRFunction*> funcDefSet;
  std::unordered_set<MIRFunction*> funcUndefSet;
  std::unordered_set<MIRSymbol*> dataDefSet;
  std::unordered_set<MIRSymbol*> dataUndefSet;
  std::unordered_set<MIRSymbol*> superClassArraySymbolSet;
  void AddDefFunc(MIRFunction *func) {
    funcDefSet.insert(func);
  }

  void AddUndefFunc(MIRFunction *func) {
    funcUndefSet.insert(func);
  }

  void AddDefData(MIRSymbol *sym) {
    dataDefSet.insert(sym);
  }

  void AddUndefData(MIRSymbol *sym) {
    dataUndefSet.insert(sym);
  }

#define __MRT_MAGIC_PASTE(x, y) __MRT_MAGIC_PASTE2(x, y)
#define __MRT_MAGIC_PASTE2(x, y) x##y
#define CLASS_PREFIX(classname) TO_STR(__MRT_MAGIC_PASTE(CLASSINFO_PREFIX, classname)),
  const std::unordered_set<std::string> preloadedClassInfo = {
#include "white_list.def"
  };
#undef CLASS_PREFIX
#undef __MRT_MAGIC_PASTE2
#undef __MRT_MAGIC_PASTE
  const std::unordered_set<std::string> reflectionList = {
#include "reflection_list.def"
  };
  bool isLibcore = false;
  MIRSymbol *funcDefTabSym = nullptr;
  MIRSymbol *funcDefOrigTabSym = nullptr;
  MIRSymbol *funcInfTabSym = nullptr;
  MIRSymbol *funcUndefTabSym = nullptr;
  MIRSymbol *dataDefTabSym = nullptr;
  MIRSymbol *dataDefOrigTabSym = nullptr;
  MIRSymbol *dataUndefTabSym = nullptr;
  MIRSymbol *funcDefMuidTabSym = nullptr;
  MIRSymbol *funcUndefMuidTabSym = nullptr;
  MIRSymbol *dataDefMuidTabSym = nullptr;
  MIRSymbol *dataUndefMuidTabSym = nullptr;
  MIRSymbol *funcMuidIdxTabSym = nullptr;
  MIRSymbol *rangeTabSym = nullptr;
  MIRSymbol *funcProfileTabSym = nullptr;
  MIRSymbol *funcProfInfTabSym = nullptr;
  std::map<MUID, SymIdxPair> funcDefMap;
  std::map<MUID, SymIdxPair> dataDefMap;
  std::map<MUID, SymIdxPair> funcUndefMap;
  std::map<MUID, SymIdxPair> dataUndefMap;
  std::map<MUID, uint32> defMuidIdxMap;
  std::map<MUID, SourceIndexPair> sourceIndexMap;
  std::map<MUID, const SourceFileMethod> sourceFileMethodMap;
  std::map<MUID, const SourceFileField> sourceFileFieldMap;
  static MUID mplMuid;
  std::string mplMuidStr;
};

MAPLE_MODULE_PHASE_DECLARE(M2MMuidReplacement)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_MUID_REPLACEMENT_H
