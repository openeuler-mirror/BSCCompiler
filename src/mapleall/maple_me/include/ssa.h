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
#ifndef MAPLE_ME_INCLUDE_SSA_H
#define MAPLE_ME_INCLUDE_SSA_H
#include <iostream>
#include "mir_module.h"
#include "mir_nodes.h"
#include "orig_symbol.h"

namespace maple {
class BB;  // circular dependency exists, no other choice
class MeCFG;  // circular dependency exists, no other choice
class VersionSt;  // circular dependency exists, no other choice
class OriginalStTable;  // circular dependency exists, no other choice
class VersionStTable;  // circular dependency exists, no other choice
class SSATab;  // circular dependency exists, no other choice
class Dominance;  // circular dependency exists, no other choice
bool IsLocalTopLevelOst(const OriginalSt &ost);
class PhiNode {
 public:
  PhiNode(MapleAllocator &alloc, VersionSt &vsym) : result(&vsym), phiOpnds(kNumOpnds, nullptr, alloc.Adapter()) {
    phiOpnds.pop_back();
    phiOpnds.pop_back();
  }

  ~PhiNode() = default;

  void Dump();

  VersionSt *GetResult() {
    return result;
  }
  const VersionSt *GetResult() const {
    return result;
  }

  void SetResult(VersionSt &resultPara) {
    result = &resultPara;
  }

  MapleVector<VersionSt*> &GetPhiOpnds() {
    return phiOpnds;
  }
  const MapleVector<VersionSt*> &GetPhiOpnds() const {
    return phiOpnds;
  }

  VersionSt *GetPhiOpnd(size_t index) {
    ASSERT(index < phiOpnds.size(), "out of range in PhiNode::GetPhiOpnd");
    return phiOpnds.at(index);
  }
  const VersionSt *GetPhiOpnd(size_t index) const {
    ASSERT(index < phiOpnds.size(), "out of range in PhiNode::GetPhiOpnd");
    return phiOpnds.at(index);
  }

  void SetPhiOpnd(size_t index, VersionSt &opnd) {
    ASSERT(index < phiOpnds.size(), "out of range in PhiNode::SetPhiOpnd");
    phiOpnds[index] = &opnd;
  }

  void SetPhiOpnds(const MapleVector<VersionSt*> &phiOpndsPara) {
    phiOpnds = phiOpndsPara;
  }

 private:
  VersionSt *result;
  static constexpr uint32 kNumOpnds = 2;
  MapleVector<VersionSt*> phiOpnds;
};

class SSA {
 public:
  SSA(SSATab &stab, MapleVector<BB *> &bbvec, Dominance *dm, SSALevel level = kSSAInvalid)
      : ssaTab(&stab),
        bbVec(bbvec),
        dom(dm),
        targetLevel(level) {}

  virtual ~SSA() = default;

  virtual void InsertPhiNode();
  void RenameAllBBs(MeCFG *cfg);

  void UpdateDom(Dominance *dm) {
    dom = dm;
  }
 protected:
  void InsertPhiForDefBB(size_t bbid, VersionSt *vst);
  void InitRenameStack(const OriginalStTable&, const VersionStTable&, MapleAllocator &renameAlloc);
  VersionSt *CreateNewVersion(VersionSt &vSym, BB &defBB);
  void PushToRenameStack(VersionSt *vSym);
  void RenamePhi(BB &bb);
  void RenameDefs(StmtNode &stmt, BB &defBB);
  void RenameMustDefs(const StmtNode &stmt, BB &defBB);
  void RenameExpr(BaseNode &expr);
  void RenameUses(const StmtNode &stmt);
  void RenamePhiUseInSucc(const BB &bb);
  void RenameMayUses(const BaseNode &node);
  void RenameBB(BB &bb);

  const MapleVector<MapleStack<VersionSt*>*> &GetVstStacks() const {
    return *vstStacks;
  }

  const MapleStack<VersionSt*> *GetVstStack(size_t idx) const {
    ASSERT(idx < vstStacks->size(), "out of range of vstStacks");
    return vstStacks->at(idx);
  }
  void PopVersionSt(size_t idx) {
    vstStacks->at(idx)->pop();
  }

  SSATab *GetSSATab() {
    return ssaTab;
  }

  bool BuildSSATopLevel() const {
    return targetLevel & kSSATopLevel;
  }
  bool BuildSSAAddrTaken() const {
    return targetLevel & kSSAAddrTaken;
  }
  bool BuildSSAAllLevel() const {
    return BuildSSAAddrTaken() && BuildSSATopLevel();
  }
  // Check if ost should be processed according to target ssa level set before
  bool ShouldProcessOst(const OriginalSt &ost) const;

  bool ShouldRenameVst(const VersionSt *vst) const;

  MapleVector<MapleStack<VersionSt*>*> *vstStacks = nullptr;    // rename stack for variable versions
  SSATab *ssaTab = nullptr;
  MapleVector<BB *> &bbVec;
  Dominance *dom = nullptr;
  SSALevel targetLevel = kSSAInvalid;                // ssa level to build

 public:
  bool runRenameOnly = false;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SSA_H
