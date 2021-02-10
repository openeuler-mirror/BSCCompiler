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
#ifndef MPLFE_INCLUDE_COMMON_FE_MANAGER_H
#define MPLFE_INCLUDE_COMMON_FE_MANAGER_H
#include <memory>
#include "mir_module.h"
#include "mir_builder.h"
#include "fe_type_manager.h"
#include "fe_java_string_manager.h"

namespace maple {
class FEManager {
 public:
  static FEManager &GetManager() {
    ASSERT(manager != nullptr, "manager is not initialize");
    return *manager;
  }

  static FETypeManager &GetTypeManager() {
    ASSERT(manager != nullptr, "manager is not initialize");
    return manager->typeManager;
  }

  static FEJavaStringManager &GetJavaStringManager() {
    ASSERT(manager != nullptr, "manager is not initialize");
    return manager->javaStringManager;
  }

  static MIRBuilder &GetMIRBuilder() {
    ASSERT(manager != nullptr, "manager is not initialize");
    return manager->builder;
  }

  static MIRModule &GetModule() {
    ASSERT(manager != nullptr, "manager is not initialize");
    return manager->module;
  }

  static void Init(MIRModule &moduleIn) {
    manager = new FEManager(moduleIn);
  }

  static void Release() {
    if (manager != nullptr) {
      manager->typeManager.ReleaseMemPool();
      delete manager;
      manager = nullptr;
    }
  }

  StructElemNameIdx *GetFieldStructElemNameIdx(uint64 index) {
    auto it = mapFieldStructElemNameIdx.find(index);
    if (it != mapFieldStructElemNameIdx.end()) {
      return it->second;
    }
    return nullptr;
  }

  void SetFieldStructElemNameIdx(uint64 index, StructElemNameIdx &structElemNameIdx) {
    std::lock_guard<std::mutex> lk(feManagerMapStructElemNameIdxMtx);
    mapFieldStructElemNameIdx[index] = &structElemNameIdx;
  }

  StructElemNameIdx *GetMethodStructElemNameIdx(uint64 index) {
    auto it = mapMethodStructElemNameIdx.find(index);
    if (it != mapMethodStructElemNameIdx.end()) {
      return it->second;
    }
    return nullptr;
  }

  void SetMethodStructElemNameIdx(uint64 index, StructElemNameIdx &structElemNameIdx) {
    std::lock_guard<std::mutex> lk(feManagerMapStructElemNameIdxMtx);
    mapMethodStructElemNameIdx[index] = &structElemNameIdx;
  }

  MemPool *GetStructElemMempool() {
    return structElemMempool;
  }

  void ReleaseStructElemMempool() {
    if (structElemMempool != nullptr) {
      delete structElemMempool;
      structElemMempool = nullptr;
    }
  }

  uint32 RegisterSourceFileIdx(const GStrIdx &strIdx) {
    auto it = sourceFileIdxMap.find(strIdx);
    if (it != sourceFileIdxMap.end()) {
      return it->second;
    } else {
      // make src files start from #2, #1 is mpl file
      size_t num = sourceFileIdxMap.size() + 2;
      (void)sourceFileIdxMap.emplace(strIdx, num);
#ifdef DEBUG
      idxSourceFileMap.emplace(num, strIdx);
#endif
      module.PushbackFileInfo(MIRInfoPair(strIdx, num));
      return static_cast<uint32>(num);
    }
  }

  std::string GetSourceFileNameFromIdx(uint32 idx) const {
    auto it = idxSourceFileMap.find(idx);
    if (it != idxSourceFileMap.end()) {
      return GlobalTables::GetStrTable().GetStringFromStrIdx(it->second);
    }
    return "unknown";
  }

 private:
  static FEManager *manager;
  MIRModule &module;
  FETypeManager typeManager;
  MIRBuilder builder;
  FEJavaStringManager javaStringManager;
  MemPool *structElemMempool;
  MapleAllocator structElemAllocator;
  std::unordered_map<uint64, StructElemNameIdx*> mapFieldStructElemNameIdx;
  std::unordered_map<uint64, StructElemNameIdx*> mapMethodStructElemNameIdx;
  std::map<GStrIdx, uint32> sourceFileIdxMap;
  std::map<uint32, GStrIdx> idxSourceFileMap;
  explicit FEManager(MIRModule &moduleIn)
      : module(moduleIn),
        typeManager(module),
        builder(&module),
        javaStringManager(moduleIn, builder),
        structElemMempool(memPoolCtrler.NewMemPool("MemPool for StructElemNameIdx")),
        structElemAllocator(structElemMempool) {}
  ~FEManager() {
    if (structElemMempool != nullptr) {
      delete structElemMempool;
      structElemMempool = nullptr;
    }
  }
  mutable std::mutex feManagerMapStructElemNameIdxMtx;
};
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_FE_MANAGER_H
