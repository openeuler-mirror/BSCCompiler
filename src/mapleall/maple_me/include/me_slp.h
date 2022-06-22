/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_SLP
#define MAPLE_ME_INCLUDE_ME_SLP
#include <optional>
#include "me_function.h"
#include "maple_phase_manager.h"

namespace maple {
MIRType* GenVecType(PrimType sPrimType, uint8 lanes);
MIRType *GetScalarUnsignedTypeBySize(uint32 typeSize);

// ----------------- //
//  Memory Location  //
// ----------------- //
class MemBasePtr {
 public:
  bool IsFromIvar() const {
    return addends != nullptr;
  }

  void PushAddendExpr(MapleAllocator &alloc, const MeExpr &expr, bool isNeg) {
    if (addends == nullptr) {
      addends = alloc.New<MapleVector<int32>>(alloc.Adapter());
    }
    int32 id = expr.GetExprID();
    addends->push_back(isNeg ? -id : id);
  }

  // Get sum of all signed exprId, we generally use the sum as key
  int32 GetHashKey() const {
    if (!IsFromIvar()) {
      return baseOstIdx;
    }
    int32 sum = 0;
    for (auto id : *addends) {
      sum += id;
    }
    return sum;
  }

  // Descending sort
  void Sort() {
    if (addends == nullptr) {
      return;
    }
    std::stable_sort(addends->begin(), addends->end(), std::greater<int32>{});
  }

  // Make sure `addends` is sorted before calling operator==
  bool operator==(const MemBasePtr &rhs) const {
    if (static_cast<uint32>(IsFromIvar()) ^ static_cast<uint32>(rhs.IsFromIvar())) {
      return false;
    }
    if (IsFromIvar()) {
      return *addends == *rhs.addends;
    }
    return baseOstIdx == rhs.baseOstIdx;
  }

  void SetBaseOstIdx(int32 idx) {
    baseOstIdx = idx;
  }

  // Dump example:
  // for ivar: (mx4 mx3 -mx5)
  // for  var: (ost42)
  void Dump() const {
    if (!IsFromIvar()) {
      LogInfo::MapleLogger() << "(ost" << baseOstIdx << ")";
      return;
    }
    LogInfo::MapleLogger() << "(";
    size_t sz = addends->size();
    for (size_t i = 0; i < sz; ++i) {
      int32 id = (*addends)[i];
      if (id >= 0) {
        LogInfo::MapleLogger() << "mx" << id;
      } else {
        LogInfo::MapleLogger() << "-mx" << (-id);
      }
      if (i != sz - 1) {
        LogInfo::MapleLogger() << " ";
      }
    }
    LogInfo::MapleLogger() << ")";
  }
 private:
  // vector of signed exprId, a negative exprId means we should substract this expr
  MapleVector<int32> *addends = nullptr;  // for ivar
  int32 baseOstIdx = 0;                   // for var
};

struct MemBasePtrCmp {
  bool operator()(const MemBasePtr *a, const MemBasePtr *b) const {
    return a->GetHashKey() < b->GetHashKey();
  }
};

// Representation for Memory Location
// Each pointer MeExpr can be divided into two parts:
// (1) base pointer part: the sum of a set of addend expr
// (2) const offset part: a constant offset
// Example:
// For pointer expr `add (add (mx3, sub (mx4, mx5)), 10)`, we get the following MemLoc object:
//   {
//     base = { -mx5, mx3, mx4 },
//     offset = 10
//   }
// A negative expr means we should substract this expr
struct MemLoc {
  MemBasePtr *base = nullptr; // base part
  MIRType *type = nullptr;    // memory type, we can get memory size from the type
  MeExpr *rawExpr = nullptr;  // [for emit] such as ivar
  int32 offset = 0;           // total offset part (in byte)
  int32 extraOffset = 0;      // [for emit] extra offset from places besides rawExpr such as ivar.fieldId, ivar.offset

  void DumpWithoutEndl() const;
  void Dump() const;
  // emit MemLoc to MeExpr
  MeExpr *Emit(IRMap &irMap) const;
};

class MemoryHelper {
 public:
  explicit MemoryHelper(MemPool &memPool) : alloc(&memPool) {}
  // return true if mem1 and mem2 have same base pointer
  static bool HaveSameBase(const MemLoc &mem1, const MemLoc &mem2);
  // return distance (in byte) from the first memLoc to the second memLoc if they have same base pointer,
  // return none otherwise.
  static std::optional<int32> Distance(const MemLoc &from, const MemLoc &to);
  static bool IsConsecutive(const MemLoc &mem1, const MemLoc &mem2, bool mustSameType);
  static bool IsConsecutive(const IvarMeExpr &ivar1, const IvarMeExpr &ivar2, bool mustSameType);
  static bool MustHaveNoOverlap(const MemLoc &mem1, const MemLoc &mem2);

  // Get or create MemLoc object from ivar
  // We can support more overloaded GetMemLoc if needed
  MemLoc *GetMemLoc(IvarMeExpr &ivar);
  MemLoc *GetMemLoc(VarMeExpr &var);
  MemLoc *GetMemLoc(MeExpr &meExpr);  // Only support ivar and var

  bool IsAllIvarConsecutive(const std::vector<MeExpr*> &ivarVec, bool mustSameType);

 private:
  void ExtractAddendOffset(MapleAllocator &alloc, const MeExpr &expr, bool isNeg, MemLoc &memLoc);
  // Each MemBasePtr object must be unique, just like MIRType object.
  // Because we may use the address of MemBasePtr objects as the key of map containers
  void UniqueMemLocBase(MemLoc &memLoc);

  MapleAllocator alloc;  // for allocating MemLoc objects
  std::unordered_map<MeExpr*, MemLoc*> cache;  // for accelerating GetMemLoc
  // To unique repeated MemBasePtr objects because we use their address as key
  std::map<int32, std::vector<MemBasePtr*>> basePtrSet; // key is sum of addends
};

MAPLE_FUNC_PHASE_DECLARE(MESLPVectorizer, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_SLP

