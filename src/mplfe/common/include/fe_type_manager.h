/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPLFE_INCLUDE_COMMON_FE_TYPE_MANAGER_H
#define MPLFE_INCLUDE_COMMON_FE_TYPE_MANAGER_H
#include <map>
#include <memory>
#include <list>
#include "mempool_allocator.h"
#include "mir_module.h"
#include "mir_type.h"
#include "mir_builder.h"
#include "feir_type.h"
#include "fe_struct_elem_info.h"
#include "fe_utils.h"

namespace maple {
enum FETypeFlag : uint16 {
  kDefault = 0,
  kSrcMpltSys = 1,
  kSrcInput = 1 << 1,
  kSrcMpltApk = 1 << 2,
  kSrcMplt = 1 << 3,
  kSrcExtern = 1 << 4,
  kSrcUnknown = 1 << 5,
  kSrcMask = 0x3F,
};

class FETypeSameNamePolicy {
 public:
  FETypeSameNamePolicy() : flag(0) {}
  ~FETypeSameNamePolicy() = default;
  bool IsUseLastest() const {
    return (flag & kFlagUseLastest) != 0;
  }

  bool IsFatal() const {
    return (flag & kFlagFatal) != 0;
  }

  void SetFlag(uint8 flagIn) {
    flag = flagIn;
  }

  static constexpr uint8 kFlagUseLastest = 0x1;
  static constexpr uint8 kFlagFatal = 0x2;

 private:
  // bitwise flag
  // bit0: 0-use first, 1-use lastest
  // bit1: 0-warning, 1-fatal
  uint8 flag;
};

using FEStructTypePair = std::pair<MIRStructType*, FETypeFlag>;

class FETypeManager {
 public:
  // ---------- prim FEIRType ----------
  const static UniqueFEIRType kPrimFEIRTypeUnknown;
  const static UniqueFEIRType kPrimFEIRTypeU1;
  const static UniqueFEIRType kPrimFEIRTypeI8;
  const static UniqueFEIRType kPrimFEIRTypeU8;
  const static UniqueFEIRType kPrimFEIRTypeI16;
  const static UniqueFEIRType kPrimFEIRTypeU16;
  const static UniqueFEIRType kPrimFEIRTypeI32;
  const static UniqueFEIRType kPrimFEIRTypeU32;
  const static UniqueFEIRType kPrimFEIRTypeI64;
  const static UniqueFEIRType kPrimFEIRTypeU64;
  const static UniqueFEIRType kPrimFEIRTypeF32;
  const static UniqueFEIRType kPrimFEIRTypeF64;
  const static UniqueFEIRType kFEIRTypeJavaObject;
  const static UniqueFEIRType kFEIRTypeJavaClass;
  const static UniqueFEIRType kFEIRTypeJavaString;

  explicit FETypeManager(MIRModule &moduleIn);
  ~FETypeManager();
  void ReleaseMemPool();
  bool LoadMplts(const std::list<std::string> &mpltNames, FETypeFlag flag, const std::string &phaseName);
  bool LoadMplt(const std::string &mpltName, FETypeFlag flag);
  void CheckSameNamePolicy() const;
  // method for Class or Interface generation
  MIRStructType *GetClassOrInterfaceType(const GStrIdx &nameIdx) const;
  MIRStructType *GetClassOrInterfaceType(const std::string &name) const {
    GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    return GetClassOrInterfaceType(nameIdx);
  }

  FETypeFlag GetClassOrInterfaceTypeFlag(const GStrIdx &nameIdx) const;
  FETypeFlag GetClassOrInterfaceTypeFlag(const std::string &name) const {
    GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    return GetClassOrInterfaceTypeFlag(nameIdx);
  }

  MIRStructType *CreateClassOrInterfaceType(const GStrIdx &nameIdx, bool isInterface, FETypeFlag typeFlag);
  MIRStructType *CreateClassOrInterfaceType(const std::string &name, bool isInterface, FETypeFlag typeFlag) {
    GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    return CreateClassOrInterfaceType(nameIdx, isInterface, typeFlag);
  }

  MIRStructType *GetOrCreateClassOrInterfaceType(const GStrIdx &nameIdx, bool isInterface, FETypeFlag typeFlag,
                                                 bool &isCreate);
  MIRStructType *GetOrCreateClassOrInterfaceType(const std::string &name, bool isInterface, FETypeFlag typeFlag,
                                                 bool &isCreate) {
    GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    return GetOrCreateClassOrInterfaceType(nameIdx, isInterface, typeFlag, isCreate);
  }

  MIRType *GetOrCreateClassOrInterfacePtrType(const GStrIdx &nameIdx, bool isInterface, FETypeFlag typeFlag,
                                              bool &isCreate);
  MIRType *GetOrCreateClassOrInterfacePtrType(const std::string &name, bool isInterface, FETypeFlag typeFlag,
                                              bool &isCreate) {
    GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    return GetOrCreateClassOrInterfacePtrType(nameIdx, isInterface, typeFlag, isCreate);
  }


  uint32 GetTypeIDFromMplClassName(const std::string &mplClassName) const;
  MIRStructType *GetStructTypeFromName(const std::string &name);
  MIRStructType *GetStructTypeFromName(const GStrIdx &nameIdx);
  MIRType *GetOrCreateTypeFromName(const std::string &name, FETypeFlag typeFlag, bool usePtr);
  MIRType *GetOrCreatePointerType(const MIRType &type, PrimType ptyPtr = PTY_ref);
  MIRType *GetOrCreateArrayType(MIRType &elemType, uint8 dim, PrimType ptyPtr = PTY_ref);
  MIRType *GetOrCreateJArrayType(MIRType &elemType, uint8 dim, PrimType ptyPtr = PTY_ref);
  void AddClassToModule(const MIRStructType &structType);

  // ---------- methods for StructElemInfo ----------
  // structIdx = 0: global field/function without owner structure
  FEStructElemInfo *RegisterStructFieldInfo(
      const StructElemNameIdx &argStructElemNameIdx, MIRSrcLang argSrcLang, bool isStatic);
  FEStructElemInfo *RegisterStructMethodInfo(
      const StructElemNameIdx &argStructElemNameIdx, MIRSrcLang argSrcLang, bool isStatic);
  FEStructElemInfo *GetStructElemInfo(const GStrIdx &fullNameIdx) const;

  // ---------- methods for MIRFunction ----------
  /**
   * Function Name: GetMIRFunction
   * Description: get MIRFunction by class method name
   *   1. return if exist in mpltFuncNameSymbolMap
   *   2. return if exist in nameFuncMap
   *   3. return null otherwise
   * Parallel note: Parallelable
   */
  MIRFunction *GetMIRFunction(const std::string &classMethodName, bool isStatic);
  MIRFunction *GetMIRFunction(const GStrIdx &nameIdx, bool isStatic);

  /*
   * Function Name: CreateMIRFunction
   * Description: create MIRFunction by class method name
   *   1. call GetMIRFunction first, if exist, return
   *   2. create MIRFunction
   * Parallel note: Non-parallelable
   */
  MIRFunction *CreateFunction(const GStrIdx &nameIdx, const TyIdx &retTypeIdx, const std::vector<TyIdx> &argsTypeIdx,
                              bool isVarg, bool isStatic);
  MIRFunction *CreateFunction(const std::string &methodName, const std::string &returnTypeName,
                              const std::vector<std::string> &argTypeNames, bool isVarg, bool isStatic);

  // FEIRType GetOrCreate
  const FEIRType *GetOrCreateFEIRTypeByName(const std::string &typeName, const GStrIdx &typeNameIdx,
                                            MIRSrcLang argSrcLang = kSrcLangJava);
  const FEIRType *GetOrCreateFEIRTypeByName(const GStrIdx &typeNameIdx, MIRSrcLang argSrcLang = kSrcLangJava);
  const FEIRType *GetFEIRTypeByName(const std::string &typeName) const;
  const FEIRType *GetFEIRTypeByName(const GStrIdx &typeNameIdx) const;

  // MCC function
  void InitMCCFunctions();
  MIRFunction *GetMCCFunction(const std::string &funcName) const;
  MIRFunction *GetMCCFunction(const GStrIdx &funcNameIdx) const;
  PUIdx GetPuIdxForMCCGetOrInsertLiteral() const {
    return funcMCCGetOrInsertLiteral->GetPuidx();
  }

  // anti-proguard
  bool IsAntiProguardFieldStruct(const GStrIdx &structNameIdx);

  static bool IsStructType(const MIRType &type);
  static PrimType GetPrimType(const std::string &name);
  static MIRType *GetMIRTypeForPrim(char c);
  static MIRType *GetMIRTypeForPrim(const std::string &name) {
    if (name.length() != 1) {
      return nullptr;
    }
    return GetMIRTypeForPrim(name[0]);
  }

  static std::string GetBaseTypeName(const std::string &name, uint32 &dim, bool inMpl = true);
  static std::string GetBaseTypeName(const std::string &name, bool inMpl = true) {
    uint32 dim = 0;
    return GetBaseTypeName(name, dim, inMpl);
  }
  static void SetComplete(MIRStructType &structType);
  static std::string TypeAttrsToString(const TypeAttrs &attrs);
  bool IsImportedType(const GStrIdx &typeNameIdx) const {
    return structNameTypeMap.find(typeNameIdx) != structNameTypeMap.end();
  }
  void MarkExternStructType();
  void SetMirImportedTypes(FETypeFlag flag);

 private:
  void UpdateStructNameTypeMapFromTypeTable(const std::string &mpltName, FETypeFlag flag);
  void UpdateNameFuncMapFromTypeTable();
  void UpdateDupTypes(const GStrIdx &nameIdx, bool isInterface,
                      const std::unordered_map<GStrIdx, FEStructTypePair, GStrIdxHash>::iterator &importedTypeIt);

  // MCC function
  void InitFuncMCCGetOrInsertLiteral();
  void InitFuncMCCStaticField();

  MIRModule &module;
  MemPool *mp;
  MapleAllocator allocator;
  MIRBuilder builder;
  // map<structNameIdx, pair>
  std::unordered_map<GStrIdx, FEStructTypePair, GStrIdxHash> structNameTypeMap;
  // map<structNameIdx, mpltNameIdx>
  std::unordered_map<GStrIdx, GStrIdx, GStrIdxHash> structNameSrcMap;
  // list<pair<structNameIdx, mpltNameIdx>>
  std::list<std::pair<GStrIdx, GStrIdx>> structSameNameSrcList;
  FETypeSameNamePolicy sameNamePolicy;
  MIRSrcLang srcLang;

  // ---------- class name ---> type id map info ----------
  std::unordered_map<std::string, uint32> classNameTypeIDMap;

  // ---------- struct elem info ----------
  std::map<GStrIdx, FEStructElemInfo*> mapStructElemInfo;

  // ---------- function list ----------
  std::unordered_map<GStrIdx, MIRFunction*, GStrIdxHash> nameFuncMap;
  std::unordered_map<GStrIdx, MIRFunction*, GStrIdxHash> nameStaticFuncMap;
  std::unordered_map<GStrIdx, MIRFunction*, GStrIdxHash> mpltNameFuncMap;
  std::unordered_map<GStrIdx, MIRFunction*, GStrIdxHash> mpltNameStaticFuncMap;

  // ---------- FEIRType list ----------
  std::unordered_map<GStrIdx, const FEIRType*, GStrIdxHash> nameFEIRTypeMap;
  std::list<UniqueFEIRType> nameFEIRTypeList;

  // ---------- MCC function list  ----------
  std::unordered_map<GStrIdx, MIRFunction*, GStrIdxHash> nameMCCFuncMap;
  MIRFunction *funcMCCGetOrInsertLiteral;

  MIRFunction *funcMCCStaticFieldGetBool = nullptr;
  MIRFunction *funcMCCStaticFieldGetByte = nullptr;
  MIRFunction *funcMCCStaticFieldGetShort = nullptr;
  MIRFunction *funcMCCStaticFieldGetChar = nullptr;
  MIRFunction *funcMCCStaticFieldGetInt = nullptr;
  MIRFunction *funcMCCStaticFieldGetLong = nullptr;
  MIRFunction *funcMCCStaticFieldGetFloat = nullptr;
  MIRFunction *funcMCCStaticFieldGetDouble = nullptr;
  MIRFunction *funcMCCStaticFieldGetObject = nullptr;

  MIRFunction *funcMCCStaticFieldSetBool = nullptr;
  MIRFunction *funcMCCStaticFieldSetByte = nullptr;
  MIRFunction *funcMCCStaticFieldSetShort = nullptr;
  MIRFunction *funcMCCStaticFieldSetChar = nullptr;
  MIRFunction *funcMCCStaticFieldSetInt = nullptr;
  MIRFunction *funcMCCStaticFieldSetLong = nullptr;
  MIRFunction *funcMCCStaticFieldSetFloat = nullptr;
  MIRFunction *funcMCCStaticFieldSetDouble = nullptr;
  MIRFunction *funcMCCStaticFieldSetObject = nullptr;

  // ---------- antiproguard ----------
  std::set<GStrIdx> setAntiProguardFieldStructIdx;
  mutable std::mutex feTypeManagerMtx;
};
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_FE_TYPE_MANAGER_H
