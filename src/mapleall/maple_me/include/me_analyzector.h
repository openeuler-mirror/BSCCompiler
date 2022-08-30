/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEME_INCLUDE_ANALYZECTOR_H
#define MAPLEME_INCLUDE_ANALYZECTOR_H

#include "me_function.h"
#include "class_hierarchy_phase.h"
#include "me_dominance.h"
#include "me_irmap_build.h"

namespace maple {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"

class AnalyzeCtor {
 public:
  AnalyzeCtor(MeFunction &func, Dominance &dom, KlassHierarchy &kh) : func(&func), dominance(&dom), klassh(&kh) {}
  virtual ~AnalyzeCtor() = default;

  virtual void ProcessFunc();
  void ProcessStmt(MeStmt &stmt);

 private:
  bool hasSideEffect = false;
  std::unordered_set<FieldID> fieldSet;
  MeFunction *func;
  Dominance *dominance;
  KlassHierarchy *klassh;

};
#pragma clang diagnostic pop

MAPLE_FUNC_PHASE_DECLARE(MEAnalyzeCtor, MeFunction)
}  // namespace maple
#endif  // MAPLEME_INCLUDE_ANALYZECTOR_H
