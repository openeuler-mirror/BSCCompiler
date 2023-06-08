/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_VERIFY_MEMORDER_H
#define MPL2MPL_INCLUDE_VERIFY_MEMORDER_H
#include "mir_function.h"
#include "mir_nodes.h"
#include "phase_impl.h"

namespace maple {
class VerifyMemorder : public FuncOptimizeImpl {
 public:
  explicit VerifyMemorder(MIRModule &m)
      : FuncOptimizeImpl(m, nullptr, false), mod(m) {}

  VerifyMemorder(MIRModule &m, KlassHierarchy *kh,  bool trace)
      : FuncOptimizeImpl(m, kh, trace), mod(m) {}

  ~VerifyMemorder() override = default;
  void Run();

  FuncOptimizeImpl *Clone() override {
    return new VerifyMemorder(*this);
  }

 protected:
  void ProcessStmt(StmtNode &stmt) override;

 private:
  int64 GetMemorderValue(const ConstvalNode &memorderNode) const;
  int64 HandleMemorder(IntrinsiccallNode &intrn, size_t opnd);
  void SetMemorderNode(IntrinsiccallNode &intrn, int64 value, size_t opnd);
  void HandleCompareExchangeMemorder(IntrinsiccallNode &intrn, size_t opnd);
  void HandleStoreMemorder(IntrinsiccallNode &intrn, size_t opnd);
  void HandleLoadMemorder(IntrinsiccallNode &intrn, size_t opnd);

  MIRModule &mod;
};

MAPLE_MODULE_PHASE_DECLARE(M2MVerifyMemorder)
} // namespace maple
#endif // MPL2MPL_INCLUDE_VERIFY_MEMORDER_H