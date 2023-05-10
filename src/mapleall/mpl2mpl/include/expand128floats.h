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
#ifndef MPL2MPL_INCLUDE_EXPAND128FLOATS_H
#define MPL2MPL_INCLUDE_EXPAND128FLOATS_H
#include "phase_impl.h"
#include "maple_phase_manager.h"

#include <string>

namespace maple {
class Expand128Floats : public FuncOptimizeImpl {
 public:
  Expand128Floats(MIRModule &mod, KlassHierarchy *kh, bool trace) : FuncOptimizeImpl(mod, kh, trace) {}
  explicit Expand128Floats(MIRModule &mod) : FuncOptimizeImpl(mod, nullptr, false) {}
  ~Expand128Floats() override = default;

  FuncOptimizeImpl *Clone() override {
    return new Expand128Floats(*this);
  }

  void ProcessFunc(MIRFunction *func) override;
  void Finish() override {}

 private:
  std::string GetSequentialName0(const std::string &prefix, uint32_t num) const;
  uint32 GetSequentialNumber() const;
  std::string GetSequentialName(const std::string &prefix) const;
  std::string SelectSoftFPCall(Opcode opCode, BaseNode *node) const;
  void ReplaceOpNode(BlockNode *block, BaseNode *baseNode, size_t opndId,
                     BaseNode *currNode, MIRFunction *func, StmtNode *stmt);
  bool CheckAndUpdateOp(BlockNode *block, BaseNode *node,
                        MIRFunction *func, StmtNode *stmt);
  void ProcessBody(BlockNode *block, MIRFunction *func);
  void ProcessBody(BlockNode *block, StmtNode *stmt, MIRFunction *func);
};

MAPLE_MODULE_PHASE_DECLARE(M2MExpand128Floats)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_EXPAND128FLOATS_H
