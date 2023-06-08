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

#ifndef MAPLE_ME_INCLUDE_MEEXPRUSEINFO_H
#define MAPLE_ME_INCLUDE_MEEXPRUSEINFO_H
#include "me_ir.h"
#include "me_function.h"

#include <variant>

namespace maple {

class UseItem final {
 public:
  explicit UseItem(MeStmt *useStmt) : useNode(useStmt) {}
  explicit UseItem(MePhiNode *phi) : useNode(phi) {}

  const BB *GetUseBB() const {
    return std::visit(
        [this](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, MeStmt *>) {
            return Get<MeStmt>()->GetBB();
          } else {
            return Get<MePhiNode>()->GetDefBB();
          }
        },
        useNode);
  }

  bool IsUseByPhi() const {
    return std::holds_alternative<MePhiNode *>(useNode);
  }

  bool IsUseByStmt() const { return std::holds_alternative<MeStmt *>(useNode); }

  const MeStmt *GetStmt() const { return Get<MeStmt>(); }
  MeStmt *GetStmt() { return Get<MeStmt>(); }

  const MePhiNode *GetPhi() const { return Get<MePhiNode>(); }
  MePhiNode *GetPhi() { return Get<MePhiNode>(); }

  uint32 GetRef() const {
    return ref;
  }

  void IncreaseRef() {
    ref++;
  }

  bool operator==(const UseItem &other) const {
    return useNode == other.useNode;
  }
  bool operator!=(const UseItem &other) const { return !(*this == other); }

 private:
  template <typename T> const T *Get() const {
    assert(std::holds_alternative<T *>(useNode) && "Invalid use type.");
    return std::get<T *>(useNode);
  }
  template <typename T> T *Get() {
    assert(std::holds_alternative<T *>(useNode) && "Invalid use type.");
    return std::get<T *>(useNode);
  }

  std::variant<MeStmt *, MePhiNode *> useNode;
  uint32 ref = 1;
};

enum MeExprUseInfoState {
  kUseInfoInvalid = 0,
  kUseInfoOfScalar = 1,
  kUseInfoOfAllExpr = 3,
};

using UseSitesType = MapleList<UseItem>;
using ExprUseInfoPair = std::pair<MeExpr*, UseSitesType*>;
class MeExprUseInfo final {
 public:
  explicit MeExprUseInfo(MemPool *memPool)
      : allocator(memPool),
        useSites(nullptr),
        useInfoState(kUseInfoInvalid) {}

  ~MeExprUseInfo() {}
  void CollectUseInfoInFunc(IRMap *irMap, Dominance *domTree, MeExprUseInfoState state);
  UseSitesType *GetUseSitesOfExpr(const MeExpr *expr) const;
  const MapleVector<ExprUseInfoPair> &GetUseSites() const {
    return *useSites;
  }

  template<class T>
  void AddUseSiteOfExpr(MeExpr *expr, T *useSite);

  void CollectUseInfoInExpr(MeExpr *expr, MeStmt *stmt);
  void CollectUseInfoInStmt(MeStmt *stmt);
  void CollectUseInfoInBB(BB *bb);

  // return true if use sites of scalarA all replaced by scalarB
  bool ReplaceScalar(IRMap *irMap, const ScalarMeExpr *scalarA, ScalarMeExpr *scalarB);

  MapleVector<ExprUseInfoPair> &GetUseSites() {
    return *useSites;
  }

  void SetUseSites(MapleVector<ExprUseInfoPair> *sites) {
    useSites = sites;
  }

  void SetState(MeExprUseInfoState state) {
    useInfoState = state;
  }

  void InvalidUseInfo() {
    useInfoState = kUseInfoInvalid;
  }

  bool IsInvalid() const {
    return useInfoState == kUseInfoInvalid;
  }

  bool UseInfoOfScalarIsValid() const {
    return useInfoState >= kUseInfoOfScalar;
  }

  bool UseInfoOfAllIsValid() const {
    return useInfoState == kUseInfoOfAllExpr;
  }

 private:
  MapleAllocator allocator;
  MapleVector<ExprUseInfoPair> *useSites; // index is exprId
  MeExprUseInfoState useInfoState;
};
}  // namespace maple
#endif //MAPLE_ME_INCLUDE_MEEXPRUSEINFO_H
