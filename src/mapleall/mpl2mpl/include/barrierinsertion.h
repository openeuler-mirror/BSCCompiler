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
#ifndef MPL2MPL_INCLUDE_BARRIERINSERTION_H
#define MPL2MPL_INCLUDE_BARRIERINSERTION_H

namespace maple {
#ifdef NOT_USED
class BarrierInsertion : public ModulePhase {
 public:
  explicit BarrierInsertion(ModulePhaseID id) : ModulePhase(id) {}

  ~BarrierInsertion() = default;

  AnalysisResult *Run(MIRModule *module, ModuleResultMgr *m) override;
  std::string PhaseName() const override {
    return "barrierinsertion";
  }

 private:
  class RunFunction;
  friend RunFunction;
  class RunFunction {
    friend BarrierInsertion;
    RunFunction(BarrierInsertion &phase, MIRModule &module, MIRFunction &func)
        : phase(&phase), mirModule(&module), mirFunc(&func), builder(module.GetMIRBuilder()), backupVarCount(0) {}

    ~RunFunction() = default;

    CallNode *CreateWriteRefVarCall(BaseNode &var, BaseNode &value) const;
    CallNode *CreateWriteRefFieldCall(BaseNode &obj, BaseNode &field, BaseNode &value) const;
    CallNode *CreateReleaseRefVarCall(BaseNode &var) const;
    // MCC_CheckObjMem(address_t obj)
    CallNode *CreateMemCheckCall(BaseNode &obj) const;
    void HandleBlock(BlockNode &block) const;
    StmtNode *HandleStmt(StmtNode &stmt, BlockNode &block) const;
    StmtNode *CheckRefRead(BaseNode *opnd, const StmtNode &stmt, BlockNode &block) const;
    void InsertPrologue();
    void HandleReturn(const NaryStmtNode &retNode);
    MIRSymbol *NewBackupVariable(const std::string &suffix);
    void Run();
    std::string PhaseName() const {
      return phase->PhaseName();
    }

    bool SkipRHS(const BaseNode &rhs) const;
    BarrierInsertion *phase;
    MIRModule *mirModule;
    MIRFunction *mirFunc;
    MIRBuilder *builder;
    int backupVarCount;
    std::set<StIdx> backupVarIndices;
    std::set<StIdx> assignedParams;
    std::set<NaryStmtNode*> rets;  // there could be multiple return in the function
  };

  void EnsureLibraryFunction(MIRModule &module, const char &name, const MIRType &retType, const ArgVector &args);
  void EnsureLibraryFunctions(MIRModule &module);
};
#endif
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_BARRIERINSERTION_H
