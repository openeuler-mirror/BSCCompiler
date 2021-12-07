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

#ifndef HIR2MPL_BC_INPUT_CLASS_LINKER_H
#define HIR2MPL_BC_INPUT_CLASS_LINKER_H
#include "class_loader_context.h"

namespace maple {
class ClassLinker {
 public:
  explicit ClassLinker(std::vector<std::unique_ptr<bc::DexParser>> &classPath);
  virtual ~ClassLinker() = default;

  void FindClass(const std::string &className, ClassLoaderInfo *classLoader,
                 std::list<std::unique_ptr<bc::BCClass>> &list, bool isDefClass = false);
  void LoadSuperAndInterfaces(const std::unique_ptr<bc::BCClass> &klass, ClassLoaderInfo *classLoader,
                              std::list<std::unique_ptr<bc::BCClass>> &klassList);
 private:
  std::unique_ptr<bc::BCClass> FindInSharedLib(const std::string &className, ClassLoaderInfo &classLoader);
  std::unique_ptr<bc::BCClass> FindInBaseClassLoader(const std::string &className, ClassLoaderInfo *classLoader);
  std::unique_ptr<bc::BCClass> FindInClassLoaderClassPath(const std::string &className,
                                                          ClassLoaderInfo &classLoader) const;
  std::vector<std::unique_ptr<bc::DexParser>> bootClassPath;
  std::unordered_set<GStrIdx, GStrIdxHash> processedClassName;
};
}  // end namespace maple
#endif  // HIR2MPL_BC_INPUT_CLASS_LINKER_H
