/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "phase_impl.h"
#include "mpl_timer.h"

namespace maple {
// ========== FuncOptimizeImpl ==========
FuncOptimizeImpl::FuncOptimizeImpl(MIRModule &mod, KlassHierarchy *kh, bool currTrace)
    : klassHierarchy(kh), trace(currTrace), module(&mod) {
  builder = new (std::nothrow) MIRBuilderExt(module);
  CHECK_FATAL(builder != nullptr, "New fail in FuncOptimizeImpl ctor!");
}

FuncOptimizeImpl::~FuncOptimizeImpl() {
  if (builder != nullptr) {
    delete builder;
    builder = nullptr;
  }
  klassHierarchy = nullptr;
  currFunc = nullptr;
  currBlock = nullptr;
  module = nullptr;
}

void FuncOptimizeImpl::CreateLocalBuilder(pthread_mutex_t &mtx) {
  // Each thread needs to use its own MIRBuilderExt.
  builder = new (std::nothrow) MIRBuilderExt(module, &mtx);
  CHECK_FATAL(builder != nullptr, "New fail in CreateLocalBuilder ctor!");
}

void FuncOptimizeImpl::ProcessFunc(MIRFunction *func) {
  currFunc = func;
  SetCurrentFunction(*func);
  if (func->GetBody() != nullptr) {
    ProcessBlock(*func->GetBody());
  }
}

void FuncOptimizeImpl::ProcessBlock(StmtNode &stmt) {
  switch (stmt.GetOpCode()) {
    case OP_if: {
      ProcessStmt(stmt);
      IfStmtNode &ifStmtNode = static_cast<IfStmtNode&>(stmt);
      if (ifStmtNode.GetThenPart() != nullptr) {
        ProcessBlock(*ifStmtNode.GetThenPart());
      }
      if (ifStmtNode.GetElsePart() != nullptr) {
        ProcessBlock(*ifStmtNode.GetElsePart());
      }
      break;
    }
    case OP_while:
    case OP_dowhile: {
      ProcessStmt(stmt);
      WhileStmtNode &whileStmtNode = static_cast<WhileStmtNode&>(stmt);
      if (whileStmtNode.GetBody() != nullptr) {
        ProcessBlock(*whileStmtNode.GetBody());
      }
      break;
    }
    case OP_block: {
      BlockNode &block = static_cast<BlockNode&>(stmt);
      for (StmtNode *stmtNode = block.GetFirst(), *next = nullptr; stmtNode != nullptr; stmtNode = next) {
        SetCurrentBlock(block);
        ProcessBlock(*stmtNode);
        next = stmtNode->GetNext();
      }
      break;
    }
    default: {
      ProcessStmt(stmt);
      break;
    }
  }
}

// ========== FuncOptimizeIterator ==========
thread_local FuncOptimizeImpl *FuncOptimizeIterator::phaseImplLocal = nullptr;

FuncOptimizeIterator::FuncOptimizeIterator(const std::string &phaseName, std::unique_ptr<FuncOptimizeImpl> phaseImpl)
    : MplScheduler(phaseName), phaseImpl(std::move(phaseImpl)) {
  char *envStr = getenv("MP_DUMPTIME");
  mplDumpTime = (envStr != nullptr && std::stoi(envStr) == 1);
}

FuncOptimizeIterator::~FuncOptimizeIterator() = default;

void FuncOptimizeIterator::Run(uint32 threadNum, bool isSeq) {
  if (threadNum == 1) {
    RunSerial();
  } else {
    RunParallel(threadNum, isSeq);
  }
}

void FuncOptimizeIterator::RunSerial() {
  MPLTimer timer;
  if (mplDumpTime) {
    timer.Start();
  }

  CHECK_NULL_FATAL(phaseImpl);
  for (MIRFunction *func : std::as_const(phaseImpl->GetMIRModule().GetFunctionList())) {
    auto dumpPhase = [this, func](bool dumpOption, std::string &&s) {
      if (dumpOption) {
        LogInfo::MapleLogger() << ">>>>> Dump " << s << schedulerName << " <<<<<\n";
        func->Dump();
        LogInfo::MapleLogger() << ">>>>> Dump " << s << schedulerName << " end <<<<<\n";
      }
    };
    bool dumpFunc = (Options::dumpFunc == "*" || Options::dumpFunc == func->GetName()) &&
                    (Options::dumpPhase == "*" || Options::dumpPhase == schedulerName);
    dumpPhase(Options::dumpBefore && dumpFunc, "before ");
    phaseImpl->SetDump(dumpFunc);
    phaseImpl->ProcessFunc(func);
    dumpPhase(Options::dumpAfter && dumpFunc, "after ");
  }

  phaseImpl->Finish();

  if (mplDumpTime) {
    timer.Stop();
    INFO(kLncInfo, "FuncOptimizeIterator::RunSerial (%s): %lf ms", schedulerName.c_str(),
         timer.ElapsedMicroseconds() / 1000.0);
  }
}

void FuncOptimizeIterator::RunParallel(uint32 threadNum, bool isSeq) {
  MPLTimer timer;
  if (mplDumpTime) {
    timer.Start();
  }
  Reset();

  CHECK_NULL_FATAL(phaseImpl);
  for (MIRFunction *func : std::as_const(phaseImpl->GetMIRModule().GetFunctionList())) {
    std::unique_ptr<Task> task = std::make_unique<Task>(*func);
    ASSERT_NOT_NULL(task);
    AddTask(*task.get());
    tasksUniquePtr.emplace_back(std::move(task));
  }

  if (mplDumpTime) {
    timer.Stop();
    INFO(kLncInfo, "FuncOptimizeIterator::RunParallel (%s): AddTask() = %lf ms", schedulerName.c_str(),
         timer.ElapsedMicroseconds() / 1000.0);

    timer.Start();
  }

  int ret = RunTask(threadNum, isSeq);
  CHECK_FATAL(ret == 0, "RunTask failed");
  phaseImpl->Finish();
  Reset();

  if (mplDumpTime) {
    timer.Stop();
    INFO(kLncInfo, "FuncOptimizeIterator::RunParallel (%s): RunTask() = %lf ms", schedulerName.c_str(),
         timer.ElapsedMicroseconds() / 1000.0);
  }
}
}  // namespace maple
