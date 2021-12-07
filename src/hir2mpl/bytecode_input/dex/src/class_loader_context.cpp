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
#include <android-base/file.h>
#include "dexfile_libdexfile.h"
#include "class_loader_context.h"

namespace maple {
const char ClassLoaderContext::kPathClassLoaderString[] = "PCL";
const char ClassLoaderContext::kDelegateLastClassLoaderString[] = "DLC";
const char ClassLoaderContext::kInMemoryDexClassLoaderString[] = "IMC";
const char ClassLoaderContext::kClassLoaderOpeningMark = '[';
const char ClassLoaderContext::kClassLoaderClosingMark = ']';
const char ClassLoaderContext::kClassLoaderSharedLibraryOpeningMark = '{';
const char ClassLoaderContext::kClassLoaderSharedLibraryClosingMark = '}';
const char ClassLoaderContext::kClassLoaderSharedLibrarySeparator = '#';
const char ClassLoaderContext::kClassLoaderSeparator = ';';
const char ClassLoaderContext::kClasspathSeparator = ',';
const char ClassLoaderContext::kSpecialSharedLib[] = "&";

size_t ClassLoaderContext::FindMatchingSharedLibraryCloseMarker(const std::string& spec,
                                                                size_t sharedLibraryOpenIndex) {
  uint32_t counter = 1;
  size_t stringIndex = sharedLibraryOpenIndex + 1;
  size_t sharedLibraryClose = std::string::npos;
  while (counter != 0) {
    sharedLibraryClose = spec.find_first_of(kClassLoaderSharedLibraryClosingMark, stringIndex);
    size_t sharedLibraryOpen = spec.find_first_of(kClassLoaderSharedLibraryOpeningMark, stringIndex);
    if (sharedLibraryClose == std::string::npos) {
      break;
    }
    if ((sharedLibraryOpen == std::string::npos) ||
        (sharedLibraryClose < sharedLibraryOpen)) {
      --counter;
      stringIndex = sharedLibraryClose + 1;
    } else {
      ++counter;
      stringIndex = sharedLibraryOpen + 1;
    }
  }
  return sharedLibraryClose;
}

ClassLoaderType ClassLoaderContext::GetCLType(const std::string& clString) {
  if (clString.compare(0, strlen(kPathClassLoaderString), kPathClassLoaderString) == 0) {
    return kPathClassLoader;
  } else if (clString.compare(0, strlen(kDelegateLastClassLoaderString), kDelegateLastClassLoaderString) == 0) {
    return kDelegateLastClassLoader;
  } else if (clString.compare(0, strlen(kInMemoryDexClassLoaderString), kInMemoryDexClassLoaderString) == 0) {
    return kInMemoryDexClassLoader;
  }
  return kInvalidClassLoader;
}

const char* ClassLoaderContext::GetCLTypeName(ClassLoaderType type) const {
  switch (type) {
    case kPathClassLoader: {
      return kPathClassLoaderString;
    }
    case kDelegateLastClassLoader: {
      return kDelegateLastClassLoaderString;
    }
    case kInMemoryDexClassLoader: {
      return kInMemoryDexClassLoaderString;
    }
    default: {
      return nullptr;
    }
  }
}

ClassLoaderInfo *ClassLoaderContext::ParseClassLoaderSpec(const std::string &spec) {
  ClassLoaderType classLoaderType = GetCLType(spec);
  if (classLoaderType == kInvalidClassLoader) {
    return nullptr;
  }
  if (classLoaderType == kInMemoryDexClassLoader) {
    return nullptr;
  }
  const char* classLoaderTypeStr = GetCLTypeName(classLoaderType);
  size_t typeStrZize = strlen(classLoaderTypeStr);
  // Check the opening and closing markers.
  if (spec[typeStrZize] != kClassLoaderOpeningMark) {
    return nullptr;
  }
  if ((spec[spec.length() - 1] != kClassLoaderClosingMark) &&
      (spec[spec.length() - 1] != kClassLoaderSharedLibraryClosingMark)) {
    return nullptr;
  }
  size_t closingIndex = spec.find_first_of(kClassLoaderClosingMark);
  std::string classpath = spec.substr(typeStrZize + 1, closingIndex - typeStrZize - 1);
  ClassLoaderInfo *info = mp.New<ClassLoaderInfo>();
  info->type = classLoaderType;
  OpenDexFiles(classpath, info->hexElements);
  if ((spec[spec.length() - 1] == kClassLoaderSharedLibraryClosingMark) &&
      (spec[spec.length() - 2] != kClassLoaderSharedLibraryOpeningMark)) {
    size_t startIndex = spec.find_first_of(kClassLoaderSharedLibraryOpeningMark);
    if (startIndex == std::string::npos) {
      return nullptr;
    }
    std::string sharedLibrariesSpec = spec.substr(startIndex + 1, spec.length() - startIndex - 2);
    if (!ParseSharedLibraries(sharedLibrariesSpec, *info)) {
      return nullptr;
    }
  }
  return info;
}

bool ClassLoaderContext::ParseSharedLibraries(std::string &sharedLibrariesSpec, ClassLoaderInfo &info) {
  size_t cursor = 0;
  while (cursor != sharedLibrariesSpec.length()) {
    size_t sharedLibrarySeparator = sharedLibrariesSpec.find_first_of(kClassLoaderSharedLibrarySeparator, cursor);
    size_t sharedLibraryOpen = sharedLibrariesSpec.find_first_of(kClassLoaderSharedLibraryOpeningMark, cursor);
    std::string sharedLibrarySpec;
    if (sharedLibrarySeparator == std::string::npos) {
      sharedLibrarySpec = sharedLibrariesSpec.substr(cursor, sharedLibrariesSpec.length() - cursor);
      cursor = sharedLibrariesSpec.length();
    } else if ((sharedLibraryOpen == std::string::npos) || (sharedLibraryOpen > sharedLibrarySeparator)) {
      sharedLibrarySpec = sharedLibrariesSpec.substr(cursor, sharedLibrarySeparator - cursor);
      cursor = sharedLibrarySeparator + 1;
    } else {
      size_t closing_marker = FindMatchingSharedLibraryCloseMarker(sharedLibrariesSpec, sharedLibraryOpen);
      if (closing_marker == std::string::npos) {
        return false;
      }
      sharedLibrarySpec = sharedLibrariesSpec.substr(cursor, closing_marker + 1 - cursor);
      cursor = closing_marker + 1;
      if (cursor != sharedLibrariesSpec.length() &&
          sharedLibrariesSpec[cursor] == kClassLoaderSharedLibrarySeparator) {
        ++cursor;
      }
    }
    ClassLoaderInfo *sharedLibrary = ParseInternal(sharedLibrarySpec);
    if (sharedLibrary == nullptr) {
      return false;
    }
    info.sharedLibraries.push_back(sharedLibrary);
  }
  return true;
}

ClassLoaderInfo *ClassLoaderContext::ParseInternal(const std::string &spec) {
  std::string remaining = spec;
  ClassLoaderInfo *first = nullptr;
  ClassLoaderInfo *previousIteration = nullptr;
  while (!remaining.empty()) {
    std::string currentSpec;
    size_t classLoaderSeparator = remaining.find_first_of(kClassLoaderSeparator);
    size_t sharedLibraryOpen = remaining.find_first_of(kClassLoaderSharedLibraryOpeningMark);
    if (classLoaderSeparator == std::string::npos) {
      currentSpec = remaining;
      remaining = "";
    } else if ((sharedLibraryOpen == std::string::npos) || (sharedLibraryOpen > classLoaderSeparator)) {
      currentSpec = remaining.substr(0, classLoaderSeparator);
      remaining = remaining.substr(sharedLibraryOpen + 1, remaining.size() - classLoaderSeparator - 1);
    } else {
      size_t sharedLibraryClose = FindMatchingSharedLibraryCloseMarker(remaining, sharedLibraryOpen);
      CHECK_FATAL(sharedLibraryClose != std::string::npos,
                  "Invalid class loader spec: %s", currentSpec.c_str());
      currentSpec = remaining.substr(0, sharedLibraryClose + 1);
      if (remaining.size() == sharedLibraryClose + 1) {
        remaining = "";
      } else if ((remaining.size() == sharedLibraryClose + 2) ||
                 (remaining.at(sharedLibraryClose + 1) != kClassLoaderSeparator)) {
        CHECK_FATAL(false, "Invalid class loader spec: %s", currentSpec.c_str());
        return nullptr;
      } else {
        remaining = remaining.substr(sharedLibraryClose + 2, remaining.size() - sharedLibraryClose - 2);
      }
    }
    ClassLoaderInfo *info = ParseClassLoaderSpec(currentSpec);
    CHECK_FATAL(info != nullptr, "Invalid class loader spec: %s", currentSpec.c_str());
    if (first == nullptr) {
      first = info;
      previousIteration = first;
    } else {
      CHECK_NULL_FATAL(previousIteration);
      previousIteration->parent = info;
      previousIteration = previousIteration->parent;
    }
  }
  return first;
}

// Process PCL/DLC string, and open the corresponding byte code file (dex/jar/apk)
bool ClassLoaderContext::Parse(const std::string &spec) {
  if (spec.empty()) {
    return false;
  }
  // Output to IFile, used by collision check when trying to load IFile.
  if (spec.compare(kSpecialSharedLib) == 0) {
    isSpecialSharedLib = true;
    return false;
  }
  loaderChain = ParseInternal(spec);
  return loaderChain != nullptr;
}

// PCL
ClassLoaderContext *ClassLoaderContext::Create(const std::string &spec, MemPool &mp) {
  ClassLoaderContext *retv = mp.New<ClassLoaderContext>(mp);
  if (retv->Parse(spec)) {
    return retv;
  } else {
    return nullptr;
  }
}

// compiled File
ClassLoaderInfo *ClassLoaderContext::CreateClassLoader(const std::string &spec) {
  ClassLoaderInfo *classLoader = mp.New<ClassLoaderInfo>();
  classLoader->type = kPathClassLoader;
  classLoader->parent = loaderChain;
  loaderChain = classLoader;
  OpenDexFiles(spec, classLoader->hexElements);
  return loaderChain;
}

bool ClassLoaderContext::OpenDexFiles(const std::string &spec,
                                      std::vector<std::unique_ptr<bc::DexParser>> &openedFileParsers) {
  if (spec.empty()) {
    return false;
  }
  bool isSuccess = true;
  const std::vector<std::string> &fileNames = FEUtils::Split(spec, kClasspathSeparator);
  uint depFileIdx = UINT32_MAX;
  for (const std::string &fileName : fileNames) {
    const bool verifyChecksum = true;
    const bool verify = true;
    std::unique_ptr<std::string> content = std::make_unique<std::string>();
    // If the file is not a .dex file, the function tries .zip/.jar/.apk files,
    // all of which are Zip archives with "classes.dex" inside.
    if (!android::base::ReadFileToString(fileName, content.get())) {
      WARN(kLncErr, "%s ReadFileToString failed", fileName.c_str());
      isSuccess = false;
      continue;
    }
    // content size must > 0, otherwise previous step return false
    const art::DexFileLoader dexFileLoader;
    art::DexFileLoaderErrorCode errorCode;
    std::string errorMsg;
    std::vector<std::unique_ptr<const art::DexFile>> dexFiles;
    if (!dexFileLoader.OpenAll(reinterpret_cast<const uint8_t*>(content->data()), content->size(), fileName, verify,
                               verifyChecksum, &errorCode, &errorMsg, &dexFiles)) {
      // Display returned error message to user. Note that this error behavior
      // differs from the error messages shown by the original Dalvik dexdump.
      WARN(kLncErr, "%s open fialed, errorMsg: %s", fileName.c_str(), errorMsg.c_str());
      isSuccess = false;
      continue;
    }
    for (size_t i = 0; i < dexFiles.size(); ++i) {
      std::unique_ptr<IDexFile> iDexFile = std::make_unique<LibDexFile>(
          std::move(dexFiles[i]), std::move(content));
      iDexFile->SetFileIdx(depFileIdx);
      const std::list<std::string> &inputClassNames = {};
      const std::string str = fileName + " (classes"  + std::to_string(i + 1) + ")";  // mark dependent dexfile name
      std::unique_ptr<bc::DexParser> bcParser = std::make_unique<bc::DexParser>(depFileIdx, str, inputClassNames);
      bcParser->SetDexFile(std::move(iDexFile));
      openedFileParsers.push_back(std::move(bcParser));
      --depFileIdx;
    }
  }
  return isSuccess;
}
}
