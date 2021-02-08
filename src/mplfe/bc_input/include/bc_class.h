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

#ifndef MPLFE_BC_INPUT_INCLUDE_BC_CLASS_H
#define MPLFE_BC_INPUT_INCLUDE_BC_CLASS_H
#include <vector>
#include <list>
#include <map>
#include <memory>
#include "types_def.h"
#include "bc_instruction.h"
#include "feir_stmt.h"
#include "bc_parser_base.h"
#include "feir_var.h"
#include "mempool.h"
#include "bc_pragma.h"
namespace maple {
namespace bc {
using BCAttrKind = uint32;
class BCClass;

class BCAttr {
 public:
  BCAttr(BCAttrKind k, const GStrIdx &idx) : kind(k), nameIdx(idx) {}
  ~BCAttr() = default;

 protected:
  BCAttrKind kind;
  GStrIdx nameIdx;
};

class BCAttrMap {
 public:
  BCAttrMap() = default;
  ~BCAttrMap() = default;

 protected:
  std::map<BCAttrKind, std::unique_ptr<std::list<std::unique_ptr<BCAttr>>>> mapAttrs;
};

class BCClassElem {
 public:
  BCClassElem(const BCClass &klassIn, uint32 acc, const std::string &nameIn, const std::string &descIn);
  virtual ~BCClassElem() = default;
  const std::string &GetName() const;
  const std::string &GetDescription() const;
  uint32 GetItemIdx() const;
  uint32 GetIdx() const;
  uint32 GetAccessFlag() const;
  bool IsStatic() const;
  const std::string &GetClassName() const;
  const BCClass &GetBCClass() const;
  GStrIdx GetClassNameMplIdx() const;

 protected:
  virtual uint32 GetItemIdxImpl() const = 0;
  virtual uint32 GetIdxImpl() const = 0;
  virtual bool IsStaticImpl() const = 0;
  const BCClass &klass;
  uint32 accessFlag;
  std::string name;
  std::string descriptor;
  BCAttrMap attrMap;
};

class BCClassField : public BCClassElem {
 public:
  BCClassField(const BCClass &klassIn, uint32 acc, const std::string &nameIn, const std::string &descIn)
      : BCClassElem(klassIn, acc, nameIn, descIn) {}
  ~BCClassField() = default;

 protected:
  uint32 GetItemIdxImpl() const override = 0;
  uint32 GetIdxImpl() const override = 0;
};

class BCCatchInfo {
 public:
  BCCatchInfo(uint32 handlerAddrIn, const GStrIdx &argExceptionNameIdx, bool isCatchAllIn);
  ~BCCatchInfo() = default;

  uint32 GetHandlerAddr() const {
    return handlerAddr;
  }

  GStrIdx GetExceptionNameIdx() const {
    return exceptionNameIdx;
  }

  bool GetIsCatchAll() const {
    return isCatchAll;
  }

 protected:
  uint32 handlerAddr;
  GStrIdx exceptionNameIdx;
  bool isCatchAll = false;
};

class BCTryInfo {
 public:
  BCTryInfo(uint32 startAddrIn, uint32 endAddrIn, std::unique_ptr<std::list<std::unique_ptr<BCCatchInfo>>> catchesIn)
      : startAddr(startAddrIn), endAddr(endAddrIn), catches(std::move(catchesIn)) {}
  ~BCTryInfo() = default;

  uint32 GetStartAddr() const {
    return startAddr;
  }

  uint32 GetEndAddr() const {
    return endAddr;
  }

  const std::list<std::unique_ptr<BCCatchInfo>> *GetCatches() const {
    return catches.get();
  }
  static void DumpTryCatchInfo(const std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> &tryInfos);

 protected:
  uint32 startAddr;
  uint32 endAddr;
  std::unique_ptr<std::list<std::unique_ptr<BCCatchInfo>>> catches;
};

class BCClassMethod : public BCClassElem {
 public:
  BCClassMethod(const BCClass &klassIn, uint32 acc, bool isVirtualIn, const std::string &nameIn,
                const std::string &descIn)
      : BCClassElem(klassIn, acc, nameIn, descIn), isVirtual(isVirtualIn),
        methodMp(memPoolCtrler.NewMemPool("MemPool for BCClassMethod")),
        allocator(methodMp) {}
  ~BCClassMethod() {
    if (methodMp != nullptr) {
      delete methodMp;
      methodMp = nullptr;
    }
  }
  void SetPCBCInstructionMap(MapleMap<uint32, BCInstruction*> *mapIn) {
    pcBCInstructionMap = mapIn;
  }
  void SetMethodInstOffset(const uint16 *pos);
  void SetRegisterTotalSize(uint16 size);
  uint16 GetRegisterTotalSize() const;
  void SetRegisterInsSize(uint16 size);
  void AddMethodDepSet(const std::string &depType);
  bool IsVirtual() const;
  bool IsNative() const;
  bool IsInit() const;
  bool IsClinit() const;
  std::string GetFullName() const;
  void ProcessInstructions();
  std::list<UniqueFEIRStmt> EmitInstructionsToFEIR() const;
  const uint16 *GetInstPos() const;

  void SetTryInfos(std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> infos) {
    tryInfos = std::move(infos);
  }

  uint16 GetTriesSize() const {
    return tryInfos->size();
  }

  uint32 GetThisRegNum() const {
    return IsStatic() ? UINT32_MAX : static_cast<uint32>(registerTotalSize - registerInsSize);
  }

  void SetSrcPositionInfo(const std::map<uint32, uint32> &srcPosInfo) {
#ifdef DEBUG
    pSrcPosInfo = &srcPosInfo;
#endif
  }

  std::vector<std::unique_ptr<FEIRVar>> GenArgVarList() const;
  void GenArgRegs();
  std::vector<std::string> GetSigTypeNames() const {
    return sigTypeNames;
  }
  bool HasCode() const {
    return pcBCInstructionMap != nullptr && !pcBCInstructionMap->empty();
  }

  const MemPool *GetMemPool() const {
    return methodMp;
  }

  void ReleaseMempool() {
    if (methodMp != nullptr) {
      delete methodMp;
      methodMp = nullptr;
    }
  }

  MapleAllocator &GetAllocator() {
    return allocator;
  }

 protected:
  uint32 GetItemIdxImpl() const override = 0;
  uint32 GetIdxImpl() const override = 0;
  virtual bool IsVirtualImpl() const = 0;
  virtual bool IsNativeImpl() const = 0;
  virtual bool IsInitImpl() const = 0;
  virtual bool IsClinitImpl() const = 0;
  void ProcessTryCatch();
  virtual std::vector<std::unique_ptr<FEIRVar>> GenArgVarListImpl() const = 0;
  // This struct is used in TypeInfer only.
  struct TypeInferItem {
    BCReg *reg = nullptr;
    std::set<BCRegTypeItem*> aliveUsedTypes;
    TypeInferItem *prev = nullptr;
  };

  void TypeInfer();
  void InsertPhi(const std::vector<TypeInferItem*> &dest, std::vector<TypeInferItem*> &src);
  static TypeInferItem *RegisterTypeInferItem(std::list<std::unique_ptr<TypeInferItem>> &items, BCReg* bcReg,
                                              TypeInferItem *prev);
  static std::vector<TypeInferItem*> ConstructNewRegTypeMap(const std::vector<TypeInferItem*> &regTypeMap,
      std::list<std::unique_ptr<BCClassMethod::TypeInferItem>> &items);
  std::list<UniqueFEIRStmt> GenReTypeStmtsThroughArgs() const;
  void PrecisifyRegType();
  virtual void GenArgRegsImpl() = 0;
  static void LinkJumpTarget(const std::map<uint32, FEIRStmtPesudoLabel2*> &targetFEIRStmtMap,
                             const std::list<FEIRStmtGoto2*> &gotoFEIRStmts,
                             const std::list<FEIRStmtSwitch2*> &switchFEIRStmts);
  void DumpBCInstructionMap() const;
  void SetSrcPosInfo();
  MapleMap<uint32, BCInstruction*> *pcBCInstructionMap;
  std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> tryInfos;
#ifdef DEBUG
  const std::map<uint32, uint32> *pSrcPosInfo = nullptr;
#endif
  std::vector<std::unique_ptr<BCReg>> argRegs;
  std::vector<std::string> sigTypeNames;
  bool isVirtual = false;
  uint16 registerTotalSize = UINT16_MAX;
  uint16 registerInsSize = UINT16_MAX;
  const uint16 *instPos = nullptr;  // method instructions start pos in bc file
  std::set<uint32> visitedPcSet;
  std::set<uint32> multiInDegreeSet;
  std::list<BCRegType*> regTypes;

  MemPool *methodMp;
  MapleAllocator allocator;
};

class BCClass {
 public:
  BCClass(uint32 idx, const BCParserBase &parserIn) : classIdx(idx), parser(parserIn) {}
  ~BCClass() = default;

  uint32 GetClassIdx() const {
    return classIdx;
  }

  const BCParserBase &GetBCParser() const {
    return parser;
  }

  bool IsInterface() const {
    return isInterface;
  }

  void SetFilePathName(const std::string &path) {
    filePathName = path;
  }

  void SetSrcFileInfo(const std::string &name);

  uint32 GetSrcFileIdx() const {
    return srcFileIdx;
  }

  void SetIsInterface(bool flag) {
    isInterface = flag;
  }
  void SetSuperClasses(const std::list<std::string> &names);
  void SetInterface(const std::string &name);
  void SetAccFlag(uint32 flag);
  void SetField(std::unique_ptr<BCClassField> field);
  void SetMethod(std::unique_ptr<BCClassMethod> method);
  void SetClassName(const std::string &classNameOrinIn);
  void SetAnnotationsDirectory(std::unique_ptr<BCAnnotationsDirectory> annotationsDirectory);
  void InsertStaticFieldConstVal(MIRConst *cst);
  void InsertFinalStaticStringID(uint32 stringID);
  std::vector<uint32> GetFinalStaticStringIDVec() const {
    return finalStaticStringID;
  }
  std::vector<MIRConst*> GetStaticFieldsConstVal() const;
  const std::string &GetClassName(bool mapled) const;

  GStrIdx GetClassNameMplIdx() const {
    return classNameMplIdx;
  }

  const std::list<std::string> &GetSuperClassNames() const;
  const std::vector<std::string> &GetSuperInterfaceNames() const;
  std::string GetSourceFileName() const;
  uint32 GetAccessFlag() const;

  const std::vector<std::unique_ptr<BCClassField>> &GetFields() const;
  std::vector<std::unique_ptr<BCClassMethod>> &GetMethods();
  const std::unique_ptr<BCAnnotationsDirectory> &GetAnnotationsDirectory() const;

 protected:
  bool isInterface = false;
  uint32 classIdx;
  uint32 srcFileIdx = 0;
  uint32 accFlag = 0;
  GStrIdx classNameOrinIdx;
  GStrIdx classNameMplIdx;
  GStrIdx srcFileNameIdx;
  const BCParserBase &parser;
  std::list<std::string> superClassNameList;
  std::vector<std::string> interfaces;
  std::vector<std::unique_ptr<BCClassField>> fields;
  std::vector<std::unique_ptr<BCClassMethod>> methods;
  std::unique_ptr<BCAnnotationsDirectory> annotationsDirectory;
  std::vector<MIRConst*> staticFieldsConstVal;
  std::vector<uint32> finalStaticStringID;
  BCAttrMap attrMap;
  std::string filePathName;
  mutable std::mutex bcClassMtx;
};
}
}
#endif // MPLFE_BC_INPUT_INCLUDE_BC_CLASS_H