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
#include "int128_util.h"
#include "lexer.h"
#include "mempool_allocator.h"

using namespace maple;

TEST(MIRLexer, LexInt128Const) {
  std::array<uint64, 2> init;

  // init MIRLexer
  MemPool *funcDatatMp = memPoolCtrler.NewMemPool("mempool", true);
  MapleAllocator funcDataMa(funcDatatMp);
  MIRLexer lexer(nullptr, funcDataMa);

  // parse positive decimal constant
  std::string decConstString = "295147905179352825855";  // 2^68 - 1
  lexer.PrepareForString(decConstString);
  ASSERT_EQ(lexer.GetTokenKind(), TK_intconst);
  IntVal decIntVal = lexer.GetTheInt128Val();
  init = {0xffffffffffffffff, 0xf};
  ASSERT_EQ(decIntVal, IntVal(init.data(), kInt128BitSize, /*sign*/ false));

  // parse negative decimal constant
  std::string negConstString = "-1";
  lexer.PrepareForString(negConstString);
  ASSERT_EQ(lexer.GetTokenKind(), TK_intconst);
  IntVal negIntVal = lexer.GetTheInt128Val();
  init = {0xffffffffffffffff, 0xffffffffffffffff};
  ASSERT_EQ(negIntVal, IntVal(init.data(), kInt128BitSize, /*sign*/ true));

  // parse hex constant
  std::string hexConstString = "0xabcdef0123456789abcdef0123456789";
  lexer.PrepareForString(hexConstString);
  ASSERT_EQ(lexer.GetTokenKind(), TK_intconst);
  IntVal hexIntVal = lexer.GetTheInt128Val();
  init = {0xabcdef0123456789, 0xabcdef0123456789};
  ASSERT_EQ(hexIntVal, IntVal(init.data(), kInt128BitSize, /*sign*/ false));
}
