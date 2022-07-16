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
#ifndef OPENARKCOMPILER_AARCH64_OPT_UTILES_H
#define OPENARKCOMPILER_AARCH64_OPT_UTILES_H
#include "types_def.h"

namespace maplebe {
using namespace maple;
enum ExMOpType : uint8 {
  kExUndef,
  kExAdd,  /* MOP_xaddrrr | MOP_xxwaddrrre | MOP_xaddrrrs */
  kEwAdd,  /* MOP_waddrrr | MOP_wwwaddrrre | MOP_waddrrrs */
  kExSub,  /* MOP_xsubrrr | MOP_xxwsubrrre | MOP_xsubrrrs */
  kEwSub,  /* MOP_wsubrrr | MOP_wwwsubrrre | MOP_wsubrrrs */
  kExCmn,  /* MOP_xcmnrr | MOP_xwcmnrre | MOP_xcmnrrs */
  kEwCmn,  /* MOP_wcmnrr | MOP_wwcmnrre | MOP_wcmnrrs */
  kExCmp,  /* MOP_xcmprr | MOP_xwcmprre | MOP_xcmprrs */
  kEwCmp,  /* MOP_wcmprr | MOP_wwcmprre | MOP_wcmprrs */
};

enum LsMOpType : uint8 {
  kLsUndef,
  kLxAdd,  /* MOP_xaddrrr | MOP_xaddrrrs */
  kLwAdd,  /* MOP_waddrrr | MOP_waddrrrs */
  kLxSub,  /* MOP_xsubrrr | MOP_xsubrrrs */
  kLwSub,  /* MOP_wsubrrr | MOP_wsubrrrs */
  kLxCmn,  /* MOP_xcmnrr | MOP_xcmnrrs */
  kLwCmn,  /* MOP_wcmnrr | MOP_wcmnrrs */
  kLxCmp,  /* MOP_xcmprr | MOP_xcmprrs */
  kLwCmp,  /* MOP_wcmprr | MOP_wcmprrs */
  kLxEor,  /* MOP_xeorrrr | MOP_xeorrrrs */
  kLwEor,  /* MOP_weorrrr | MOP_weorrrrs */
  kLxNeg,  /* MOP_xinegrr | MOP_xinegrrs */
  kLwNeg,  /* MOP_winegrr | MOP_winegrrs */
  kLxIor,  /* MOP_xiorrrr | MOP_xiorrrrs */
  kLwIor,  /* MOP_wiorrrr | MOP_wiorrrrs */
};

enum SuffixType : uint8 {
  kNoSuffix, /* no suffix or do not perform the optimization. */
  kLSL,      /* logical shift left */
  kLSR,      /* logical shift right */
  kASR,      /* arithmetic shift right */
  kExten     /* ExtendOp */
};

inline constexpr uint32 kExtenAddShiftNum = 5;
inline SuffixType kDoOptimizeTable[kExtenAddShiftNum][kExtenAddShiftNum] = {
    { kNoSuffix, kLSL, kLSR, kASR, kExten },
    { kNoSuffix, kLSL, kNoSuffix, kNoSuffix, kExten },
    { kNoSuffix, kNoSuffix, kLSR, kNoSuffix, kNoSuffix },
    { kNoSuffix, kNoSuffix, kNoSuffix, kASR, kNoSuffix },
    { kNoSuffix, kNoSuffix, kNoSuffix, kNoSuffix, kExten }
};
}

#endif /* OPENARKCOMPILER_AARCH64_OPT_UTILES_H */
