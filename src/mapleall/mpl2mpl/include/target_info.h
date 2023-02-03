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
#ifndef MPL2MPL_INCLUDE_TARGET_INFO_H
#define MPL2MPL_INCLUDE_TARGET_INFO_H
#include <map>
#include <stack>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_function.h"
#include "intrinsic_op.h"

namespace maple {
constexpr size_t kMaxMoveBytes = 8;
enum class BackEnd {
  AARCH64,
  ARM32,
  RISCV64,
  X86_64
};

// ---------- TargetInfo ----------
class TargetInfo {
 public:
  TargetInfo() {}
  virtual ~TargetInfo() = default;

  static TargetInfo *CreateTargetInfo(MapleAllocator &alloc, BackEnd backEnd = BackEnd::AARCH64);
  virtual size_t GetMaxMoveBytes() = 0;
  virtual size_t GetIntrinsicInsnNum(MIRIntrinsicID id) = 0;
};

// ---------- AArch64TargetInfo ----------
class AArch64TargetInfo : public TargetInfo {
 public:
  AArch64TargetInfo() : TargetInfo() {}
  ~AArch64TargetInfo() override = default;

  size_t GetMaxMoveBytes() override { return kMaxMoveBytes; }
  size_t GetIntrinsicInsnNum(MIRIntrinsicID id) override;
};

// ---------- X64TargetInfo ----------
class X64TargetInfo : public TargetInfo {
 public:
  X64TargetInfo() : TargetInfo() {}
  ~X64TargetInfo() override = default;

  size_t GetMaxMoveBytes() override { return kMaxMoveBytes; }
  size_t GetIntrinsicInsnNum(MIRIntrinsicID id) override;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_TARGET_INFO_H
