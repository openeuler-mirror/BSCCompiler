/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "fe_diag_manager.h"
#include "mpl_logging.h"

namespace maple {
void FEDiagManager::IncErrNum() {
  std::lock_guard<std::mutex> lk(errNumMtx);
  ++errNum;
}

int FEDiagManager::GetDiagRes() const {
  if (errNum > 0) {
    INFO(kLncInfo, "%d error generated.", errNum);
    return FEErrno::kFEError;
  }
  return FEErrno::kNoError;
}

void FEDiagManager::PrintFeErrorMessages() const {
  for (auto it = feErrsMap.cbegin(); it != feErrsMap.cend(); ++it) {
    std::vector<std::string> errs = it->second;
    for (size_t i = 0; i < errs.size(); ++i) {
      std::cerr << errs[i] << '\n';
    }
  }
}
}  // namespace maple