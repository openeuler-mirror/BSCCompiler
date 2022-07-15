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
#ifndef MPL_FE_BC_INPUT_DEX_CLASS_H
#define MPL_FE_BC_INPUT_DEX_CLASS_H
#include "bc_class.h"
namespace maple {
namespace bc {
class DexClass : public BCClass {
 public:
  DexClass(uint32 idx, const BCParserBase &parser) : BCClass(idx, parser) {}
  ~DexClass() = default;
};

class DexClassField : public BCClassField {
 public:
  DexClassField(const BCClass &klassIn, uint32 itemIdxIn, uint32 idxIn, uint32 acc, const std::string &nameIn,
                const std::string &descIn)
      : BCClassField(klassIn, acc, nameIn, descIn),
        itemIdx(itemIdxIn),
        idx(idxIn) {}
  ~DexClassField() = default;

 protected:
  uint32 GetItemIdxImpl() const override;
  uint32 GetIdxImpl() const override;
  bool IsStaticImpl() const override;
  uint32 itemIdx;
  uint32 idx;
};

class DEXTryInfo : public BCTryInfo {
 public:
  DEXTryInfo(uint32 startAddrIn, uint32 endAddrIn, std::unique_ptr<std::list<std::unique_ptr<BCCatchInfo>>> catchesIn)
      : BCTryInfo(startAddrIn, endAddrIn, std::move(catchesIn)) {}
  ~DEXTryInfo() = default;
};

class DEXCatchInfo : public BCCatchInfo {
 public:
  DEXCatchInfo(uint32 pcIn, const GStrIdx &exceptionNameIdx, bool catchAll)
      : BCCatchInfo(pcIn, exceptionNameIdx, catchAll) {}
  ~DEXCatchInfo() = default;
};

class DexClassMethod : public BCClassMethod {
 public:
  DexClassMethod(const BCClass &klassIn, uint32 itemIdxIn, uint32 idxIn, bool isVirtualIn, uint32 acc,
                 const std::string &nameIn, const std::string &descIn)
      : BCClassMethod(klassIn, acc, isVirtualIn, nameIn, descIn),
        itemIdx(itemIdxIn),
        idx(idxIn) {}
  ~DexClassMethod() = default;

 protected:
  uint32 GetItemIdxImpl() const override;
  uint32 GetIdxImpl() const override;
  bool IsStaticImpl() const override;
  bool IsVirtualImpl() const override;
  bool IsNativeImpl() const override;
  bool IsInitImpl() const override;
  bool IsClinitImpl() const override;
  std::vector<std::unique_ptr<FEIRVar>> GenArgVarListImpl() const override;
  void GenArgRegsImpl() override;
  uint32 itemIdx;
  uint32 idx;
};
}  // namespace bc
}  // namespace maple
#endif  // MPL_FE_BC_INPUT_DEX_CLASS_H
