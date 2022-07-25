/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_INLINE_ANALYZER_H
#define MPL2MPL_INCLUDE_INLINE_ANALYZER_H
#include "call_graph.h"
#include "maple_phase_manager.h"
#include "me_ir.h"
#include "me_option.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_builder.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "opcode_info.h"
#include "string_utils.h"
#include "target_info.h"

namespace maple {

constexpr size_t kSizeScale = 100;
constexpr size_t kInsn2Threshold = 2;

class InlineAnalyzer {
 public:
  InlineAnalyzer(MemPool *tempMemPool, MIRFunction *func) : alloc(tempMemPool), curFunc(func) {
    Init();
  };
  void Init();
  int64 GetStmtsCost(BlockNode *block);
  int64 GetStmtCost(StmtNode *stmt);
  int64 GetExprCost(BaseNode *expr);
  int64 GetMeStmtCost(MeStmt *meStmt);
  int64 GetMeExprCost(MeExpr *meExpr);
  int64 GetMoveCost(size_t sizeInByte);
  TargetInfo *GetTargetInfo() { return ti; }
  MIRType *GetMIRTypeFromStIdxAndField(const StIdx idx, FieldID fieldID);

  ~InlineAnalyzer() {
    curFunc = nullptr;
    ti = nullptr;
  }

 private:
  MapleAllocator alloc;
  MIRFunction *curFunc = nullptr;  // the current function that being analyzed.
  TargetInfo *ti = nullptr;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_INLINE_ANALYZER_H
