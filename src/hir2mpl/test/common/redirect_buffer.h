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
#ifndef HIR2MPL_INCLUDE_REDIRECT_BUFFER_H
#define HIR2MPL_INCLUDE_REDIRECT_BUFFER_H
#include <sstream>
#include <iostream>

namespace maple {
class RedirectBuffer {
 public:
  RedirectBuffer() : sbuf(std::cout.rdbuf()), outByErr(false) {}
  ~RedirectBuffer() = default;

  void RedirectCout(bool outByErrIn = false) {
    sbuf = RedirectCoutBuf(ss.rdbuf());
    ss.str("");
    outByErr = outByErrIn;
  }

  void RestoreCout() {
    ss.str("");
    RedirectCoutBuf(sbuf);
  }

  std::string GetBufferString() {
    std::string str = ss.str();
    ss.str("");
    if (outByErr) {
      std::cerr << str;
    }
    return str;
  }

  void ClearBufferString() {
    ss.str("");
  }

  std::string RemoveLastReturnChar(const std::string &input) {
    size_t length = input.length();
    while (input[length - 1] == '\n') {
      length--;
    }
    return input.substr(0, length);
  }

 private:
  std::stringstream ss;
  std::streambuf *sbuf;
  bool outByErr;

  std::streambuf *RedirectCoutBuf(std::streambuf *newBuf) {
    std::streambuf *streamBuf = std::cout.rdbuf();
    std::cout.rdbuf(newBuf);
    return streamBuf;
  }
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_REDIRECT_BUFFER_H