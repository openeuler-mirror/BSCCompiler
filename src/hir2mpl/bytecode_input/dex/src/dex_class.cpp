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
#include "dex_class.h"
#include "dex_file_util.h"
#include "dex_op.h"
#include "fe_utils_java.h"
namespace maple {
namespace bc {
// ========== DexClassField ==========
bool DexClassField::IsStaticImpl() const {
  return accessFlag & DexFileUtil::kDexAccStatic;
}

uint32 DexClassField::GetItemIdxImpl() const {
  return itemIdx;
}

uint32 DexClassField::GetIdxImpl() const {
  return idx;
}

// ========== DexClassMethod ==========
bool DexClassMethod::IsStaticImpl() const {
  return accessFlag & DexFileUtil::kDexAccStatic;
}

bool DexClassMethod::IsVirtualImpl() const {
  return isVirtual;
}

bool DexClassMethod::IsNativeImpl() const {
  return accessFlag & DexFileUtil::kDexAccNative;
}

bool DexClassMethod::IsInitImpl() const {
  return GetName().compare("<init>") == 0;
}

bool DexClassMethod::IsClinitImpl() const {
  return GetName().compare("<clinit>") == 0;
}

uint32 DexClassMethod::GetItemIdxImpl() const {
  return itemIdx;
}

uint32 DexClassMethod::GetIdxImpl() const {
  return idx;
}

std::vector<std::unique_ptr<FEIRVar>> DexClassMethod::GenArgVarListImpl() const {
  std::vector<std::unique_ptr<FEIRVar>> args;
  for (const auto &reg : argRegs) {
    args.emplace_back(reg->GenFEIRVarReg());
  }
  return args;
}

void DexClassMethod::GenArgRegsImpl() {
  uint32 regNum = registerTotalSize - registerInsSize;
  std::string proto = GetDescription();
  sigTypeNames = FEUtilJava::SolveMethodSignature(proto);
  if (!IsClinit() && ((GetAccessFlag() & DexFileUtil::kDexAccStatic) == 0)) {
    std::unique_ptr<BCReg> reg = std::make_unique<BCReg>();
    reg->regNum = regNum;
    reg->isDef = true;
    regNum += 1;
    reg->regType = allocator.GetMemPool()->New<BCRegType>(allocator, *reg, GetClassNameMplIdx());
    argRegs.emplace_back(std::move(reg));
  }
  if (sigTypeNames.size() > 1) {
    for (size_t i = 1; i < sigTypeNames.size(); ++i) {
      std::unique_ptr<BCReg> reg = std::make_unique<BCReg>();
      reg->regNum = regNum;
      reg->isDef = true;
      GStrIdx sigTypeNamesIdx =
          GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(sigTypeNames.at(i)));
      reg->regType = allocator.GetMemPool()->New<BCRegType>(allocator, *reg, sigTypeNamesIdx);
      argRegs.emplace_back(std::move(reg));
      regNum += (BCUtil::IsWideType(sigTypeNamesIdx) ? 2 : 1);
    }
  }
}
}  // namespace bc
}  // namespace maple