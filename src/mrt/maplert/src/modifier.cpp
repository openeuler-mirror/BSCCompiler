/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "modifier.h"

namespace maplert {
void modifier::JavaAccessFlagsToString(uint32_t accessFlags, std::string &result) {
  if ((accessFlags & kModifierPublic) != 0) {
    result += "public ";
  }
  if ((accessFlags & kModifierProtected) != 0) {
    result += "protected ";
  }
  if ((accessFlags & kModifierPrivate) != 0) {
    result += "private ";
  }
  if ((accessFlags & kModifierFinal) != 0) {
    result += "final ";
  }
  if ((accessFlags & kModifierStatic) != 0) {
    result += "static ";
  }
  if ((accessFlags & kModifierAbstract) != 0) {
    result += "abstract ";
  }
  if ((accessFlags & kModifierInterface) != 0) {
    result += "interface ";
  }
  if ((accessFlags & kModifierTransient) != 0) {
    result += "transient ";
  }
  if ((accessFlags & kModifierVolatile) != 0) {
    result += "volatile ";
  }
  if ((accessFlags & kModifierSynchronized) != 0) {
    result += "synchronized ";
  }
  if ((accessFlags & kModifierRCUnowned) != 0) {
    result += "rcunowned ";
  }
  if ((accessFlags & kModifierRCWeak) != 0) {
    result += "rcweak ";
  }
}
} // namespace maplert
