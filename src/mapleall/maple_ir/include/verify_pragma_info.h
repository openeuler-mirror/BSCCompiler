/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEIR_VERIFY_PRAGMA_INFO_H
#define MAPLEIR_VERIFY_PRAGMA_INFO_H
#include <string>
#include <utility>

namespace maple {
enum PragmaInfoType {
  kThrowVerifyError,
  kAssignableCheck,
  kExtendFinalCheck,
  kOverrideFinalCheck
};

class VerifyPragmaInfo {
 public:
  VerifyPragmaInfo() = default;;
  virtual ~VerifyPragmaInfo() = default;

  virtual PragmaInfoType GetPragmaType() const = 0;
  bool IsEqualTo(const VerifyPragmaInfo &pragmaInfo) const {
    return GetPragmaType() == pragmaInfo.GetPragmaType();
  }
  bool IsVerifyError() const {
    return GetPragmaType() == kThrowVerifyError;
  }
  bool IsAssignableCheck() const {
    return GetPragmaType() == kAssignableCheck;
  }
  bool IsExtendFinalCheck() const {
    return GetPragmaType() == kExtendFinalCheck;
  }
  bool IsOverrideFinalCheck() const {
    return GetPragmaType() == kOverrideFinalCheck;
  }
};

class ThrowVerifyErrorPragma : public VerifyPragmaInfo {
 public:
  explicit ThrowVerifyErrorPragma(std::string errorMessage)
      : VerifyPragmaInfo(),
        errorMessage(std::move(errorMessage)) {}
  ~ThrowVerifyErrorPragma() override = default;

  PragmaInfoType GetPragmaType() const override {
    return kThrowVerifyError;
  }

  const std::string &GetMessage() const {
    return errorMessage;
  }

 private:
  std::string errorMessage;
};

class AssignableCheckPragma : public VerifyPragmaInfo {
 public:
  AssignableCheckPragma(std::string fromType, std::string toType)
      : VerifyPragmaInfo(),
        fromType(std::move(fromType)),
        toType(std::move(toType)) {}
  ~AssignableCheckPragma() override = default;

  PragmaInfoType GetPragmaType() const override {
    return kAssignableCheck;
  }

  bool IsEqualTo(const AssignableCheckPragma &pragma) const {
    return fromType == pragma.GetFromType() && toType == pragma.GetToType();
  }

  const std::string &GetFromType() const {
    return fromType;
  }

  const std::string &GetToType() const {
    return toType;
  }

 private:
  std::string fromType;
  std::string toType;
};

class ExtendFinalCheckPragma : public VerifyPragmaInfo {
 public:
  ExtendFinalCheckPragma() : VerifyPragmaInfo() {}
  ~ExtendFinalCheckPragma() override = default;

  PragmaInfoType GetPragmaType() const override {
    return kExtendFinalCheck;
  }
};

class OverrideFinalCheckPragma : public VerifyPragmaInfo {
 public:
  OverrideFinalCheckPragma() : VerifyPragmaInfo() {}
  ~OverrideFinalCheckPragma() override = default;

  PragmaInfoType GetPragmaType() const override {
    return kOverrideFinalCheck;
  }
};
}  // namespace maple
#endif // MAPLEIR_VERIFY_PRAGMA_INFO_H
