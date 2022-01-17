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

#ifndef MAPLEBE_INCLUDE_CG_DCE_H
#define MAPLEBE_INCLUDE_CG_DCE_H
#include "cgfunc.h"
#include "cg_ssa.h"

namespace maplebe {
/* dead code elimination*/
class CGDce {
 public:
  CGDce(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo) : memPool(&mp), cgFunc(&f), ssaInfo(&sInfo) {}
  virtual ~CGDce() = default;

  void DoDce();
  /* provide public use in ssa opt */
  virtual bool RemoveUnuseDef(VRegVersion &defVersion) = 0;
  CGSSAInfo *GetSSAInfo() {
    return ssaInfo;
  }

 protected:
  MemPool *memPool;
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
};

class DeleteRegUseVisitor : public OperandVisitorBase,
                            public OperandVisitors<RegOperand, ListOperand, MemOperand> {
 public:
  DeleteRegUseVisitor(CGSSAInfo &cgSSAInfo, uint32 dInsnID) : deleteInsnId(dInsnID), ssaInfo(&cgSSAInfo) {}
  virtual ~DeleteRegUseVisitor() = default;

 protected:
  CGSSAInfo *GetSSAInfo() {
    return ssaInfo;
  }
  uint32 deleteInsnId;
 private:
  CGSSAInfo *ssaInfo;
};

MAPLE_FUNC_PHASE_DECLARE(CgDce, maplebe::CGFunc)
}
#endif /* MAPLEBE_INCLUDE_CG_DCE_H */
