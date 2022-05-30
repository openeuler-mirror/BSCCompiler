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
#include "cl_option.h"
#include "cl_parser.h"
#include <algorithm>
#include <iterator>

using namespace maplecl;

void OptionInterface::FinalizeInitialization(const std::vector<std::string> &optnames,
                                             const std::string &descr,
                                             const std::vector<OptionCategoryRefWrp> &optionCategories) {
  optDescription = descr;
  auto &cl = CommandLine::GetCommandLine();

  std::copy(optnames.begin(), optnames.end(), std::back_inserter(names));

  if (optionCategories.empty()) {
    cl.Register(optnames, *this, cl.defaultCategory);
  } else {
    for (auto &cat : optionCategories) {
      cl.Register(optnames, *this, cat.get());
    }
  }
}
