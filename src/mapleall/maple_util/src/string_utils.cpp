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
#include "string_utils.h"

namespace maple {
std::string StringUtils::Trim(const std::string &src) {
  // remove space
  return std::regex_replace(src, std::regex("\\s+"), "");
}

std::string StringUtils::LTrim(const std::string &src) {
  size_t start = src.find_first_not_of(" ");
  return (start == std::string::npos) ? "" : src.substr(start);
}

std::string StringUtils::RTrim(const std::string &src) {
  size_t end = src.find_last_not_of(" ");
  return (end == std::string::npos) ? "" : src.substr(0, end + 1);
}

std::string StringUtils::TrimWhitespace(const std::string &src) {
  return RTrim(LTrim(src));
}

std::string StringUtils::Replace(const std::string &src, const std::string &target,
                                 const std::string &replacement) {
  std::string::size_type replaceLen = replacement.size();
  std::string::size_type targetLen = target.size();
  std::string temp = src;
  std::string::size_type index = 0;
  index = temp.find(target, index);
  while (index != std::string::npos) {
    temp.replace(index, targetLen, replacement);
    index += replaceLen;
    index = temp.find(target, index);
  }
  return temp;
}

std::string StringUtils::Append(const std::string &src, const std::string &target, const std::string &spliter) {
  return src + spliter + target;
}

std::string StringUtils::GetStrAfterLast(const std::string &src, const std::string &target,
                                         bool isReturnEmpty) {
  size_t pos = src.find_last_of(target);
  if (pos == std::string::npos) {
    return isReturnEmpty ? "" : src;
  }
  return src.substr(pos + 1);
}

std::string StringUtils::GetStrBeforeLast(const std::string &src, const std::string &target, bool isReturnEmpty) {
  size_t pos = src.find_last_of(target);
  if (pos == std::string::npos) {
    return isReturnEmpty ? "" : src;
  }
  return src.substr(0, pos);
}

std::string StringUtils::GetStrBeforeFirst(const std::string &src, const std::string &target, bool isReturnEmpty) {
  size_t pos = src.find_first_of(target);
  if (pos == std::string::npos) {
    return isReturnEmpty ? "" : src;
  }
  return src.substr(0, pos);
}

void StringUtils::Split(const std::string &src, std::unordered_set<std::string> &container, char delim) {
  if (Trim(src).empty()) {
    return;
  }
  std::stringstream strStream(src + delim);
  std::string item;
  while (std::getline(strStream, item, delim)) {
    container.insert(item);
  }
}

std::regex StringUtils::kCommandInjectionRegex("[;\\|\\&\\$\\>\\<`]");
}  // namespace maple
