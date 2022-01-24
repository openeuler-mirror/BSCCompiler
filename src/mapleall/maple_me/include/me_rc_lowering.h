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
#ifndef MAPLE_ME_INCLUDE_ME_RC_LOWERING_H
#define MAPLE_ME_INCLUDE_ME_RC_LOWERING_H
#include "class_hierarchy_phase.h"
#include "me_function.h"
#include "me_irmap_build.h"
#include "mir_builder.h"
#include "me_cfg.h"

namespace maple {
class RCLowering {
 public:
  RCLowering(MeFunction &f, KlassHierarchy &kh, bool enabledDebug)
      : func(f),
        mirModule(f.GetMIRModule()),
        cfg(f.GetCfg()),
        irMap(*f.GetIRMap()),
        ssaTab(*f.GetMeSSATab()),
        klassHierarchy(kh),
        enabledDebug(enabledDebug) {}

  virtual ~RCLowering() = default;

  void Prepare();
  void PreRCLower();
  void RCLower();
  void PostRCLower();
  void Finish();
  void FastBBLower(BB &bb);
  void SetDominance(Dominance &domin) {
    dominance = &domin;
  }
  bool GetIsAnalyzed() const {
    return isAnalyzed;
  }

 private:
  void MarkLocalRefVar();
  void MarkAllRefOpnds();
  void UpdateRefVarVersions(BB &bb);
  void RecordVarPhiVersions(std::map<OStIdx, size_t> &savedStackSize, const BB &bb);
  void TraverseAllStmts(BB &bb);
  void RestoreVersionRecords(std::map<OStIdx, size_t> &savedStackSize);
  void UnmarkNotNeedDecRefOpnds();
  void EpreFixup(BB &bb);
  void BBLower(BB &bb);
  void CreateCleanupIntrinsics();
  void HandleArguments();
  void CompactRC(BB &bb);
  void CompactIncAndDec(MeStmt &incStmt, MeStmt &decStmt);
  void CompactIncAndDecReset(MeStmt &incStmt, MeStmt &resetStmt);
  void ReplaceDecResetWithDec(MeStmt &prevStmt, MeStmt &stmt);
  void CompactAdjacentDecReset(MeStmt &prevStmt, MeStmt &stmt);
  // create new symbol from name and return its ost
  OriginalSt *RetrieveOSt(const std::string &name, bool isLocalrefvar) const;
  // create new symbol from temp name and return its VarMeExpr
  // new symbols are stored in a set
  VarMeExpr *CreateNewTmpVarMeExpr(bool isLocalrefvar);
  VarMeExpr *CreateVarMeExprFromSym(MIRSymbol &sym) const;
  // return true if the rhs is simple so we can adjust RC count before assignments
  bool RCFirst(MeExpr &rhs);
  IntrinsiccallMeStmt *GetVarRHSHandleStmt(const MeStmt &stmt);
  IntrinsiccallMeStmt *GetIvarRHSHandleStmt(const MeStmt &stmt);
  MIRIntrinsicID PrepareVolatileCall(const MeStmt &stmt, MIRIntrinsicID index = INTRN_UNDEFINED);
  IntrinsiccallMeStmt *CreateRCIntrinsic(MIRIntrinsicID intrnID, const MeStmt &stmt, std::vector<MeExpr*> &opnds,
                                         bool assigned = false);
  MeExpr *HandleIncRefAndDecRefStmt(MeStmt &stmt);
  void InitializedObjectFields(MeStmt &stmt);
  bool IsInitialized(IvarMeExpr &ivar);
  void PreprocessAssignMeStmt(MeStmt &stmt);
  void HandleAssignMeStmtRHS(MeStmt &stmt);
  void HandleAssignMeStmtRegLHS(const MeStmt &stmt);
  void HandleAssignToGlobalVar(MeStmt &stmt);
  void HandleAssignToLocalVar(const MeStmt &stmt, MeExpr *pendingDec);
  void HandleAssignMeStmtVarLHS(MeStmt &stmt, MeExpr *pendingDec);
  void HandleAssignMeStmtIvarLHS(MeStmt &stmt);
  void HandleCallAssignedMeStmt(MeStmt &stmt, MeExpr *pendingDec);
  void IntroduceRegRetIntoCallAssigned(MeStmt &stmt);
  void HandleRetOfCallAssignedMeStmt(MeStmt &stmt, MeExpr &pendingDec);
  void HandleReturnVar(RetMeStmt &ret);
  void HandleReturnGlobal(RetMeStmt &ret);
  void HandleReturnRegread(const RetMeStmt &ret);
  void HandleReturnFormal(RetMeStmt &ret);
  void HandleReturnIvar(RetMeStmt &ret);
  void HandleReturnReg(RetMeStmt &ret);
  void HandleReturnWithCleanup();
  void HandleReturnNeedBackup();
  void HandleReturnStmt();
  void HandleAssignMeStmt(MeStmt &stmt, MeExpr *pendingDec);
  void HandlePerManent(MeStmt &stmt);
  bool HasCallOrBranch(const MeStmt &from, const MeStmt &to);
  MIRIntrinsicID SelectWriteBarrier(const MeStmt &stmt);
  MIRType *GetArrayNodeType(const VarMeExpr &var);
  void CheckArrayStore(IntrinsiccallMeStmt &writeRefCall);
  void FastLowerThrowStmt(MeStmt &stmt, MapleMap<uint32, MeStmt*> &exceptionAllocsites);
  void FastLowerRetStmt(MeStmt &stmt);
  void FastLowerRetVar(RetMeStmt &stmt);
  void FastLowerRetIvar(RetMeStmt &stmt);
  void FastLowerRetReg(const RetMeStmt &stmt);
  void FastLowerAssignToVar(MeStmt &stmt, MapleMap<uint32, MeStmt*> &exceptionAllocsites);
  void FastLowerAssignToIvar(MeStmt &stmt);
  void FastLowerCallAssignedStmt(MeStmt &stmt);
  void CheckRefs();
  void ParseCheckFlag();
  void CheckFormals();
  void CheckRefsInAssignStmt(BB &bb);
  void CheckRefReturn(BB &bb);
  MeFunction &func;
  MIRModule &mirModule;
  MeCFG *cfg;
  IRMap &irMap;
  SSATab &ssaTab;
  KlassHierarchy &klassHierarchy;
  std::vector<MeStmt*> rets{};  // std::vector of return statement
  unsigned int tmpCount = 0;
  uint32_t checkRCIndex = 0;
  bool needSpecialHandleException = false;
  bool isAnalyzed = false;
  bool rcCheckReferent = false;
  Dominance *dominance = nullptr;
  std::set<const MIRSymbol*> assignedPtrSym;
  std::set<VarMeExpr*> tmpLocalRefVars;
  std::set<MeExpr*> gcMallocObjects{};
  std::map<OStIdx, VarMeExpr*> cleanUpVars{};
  std::map<OStIdx, OriginalSt*> varOStMap{};
  // used to store initialized map, help to optimize dec ref in first assignment
  std::unordered_map<MeExpr*, std::set<FieldID>> initializedFields{};
  std::map<MeStmt*, MeExpr*> decOpnds{};
  std::map<OStIdx, std::vector<MeExpr*>> varVersions{};
  bool checkRefFormal = false;
  bool checkRefAssign = false;
  bool checkRefReturn = false;
  bool enabledDebug;
};

MAPLE_FUNC_PHASE_DECLARE(MERCLowering, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_RC_LOWERING_H
