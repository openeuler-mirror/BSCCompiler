/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_UTIL_INCLUDE_STRING_UTILS_H
#define MAPLE_UTIL_INCLUDE_STRING_UTILS_H
#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <regex>

namespace maple {
class StringUtils {
 public:
  template <class Container>
  static void Split(const std::string &src, Container &container, char delim) {
    if (Trim(src).empty()) {
      return;
    }

    std::stringstream strStream(src + delim);
    std::string item;
    while (std::getline(strStream, item, delim)) {
      container.emplace_back(item);
    }
  }

  template <typename Container>
  static void SplitSV(const std::string_view &src, Container &container, char delim) {
    size_t startInd = 0;
    while (startInd < src.size()) {
      if (src[startInd] == delim) {
        ++startInd;
        continue;
      }

      size_t endInd = src.find_first_of(delim, startInd);
      if (startInd != endInd) {
        container.emplace_back(src.substr(startInd,  endInd - startInd));
      }

      if (endInd == std::string_view::npos) {
        break;
      }
      startInd = endInd + 1;
    }
  }

  static void Split(const std::string &src, std::unordered_set<std::string> &container, char delim);

  static std::string Trim(const std::string &src);
  static std::string Replace(const std::string &src, const std::string &target, const std::string &replacement);
  static std::string Append(const std::string &src, const std::string &target, const std::string &spliter);
  static std::string GetStrAfterLast(const std::string &src, const std::string &target,
                                     bool isReturnEmpty = false);
  static std::string GetStrBeforeLast(const std::string &src, const std::string &target,
                                      bool isReturnEmpty = false);
  static std::string GetStrBeforeFirst(const std::string &src, const std::string &target,
                                       bool isReturnEmpty = false);
  static bool HasCommandInjectionChar(const std::string &s) {
    return std::regex_search(s, kCommandInjectionRegex);
  }

  static bool EndsWith(const std::string &str, const std::string &end) {
    if (end.size() > str.size()) {
      return false;
    }

    return std::equal(end.rbegin(), end.rend(), str.rbegin());
  }

  static bool StartsWith(const std::string &str, const std::string &start) {
    if (start.size() > str.size()) {
      return false;
    }

    return std::equal(start.cbegin(), start.cend(), str.cbegin());
  }

 private:
  static std::regex kCommandInjectionRegex;
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_STRING_UTILS_H
