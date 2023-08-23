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
#ifndef MAPLE_RUNTIME_MPL_HOTFIX_H_
#define MAPLE_RUNTIME_MPL_HOTFIX_H_

#include "linker_model.h"

namespace maplert {
class Hotfix : public FeatureModel {
 public:
  static FeatureName featureName;
  explicit Hotfix(LinkerInvoker &invoker) : pInvoker(&invoker) {}
  ~Hotfix() {
    pInvoker = nullptr;
  }
  void SetClassLoaderParent(const MObject *classLoader, const MObject *newParent);
  void InsertClassesFront(const MObject *classLoader, const LinkerMFileInfo &mplInfo, const std::string &path);
  void SetPatchPath(const std::string &path, int32_t mode);
  std::string GetPatchPath();
  bool IsFrontPatchMode(const std::string &path);
  bool ResetResolvedFlags(LinkerMFileInfo &mplInfo);
  bool ReleaseCaches(LinkerMFileInfo &mplInfo);
 private:
  int32_t patchMode = 1; // 0 - classloader parent, 1 - dexpath list
  std::string patchPath;
  LinkerInvoker *pInvoker = nullptr;
};
} // namespace maplert
#endif // MAPLE_RUNTIME_MPL_HOTFIX_H_
