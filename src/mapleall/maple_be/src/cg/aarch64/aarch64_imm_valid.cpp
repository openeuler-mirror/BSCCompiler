/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_imm_valid.h"
#include <array>
#include <iomanip>
#include <set>

namespace maplebe {
const std::set<uint64> ValidBitmaskImmSet = {
#include "valid_bitmask_imm.txt"
};
constexpr uint32 kMaxBitTableSize = 5;
constexpr std::array<uint64, kMaxBitTableSize> kBitmaskImmMultTable = {
    0x0000000100000001UL, 0x0001000100010001UL, 0x0101010101010101UL, 0x1111111111111111UL, 0x5555555555555555UL,
};
namespace aarch64 {
bool IsBitmaskImmediate(uint64 val, uint32 bitLen) {
  ASSERT(val != 0, "IsBitmaskImmediate() don's accept 0 or -1");
  ASSERT(static_cast<int64>(val) != -1, "IsBitmaskImmediate() don's accept 0 or -1");
  if ((bitLen == k32BitSize) && (static_cast<int32>(val) == -1)) {
    return false;
  }
  uint64 val2 = val;
  if (bitLen == k32BitSize) {
    val2 = (val2 << k32BitSize) | (val2 & ((1ULL << k32BitSize) - 1));
  }
  bool expectedOutcome = (ValidBitmaskImmSet.find(val2) != ValidBitmaskImmSet.end());

  if ((val & 0x1) != 0) {
    /*
     * we want to work with
     * 0000000000000000000000000000000000000000000001100000000000000000
     * instead of
     * 1111111111111111111111111111111111111111111110011111111111111111
     */
    val = ~val;
  }

  if (bitLen == k32BitSize) {
    val = (val << k32BitSize) | (val & ((1ULL << k32BitSize) - 1));
  }

  /* get the least significant bit set and add it to 'val' */
  uint64 tmpVal = val + (val & static_cast<uint64>(UINT64_MAX - val + 1));

  /* now check if tmp is a power of 2 or tmpVal==0. */
  tmpVal = tmpVal & (tmpVal - 1);
  if (tmpVal == 0) {
    if (!expectedOutcome) {
#if defined(DEBUG) && DEBUG
      LogInfo::MapleLogger() << "0x" << std::hex << std::setw(static_cast<int>(k16ByteSize)) <<
          std::setfill('0') << static_cast<uint64>(val) << "\n";
#endif
      return false;
    }
    ASSERT(expectedOutcome, "incorrect implementation: not valid value but returning true");
    /* power of two or zero ; return true */
    return true;
  }

  int32 p0 = __builtin_ctzll(val);
  int32 p1 = __builtin_ctzll(tmpVal);
  int64 diff = p1 - p0;

  /* check if diff is a power of two; return false if not. */
  if ((static_cast<uint64>(diff) & (static_cast<uint64>(diff) - 1)) != 0) {
    ASSERT(!expectedOutcome, "incorrect implementation: valid value but returning false");
    return false;
  }

  uint32 logDiff = static_cast<uint32>(__builtin_ctzll(static_cast<uint64>(diff)));
  uint64 pattern = val & ((1ULL << static_cast<uint64>(diff)) - 1);
#if defined(DEBUG) && DEBUG
  bool ret = (val == pattern * kBitmaskImmMultTable[kMaxBitTableSize - logDiff]);
  ASSERT(expectedOutcome == ret, "incorrect implementation: return value does not match expected outcome");
  return ret;
#else
  return val == pattern * kBitmaskImmMultTable[kMaxBitTableSize - logDiff];
#endif
}
}  // namespace aarch64
}  // namespace maplebe

