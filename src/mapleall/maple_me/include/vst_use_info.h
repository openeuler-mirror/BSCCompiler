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

#ifndef MAPLE_ME_INCLUDE_VST_USE_INFO_H
#define MAPLE_ME_INCLUDE_VST_USE_INFO_H
#include <variant>
#include "mir_nodes.h"
#include "ssa.h"
#include "bb.h"
#include "me_function.h"
#include "dominance.h"

namespace maple {
class VstUseItem final {
 public:
  explicit VstUseItem(StmtNode *useStmt) : useNode(useStmt) {}
  explicit VstUseItem(PhiNode *usePhi) : useNode(usePhi) {}
  explicit VstUseItem(MayUseNode *mayUse) : useNode(mayUse) {}

  bool IsUsedByPhiNode() const {
    return std::holds_alternative<PhiNode *>(useNode);
  }
  bool IsUsedByStmt() const {
    return std::holds_alternative<StmtNode *>(useNode);
  }
  bool IsUsedByMayUseNode() const {
    return std::holds_alternative<MayUseNode *>(useNode);
  }

  const StmtNode *GetStmt() const {
    return Get<StmtNode>();
  }

  StmtNode *GetStmt() {
    return Get<StmtNode>();
  }

  const PhiNode *GetPhi() const {
    return Get<PhiNode>();
  }

  PhiNode *GetPhi() {
    return Get<PhiNode>();
  }

  const MayUseNode *GetMayUse() const {
    return Get<MayUseNode>();
  }

  MayUseNode *GetMayuse() {
    return Get<MayUseNode>();
  }

  bool operator==(const VstUseItem &other) const {
    return useNode == other.useNode;
  }
  bool operator!=(const VstUseItem &other) const {
    return !(*this == other);
  }

  template<typename T> const T *Get() const {
    ASSERT(std::holds_alternative<T *>(useNode), "Vst use site type error!");
    return std::get<T *>(useNode);
  }
  template<typename T> T *Get() {
    ASSERT(std::holds_alternative<T *>(useNode), "Vst use site type error!");
    return std::get<T *>(useNode);
  }

 private:
  std::variant<StmtNode *, PhiNode *, MayUseNode *> useNode;
};

using VstUseSiteList = MapleList<VstUseItem>;

enum VstUnseInfoState {
  kVstUseInfoInvalid = 0,
  kVstUseInfoTopLevelVst = 1,
  kVstUseInfoAddrTakenVst = 2
};

class VstUseInfo final {
 public:
  explicit VstUseInfo(MemPool *mp) : allocator(mp) {}

  ~VstUseInfo() = default;

  bool IsUseInfoOfTopLevelValid() const {
    return useInfoState & kVstUseInfoTopLevelVst;
  }
  bool IsUseInfoInvalid() const {
    return useInfoState == kVstUseInfoInvalid;
  }

  MapleVector<VstUseSiteList*> &GetAllUseSites() {
    return *useSites;
  }

  VstUseSiteList *GetUseSitesOf(const VersionSt &vst) {
    return (*useSites)[vst.GetIndex()];
  }

  template<typename T>
  void AddUseSiteOfVst(const VersionSt *vst, T *useSite);

  template<typename T>
  void RemoveUseSiteOfVst(const VersionSt *vst, T *useSite);

  template<typename T>
  void UpdateUseSiteOfVst(const VersionSt *vst, T *oldUseSite, T *newUseSite);

  void CollectUseInfoInBB(BB *bb);
  void CollectUseInfoInStmt(StmtNode *stmt);
  void CollectUseInfoInFunc(MeFunction *f, Dominance *dom, VstUnseInfoState state = kVstUseInfoTopLevelVst);

 private:
  void CollectUseInfoInExpr(BaseNode *expr, StmtNode *stmt);

  MapleAllocator allocator;
  SSATab *ssaTab = nullptr;
  MapleVector<VstUseSiteList*> *useSites = nullptr; // index is vstIdx
  VstUnseInfoState useInfoState = kVstUseInfoInvalid;
};

template<typename T>
void VstUseInfo::AddUseSiteOfVst(const VersionSt *vst, T *useSite) {
  if (vst == nullptr) {
    return;
  }
  if (useInfoState == kVstUseInfoTopLevelVst) {
    if (!IsLocalTopLevelOst(*vst->GetOst())) {
      return;
    }
  }
  size_t vstIdx = vst->GetIndex();
  ASSERT(vstIdx < useSites->size(), "VersionSt out of range!");
  if ((*useSites)[vstIdx] == nullptr) { // first use site
    (*useSites)[vstIdx] = allocator.New<MapleList<VstUseItem>>(allocator.Adapter());
  }
  VstUseItem use(useSite);
  if ((*useSites)[vstIdx]->front() != use) {
    (*useSites)[vstIdx]->emplace_front(use);
  }
}

template<typename T>
void VstUseInfo::RemoveUseSiteOfVst(const VersionSt *vst, T *useSite) {
  if (vst == nullptr) {
    return;
  }
  size_t vstIdx = vst->GetIndex();
  if (vstIdx >= useSites->size()) {
    return;
  }
  auto *useList = (*useSites)[vstIdx];
  if (useList == nullptr) {
    return;
  }
  for (auto it = useList->begin(); it != useList->end(); ++it) {
    if ((*it) == VstUseItem(useSite)) {
      useList->erase(it);
      return;
    }
  }
}

template<typename T>
void VstUseInfo::UpdateUseSiteOfVst(const VersionSt *vst, T *oldUseSite, T *newUseSite) {
  RemoveUseSiteOfVst(vst, oldUseSite);
  AddUseSiteOfVst(vst, newUseSite);
}
} // namespace maple
#endif // MAPLE_ME_INCLUDE_VST_USE_INFO_H
