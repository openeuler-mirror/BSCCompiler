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

namespace maple {
enum UseType {
  kUseByStmt,
  kUseByPhi,
};

class UseItem final {
 public:
  explicit UseItem(MeStmt *useStmt) : useType(kUseByStmt) {
    useNode.stmt = useStmt;
  }

  explicit UseItem(MePhiNode *phi) : useType(kUseByPhi) {
    useNode.phi = phi;
  }
  ~UseItem() = default;

  BB *GetUseBB() const {
    if (useType == kUseByStmt) {
      return useNode.stmt->GetBB();
    }
    ASSERT(useType == kUseByPhi, "must used in phi");
    return useNode.phi->GetDefBB();
  }

  bool IsUseByPhi() const {
    return useType == kUseByPhi;
  }

  bool IsUseByStmt() const {
    return useType == kUseByStmt;
  }

  MeStmt *GetStmt() const {
    return useNode.stmt;
  }

  MePhiNode *GetPhi() const {
    return useNode.phi;
  }

  bool SameUseItem(const MeStmt *stmt) const {
    return stmt == useNode.stmt;
  }

  bool SameUseItem(const MePhiNode *phi) const {
    return phi == useNode.phi;
  }

  bool operator==(const UseItem &other) const {
    return useType == other.useType && useNode.stmt == other.useNode.stmt;
  }

 private:
  UseType useType;
  union UseSite {
    MeStmt *stmt;
    MePhiNode *phi;
  } useNode;
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
   MeExprUseInfo(MemPool *memPool)
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
  template<class T>
  void RemoveUseSiteOfExpr(const MeExpr *expr, T *useSite);

  void CollectUseInfoInExpr(MeExpr *expr, MeStmt *stmt);
  void CollectUseInfoInStmt(MeStmt *stmt);
  void CollectUseInfoInBB(BB *bb);

  // return true if use sites of scalarA all replaced by scalarB
  bool ReplaceScalar(IRMap *irMap, const ScalarMeExpr *scalarA, ScalarMeExpr *scalarB);

  MapleVector<ExprUseInfoPair> &GetUseSites() {
    return *useSites;
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

 private:
  MapleAllocator allocator;
  MapleVector<ExprUseInfoPair> *useSites; // index is exprId
  MeExprUseInfoState useInfoState;
};
}  // namespace maple
#endif //MAPLE_ME_INCLUDE_MEEXPRUSEINFO_H
