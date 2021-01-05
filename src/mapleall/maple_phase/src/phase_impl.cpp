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
#include <cstdlib>
#include "mpl_timer.h"

namespace maple {
// ========== FuncOptimizeImpl ==========
FuncOptimizeImpl::FuncOptimizeImpl(MIRModule &mod, KlassHierarchy *kh, bool currTrace)
    : klassHierarchy(kh), trace(currTrace), module(&mod) {
  builder = module->GetMIRBuilder();
}

FuncOptimizeImpl::~FuncOptimizeImpl() {
  klassHierarchy = nullptr;
  currFunc = nullptr;
  module = nullptr;
}


void FuncOptimizeImpl::ProcessFunc(MIRFunction *func) {
  currFunc = func;
  builder->SetCurrentFunction(*func);
  if (func->GetBody() != nullptr) {
    ProcessBlock(*func->GetBody());
  }
}

void FuncOptimizeImpl::ProcessBlock(StmtNode &stmt) {
  switch (stmt.GetOpCode()) {
    case OP_if: {
      IfStmtNode &ifStmtNode = static_cast<IfStmtNode&>(stmt);
      if (ifStmtNode.GetThenPart() != nullptr) {
        ProcessBlock(*ifStmtNode.GetThenPart());
      }
      break;
    }
    case OP_while:
    case OP_dowhile: {
      WhileStmtNode &whileStmtNode = static_cast<WhileStmtNode&>(stmt);
      if (whileStmtNode.GetBody() != nullptr) {
        ProcessBlock(*whileStmtNode.GetBody());
      }
      break;
    }
    case OP_block: {
      BlockNode &block = static_cast<BlockNode&>(stmt);
      for (StmtNode *stmtNode = block.GetFirst(), *next = nullptr; stmtNode != nullptr; stmtNode = next) {
        next = stmtNode->GetNext();
        ProcessBlock(*stmtNode);
      }
      break;
    }
    default: {
      ProcessStmt(stmt);
      break;
    }
  }
}

FuncOptimizeIterator::FuncOptimizeIterator(const std::string &phaseName, std::unique_ptr<FuncOptimizeImpl> phaseImpl)
    : phaseImpl(std::move(phaseImpl)) {}

FuncOptimizeIterator::~FuncOptimizeIterator() = default;

void FuncOptimizeIterator::Run() {
  CHECK_NULL_FATAL(phaseImpl);
  for (MIRFunction *func : phaseImpl->GetMIRModule().GetFunctionList()) {
    phaseImpl->ProcessFunc(func);
  }
  phaseImpl->Finish();
}
}  // namespace maple
