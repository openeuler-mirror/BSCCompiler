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

#ifndef MAPLEBE_INCLUDE_AARCH64_DCE_H
#define MAPLEBE_INCLUDE_AARCH64_DCE_H

#include "cg_dce.h"
namespace maplebe {
class AArch64Dce : public CGDce {
 public:
  AArch64Dce(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo) : CGDce(mp, f, sInfo) {}
  ~AArch64Dce() override = default;

 private:
  bool RemoveUnuseDef(VRegVersion &defVersion) override;
};

class A64DeleteRegUseVisitor : public DeleteRegUseVisitor {
 public:
  A64DeleteRegUseVisitor(CGSSAInfo &cgSSAInfo, uint32 dInsnID) : DeleteRegUseVisitor(cgSSAInfo, dInsnID) {}
  ~A64DeleteRegUseVisitor() override = default;

 private:
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *v) final;
  void Visit(PhiOperand *v) final;
};
}
#endif /* MAPLEBE_INCLUDE_AARCH64_DCE_H */
