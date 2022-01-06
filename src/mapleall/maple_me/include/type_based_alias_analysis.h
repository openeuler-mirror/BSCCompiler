/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_ME_INCLUDE_TYPE_BASED_ALIAS_ANALYSIS_H
#define MAPLE_ME_INCLUDE_TYPE_BASED_ALIAS_ANALYSIS_H
#include "alias_class.h"
namespace maple{
class TypeBasedAliasAnalysis {
 public:
  TypeBasedAliasAnalysis() = default;
  static bool MayAlias(const OriginalSt *ostA, const OriginalSt *ostB);
  static bool FilterAliasElemOfRHSForIassign(const OriginalSt *aliasElemOst, const OriginalSt *lhsOst,
                                             const OriginalSt *rhsOst);
};
} // namespace maple
#endif //MAPLE_ME_INCLUDE_TYPE_BASED_ALIAS_ANALYSIS_H