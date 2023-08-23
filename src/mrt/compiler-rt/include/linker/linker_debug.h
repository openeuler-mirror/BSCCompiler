/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_RUNTIME_MPL_DEBUG_H_
#define MAPLE_RUNTIME_MPL_DEBUG_H_

#include "linker_model.h"

namespace maplert {
class Debug : public FeatureModel {
 public:
  static FeatureName featureName;
  explicit Debug(LinkerInvoker &invoker) : pInvoker(&invoker) {};
  ~Debug() {
    pInvoker = nullptr;
  }
  void DumpAllMplSectionInfo(std::ostream &os);
  void DumpAllMplFuncProfile(std::unordered_map<std::string, std::vector<FuncProfInfo>> &funcProfileRaw);
  void DumpAllMplFuncIRProfile(std::unordered_map<std::string, MFileIRProfInfo> &funcProfileRaw);
  uint64_t DumpMetadataSectionSize(std::ostream &os, void *handle, const std::string sectionName);
  bool DumpMethodUndefSymbol(LinkerMFileInfo &mplInfo);
  bool DumpMethodSymbol(LinkerMFileInfo &mplInfo);
  bool DumpDataUndefSymbol(LinkerMFileInfo &mplInfo);
  bool DumpDataSymbol(LinkerMFileInfo &mplInfo);
  void DumpStackInfoInLog();
  void DumpBBProfileInfo(std::ostream &os);
 private:
  LinkerInvoker *pInvoker = nullptr;
};
} // namespace maplert
#endif // MAPLE_RUNTIME_MPL_DEBUG_H_
