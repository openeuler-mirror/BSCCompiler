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
#ifndef MAPLE_CPP2MPL_OPTION_H
#define MAPLE_CPP2MPL_OPTION_H
#include "option_descriptor.h"

namespace maple {
enum CppOptionIndex {
  kInAst,
  kInCpp,
  kCpp2mplHelp,
};

const mapleOption::Descriptor cppUsage[] = {
  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
    "========================================\n"
    " Usage: cpp2mpl [ options ]\n"
    " options:\n",
    "cpp2mpl",
    {} },

  { kInAst, 0, "", "inast", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -in-ast file1.ast,file2.ast\n"
    "                         : input ast files",
    "cpp2mpl",
    {} },

  { kInCpp, 0, "", "incpp", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -in-cpp file1.cpp,file2.cpp\n"
    "                         : input cpp files",
    "cpp2mpl",
    {} },

  { kCpp2mplHelp, 0, "h", "help", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
    "   -h, --help          : print usage and exit.\n",
    "cpp2mpl",
    {} },

  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "",
    "cpp2mpl",
    {} }
};
} // namespace maple

#endif //MAPLE_CPP2MPL_OPTION_H

