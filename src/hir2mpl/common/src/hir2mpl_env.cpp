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
#include "hir2mpl_env.h"
#include "global_tables.h"
#include "mpl_logging.h"
#include "fe_options.h"

namespace maple {
HIR2MPLEnv HIR2MPLEnv::instance;

void HIR2MPLEnv::Init() {
  srcFileIdxNameMap.clear();
  srcFileIdxNameMap[0] = GStrIdx(0);
  globalLabelIdx = 1;
}

void HIR2MPLEnv::Finish() {
  srcFileIdxNameMap.clear();
}

uint32 HIR2MPLEnv::NewSrcFileIdx(const GStrIdx &nameIdx) {
  size_t idx = srcFileIdxNameMap.size() + 1; // 1: already occupied by VtableImpl.mpl
  CHECK_FATAL(idx < UINT32_MAX, "idx is out of range");
  srcFileIdxNameMap[idx] = nameIdx;
  return static_cast<uint32>(idx);
}

GStrIdx HIR2MPLEnv::GetFileNameIdx(uint32 fileIdx) const {
  auto it = srcFileIdxNameMap.find(fileIdx);
  if (it == srcFileIdxNameMap.end()) {
    return GStrIdx(0);
  } else {
    return it->second;
  }
}

std::string HIR2MPLEnv::GetFileName(uint32 fileIdx) const {
  auto it = srcFileIdxNameMap.find(fileIdx);
  if (it == srcFileIdxNameMap.end() || it->second == 0) {
    return "unknown";
  } else {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(it->second);
  }
}
}  // namespace maple
