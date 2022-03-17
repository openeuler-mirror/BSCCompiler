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
#include "hir2mpl_ut_regx.h"
#include <sstream>
#include <iostream>

namespace maple {
bool HIR2MPLUTRegx::Match(const std::string &str, const std::string &pattern) {
  if (std::regex_match(str, std::regex(pattern))) {
    return true;
  } else {
    std::cerr << "Pattern: " << pattern << std::endl;
    std::cerr << "String: " << str << std::endl;
    return false;
  }
}

std::string HIR2MPLUTRegx::RegName(uint32 regNum) {
  std::stringstream ss;
  if (regNum == kAnyNumber) {
    ss << "Reg[0-9]+";
  } else {
    ss << "Reg" << regNum;
  }
  return ss.str();
}

std::string HIR2MPLUTRegx::RefIndex(uint32 typeIdx) {
  std::stringstream ss;
  if (typeIdx == kAnyNumber) {
    ss << "R[0-9]+";
  } else {
    ss << "R" << typeIdx;
  }
  return ss.str();
}

std::string HIR2MPLUTRegx::AnyNum(uint32 typeIdx) {
  std::stringstream ss;
  if (typeIdx == kAnyNumber) {
    ss << "[0-9]+";
  } else {
    ss << "" << typeIdx;
  }
  return ss.str();
}
}  // namespace maple