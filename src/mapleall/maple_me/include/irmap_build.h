/*
 * Copyright (c) [2020] Huawei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLE_ME_INCLUDE_IRMAP_BUILD_H
#define MAPLE_ME_INCLUDE_IRMAP_BUILD_H
#include "irmap.h"
#include "dominance.h"

namespace maple {

// This class contains methods to convert Maple IR to MeIR.
class IRMapBuild {
 public:
  IRMapBuild(IRMap *hmap, Dominance *dom) : irMap(hmap),
    mirModule(hmap->GetMIRModule()), ssaTab(irMap->GetSSATab()), dominance(*dom), curBB(nullptr) {
    static auto stmtBuildPolicyLoader = InitMeStmtFactory();
    (void)stmtBuildPolicyLoader;
  }
  ~IRMapBuild() {}
  void BuildBB(BB &bb, std::vector<bool> &bbIRMapProcessed);

 private:
  VarMeExpr *GetOrCreateVarFromVerSt(const VersionSt &vst);
  RegMeExpr *GetOrCreateRegFromVerSt(const VersionSt &vst);

  MeExpr *BuildLHSVar(const VersionSt &vst, DassignMeStmt &defMeStmt);
  MeExpr *BuildLHSReg(const VersionSt &vst, RegassignMeStmt &defMeStmt, const RegassignNode &regassign);
  void BuildChiList(MeStmt&, TypeOfMayDefList&, MapleMap<OStIdx, ChiMeNode*>&);
  void BuildMustDefList(MeStmt &meStmt, TypeOfMustDefList&, MapleVector<MustDefMeNode>&);
  void BuildMuList(TypeOfMayUseList&, MapleMap<OStIdx, VarMeExpr*>&);
  void BuildPhiMeNode(BB&);
  void SetMeExprOpnds(MeExpr &meExpr, BaseNode &mirNode);
  static bool InitMeStmtFactory();
  MeStmt *BuildDassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildRegassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildIassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildMaydassignMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildCallMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildNaryMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildRetMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildWithMuMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildGosubMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildThrowMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildSyncMeStmt(StmtNode &stmt, AccessSSANodes &ssaPart);
  MeStmt *BuildMeStmtWithNoSSAPart(StmtNode &stmt);
  MeStmt *BuildMeStmt(StmtNode&);
  MeExpr *BuildExpr(BaseNode&);

 private:
  IRMap *irMap;
  MIRModule &mirModule;
  SSATab &ssaTab;
  Dominance &dominance;
  BB *curBB;  // current mapleme::BB being visited
};

}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_IRMAP_BUILD_H
