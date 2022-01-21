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

#ifndef MAPLEBE_INCLUDE_CG_PROP_H
#define MAPLEBE_INCLUDE_CG_PROP_H

#include "cgfunc.h"
#include "cg_ssa.h"
#include "cg_dce.h"
#include "cg.h"

namespace maplebe {
class CGProp {
 public:
  CGProp(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo)
      : memPool(&mp),
        cgFunc(&f),
        propAlloc(&mp),
        ssaInfo(&sInfo) {
    cgDce = f.GetCG()->CreateCGDce(mp, f, sInfo);
  }
  virtual ~CGProp() = default;

  void DoCopyProp();
  void DoTargetProp();

 protected:
  MemPool *memPool;
  CGFunc *cgFunc;
  MapleAllocator propAlloc;
  CGSSAInfo *GetSSAInfo() {
    return ssaInfo;
  }
  CGDce *GetDce() {
    return cgDce;
  }

 private:
  virtual void CopyProp(Insn &insn) = 0;
  virtual void TargetProp(Insn &insn) = 0;
  virtual void PropPatternOpt() = 0;
  CGSSAInfo *ssaInfo;
  CGDce *cgDce = nullptr;
};

class ReplaceRegOpndVisitor : public OperandVisitorBase,
                              public OperandVisitors<RegOperand, ListOperand, MemOperand>,
                              public OperandVisitor<PhiOperand> {
 public:
  ReplaceRegOpndVisitor(CGFunc &f, Insn &cInsn, uint32 cIdx, RegOperand &oldR ,RegOperand &newR)
      : cgFunc(&f),
        insn(&cInsn),
        idx(cIdx),
        oldReg(&oldR),
        newReg(&newR) {}
  virtual ~ReplaceRegOpndVisitor() = default;

 protected:
  CGFunc *cgFunc;
  Insn *insn;
  uint32 idx;
  RegOperand *oldReg;
  RegOperand *newReg;
};

MAPLE_FUNC_PHASE_DECLARE(CgCopyProp, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE(CgTargetProp, maplebe::CGFunc)
}
#endif /* MAPLEBE_INCLUDE_CG_PROP_H */
