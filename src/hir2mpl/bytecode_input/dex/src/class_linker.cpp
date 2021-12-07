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
#include "class_linker.h"
#include "fe_macros.h"

namespace maple {
ClassLinker::ClassLinker(std::vector<std::unique_ptr<bc::DexParser>> &classPath) {
  for (std::unique_ptr<bc::DexParser>& dexFileParser : classPath) {
    bootClassPath.push_back(std::move(dexFileParser));
  }
}

static std::unique_ptr<bc::BCClass> FindInClassPath(const std::string &descriptor,
                                                    std::vector<std::unique_ptr<bc::DexParser>> &dexFileParsers) {
  for (std::unique_ptr<bc::DexParser> &dexFileParser : dexFileParsers) {
    CHECK_NULL_FATAL(dexFileParser);
    std::unique_ptr<bc::BCClass> klass = dexFileParser->FindClassDef(descriptor);
    if (klass != nullptr) {
      return klass;
    }
  }
  return nullptr;
}

std::unique_ptr<bc::BCClass> ClassLinker::FindInSharedLib(const std::string &className, ClassLoaderInfo &classLoader) {
  for (uint32 i = 0; i < classLoader.sharedLibraries.size(); ++i) {
    std::unique_ptr<bc::BCClass> result = FindInBaseClassLoader(className, classLoader.sharedLibraries[i]);
    if (result) {
      return result;
    }
  }
  return nullptr;
}

std::unique_ptr<bc::BCClass> ClassLinker::FindInClassLoaderClassPath(const std::string &className,
                                                                     ClassLoaderInfo &classLoader) const {
  return FindInClassPath(className, classLoader.hexElements);
}

std::unique_ptr<bc::BCClass> ClassLinker::FindInBaseClassLoader(const std::string &className,
                                                                ClassLoaderInfo *classLoader) {
  if (classLoader == nullptr) {
    return FindInClassPath(className, bootClassPath);
  }
  if (classLoader->type == kPathClassLoader || classLoader->type == kInMemoryDexClassLoader) {
    // Parent; Shared Libraries; Class loader dex files
    std::unique_ptr<bc::BCClass> result = FindInBaseClassLoader(className, classLoader->parent);
    if (result != nullptr) {
      return result;
    }
    result = FindInSharedLib(className, *classLoader);
    if (result != nullptr) {
      return result;
    }
    return FindInClassLoaderClassPath(className, *classLoader);
  }
  if (classLoader->type == kDelegateLastClassLoader) {
    // Boot class path; Shared class path; Class loader dex files; Parent
    std::unique_ptr<bc::BCClass> result = FindInClassPath(className, bootClassPath);
    if (result != nullptr) {
      return result;
    }
    result = FindInSharedLib(className, *classLoader);
    if (result != nullptr) {
      return result;
    }
    result = FindInClassLoaderClassPath(className, *classLoader);
    if (result != nullptr) {
      return result;
    }
    return FindInBaseClassLoader(className, classLoader->parent);
  }
  return nullptr;
}

void ClassLinker::LoadSuperAndInterfaces(const std::unique_ptr<bc::BCClass> &klass, ClassLoaderInfo *classLoader,
                                         std::list<std::unique_ptr<bc::BCClass>> &klassList) {
  if (klass->GetSuperClassNames().size() > 0) {
    FindClass(klass->GetSuperClassNames().front(), classLoader, klassList);
  }
  for (std::string interfaceName : klass->GetSuperInterfaceNames()) {
    FindClass(interfaceName, classLoader, klassList);
  }
}

void ClassLinker::FindClass(const std::string &className, ClassLoaderInfo *classLoader,
                            std::list<std::unique_ptr<bc::BCClass>> &klassList, bool isDefClass) {
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(className);
  auto it = processedClassName.find(nameIdx);
  if (it != processedClassName.end()) {
    if (isDefClass) {
      FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "Same class is existed: %s", className.c_str());
    }
    return;
  }
  // Find in bootclasspath
  std::unique_ptr<bc::BCClass> klass;
  if (classLoader == nullptr) {
    klass = FindInClassPath(className, bootClassPath);
  } else { // Find in classLoader files
    klass = FindInBaseClassLoader(className, classLoader);
  }
  if (klass != nullptr) {
    if (isDefClass) {
      FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "Same class is existed: %s", className.c_str());
    }
    FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "klassName=%s", klass->GetClassName(false).c_str());
    LoadSuperAndInterfaces(klass, classLoader, klassList);
    (void)processedClassName.insert(nameIdx);
    klassList.push_back(std::move(klass));
  } else if (!isDefClass) {
    WARN(kLncWarn, "Find class failed: %s", className.c_str());
  }
}
}
