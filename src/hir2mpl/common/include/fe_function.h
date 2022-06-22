/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_COMMON_FE_FUNCTION_H
#define HIR2MPL_INCLUDE_COMMON_FE_FUNCTION_H
#include <memory>
#include <vector>
#include <list>
#include "types_def.h"
#include "mempool_allocator.h"
#include "safe_ptr.h"
#include "mir_function.h"
#include "fe_utils.h"
#include "feir_bb.h"
#include "feir_stmt.h"
#include "fe_timer_ns.h"
#include "feir_lower.h"
#include "feir_cfg.h"
#include "fe_function_phase_result.h"
#include "feir_type_infer.h"

namespace maple {
#define SET_FUNC_INFO_PAIR(A, B, C, D)                                                          \
  A.PushbackMIRInfo(MIRInfoPair(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(B), C)); \
  A.PushbackIsString(D)

class FEFunction {
 public:
  FEFunction(MIRFunction &argMIRFunction, const std::unique_ptr<FEFunctionPhaseResult> &argPhaseResultTotal);
  virtual ~FEFunction();

  // element memory manage method
  FEIRStmt *RegisterGeneralStmt(std::unique_ptr<FEIRStmt> stmt);
  const std::unique_ptr<FEIRStmt> &RegisterGeneralStmtUniqueReturn(std::unique_ptr<FEIRStmt> stmt);
  FEIRBB *RegisterFEIRBB(std::unique_ptr<FEIRBB> bb);
  FEIRStmt *RegisterFEIRStmt(UniqueFEIRStmt stmt);
  std::string GetDescription();
  void OutputUseDefChain();
  void OutputDefUseChain();
  void PushStmtScope(const SrcPosition &startOfScope, const SrcPosition &endOfScope, MIRScope *scope = nullptr);
  void PopTopStmtScope();
  MIRScope *GetTopStmtScope();
  MIRScope *GetFunctionScope();
  void AddAliasInMIRScope(MIRScope *scope, const std::string &srcVarName, const MIRSymbol *symbol);
  void SetSrcFileName(const std::string &fileName) {
    srcFileName = fileName;
  }

  const FEIRStmt *GetFEIRStmtHead() const {
    ASSERT_NOT_NULL(feirStmtHead);
    return feirStmtHead;
  }

  const FEIRStmt *GetFEIRStmtTail() const {
    ASSERT_NOT_NULL(feirStmtTail);
    return feirStmtTail;
  }

  void Init() {
    InitImpl();
  }

  void AppendFEIRStmts(std::list<UniqueFEIRStmt> &stmts);
  void InsertFEIRStmtsBefore(FEIRStmt &pos, std::list<UniqueFEIRStmt> &stmts);

  void PreProcess() {
    PreProcessImpl();
  }

  bool Process() {
    return ProcessImpl();
  }

  void Finish() {
    FinishImpl();
  }

  uint32 GetStmtCount() const {
    return stmtCount;
  }

  std::unordered_map<uint32, std::pair<StIdx, StIdx>> &GetBoundaryMap() {
    return boundaryMap;
  }

  const std::unordered_map<uint32, std::pair<StIdx, StIdx>> &GetBoundaryMap() const {
    return boundaryMap;
  }

  void SetBoundaryMap(uint32 tag, const std::pair<StIdx, StIdx> &boundaryVar) {
    boundaryMap[tag] = boundaryVar;
  }

  std::stack<bool> &GetSafeRegionFlag() {
    return safeRegionFlag;
  }

  const std::stack<bool> &GetSafeRegionFlag() const {
    return safeRegionFlag;
  }

 LLT_PROTECTED:
  // run phase routines
  virtual bool GenerateGeneralStmt(const std::string &phaseName) = 0;
  virtual bool LabelGeneralStmts(const std::string &phaseName);
  virtual bool LabelFEIRBBs(const std::string &phaseName);
  virtual bool ProcessFEIRFunction();

  virtual bool GenerateArgVarList(const std::string &phaseName) = 0;
  virtual bool GenerateAliasVars(const std::string &phaseName) = 0;
  virtual bool EmitToFEIRStmt(const std::string &phaseName) = 0;
  virtual bool BuildMapLabelStmt(const std::string &phaseName);
  virtual bool SetupFEIRStmtJavaTry(const std::string &phaseName);
  virtual bool SetupFEIRStmtBranch(const std::string &phaseName);
  virtual bool UpdateRegNum2This(const std::string &phaseName);
  bool LowerFunc(const std::string &phaseName);
  bool DumpFEIRBBs(const std::string &phaseName);
  bool DumpFEIRCFGGraph(const std::string &phaseName);
  bool BuildFEIRDFG(const std::string &phaseName);  // process fe ir check point, build fe ir DFG
  bool BuildFEIRUDDU(const std::string &phaseName);  // build fe ir UD DU chain
  bool TypeInfer(const std::string &phaseName);  // feir based Type Infer

  // finish phase routines
  bool BuildGeneralStmtBBMap(const std::string &phaseName);
  bool UpdateFormal(const std::string &phaseName);
  virtual bool EmitToMIR(const std::string &phaseName);

  // interface methods
  virtual void InitImpl();
  virtual void PreProcessImpl() {}
  virtual bool ProcessImpl() {
    return true;
  }

  virtual void FinishImpl() {}
  virtual bool PreProcessTypeNameIdx() = 0;
  virtual void GenerateGeneralStmtFailCallBack() = 0;
  virtual void GenerateGeneralDebugInfo() = 0;
  virtual bool VerifyGeneral() = 0;
  virtual void VerifyGeneralFailCallBack() = 0;
  virtual void DumpGeneralStmts();
  virtual std::string GetGeneralFuncName() const;
  void EmitToMIRStmt();

  void PhaseTimerStart(FETimerNS &timer);
  void PhaseTimerStopAndDump(FETimerNS &timer, const std::string &label);
  virtual void DumpFEIRCFGGraphForDFGEdge(std::ofstream &file);
  virtual bool HasThis() = 0;
  virtual bool IsNative() = 0;
  void BuildMapLabelIdx();
  bool CheckPhaseResult(const std::string &phaseName);

  FEIRStmt *genStmtHead;
  FEIRStmt *genStmtTail;
  std::list<FEIRStmt*> genStmtListRaw;
  FEIRBB *genBBHead;
  FEIRBB *genBBTail;
  FEIRStmt *feirStmtHead;
  FEIRStmt *feirStmtTail;
  FEIRBB *feirBBHead;
  FEIRBB *feirBBTail;
  std::unique_ptr<FEIRLower> feirLower;
  std::unique_ptr<FEIRCFG> feirCFG;
  std::map<const FEIRStmt*, FEIRBB*> genStmtBBMap;
  std::vector<std::unique_ptr<FEIRVar>> argVarList;
  std::map<uint32, LabelIdx> mapLabelIdx;
  std::map<uint32, FEIRStmtPesudoLabel*> mapLabelStmt;
  FEFunctionPhaseResult phaseResult;
  const std::unique_ptr<FEFunctionPhaseResult> &phaseResultTotal;
  std::string srcFileName = "";
  MIRSrcLang srcLang = kSrcLangJava;
  MIRFunction &mirFunction;

 LLT_PRIVATE:
  void OutputStmts();
  bool SetupFEIRStmtGoto(FEIRStmtGoto &stmt);
  bool SetupFEIRStmtSwitch(FEIRStmtSwitch &stmt);
  const FEIRStmtPesudoLOC *GetLOCForStmt(const FEIRStmt &feStmt) const;
  void AddLocForStmt(const FEIRStmt &feIRStmt, std::list<StmtNode*> &mirStmts) const;
  void LabelFEIRStmts();  // label fe ir stmts
  FEIRBB *NewFEIRBB(uint32 &id);
  bool IsBBEnd(const FEIRStmt &stmt) const;
  bool MayBeBBEnd(const FEIRStmt &stmt) const;
  bool ShouldNewBB(const FEIRBB *currBB, const FEIRStmt &currStmt) const;
  void LinkFallThroughBBAndItsNext(FEIRBB &bb);
  void LinkBranchBBAndItsTargets(FEIRBB &bb);
  void LinkGotoBBAndItsTarget(FEIRBB &bb, const FEIRStmt &stmtTail);
  void LinkSwitchBBAndItsTargets(FEIRBB &bb, const FEIRStmt &stmtTail);
  void LinkBB(FEIRBB &predBB, FEIRBB &succBB);
  FEIRBB &GetFEIRBBByStmt(const FEIRStmt &stmt);
  bool CheckBBsStmtNoAuxTail(const FEIRBB &bb);
  void ProcessCheckPoints();
  void InsertCheckPointForBBs();
  void InsertCheckPointForTrys();
  void InitFirstVisibleStmtForCheckPoints();
  void InitFEIRStmtCheckPointMap();
  void RegisterDFGNodes2CheckPoints();
  void RegisterDFGNodesForFuncParameters();
  void RegisterDFGNodesForStmts();
  bool CalculateDefs4AllUses();
  void InitTrans4AllVars();
  void InsertRetypeStmtsAfterDef(const UniqueFEIRVar& def);
  FEIRStmtPesudoJavaTry2 &GetJavaTryByCheckPoint(FEIRStmtCheckPoint &checkPoint);
  FEIRStmtCheckPoint &GetCheckPointByFEIRStmt(const FEIRStmt &stmt);
  void SetUpDefVarTypeScatterStmtMap();
  FEIRStmt &GetStmtByDefVarTypeScatter(const FEIRVarTypeScatter &varTypeScatter);
  void InsertRetypeStmt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType);
  void InsertCvtStmt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType);
  void InsertJavaMergeStmt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType);
  void InsertDAssignStmt4TypeCvt(const FEIRVarTypeScatter &fromVar, const FEIRType &toType, UniqueFEIRExpr expr);
  bool WithFinalFieldsNeedBarrier(MIRClassType *classType, bool isStatic) const;
  bool IsNeedInsertBarrier();

  std::list<std::unique_ptr<FEIRStmt>> genStmtList;
  std::list<std::unique_ptr<FEIRBB>> genBBList;
  // FeirStmts generated by feirLower are inserted after the kStmtPesudoFuncEnd and linked,
  // Access sequential feirStmt instructions through a doubly linked FELinkListNode from feirStmtHead
  std::list<UniqueFEIRStmt> feirStmtList;
  std::list<std::unique_ptr<FEIRBB>> feirBBList;
  std::map<const FEIRStmt*, FEIRBB*> feirStmtBBMap;
  std::map<const FEIRStmt*, FEIRStmtCheckPoint*> feirStmtCheckPointMap;
  std::map<FEIRStmtCheckPoint*, FEIRStmtPesudoJavaTry2*> checkPointJavaTryMap;
  std::unique_ptr<FEIRTypeInfer> typeInfer;
  uint32 stmtCount = 0;
  std::map<const FEIRVarTypeScatter*, FEIRStmt*> defVarTypeScatterStmtMap;
  std::unordered_map<uint32, std::pair<StIdx, StIdx>> boundaryMap; // EnhanceC boundary var
  std::stack<bool> safeRegionFlag; // EnhanceC saferegion(true: safe, false: unsafe)
  // an element of the stack represents the scope of one compoundstmt or forstmt
  std::stack<MIRScope*> stmtsScopeStack;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FE_FUNCTION_H