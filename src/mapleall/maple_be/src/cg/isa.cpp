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

#include "isa.h"
namespace maplebe {
#define DEFINE_MOP(op,...) const OpndDescription OpndDescription::op=__VA_ARGS__;
#include "operand.def"
#undef DEFINE_MOP
#define DEFINE_MOP(op,...) {abstract::op,__VA_ARGS__},
const InsnDescription InsnDescription::abstractId[abstract::kMopLast] = {
#include "abstract_mmir.def"
};
#undef DEFINE_MOP

bool InsnDescription::IsSame(const InsnDescription &left,
    std::function<bool (const InsnDescription &left, const InsnDescription &right)> cmp) const {
  return cmp == nullptr ? false : cmp(left, *this);
}
}
