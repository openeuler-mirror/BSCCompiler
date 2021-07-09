/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPLFE_TEST_MPLFE_UT_ENVIRONMENT_H
#define MPLFE_TEST_MPLFE_UT_ENVIRONMENT_H
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mir_module.h"
#include "mir_nodes.h"

namespace maple {
class MPLFEUTEnvironment : public ::testing::Environment {
 public:
  MPLFEUTEnvironment() = default;
  ~MPLFEUTEnvironment() = default;

  static MIRModule &GetMIRModule() {
    static MIRModule module("mplfeUT");
    return module;
  }

  void SetUp() override {
    theMIRModule = &GetMIRModule();
  }
};
}  // namespace maple
#endif  // MPLFE_TEST_MPLFE_UT_ENVIRONMENT_H