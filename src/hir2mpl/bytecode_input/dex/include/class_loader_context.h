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
#ifndef HIR2MPL_BC_INPUT_CLASS_LOADER_CONTEXT_H
#define HIR2MPL_BC_INPUT_CLASS_LOADER_CONTEXT_H
#include "dex_parser.h"

namespace maple {
enum ClassLoaderType {
  kInvalidClassLoader = 0,
  kPathClassLoader = 1,
  kDelegateLastClassLoader = 2,
  kInMemoryDexClassLoader = 3
};

class ClassLoaderInfo {
 public:
  ClassLoaderType type;
  ClassLoaderInfo *parent;
  std::vector<std::string> classPaths;
  std::vector<std::unique_ptr<bc::DexParser>> hexElements;
  std::vector<uint32_t> checksums;
  std::vector<ClassLoaderInfo*> sharedLibraries;
};

class ClassLoaderContext {
 public:
  ClassLoaderContext(MemPool &mpIn) : mp(mpIn) {}
  virtual ~ClassLoaderContext() {
    loaderChain = nullptr;
  }

  static ClassLoaderContext *Create(const std::string &spec, MemPool &mp);
  ClassLoaderInfo *CreateClassLoader(const std::string &spec);
  static bool OpenDexFiles(const std::string &spec, std::vector<std::unique_ptr<bc::DexParser>> &dexFileParsers);

  bool IsSpecialSharedLib() const {
    return isSpecialSharedLib;
  }

  const ClassLoaderInfo *GetClassLoader() const {
    return loaderChain;
  }

 private:
  ClassLoaderType GetCLType(const std::string &clString);
  const char *GetCLTypeName(ClassLoaderType type) const;
  size_t FindMatchingSharedLibraryCloseMarker(const std::string& spec, size_t sharedLibraryOpenIndex);
  ClassLoaderInfo *ParseInternal(const std::string &spec);
  ClassLoaderInfo *ParseClassLoaderSpec(const std::string &spec);
  bool ParseSharedLibraries(std::string &sharedLibrariesSpec, ClassLoaderInfo &info);
  bool Parse(const std::string &spec);
  ClassLoaderInfo *loaderChain = nullptr;
  bool isSpecialSharedLib = false;
  MemPool &mp;
  static const char kPathClassLoaderString[];
  static const char kDelegateLastClassLoaderString[];
  static const char kInMemoryDexClassLoaderString[];
  static const char kClassLoaderOpeningMark;
  static const char kClassLoaderClosingMark;
  static const char kClassLoaderSharedLibraryOpeningMark;
  static const char kClassLoaderSharedLibraryClosingMark;
  static const char kClassLoaderSharedLibrarySeparator;
  static const char kClassLoaderSeparator;
  static const char kClasspathSeparator;
  static const char kSpecialSharedLib[];
};
}  // end namespace maple
#endif  // HIR2MPL_BC_INPUT_CLASS_LOADER_CONTEXT_H
