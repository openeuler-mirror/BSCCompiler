/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef VERSION_H
#define VERSION_H
#include <cstdint>
#include <string>
#include "securec.h"
class Version {
 public:
  static std::string GetVersionStr() {
    std::ostringstream oss;
    oss << kMajorVersion << "." << kMinorVersion << "." << kReleaseVersion;

#ifdef BUILD_VERSION
    constexpr int BUILD_VERSION_LEN = 5;
    char buffer[BUILD_VERSION_LEN] = {0};
    int ret = sprintf_s(buffer, BUILD_VERSION_LEN, "B%03u", kBuildVersion);
    if (ret >= 0) {
      oss << "." << buffer;
    }
#endif // BUILD_VERSION

#ifdef GIT_REVISION
    oss << " " << kGitRevision;
#endif // GIT_REVISION
    return oss.str();
  }

  static inline uint32_t GetMajorVersion() {
    return kMajorVersion;
  }

  static inline uint32_t GetMinorVersion() {
    return kMinorVersion;
  }

  static inline const char* GetReleaseVersion() {
    return kReleaseVersion;
  }

  static inline uint32_t GetRuntimeVersion() {
    return kMinorRuntimeVersion;
  }
 private:
#ifdef ANDROID
  // compatible for Android build script
  static constexpr const uint32_t kMajorMplVersion = 4;
#endif

#ifdef ANDROID
  // compatible for Android build script
  static constexpr const uint32_t kMajorVersion = 4;
#elif MAJOR_VERSION
  static constexpr const uint32_t kMajorVersion = MAJOR_VERSION;
#else // MAJOR_VERSION
  static constexpr const uint32_t kMajorVersion = 1;
#endif // ANDROID

#ifdef MINOR_VERSION
  static constexpr const uint32_t kMinorVersion = MINOR_VERSION;
#else
  static constexpr const uint32_t kMinorVersion = 0;
#endif // MINOR_VERSION

// [a-zA-Z0-9_\-]+
#ifdef RELEASE_VERSION
  static constexpr const char* kReleaseVersion = RELEASE_VERSION;
#else
  static constexpr const char* kReleaseVersion = "0";
#endif // RELEASE_VERSION

// B[0-9]{3}
#ifdef BUILD_VERSION
  static constexpr const uint32_t kBuildVersion = BUILD_VERSION;
#endif // BUILD_VERSION

#ifdef GIT_REVISION
  static constexpr const char* kGitRevision = GIT_REVISION;
#endif // GIT_REVISION

#ifdef MINOR_RUNTIME_VERSION
  static constexpr const uint32_t kMinorRuntimeVersion = MINOR_RUNTIME_VERSION;
#else
  static constexpr const uint32_t kMinorRuntimeVersion = 0;
#endif // kMinorRuntimeVersion
};
#endif // VERSION_H
