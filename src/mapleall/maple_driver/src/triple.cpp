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

#include "triple.h"
#include "driver_options.h"

namespace opts {

maplecl::Option<bool> bigendian({"-Be", "--Be", "--BigEndian", "-be", "--be", "-mbig-endian"},
                           "  --BigEndian/-Be            \tUsing BigEndian\n"
                           "  --no-BigEndian             \tUsing LittleEndian\n",
                           {driverCategory, hir2mplCategory, dex2mplCategory, ipaCategory},
                           maplecl::DisableWith("--no-BigEndian"));

maplecl::Option<bool> ilp32({"--ilp32", "-ilp32", "--arm64-ilp32"},
                       "  --ilp32                              \tarm64 with a 32-bit ABI instead of a 64bit ABI\n",
                       {driverCategory, hir2mplCategory, dex2mplCategory, ipaCategory});

maplecl::Option<std::string> mabi({"-mabi"},
                             "  -mabi=<abi>                 \tSpecify integer and floating-point calling convention\n",
                             {driverCategory, hir2mplCategory, dex2mplCategory, ipaCategory});

}

namespace maple {

Triple::ArchType Triple::ParseArch(std::string_view archStr) const {
  if (maple::utils::Contains({"aarch64", "aarch64_le"}, archStr)) {
    return Triple::ArchType::aarch64;
  } else if (maple::utils::Contains({"aarch64_be"}, archStr)) {
    return Triple::ArchType::aarch64_be;
  }

  // Currently Triple support only aarch64
  return Triple::UnknownArch;
}

Triple::EnvironmentType Triple::ParseEnvironment(std::string_view archStr) const {
  if (maple::utils::Contains({"ilp32", "gnu_ilp32", "gnuilp32"}, archStr)) {
    return Triple::EnvironmentType::GNUILP32;
  } else if (maple::utils::Contains({"gnu"}, archStr)) {
    return Triple::EnvironmentType::GNU;
  }

  // Currently Triple support only ilp32 and default gnu/LP64 ABI
  return Triple::UnknownEnvironment;
}

void Triple::Init() {
  /* Currently Triple is used only to configure aarch64: be/le, ILP32/LP64
   * Other architectures (TARGX86_64, TARGX86, TARGARM32, TARGVM) are configured with compiler build config */
#if TARGAARCH64
  arch = (opts::bigendian) ? Triple::ArchType::aarch64_be : Triple::ArchType::aarch64;
  environment = (opts::ilp32) ? Triple::EnvironmentType::GNUILP32 : Triple::EnvironmentType::GNU;

  if (opts::mabi.IsEnabledByUser()) {
    auto tmpEnvironment = ParseEnvironment(opts::mabi.GetValue());
    if (tmpEnvironment != Triple::UnknownEnvironment) {
      environment = tmpEnvironment;
    }
  }
#endif
}

void Triple::Init(const std::string &target) {
  data = target;

  /* Currently Triple is used only to configure aarch64: be/le, ILP32/LP64.
   * Other architectures (TARGX86_64, TARGX86, TARGARM32, TARGVM) are configured with compiler build config */
#if TARGAARCH64
  Init();

  std::vector<std::string_view> components;
  maple::StringUtils::SplitSV(data, components, '-');
  if (components.size() == 0) { // as minimum 1 component must be
    return;
  }

  auto tmpArch = ParseArch(components[0]); // to not overwrite arch seting by opts::bigendian
  if (tmpArch == Triple::UnknownArch) {
    return;
  }
  arch = tmpArch;

  /* Try to check environment in option.
   * As example, it can be: aarch64-none-linux-gnu or aarch64-linux-gnu or aarch64-gnu, where gnu is environment */
  for (int i = 1; i < components.size(); ++i) {
    auto tmpEnvironment = ParseEnvironment(components[i]);
    if (tmpEnvironment != Triple::UnknownEnvironment) {
      environment = tmpEnvironment;
      break;
    }
  }
#endif
}

std::string Triple::GetArchName() const {
  switch (arch) {
    case ArchType::aarch64_be: return "aarch64_be";
    case ArchType::aarch64: return "aarch64";
    default: ASSERT(false, "Unknown Architecture Type\n");
  }
  return "";
}

std::string Triple::GetEnvironmentName() const {
  switch (environment) {
    case EnvironmentType::GNUILP32: return "gnu_ilp32";
    case EnvironmentType::GNU: return "gnu";
    default: ASSERT(false, "Unknown Environment Type\n");
  }
  return "";
}

std::string Triple::Str() const {
  if (!data.empty()) {
    return data;
  }

  if (GetArch() != ArchType::UnknownArch &&
      GetEnvironment() != Triple::EnvironmentType::UnknownEnvironment) {
    /* only linux platform is supported, so "-linux-" is hardcoded */
    return GetArchName() + "-linux-" + GetEnvironmentName();
  }

  CHECK_FATAL(false, "Only aarch64/aarch64_be GNU/GNUILP32 targets are supported\n");
  return data;
}

} // namespace maple
