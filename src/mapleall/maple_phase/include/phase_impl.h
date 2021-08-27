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
#ifndef MAPLE_PHASE_INCLUDE_PHASE_IMPL_H
#define MAPLE_PHASE_INCLUDE_PHASE_IMPL_H
#include "class_hierarchy_phase.h"
#include "mir_builder.h"
#include "mpl_scheduler.h"
#include "utils.h"

namespace maple {
class FuncOptimizeImpl : public MplTaskParam {
 public:
  FuncOptimizeImpl(MIRModule &mod, KlassHierarchy *kh = nullptr, bool trace = false);
  virtual ~FuncOptimizeImpl();
  // Each phase needs to implement its own Clone
  virtual FuncOptimizeImpl *Clone() = 0;
  MIRModule &GetMIRModule() {
    return *module;
  }

  const MIRModule &GetMIRModule() const {
    return *module;
  }

  virtual void CreateLocalBuilder(pthread_mutex_t &mtx);
  virtual void ProcessFunc(MIRFunction *func);
  virtual void Finish() {}

 protected:
  void SetCurrentFunction(MIRFunction &func) {
    currFunc = &func;
    ASSERT(builder != nullptr, "builder is null in FuncOptimizeImpl::SetCurrentFunction");
    builder->SetCurrentFunction(func);
    module->SetCurFunction(&func);
  }

  virtual void ProcessBlock(StmtNode &stmt);
  // Each phase needs to implement its own ProcessStmt
  virtual void ProcessStmt(StmtNode&) {}

  KlassHierarchy *klassHierarchy = nullptr;
  MIRFunction *currFunc = nullptr;
  MIRBuilderExt *builder = nullptr;
  bool trace = false;

 private:
  MIRModule *module = nullptr;
};

class FuncOptimizeIterator : public MplScheduler {
 public:
  class Task : public MplTask {
   public:
    explicit Task(MIRFunction &func) : function(&func) {}

    ~Task() = default;

   protected:
    int RunImpl(MplTaskParam *param) override {
      auto &impl = static_cast<FuncOptimizeImpl&>(utils::ToRef(param));
      impl.ProcessFunc(function);
      return 0;
    }

    int FinishImpl(MplTaskParam*) override {
      return 0;
    }

   private:
    MIRFunction *function;
  };

  FuncOptimizeIterator(const std::string &phaseName, std::unique_ptr<FuncOptimizeImpl> phaseImpl);
  virtual ~FuncOptimizeIterator();
  virtual void Run(uint32 threadNum = 1, bool isSeq = false);

 protected:
  thread_local static FuncOptimizeImpl *phaseImplLocal;
  void RunSerial();
  void RunParallel(uint32 threadNum, bool isSeq = false);
  virtual MplTaskParam *CallbackGetTaskRunParam() const {
    return phaseImplLocal;
  }

  virtual MplTaskParam *CallbackGetTaskFinishParam() const {
    return phaseImplLocal;
  }

  virtual void CallbackThreadMainStart() {
    phaseImplLocal = phaseImpl->Clone();
    utils::ToRef(phaseImplLocal).CreateLocalBuilder(mutexGlobal);
  }

  virtual void CallbackThreadMainEnd() {
    delete phaseImplLocal;
    phaseImplLocal = nullptr;
  }

 private:
  bool mplDumpTime = false;
  std::unique_ptr<FuncOptimizeImpl> phaseImpl;
  std::vector<std::unique_ptr<MplTask>> tasksUniquePtr;
};

#define OPT_TEMPLATE(OPT_NAME)                                                                       \
  auto *kh = static_cast<KlassHierarchy*>(mrm->GetAnalysisResult(MoPhase_CHA, mod));                 \
  ASSERT_NOT_NULL(kh);                                                                               \
  std::unique_ptr<FuncOptimizeImpl> funcOptImpl = std::make_unique<OPT_NAME>(*mod, kh, TRACE_PHASE); \
  ASSERT_NOT_NULL(funcOptImpl);                                                                      \
  FuncOptimizeIterator opt(PhaseName(), std::move(funcOptImpl));                                     \
  opt.Init();                                                                                        \
  opt.Run();


#define OPT_TEMPLATE_NEWPM(OPT_NAME, PHASEKEY)                                  \
  auto *kh = GET_ANALYSIS(M2MKlassHierarchy, PHASEKEY); \
  ASSERT_NOT_NULL((kh));                                                                                     \
  std::unique_ptr<FuncOptimizeImpl> funcOptImpl = std::make_unique<OPT_NAME>(m, (kh), TRACE_MAPLE_PHASE);    \
  ASSERT_NOT_NULL(funcOptImpl);                                                                              \
  FuncOptimizeIterator opt(PhaseName(), std::move(funcOptImpl));                                             \
  opt.Init();                                                                                                \
  opt.Run();
}  // namespace maple
#endif  // MAPLE_PHASE_INCLUDE_PHASE_IMPL_H
