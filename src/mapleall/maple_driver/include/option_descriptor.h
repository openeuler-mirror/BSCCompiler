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
#ifndef MAPLE_OPTION_DESCRIPTOR_H
#define MAPLE_OPTION_DESCRIPTOR_H
#include <vector>
#include <map>
#include <string>
#include <algorithm>

namespace mapleOption {
enum BuildType {
  kBuildTypeDefault, // Print usages of all levels
  kBuildTypeProduct,
  kBuildTypeExperimental,
  kBuildTypeDebug,
  kBuildTypeAll // Will be printed in all levels
};

enum ArgCheckPolicy {
  kArgCheckPolicyUnknown,
  kArgCheckPolicyNone,
  kArgCheckPolicyOptional,
  kArgCheckPolicyRequired,
  kArgCheckPolicyNumeric,
  kArgCheckPolicyBool
};

enum OptionPrefixType {
  shortOptPrefix,
  longOptPrefix,
  undefinedOpt,
};

constexpr size_t kMaxExtraOptions = 10;

struct Descriptor {
  // Unique option index
  const size_t index;

  // Unique option type
  const int type;

  // Short form option key
  const std::string shortOption;

  // Long form option key
  const std::string longOption;

  // The option can be seen in which build type.
  const BuildType enableBuildType;

  // Should we have a parameter?
  const ArgCheckPolicy checkPolicy;

  // Help info
  const std::string help;

  const std::string exeName;

  // option key mapping to target tool
  const std::vector<std::string> extras;

  bool IsEnabledForCurrentBuild() const {
    switch (enableBuildType) {
      case BuildType::kBuildTypeDefault:
        return true;
      case BuildType::kBuildTypeProduct:
        return true;
      case BuildType::kBuildTypeExperimental:
        return true;
      case BuildType::kBuildTypeDebug:
        return true;
      case BuildType::kBuildTypeAll:
        return true;
      default:
        // should never reach
        return true;
    }
  }
};

class Option {
 public:
  Option(const Descriptor &desc, const std::string &optionKey, const std::string &args)
      : descriptor(desc), optionKey(optionKey), args(args) {}

  Option(const Descriptor &desc, const std::string &optionKey,
         const std::string &args, OptionPrefixType prefixType,
         bool isEqualPrefix)
      : descriptor(desc), optionKey(optionKey), args(args),
        prefixType(prefixType), isEqualPrefix(isEqualPrefix) {}

  ~Option() = default;

  std::string GetPrefix() const;

  size_t Index() const {
    return descriptor.index;
  }

  int Type() const {
    return descriptor.type;
  }

  const std::string &GetExeName() const {
    return descriptor.exeName;
  }

  OptionPrefixType GetPrefixType() const {
    return prefixType;
  }

  bool CheckEqualPrefix() const {
    return isEqualPrefix;
  }

  const std::string &Args() const {
    return args;
  }

  const std::string &OptionKey() const {
    return optionKey;
  }

  const Descriptor &GetDescriptor() const {
    return descriptor;
  }

  bool HasExtra() const {
    return (descriptor.extras.size() > 0);
  }

  std::vector<std::string> GetExtras() const {
    auto ret = std::vector<std::string>();
    size_t index = 0;
    while (index < kMaxExtraOptions && index < descriptor.extras.size()) {
      ret.push_back(descriptor.extras[index++]);
    }
    return ret;
  }

  bool FindExtra(const std::string &exeName) const {
    auto item = std::find(descriptor.extras.begin(), descriptor.extras.end(), exeName);
    if (item == descriptor.extras.end()) {
      return false;
    }
    return true;
  }

 private:
  Descriptor descriptor;
  const std::string optionKey;
  const std::string args;
  OptionPrefixType prefixType = undefinedOpt;
  bool isEqualPrefix = false; // whether option contains "=", like -op=-value
};

enum EnabledIndex {
  kDisable,
  kEnable
};
}  // namespace mapleOption
#endif // MAPLE_OPTION_DESCRIPTOR_H
