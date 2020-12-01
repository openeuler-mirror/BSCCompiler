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
#ifndef MAPLE_JAVA2C_RULE_H_
#define MAPLE_JAVA2C_RULE_H_
#include <iostream>

#include "jni.h"
#include "mobject.h"
#include "exception/mrt_exception.h"

namespace maplert {
#define INIT_ARGS

#define j2cRule(INIT_ARGS) j2cRule

class Java2CRule {
 public:
  Java2CRule() = default;
  ~Java2CRule() = default;
  void Prologue() const { }
  // when a java function exit, we need to call it
  void Epilogue() const {
    if (MRT_HasPendingException()) {
      MRT_CheckThrowPendingExceptionRet();
    }
  }
};
} // namespace maplert

#endif //MAPLE_JAVA2C_RULE_H_
