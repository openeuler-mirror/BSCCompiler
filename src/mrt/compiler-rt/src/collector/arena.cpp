/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "collector/arena.h"
#include "chosen.h"
namespace maplert {
void HandleArena::VisitGCRoots(const RefVisitor &visitor) {
  VisitTopSnapShot(visitor, nullptr, nullptr);
}

ScopedHandles::ScopedHandles() {
  snapshot = TLMutator().GetHandleArena();
}

ScopedHandles::~ScopedHandles() {
  if (Collector::Instance().Type() == kNaiveRC) {
    RefVisitor decVisitor = [](address_t &obj) {
      MRT_DecRef(obj);
    };
    TLMutator().GetHandleArena().VisitTopSnapShot(decVisitor, snapshot);
  }
  TLMutator().GetHandleArena().PopBanks(snapshot);
  snapshot.Clear(); // clear to avoid destructor handling again
}

void HandleBase::Push(address_t ref) {
  handle = TLMutator().GetHandleArena().AllocateSlot();
  if (handle == nullptr) {
    LOG(FATAL) << "Get Arena handle failed" << maple::endl;
    return;
  }
  *handle = ref;
}
}  // namespace maplert
