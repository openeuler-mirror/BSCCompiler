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
#ifndef MPLFE_BC_INPUT_INCLUDE_BC_INSTRUCTION_H
#define MPLFE_BC_INPUT_INCLUDE_BC_INSTRUCTION_H
#include <string>
#include <tuple>
#include "types_def.h"
#include "bc_util.h"
#include "feir_stmt.h"
#include "feir_var_reg.h"

namespace maple {
namespace bc {
class BCClassMethod;
struct BCReg;
class BCInstruction {
 public:
  BCInstruction(MapleAllocator &allocatorIn, uint32 pcIn, uint8 opcodeIn)
      : opcode(opcodeIn),
        pc(pcIn),
        allocator(allocatorIn),
        catchedExTypeNamesIdx(allocator.Adapter()),
        handlerTargets(allocator.Adapter()),
        defedRegs(allocator.Adapter()),
        usedRegs(allocator.Adapter()),
        handlers(allocator.Adapter()) {}
  virtual ~BCInstruction() = default;
  void InitBCInStruction(uint16 kind, bool wide, bool throwable);
  BCInstructionKind GetInstKind() const;
  bool IsWide() const;
  void SetInstructionKind(BCInstructionKind kind);
  bool IsConditionBranch() const;
  bool IsGoto() const;
  bool IsSwitch() const;
  bool IsTarget() const;
  bool IsTryStart() const;
  bool IsTryEnd() const;
  bool IsCatch() const;
  void SetReturnInst(BCInstruction *inst);
  bool HasReturn() const;
  bool IsReturn() const;
  uint32 GetPC() const;
  uint8 GetOpcode() const;
  std::vector<uint32> GetTargets() const;
  void SetDefultTarget(BCInstruction *inst);
  void AddHandler(BCInstruction *handler);
  void SetWidth(uint8 size);
  uint8 GetWidth() const;
  void SetCatchable();
  bool IsCatchable() const;
  bool IsFallThru() const;
  void SetBCRegType(const BCInstruction &inst);
  void AddHandlerTarget(uint32 target);
  MapleVector<uint32> GetHandlerTargets() const;
  MapleList<BCReg*> *GetDefedRegs();
  MapleList<BCReg*> *GetUsedRegs();
  void SetRegTypeInTypeInfer();
  void SetFuncNameIdx(const GStrIdx &methodIdx);
  void SetSrcPositionInfo(uint32 fileIdxIn, uint32 lineNumIn);
  void SetOpName(const char *name);
  const char *GetOpName() const;
  void Parse(const BCClassMethod &method);
  void SetExceptionType(const GStrIdx &typeNameIdx);
  std::list<UniqueFEIRStmt> EmitToFEIRStmts();

 protected:
  virtual std::vector<uint32> GetTargetsImpl() const;
  virtual void ParseImpl(const BCClassMethod &method) = 0;
  virtual std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() = 0;
  void GenCommentStmt(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRStmt GenLabelStmt() const;
  UniqueFEIRStmt GenCatchStmt() const;
  UniqueFEIRStmt GenTryLabelStmt() const;
  UniqueFEIRStmt GenTryEndLabelStmt() const;
  virtual void SetRegTypeInTypeInferImpl() {}
  virtual void SetBCRegTypeImpl(const BCInstruction &inst) {}  // for move-result and move-exception
  std::list<UniqueFEIRStmt> GenRetypeStmtsAfterDef() const;
  std::list<UniqueFEIRStmt> GenRetypeStmtsBeforeUse() const;
  void SetSrcFileInfo(std::list<UniqueFEIRStmt> &stmts) const;
  bool isThrowable = false;
  bool isWide = false;
  bool isReturn = false;
  bool isCatchable = false;
  uint8 width = UINT8_MAX;  // Default value, unuseable
  uint8 opcode;
  BCInstructionKind instKind = kUnKnownKind;
  uint32 funcNameIdx = UINT32_MAX;
  uint32 srcFileIdx = 0;
  uint32 srcFileLineNum = 0;
  uint32 pc;
#ifdef DEBUG
  const char *opName = nullptr;
#endif
  BCInstruction *returnInst = nullptr;
  BCInstruction *defaultTarget = nullptr;
  MapleAllocator &allocator;
  MapleSet<GStrIdx> catchedExTypeNamesIdx;
  MapleVector<uint32> handlerTargets; // This instruction may throw exception and reach these handlers
  MapleList<BCReg*> defedRegs;
  MapleList<BCReg*> usedRegs;
  MapleList<BCInstruction*> handlers;
};

// Forward declaration
struct BCReg;

struct BCRegTypeItem {
  BCRegTypeItem(const GStrIdx &idx, bool isPrimPtrIn = false, bool isIndeterminateIn = false)
      : typeNameIdx(idx), isPrimPtr(isPrimPtrIn), isIndeterminate(isIndeterminateIn) {}
  BCRegTypeItem(const BCRegTypeItem &item)
      : typeNameIdx(item.typeNameIdx), isPrimPtr(item.isPrimPtr), isIndeterminate(item.isIndeterminate) {}
  ~BCRegTypeItem() = default;
  GStrIdx typeNameIdx;
  bool isPrimPtr = false;
  bool isIndeterminate = false;

  PrimType GetPrimType() const;
  PrimType GetBasePrimType() const;
  BCRegTypeItem *Clone(const MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<BCRegTypeItem>(typeNameIdx, isPrimPtr, isIndeterminate);
  }

  void Copy(const BCRegTypeItem &src) {
    typeNameIdx = src.typeNameIdx;
    isPrimPtr = src.isPrimPtr;
    isIndeterminate = src.isIndeterminate;
  }
  bool operator==(const BCRegTypeItem &item) const {
    return typeNameIdx == item.typeNameIdx && isPrimPtr == item.isPrimPtr;
  }

  bool IsRef() const {
    return GetPrimType() == PTY_ref;
  }

  bool IsMorePreciseType(const BCRegTypeItem &typeItemIn) const;
};

class BCRegType {
 public:
  BCRegType(MapleAllocator &allocatorIn, BCReg &reg, const GStrIdx &typeNameIdxIn,
            bool isPrimPtrIn = false, bool isIndeterminateIn = false);
  ~BCRegType() = default;
  MapleAllocator &GetAllocator() {
    return allocator;
  }

  void SetTypeNameIdx(const GStrIdx &idx) {
    regTypeItem->typeNameIdx = idx;
  }

  void SetIsPrimPtr(bool flag) {
    regTypeItem->isPrimPtr = flag;
  }

  bool IsIndeterminate() const {
    return regTypeItem->isIndeterminate;
  }

  void SetIsIndeterminate(bool flag) {
    regTypeItem->isIndeterminate = flag;
  }

  void UpdateDefTypeFromUse(BCRegTypeItem *typeItem);

  void UpdateDefTypeThroughPhi(BCReg &defedReg, const std::set<BCRegTypeItem*> &usedTypes);

  static void InsertUniqueTypeItem(MapleVector<BCRegTypeItem*> &itemSet, BCRegTypeItem *item) {
    for (auto &elem : itemSet) {
      if (!elem->isIndeterminate && (*elem) == (*item)) {
        return;
      }
    }
    itemSet.emplace_back(item);
  }

  void UpdateUsedSet(BCRegTypeItem *typeItem) {
    if (regTypeItem->isIndeterminate && !typeItem->isIndeterminate && ((*regTypeItem) == (*typeItem))) {
      // Make reg type determinate ASAP
      regTypeItem->Copy(*typeItem);
    }
    InsertUniqueTypeItem(typesUsedAs, typeItem);
  }

  void SetRegTypeItem(BCRegTypeItem *item) {
    regTypeItem = item;
  }

  const BCRegTypeItem *GetRegTypeItem() const {
    return regTypeItem;
  }

  void UpdateTypeSetFromPhi(BCReg &defedReg, const std::set<BCRegTypeItem*> &usedTypes);

  MapleVector<BCRegTypeItem*> *GetUsedTypes() {
    return &typesUsedAs;
  }

  void PrecisifyTypes();

  bool IsPrecisified() const {
    return precisified;
  }

  static BCRegTypeItem *GetMostPreciseType(const MapleVector<BCRegTypeItem*> &types);

  void AddElemType(BCRegType *item) {
    elemTypes.emplace(item);
  }

  void AddArrayType(BCRegType *regType) {
    arrayTypes.emplace(regType);
  }

 private:
  MapleAllocator allocator;
  BCReg &curReg;
  BCRegTypeItem *regTypeItem;
  // <typeNameIdx, isPrimPtr>
  MapleVector<BCRegTypeItem*> typesUsedAs;
  bool precisified = false;
  MapleSet<BCRegType*> elemTypes;
  MapleSet<BCRegType*> arrayTypes;
};

struct BCRegValue {
  union {
    uint64 raw64 = 0;
    uint32 raw32;
  } primValue;
  GStrIdx literalStrIdx = GStrIdx(0);  // literal string
};

struct BCReg {
  BCReg() = default;
  virtual ~BCReg() = default;
  bool isDef = false;
  uint32 regNum;
  BCRegType *regType = nullptr;
  BCRegValue *regValue = nullptr;
  BCRegTypeItem *regTypeItem = nullptr;
  bool IsConstZero() const;
  UniqueFEIRType GenFEIRType() const;
  UniqueFEIRVar GenFEIRVarReg() const;
  std::list<UniqueFEIRStmt> GenRetypeStmtsAfterDef() const;
  std::list<UniqueFEIRStmt> GenRetypeStmtsBeforeUse() const;
  // a reg maybe store a primitive pointer type var.
  // If it is a primitive ptr, GetPrimType() return a PTY_ref
  //                           GetBasePrimType() return a corresponding prim type
  // If it is not a primitive pointer type, GetPrimType() and GetBasePrimType() return a same PrimType.
  PrimType GetPrimType() const;
  PrimType GetBasePrimType() const;
  std::unique_ptr<BCReg> Clone() const {
    return CloneImpl();
  }

 protected:
  virtual UniqueFEIRType GenFEIRTypeImpl() const;
  virtual UniqueFEIRVar GenFEIRVarRegImpl() const;
  virtual std::unique_ptr<BCReg> CloneImpl() const;
};
}  // namespace bc
}  // namespace maple
#endif  // MPLFE_BC_INPUT_INCLUDE_BC_INSTRUCTION_H
