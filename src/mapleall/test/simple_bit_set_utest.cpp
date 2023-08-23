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
#include "gtest/gtest.h"
#include "simple_bit_set.h"
#include <random>
#include <functional>
#include <bitset>
#include <cstring>
#include "mpl_logging.h"
#include "common_utils.h"
#include "mir_type.h"

using namespace maple;
constexpr uint64_t kAllOnes64 = 0xFFFFFFFFFFFFFFFFULL;

SimpleBitSet<maple::k64BitSize> bitset64;
SimpleBitSet<maple::k128BitSize> bitset128;
SimpleBitSet<maplebe::k256BitSize> bitset256;
SimpleBitSet<maple::k128BitSize> bitset128copy(bitset128);
SimpleBitSet<maple::k128BitSize> bitset128const256Init(256UL);

TEST(SimpleBitSetAll, GetWordSize) {
  ASSERT_EQ(bitset64.GetWordSize(), 1);
  ASSERT_EQ(bitset128.GetWordSize(), 2);
  ASSERT_EQ(bitset256.GetWordSize(), 4);
}

  // bitset128 test
TEST(SimpleBitSet128, Method_LogicalOperator_And_GetWord) {
  bitset128 = bitset128 | 64ULL;
  ASSERT_EQ(bitset128, SimpleBitSet<maple::k128BitSize>(64)); // 64 = 0100 0000
  ASSERT_EQ(bitset128.GetWord(0), 64UL);
  ASSERT_EQ(bitset128.GetWord(1), 0UL);

  bitset128 = bitset128 | 65535ULL;
  ASSERT_EQ(bitset128, SimpleBitSet<maple::k128BitSize>(65535)); // 0xffff
  ASSERT_EQ(bitset128.GetWord(0), 65535);
  ASSERT_EQ(bitset128.GetWord(1), 0UL);

  bitset128 = bitset128 & 0xfULL;
  ASSERT_EQ(bitset128, SimpleBitSet<maple::k128BitSize>(0xf));
  ASSERT_EQ(bitset128.GetWord(0), 0xf);
  ASSERT_EQ(bitset128.GetWord(1), 0UL);

  ASSERT_EQ((~bitset128), ~(SimpleBitSet<maple::k128BitSize>(0xf)));
  ASSERT_EQ((~bitset128).GetWord(0), 0xfULL);
  ASSERT_EQ((~bitset128).GetWord(1), kAllOnes64);
}

TEST(SimpleBitSet128, Method_Set_Reset_ToBitset_And_GetWord) {
  bitset128.Reset();
  ASSERT_EQ(bitset128.Set(0), true);
  ASSERT_EQ(bitset128.GetWord(0), 0x1ULL);
  ASSERT_EQ(bitset128.Reset(64), true);
  ASSERT_EQ(bitset128.GetWord(1), 0ULL);
  ASSERT_EQ(bitset128.Reset(65), true);
  ASSERT_EQ(bitset128.GetWord(1), 0ULL);

  bitset128.Reset();
  ASSERT_EQ(bitset128, SimpleBitSet<maple::k128BitSize>());
  ASSERT_EQ(bitset128.GetWord(0), 0);
  ASSERT_EQ(bitset128.GetWord(1), 0);

  ASSERT_EQ(bitset128.ToBitset(), std::bitset<maplebe::k128BitSize>(0));

  ASSERT_EQ(bitset128.Set(0), true);
  ASSERT_EQ(bitset128, SimpleBitSet<maple::k128BitSize>(1));
  ASSERT_EQ(bitset128.GetWord(0), 1);
  ASSERT_EQ(bitset128.GetWord(1), 0);

  ASSERT_EQ(bitset128.ToBitset(), std::bitset<maplebe::k128BitSize>(1));

  ASSERT_EQ(bitset128.Set(3), true);
  ASSERT_EQ(bitset128, SimpleBitSet<maple::k128BitSize>(9));
  ASSERT_EQ(bitset128.GetWord(0), 9);
  ASSERT_EQ(bitset128.GetWord(1), 0);

  ASSERT_EQ(bitset128.ToBitset(), std::bitset<maplebe::k128BitSize>(9));

  ASSERT_EQ(bitset128[0], 1);
  ASSERT_EQ(bitset128[1], 0);
  ASSERT_EQ(bitset128[2], 0);
  ASSERT_EQ(bitset128[3], 1);
  ASSERT_EQ(bitset128[63], 0);
  ASSERT_EQ(bitset128[64], 0);
  ASSERT_EQ(bitset128[65], 0);
  ASSERT_EQ(bitset128[66], 0);
  ASSERT_EQ(bitset128[88], 0);
  ASSERT_EQ(bitset128[166], -1);
}

  // bitset64 test
TEST(SimpleBitSet64, LogicalOperator_And_GetWord) {
  bitset64 = bitset64 | 64ULL;
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(64UL));
  ASSERT_EQ(bitset64.GetWord(0), 64UL);

  bitset64 = bitset64 | kAllOnes64;
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(kAllOnes64));
  ASSERT_EQ(bitset64.GetWord(0), kAllOnes64);

  bitset64 = bitset64 & 0ULL;
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>());
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(0));
  ASSERT_EQ(bitset64.GetWord(0), 0UL);

  ASSERT_EQ((~bitset64), SimpleBitSet<maple::k64BitSize>(~0UL));

  bitset64 = bitset64 & SimpleBitSet<maple::k64BitSize>(0xf);
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(0xf));
  ASSERT_EQ(bitset64.GetWord(0), 15);

  bitset64 = bitset64 | SimpleBitSet<maple::k64BitSize>(0xffff);
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(0xffff));
  ASSERT_EQ(bitset64.GetWord(0), 65535);
}

TEST(SimpleBitSet64, Set_Reset_ToBitset_And_GetWord) {
  ASSERT_EQ(bitset64.Reset(0), true);
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(0xfffe));
  ASSERT_EQ(bitset64.GetWord(0), 65534);

  ASSERT_EQ(bitset64.ToBitset(), std::bitset<maple::k64BitSize>(65534));

  bitset64.Reset();
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>());
  ASSERT_EQ(bitset64.GetWord(0), 0);

    ASSERT_EQ(bitset64.ToBitset(), std::bitset<maple::k64BitSize>(0));

  ASSERT_EQ(bitset64.Set(0), true);
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(1));
  ASSERT_EQ(bitset64.GetWord(0), 1);

  ASSERT_EQ(bitset64.ToBitset(), std::bitset<maple::k64BitSize>(1));

  ASSERT_EQ(bitset64.Set(3), true);
  ASSERT_EQ(bitset64, SimpleBitSet<maple::k64BitSize>(9));
  ASSERT_EQ(bitset64.GetWord(0), 9);

  ASSERT_EQ(bitset64.ToBitset(), std::bitset<maple::k64BitSize>(9));

  ASSERT_EQ(bitset64[0], 1);
  ASSERT_EQ(bitset64[1], 0);
  ASSERT_EQ(bitset64[2], 0);
  ASSERT_EQ(bitset64[3], 1);
  ASSERT_EQ(bitset64[63], 0);
  ASSERT_EQ(bitset64[64], -1);
}
