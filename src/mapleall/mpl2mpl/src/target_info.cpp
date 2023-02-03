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
#include "target_info.h"

namespace maple {

// ---------- TargetInfo ----------
TargetInfo *TargetInfo::CreateTargetInfo(MapleAllocator &alloc, BackEnd backEnd) {
  // now we use macro to choose target.
  (void)backEnd;
  TargetInfo *ret = nullptr;
#if (defined(TARGAARCH64) && TARGAARCH64) || (defined(TARGRISCV64) && TARGRISCV64)
  ret = alloc.New<AArch64TargetInfo>();
#elif defined(TARGX86_64) && TARGX86_64
  ret = alloc.New<X64TargetInfo>();
#else
#error "unknown platform"
#endif
  return ret;
}

// ---------- AArch64TargetInfo ----------
size_t AArch64TargetInfo::GetIntrinsicInsnNum(MIRIntrinsicID id) {
  // A simple way to get insn number of intrinsicop/intrinsiccall.
  return IntrinDesc::intrinTable[id].GetNumInsn();
}

// ---------- X64TargetInfo ----------
size_t X64TargetInfo::GetIntrinsicInsnNum(MIRIntrinsicID id) {
  // A simple way to get insn number of intrinsicop/intrinsiccall.
  return IntrinDesc::intrinTable[id].GetNumInsn();
}
}  /* namespace maple */
