/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_MERGE_STMTS_H
#define MAPLE_ME_INCLUDE_ME_MERGE_STMTS_H
#include "me_function.h"

// Merge smaller stores into larger one
namespace maple {
class MergeStmts {
 public:
  explicit MergeStmts(MeFunction &func) : func(func) {}
  ~MergeStmts() = default;

  using vOffsetStmt = std::vector<std::pair<int32, MeStmt*> >;
  void MergeMeStmts();

 private:
  uint32 GetStructFieldSize(MIRStructType* structType, FieldID fieldID);
  uint32 GetStructFieldBitSize(MIRStructType* structType, FieldID fieldID);
  void mergeIassigns(vOffsetStmt& iassignCandidates);
  //void mergeDassigns(vOffsetStmt& dassignCandidates);

 private:
  MeFunction &func;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_MERGE_STMTS_H

