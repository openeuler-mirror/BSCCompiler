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
#ifndef MPL2MPL_INCLUDE_CLASS_INIT_H
#define MPL2MPL_INCLUDE_CLASS_INIT_H
#include "phase_impl.h"
#include "class_hierarchy_phase.h"
#include "maple_phase_manager.h"

namespace maple {
class ClassInit : public FuncOptimizeImpl {
 public:
  ClassInit(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump) {}
  ~ClassInit() override = default;

  FuncOptimizeImpl *Clone() override {
    return new ClassInit(*this);
  }

  void ProcessFunc(MIRFunction *func) override;

 private:
  void GenClassInitCheckProfile(MIRFunction &func, const MIRSymbol &classInfo, StmtNode *clinit) const;
  void GenPreClassInitCheck(MIRFunction &func, const MIRSymbol &classInfo, const StmtNode *clinit) const;
  void GenPostClassInitCheck(MIRFunction &func, const MIRSymbol &classInfo, const StmtNode *clinit) const;
  MIRSymbol *GetClassInfo(const std::string &classname);
  bool CanRemoveClinitCheck(const std::string &clinitClassname) const;
};

MAPLE_MODULE_PHASE_DECLARE(M2MClassInit)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CLASS_INIT_H
