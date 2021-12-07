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
#ifndef HIR2MPL_INCLUDE_COMMON_HIR2MPL_ENV_H
#define HIR2MPL_INCLUDE_COMMON_HIR2MPL_ENV_H
#include <fstream>
#include <string>
#include <map>
#include "mir_module.h"

namespace maple {
class HIR2MPLEnv {
 public:
  void Init();
  void Finish();
  uint32 NewSrcFileIdx(const GStrIdx &nameIdx);
  GStrIdx GetFileNameIdx(uint32 fileIdx) const;
  std::string GetFileName(uint32 fileIdx) const;
  static HIR2MPLEnv &GetInstance() {
    return instance;
  }

  uint32 GetGlobalLabelIdx() const {
    return globalLabelIdx;
  }

  void IncrGlobalLabelIdx() {
    globalLabelIdx++;
  }

 private:
  static HIR2MPLEnv instance;
  std::map<uint32, GStrIdx> srcFileIdxNameMap;
  uint32 globalLabelIdx = GStrIdx(0);
  HIR2MPLEnv() = default;
  ~HIR2MPLEnv() = default;
};  // class HIR2MPLEnv
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_HIR2MPL_ENV_H
