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
#define DEFINE_MOP(op, ...) const OpndDesc OpndDesc::op = __VA_ARGS__;
#include "operand.def"
#undef DEFINE_MOP
#define DEFINE_MOP(op, ...) {abstract::op, __VA_ARGS__},
const InsnDesc InsnDesc::abstractId[abstract::kMopLast] = {
#include "abstract_mmir.def"
};
#undef DEFINE_MOP

bool InsnDesc::IsSame(const InsnDesc &left,
    std::function<bool (const InsnDesc &left, const InsnDesc &right)> cmp) const {
  return cmp == nullptr ? false : cmp(left, *this);
}

bool InsnDesc::operator==(const InsnDesc &o) const {
  bool opndMdEqual = true;
  if (opndMD.size() == o.opndMD.size()) {
    for (size_t i = 0; i < opndMD.size(); ++i) {
      if (opndMD[i] != o.opndMD[i]) {
        opndMdEqual = false;
      }
    }
  } else {
    opndMdEqual = false;
  }
  return opc == o.opc && opndMdEqual && properties == o.properties && latencyType == o.latencyType;
}

bool InsnDesc::operator<(const InsnDesc &o) const {
  bool opndMdless = false;
  if (opndMD.size() == o.opndMD.size()) {
    for (size_t i = 0; i < opndMD.size(); ++i) {
      if (opndMD[i] < o.opndMD[i]) {
        opndMdless = true;
      }
    }
  } else if (opndMD.size() <= o.opndMD.size()) {
    opndMdless = true;
  }
  return (properties < o.properties) || (properties == o.properties && opndMdless);
}
}
