/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_TRIPLE_H
#define MAPLE_TRIPLE_H

#include <string_utils.h>
#include <utils.h>

#include <string>
#include <string_view>

namespace maple {

class Triple {
 public:
  /* Currently, only aarch64 is supported */
  enum ArchType {
    kUnknownArch,
    kAarch64,
    kAarch64Be,
    kLastArchType
  };

  /* Currently, only ILP32 and LP64 are supported */
  enum EnvironmentType {
    kUnknownEnvironment,
    kGnu,
    kGnuIlp32,
    kLastEnvironmentType
  };

  ArchType GetArch() const { return arch; }
  EnvironmentType GetEnvironment() const { return environment; }

  bool IsBigEndian() const {
    return (GetArch() == ArchType::kAarch64Be);
  }

  std::string Str() const;
  std::string GetArchName() const;
  std::string GetEnvironmentName() const;

  static Triple &GetTriple() {
    static Triple triple;
    return triple;
  }
  Triple(const Triple &) = delete;
  Triple &operator=(const Triple &) = delete;

  void Init(const std::string &target);
  void Init();

 private:
  std::string data;
  ArchType arch;
  EnvironmentType environment;

  Triple() : arch(kUnknownArch), environment(kUnknownEnvironment) {}

  Triple::ArchType ParseArch(const std::string_view archStr) const;
  Triple::EnvironmentType ParseEnvironment(const std::string_view archStr) const;
};

} // namespace maple

#endif /* MAPLE_TRIPLE_H */
