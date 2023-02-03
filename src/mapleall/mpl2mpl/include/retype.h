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
#ifndef MPL2MPL_INCLUDE_RETYPE_H
#define MPL2MPL_INCLUDE_RETYPE_H
#include "mir_nodes.h"
#include "inline.h"

namespace maple {
class Retype {
 public:
  Retype(MIRModule *mod, MemPool *memPool) : mirModule(mod), allocator(memPool) {}

  ~Retype() = default;

  void ReplaceRetypeExpr(const BaseNode &expr);
  void RetypeStmt(MIRFunction &func);
  void DoRetype();

 private:
  MIRModule *mirModule;
  MapleAllocator allocator;
  std::map<GStrIdx, MIRAliasVars> reg2varGenericInfo;
  void TransmitGenericInfo(MIRFunction &func, StmtNode &stmt);
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_RETYPE_H
