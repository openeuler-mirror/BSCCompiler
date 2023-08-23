/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef HIR2MPL_AST_INPUT_INCLUDE_PRAGMA_STATUS_H
#define HIR2MPL_AST_INPUT_INCLUDE_PRAGMA_STATUS_H
namespace maple {

enum class SafeSS {
  kNoneSS,
  kSafeSS,
  kUnsafeSS,
};

// #pragma prefer_inline ON/OFF
enum class PreferInlinePI {
  kNonePI,
  kPreferInlinePI,
  kPreferNoInlinePI,
};
}
#endif // HIR2MPL_AST_INPUT_INCLUDE_PRAGMA_STATUS_H