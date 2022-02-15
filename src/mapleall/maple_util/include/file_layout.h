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
#ifndef MIRLAYOUT_H
#define MIRLAYOUT_H
#include <iostream>

namespace maple {
// file-layout is shared between maple compiler and runtime, thus not in namespace maplert
enum LayoutType : uint8_t {
  kLayoutBootHot,
  kLayoutBothHot,
  kLayoutRunHot,
  kLayoutStartupOnly,
  kLayoutUsedOnce,
  kLayoutExecuted, // method excuted in some condition
  kLayoutUnused,
  kLayoutTypeCount
};

// this used for c string layout
static constexpr uint8_t kCStringShift = 1;
std::string GetLayoutTypeString(uint32_t type);
} // namespace maple
#endif
