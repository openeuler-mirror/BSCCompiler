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

#include <string>

#include "gtest/gtest.h"
#include "mpl_stacktrace.h"

/* Expected stacktrace (except gtest's frames)
 * _______________________
 * 0# foo8() [0x*] in mapleallUT
 * 1# foo7() [0x*] in mapleallUT
 * 2# foo6() [0x*] in mapleallUT
 * 3# foo5() [0x*] in mapleallUT
 * 4# foo4() [0x*] in mapleallUT
 * 5# foo3() [0x*] in mapleallUT
 * 6# foo2() [0x*] in mapleallUT
 * 7# foo1() [0x*] in mapleallUT
 * 8# foo0() [0x*] in mapleallUT
 * ______________________
 */

void __attribute__((noinline)) foo8() {
  maple::Stacktrace<> st;

  size_t framesCount = 8;
  // calc end of tested frame
  auto framesEnd = st.cbegin();
  std::advance(framesEnd, framesCount);

  for (auto it = st.cbegin(); it != framesEnd; ++it, --framesCount) {
    auto frame = *it;

    std::string path = frame.getFilename();
    auto pos = path.rfind('/');
    std::string filename = (pos != std::string::npos) ? path.substr(pos + 1, path.length()) : path;
    EXPECT_EQ(filename, "mapleallUT");
#ifdef DEBUG
    EXPECT_EQ(frame.getName(), "foo" + std::to_string(framesCount) + "()");
#endif
  }
}

void __attribute__((noinline)) foo7() {
  foo8();
}

void __attribute__((noinline)) foo6() {
  foo7();
}

void __attribute__((noinline)) foo5() {
  foo6();
}

void __attribute__((noinline)) foo4() {
  foo5();
}

void __attribute__((noinline)) foo3() {
  foo4();
}

void __attribute__((noinline)) foo2() {
  foo3();
}

void __attribute__((noinline)) foo1() {
  foo2();
}

void __attribute__((noinline)) foo0() {
  foo1();
}

TEST(Stacktrace, Print) {
  foo0();
}
