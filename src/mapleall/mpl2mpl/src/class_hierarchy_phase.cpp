/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#include "class_hierarchy_phase.h"
namespace maple {
bool M2MKlassHierarchy::PhaseRun(maple::MIRModule &m) {
  kh = GetPhaseMemPool()->New<KlassHierarchy>(&m, GetPhaseMemPool());
  KlassHierarchy::traceFlag = Options::dumpPhase.compare(PhaseName()) == 0;
  kh->BuildHierarchy();
#if MIR_JAVA
  if (!Options::skipVirtualMethod) {
    kh->CountVirtualMethods();
  }
#else
  kh->CountVirtualMethods();
#endif
  if (KlassHierarchy::traceFlag) {
    kh->Dump();
  }
  return true;
}
}  // namespace maple
