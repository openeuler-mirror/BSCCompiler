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
#ifndef HIR2MPL_INCLUDE_COMMON_FE_DIAG_MANAGER_H
#define HIR2MPL_INCLUDE_COMMON_FE_DIAG_MANAGER_H
#include <memory>
#include <mutex>

namespace maple {
enum FEErrno : int {
  kNoError = 0,
  kCmdParseError = 1,
  kNoValidInput = 2,
  kFEError = 3,
};

class FEDiagManager {
 public:
  FEDiagManager() = default;
  ~FEDiagManager() = default;

  void IncErrNum();
  int GetDiagRes() const;

 private:
  uint errNum = 0;
  mutable std::mutex errNumMtx;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_FE_DIAG_MANAGER_H