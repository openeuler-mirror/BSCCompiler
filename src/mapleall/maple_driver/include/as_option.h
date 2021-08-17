/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_AS_OPTION_H
#define MAPLE_AS_OPTION_H
#include "option_descriptor.h"

namespace maple {
enum AsOptionIndex {
  kAsHelp,
};

const mapleOption::Descriptor asUsage[] = {
  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
    "========================================\n"
         " Usage: as [ options ]\n"
         " options:\n",
    "as",
    {} },


  { kAsHelp, 0, "h", "help", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
    "   -h, --help          : print usage and exit.\n",
    "as",
    {} },

  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "",
    "as",
    {} }
};
} // namespace maple
#endif // MAPLE_AS_OPTION_H