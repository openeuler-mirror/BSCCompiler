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
class Prop; // circular dependency exists, no other choice

// This class contains methods to convert Maple IR to MeIR.
class IRMapBuild {
 public:
  IRMapBuild(IRMap *hmap, Dominance *dom, Prop *prop)
      : irMap(hmap),
        mirModule(hmap->GetMIRModule()),
        ssaTab(irMap->GetSSATab()),
        dominance(*dom),
        curBB(nullptr),
        propagater(prop) {
    InitMeExprBuildFactory();
    InitMeStmtFactory();
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
  void SetMeExprOpnds(MeExpr &meExpr, BaseNode &mirNode, bool atparm, bool noProp);

  OpMeExpr *BuildOpMeExpr(const BaseNode &mirNode) const {
    auto meExpr = new OpMeExpr(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds());
    return meExpr;
  }

  MeExpr *BuildAddrofMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildAddroffuncMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildAddroflabelMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildGCMallocMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildSizeoftypeMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildFieldsDistMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildIvarMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildConstMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildConststrMeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildConststr16MeExpr(const BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForCompare(const BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForTypeCvt(const BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForRetype(const BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForIread(const BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForExtractbits(const BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForJarrayMalloc(const BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForResolveFunc(const BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForArray(const BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForIntrinsicop(const BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForIntrinsicWithType(const BaseNode &mirNode) const;
  MeExpr *BuildExpr(BaseNode&, bool atParm, bool noProp);
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
  Prop *propagater;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_IRMAP_BUILD_H
