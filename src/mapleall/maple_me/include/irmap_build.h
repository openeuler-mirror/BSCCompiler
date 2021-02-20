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

#ifndef MAPLE_ME_INCLUDE_IRMAP_BUILD_H
#define MAPLE_ME_INCLUDE_IRMAP_BUILD_H
#include "irmap.h"
#include "dominance.h"

namespace maple {
// This class contains methods to convert Maple IR to MeIR.
class IRMapBuild {
 public:
  IRMapBuild(IRMap *hmap, Dominance *dom)
      : irMap(hmap),
        mirModule(hmap->GetMIRModule()),
        ssaTab(irMap->GetSSATab()),
        dominance(*dom),
        curBB(nullptr) {
    InitMeExprBuildFactory();
    InitMeStmtFactory();
  }
  ~IRMapBuild() {}
  void BuildBB(BB &bb, std::vector<bool> &bbIRMapProcessed);

 private:
  VarMeExpr *GetOrCreateVarFromVerSt(VersionSt &vst);
  RegMeExpr *GetOrCreateRegFromVerSt(VersionSt &vst);

  MeExpr *BuildLHSVar(VersionSt &vst, DassignMeStmt &defMeStmt);
  MeExpr *BuildLHSReg(VersionSt &vst, RegassignMeStmt &defMeStmt, const RegassignNode &regassign);
  void BuildChiList(MeStmt&, TypeOfMayDefList&, MapleMap<OStIdx, ChiMeNode*>&);
  void BuildMustDefList(MeStmt &meStmt, TypeOfMustDefList&, MapleVector<MustDefMeNode>&);
  void BuildMuList(TypeOfMayUseList&, MapleMap<OStIdx, VarMeExpr*>&);
  void BuildPhiMeNode(BB&);
  void SetMeExprOpnds(MeExpr &meExpr, BaseNode &mirNode);

  OpMeExpr *BuildOpMeExpr(BaseNode &mirNode) const {
    OpMeExpr *meExpr = new OpMeExpr(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds());
    return meExpr;
  }
  MeExpr *BuildAddrofMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildAddroffuncMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildAddroflabelMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildGCMallocMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildSizeoftypeMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildFieldsDistMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildIvarMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildConstMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildConststrMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildConststr16MeExpr(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForCompare(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForTypeCvt(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForRetype(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForIread(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForExtractbits(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForJarrayMalloc(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForResolveFunc(BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForArray(BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForIntrinsicop(BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForIntrinsicWithType(BaseNode &mirNode) const;
  MeExpr *BuildExpr(BaseNode&);
  static void InitMeExprBuildFactory();

  MeStmt *BuildMeStmtWithNoSSAPart(StmtNode &stmt);
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
  MeStmt *BuildMeStmt(StmtNode&);
  static void InitMeStmtFactory();

  IRMap *irMap;
  MIRModule &mirModule;
  SSATab &ssaTab;
  Dominance &dominance;
  BB *curBB;  // current mapleme::BB being visited
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_IRMAP_BUILD_H
