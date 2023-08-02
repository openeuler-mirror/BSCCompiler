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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_PROEPILOG_H
#define MAPLEBE_INCLUDE_CG_X64_X64_PROEPILOG_H

#include "proepilog.h"
#include "x64_cgfunc.h"

namespace maplebe {
using namespace maple;

class X64GenProEpilog : public GenProEpilog {
 public:
  explicit X64GenProEpilog(CGFunc &func) : GenProEpilog(func) {}
  ~X64GenProEpilog() override = default;

  bool NeedProEpilog() override;
  void Run() override;
 private:
  void GenerateProlog(BB &bb);
  void GenerateEpilog(BB &bb);
  void GenerateCalleeSavedRegs(bool isPush);
  void GeneratePushCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize);
  void GeneratePopCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize);
  void GeneratePushUnnamedVarargRegs();
  void GeneratePushRbpInsn();
  void GenerateMovRspToRbpInsn();
  void GenerateSubFrameSizeFromRspInsn();
  void GenerateAddFrameSizeToRspInsn();
  void GeneratePopInsn();
  void GenerateRetInsn();
};
}  /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_PROEPILOG_H */
