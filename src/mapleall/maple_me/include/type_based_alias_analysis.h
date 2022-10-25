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

#ifndef MAPLE_ME_INCLUDE_TYPE_BASED_ALIAS_ANALYSIS_H
#define MAPLE_ME_INCLUDE_TYPE_BASED_ALIAS_ANALYSIS_H
#include "alias_class.h"

namespace maple {
class TypeBasedAliasAnalysis {
 public:
  static bool MayAlias(const OriginalSt *ostA, const OriginalSt *ostB);
  static bool FilterAliasElemOfRHSForIassign(const OriginalSt *aliasElemOst, const OriginalSt *lhsOst,
                                             const OriginalSt *rhsOst);
  static bool MayAliasTBAAForC(const OriginalSt *ostA, const OriginalSt *ostB);
  static void ClearOstTypeUnsafeInfo();
  static std::vector<bool> &GetPtrTypeUnsafe() {
    return ptrValueTypeUnsafe;
  }

  static void SetVstValueTypeUnsafe(size_t vstIdx) {
    if (vstIdx >= ptrValueTypeUnsafe.size()) {
      ptrValueTypeUnsafe.resize(vstIdx + 1, false);
    }
    ptrValueTypeUnsafe[vstIdx] = true;
  }

  static void SetVstValueTypeUnsafe(const VersionSt &vst) {
    SetVstValueTypeUnsafe(vst.GetIndex());
  }

  static bool IsValueTypeUnsafe(const VersionSt &vst) {
    size_t vstIdx = vst.GetIndex();
    if (vstIdx >= ptrValueTypeUnsafe.size()) {
      return false;
    }
    return ptrValueTypeUnsafe[vstIdx];
  }

  static bool IsMemTypeUnsafe(const OriginalSt &ost) {
    auto prevLevVstIdx = ost.GetPointerVstIdx();
    if (prevLevVstIdx > ptrValueTypeUnsafe.size()) {
      return false;
    }
    return ptrValueTypeUnsafe[prevLevVstIdx];
  }

 private:
  static std::vector<bool> ptrValueTypeUnsafe; // index is OStIdx
};
} // namespace maple
#endif // MAPLE_ME_INCLUDE_TYPE_BASED_ALIAS_ANALYSIS_H
