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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_INSTRUCTION_H
#define HIR2MPL_BC_INPUT_INCLUDE_BC_INSTRUCTION_H
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
  void SetDefaultTarget(BCInstruction *inst);
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
  void Parse(BCClassMethod &method);
  void SetExceptionType(const GStrIdx &typeNameIdx);
  std::list<UniqueFEIRStmt> EmitToFEIRStmts();

 protected:
  virtual std::vector<uint32> GetTargetsImpl() const;
  virtual void ParseImpl(BCClassMethod &method) = 0;
  virtual std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() = 0;
  void GenCommentStmt(std::list<UniqueFEIRStmt> &stmts) const;
  UniqueFEIRStmt GenLabelStmt() const;
  UniqueFEIRStmt GenCatchStmt() const;
  UniqueFEIRStmt GenTryLabelStmt() const;
  UniqueFEIRStmt GenTryEndLabelStmt() const;
  virtual void SetRegTypeInTypeInferImpl() {}
  virtual void SetBCRegTypeImpl(const BCInstruction &inst) {
    (void) inst;
  }  // for move-result and move-exception
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
struct TypeInferItem;

struct BCRegTypeItem {
  explicit BCRegTypeItem(const GStrIdx &idx, bool isIndeterminateIn = false, bool isFromDefIn = false)
      : typeNameIdx(idx),
        isIndeterminate(isIndeterminateIn),
        isFromDef(isFromDefIn) {}
  BCRegTypeItem(const BCRegTypeItem &item)
      : typeNameIdx(item.typeNameIdx),
        isIndeterminate(item.isIndeterminate) {}
  ~BCRegTypeItem() = default;
  BCRegTypeItem& operator=(const BCRegTypeItem &item);

  PrimType GetPrimType() const;
  PrimType GetBasePrimType() const;
  BCRegTypeItem *Clone(const MapleAllocator &allocator) {
    return allocator.GetMemPool()->New<BCRegTypeItem>(typeNameIdx, isIndeterminate);
  }

  void Copy(const BCRegTypeItem &src) {
    typeNameIdx = src.typeNameIdx;
    isIndeterminate = src.isIndeterminate;
    isFromDef = src.isFromDef;
  }
  bool operator==(const BCRegTypeItem &item) const {
    return typeNameIdx == item.typeNameIdx;
  }

  bool IsRef() const {
    return GetPrimType() == PTY_ref;
  }

  bool IsMorePreciseType(const BCRegTypeItem &typeItemIn) const;

  // for debug
  void DumpTypeName() const {
    LogInfo::MapleLogger(kLlDbg) << GlobalTables::GetStrTable().GetStringFromStrIdx(typeNameIdx) << "\n";
  }

  void SetPos(uint32 pc) {
    pos = pc;
  }

  void SetDom(bool flag) {
    isDom = flag;
  }

  bool IsDom() const {
    return isDom;
  }

  GStrIdx typeNameIdx;
  bool isIndeterminate = false;
  // means `is def ` or `come from def`
  bool isFromDef = false;
  uint32 pos = UINT32_MAX;

  bool isDom = false;
};

class BCRegType {
 public:
  BCRegType(MapleAllocator &allocatorIn, BCReg &reg, const GStrIdx &typeNameIdxIn, bool isIndeterminateIn = false);
  ~BCRegType() = default;
  MapleAllocator &GetAllocator() {
    return allocator;
  }

  void SetTypeNameIdx(const GStrIdx &idx) const {
    regTypeItem->typeNameIdx = idx;
  }

  bool IsIndeterminate() const {
    return regTypeItem->isIndeterminate;
  }

  void SetIsIndeterminate(bool flag) const {
    regTypeItem->isIndeterminate = flag;
  }

  static void InsertUniqueTypeItem(MapleList<BCRegTypeItem*> &itemSet, BCRegTypeItem *item) {
    for (auto const &elem : itemSet) {
      if ((!elem->isIndeterminate && !item->isIndeterminate && (*elem) == (*item)) || elem == item) {
        return;
      }
    }
    itemSet.emplace_back(item);
  }

  void UpdateUsedSet(BCRegTypeItem *typeItem) {
    if (typeItem->isIndeterminate) {
      InsertUniqueTypeItem(fuzzyTypesUsedAs, typeItem);
    }
    InsertUniqueTypeItem(*typesUsedAs, typeItem);
  }

  void UpdateFuzzyUsedSet(BCRegTypeItem *typeItem) {
    if (typeItem->isIndeterminate) {
      InsertUniqueTypeItem(fuzzyTypesUsedAs, typeItem);
    }
  }

  void SetRegTypeItem(BCRegTypeItem *item) {
    regTypeItem = item;
  }

  const BCRegTypeItem *GetRegTypeItem() const {
    return regTypeItem;
  }

  MapleList<BCRegTypeItem*> *GetUsedTypes() {
    return typesUsedAs;
  }

  void PrecisifyTypes(bool isTry = false);

  void PrecisifyRelatedTypes(const BCRegTypeItem *realType);

  void PrecisifyElemTypes(const BCRegTypeItem *realType);

  bool IsPrecisified() const {
    return precisified;
  }

  static BCRegTypeItem *GetMostPreciseType(const MapleList<BCRegTypeItem*> &types);

  void AddElemType(BCRegTypeItem *item) {
    elemTypes.emplace(item);
  }

  void SetUsedTypes(MapleList<BCRegTypeItem*> *types) {
    typesUsedAs = types;
  }

  void SetPos(uint32 pc) {
    pos = pc;
  }

  uint32 GetPos() const {
    return pos;
  }

  void RegisterRelatedBCRegType(BCRegType *ty) {
    relatedBCRegTypes.emplace_back(ty);
  }

  void RegisterLivesInfo(uint32 posStart) {
    livesBegins.emplace_back(posStart);
  }

  bool IsBefore(uint32 posCurr, uint32 end) const {
    for (auto p : livesBegins) {
      if (p == posCurr) {
        return true;
      }
      if (p == end) {
        return false;
      }
    }
    CHECK_FATAL(false, "Should not get here.");
    return true;
  }

 private:
  MapleAllocator &allocator;
  BCReg &curReg;
  BCRegTypeItem *regTypeItem;
  MapleList<BCRegTypeItem*> *typesUsedAs = nullptr;
  MapleList<BCRegTypeItem*> fuzzyTypesUsedAs;
  bool precisified = false;
  MapleSet<BCRegTypeItem*> elemTypes;
  uint32 pos = UINT32_MAX;
  // `0` for args, `pc + 1` for local defs
  MapleVector<uint32> livesBegins;

  std::list<BCRegType*> relatedBCRegTypes;
};

struct BCRegValue {
  union {
    uint64 raw64 = 0;
    uint32 raw32;
  } primValue;
};

struct BCReg {
  BCReg() = default;
  virtual ~BCReg() = default;
  bool isDef = false;
  uint32 regNum = UINT32_MAX;
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

// for TypeInfer
struct TypeInferItem {
  TypeInferItem(MapleAllocator &alloc, uint32 pos, BCReg *regIn, TypeInferItem *prev)
      : beginPos(pos),
        prevs(alloc.Adapter()),
        aliveUsedTypes(alloc.GetMemPool()->New<MapleList<BCRegTypeItem*>>(alloc.Adapter())),
        reg(regIn) {
    if (prev == nullptr) {
      reg->regType->SetUsedTypes(aliveUsedTypes);
    } else {
      (void)RegisterInPrevs(prev);
    }
    reg->regType->RegisterLivesInfo(pos);
  }

  bool RegisterInPrevs(TypeInferItem *prev) {
    for (auto const e : prevs) {
      if (e == prev) {
        return false;
      }
    }
    prevs.emplace_back(prev);
    return true;
  }

  void InsertUniqueAliveTypes(const TypeInferItem *end, const MapleList<BCRegTypeItem*> *types) {
    if (end != nullptr && end->reg == this->reg && end->reg->regType->IsBefore(this->beginPos, end->beginPos)) {
      return;
    }
    std::vector<TypeInferItem*> workList;
    uint32 index = 0;
    workList.emplace_back(this);
    while (index < workList.size()) {
      auto currItem = workList[index++];
      currItem->isAlive = true;
      for (auto type : *types) {
        bool duplicate = false;
        for (auto const ty : *(currItem->aliveUsedTypes)) {
          if (ty == type) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate) {
          currItem->aliveUsedTypes->emplace_back(type);
          currItem->reg->regType->UpdateFuzzyUsedSet(type);
        }
      }
      // insert into its prevs cyclely
      for (auto const prev : currItem->prevs) {
        if (end != nullptr && end->reg == prev->reg && end->reg->regType->IsBefore(prev->beginPos, end->beginPos)) {
          continue;
        }
        bool exist = false;
        for (uint32 i = 0; i < workList.size(); ++i) {
          if (workList[i] == prev) {
            exist = true;
            break;
          }
        }
        if (!exist) {
          workList.emplace_back(prev);
        }
      }
    }
  }

  void InsertUniqueAliveType(const TypeInferItem *end, BCRegTypeItem *type) {
    if (end != nullptr && end->reg == this->reg && end->reg->regType->IsBefore(this->beginPos, end->beginPos)) {
      return;
    }
    std::vector<TypeInferItem*> workList;
    uint32 index = 0;
    workList.emplace_back(this);
    while (index < workList.size()) {
      auto currItem = workList[index++];
      currItem->isAlive = true;
      bool duplicate = false;
      for (auto const ty : *(currItem->aliveUsedTypes)) {
        if (ty == type) {
          duplicate = true;
          break;
        }
      }
      if (!duplicate) {
        currItem->aliveUsedTypes->emplace_back(type);
        currItem->reg->regType->UpdateFuzzyUsedSet(type);
      }
      // insert into its prevs cyclely
      for (auto prev : std::as_const(currItem->prevs)) {
        if (end != nullptr && end->reg == prev->reg && end->reg->regType->IsBefore(prev->beginPos, end->beginPos)) {
          continue;
        }
        bool exist = false;
        for (uint32 i = 0; i < workList.size(); ++i) {
          if (workList[i] == prev) {
            exist = true;
            break;
          }
        }
        if (!exist) {
          workList.emplace_back(prev);
        }
      }
    }
  }

  uint32 beginPos;
  MapleList<TypeInferItem*> prevs;
  MapleList<BCRegTypeItem*> *aliveUsedTypes = nullptr;
  BCReg *reg = nullptr;
  bool isAlive = false;
};
}  // namespace bc
}  // namespace maple
#endif  // HIR2MPL_BC_INPUT_INCLUDE_BC_INSTRUCTION_H
